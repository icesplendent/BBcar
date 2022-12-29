#include "MQTTClient.h"
#include "MQTTNetwork.h"
#include "MQTTmbed.h"
#include "bbcar.h"
#include "mbed.h"
#include "parallax_servo.h"
// #include "DynamicMessageBufferFactory.h"
// #include "UARTTransport.h"
// #include "bbcar_control_server.h"
#include "drivers/DigitalOut.h"
// #include "erpc_basic_codec.h"
// #include "erpc_crc16.h"
// #include "erpc_simple_server.h"
// Uncomment for actual BB Car operations
#include <cmath>
#include <cstdio>

Ticker servo_feedback_ticker;
Ticker servo_ticker;

Thread measure;
EventQueue measure_queue;

PwmOut pin9(D9), pin10(D10);
PwmIn pin11(D11), pin12(D12);
DigitalInOut pin8(D8);
BBCar car(pin9, pin11, pin10, pin12, servo_ticker, servo_feedback_ticker);
parallax_laserping ping1(pin8);
BusInOut qti_pin(D4, D5, D6, D7);
int baseSpeed = 60;
int cycle = 0;
int firstTime = 1;
float length = 0;
float circleLength = 20.106;  // 圓周長 cm
float angle = 0, angle_init = 0, angle_temp = 0;
float PI = 3.1415926535897932;

/*************** MQTT Service ***************/
// GLOBAL VARIABLES
WiFiInterface* wifi;
InterruptIn btn2(BUTTON1);
// InterruptIn btn3(SW3);
volatile int message_num = 0;
volatile int arrivedcount = 0;
volatile bool closed = false;

const char* topic = "Mbed";

Thread mqtt_thread(osPriorityHigh);
EventQueue mqtt_queue;
/*************** MQTT Service ***************/

/*************** MQTT Functions  ***************/

void messageArrived(MQTT::MessageData& md) {
    MQTT::Message& message = md.message;
    char msg[300];
    sprintf(msg, "Message arrived: QoS%d, retained %d, dup %d, packetID %d\r\n",
            message.qos, message.retained, message.dup, message.id);
    printf(msg);
    ThisThread::sleep_for(2000ms);
    char payload[300];
    sprintf(payload, "Payload %.*s\r\n", message.payloadlen,
            (char*)message.payload);
    printf(payload);
    ++arrivedcount;
}

void publish_message(MQTT::Client<MQTTNetwork, Countdown>* client) {
    message_num++;
    MQTT::Message message;
    char buff[100];
    // sprintf(buff, "QoS0 Hello, Python! #%d", message_num);
    sprintf(buff, "#%d length = %f, cycle = %d", message_num, length, cycle);
    message.qos = MQTT::QOS0;
    message.retained = false;
    message.dup = false;
    message.payload = (void*)buff;
    message.payloadlen = strlen(buff) + 1;
    int rc = client->publish(topic, message);

    printf("rc:  %d\r\n", rc);
    printf("Puslish message: %s\r\n", buff);
}

void close_mqtt() { closed = true; }
/************* MQTT Functions ***************/

void length_measure(MQTT::Client<MQTTNetwork, Countdown>* client) {
    angle = (car.getAngle0() + car.getAngle1()) / 2;
    length += (angle_init - angle) / 360 * 2 * PI * circleLength;
    mqtt_queue.call(&publish_message, client);
    printf("test, cycle = %d, angle = %f, angle_temp = %f\n", cycle, angle,
           angle_temp);
    angle_init = angle;
    // mqtt_queue.call(&publish_message, client, length, cycle);
    // if (angle_temp - angle > 50) {
    //     if (firstTime) {
    //         angle_init = (car.getAngle0() + car.getAngle1()) / 2;
    //         firstTime = 0;
    //     }
    //     cycle = (cycle + 1) % 4;
    //     angle_temp = angle;

    //     if (cycle == 0) {
    //         length = (angle_init - angle) / 360 * 2 * PI * circleLength;
    //         // printf("length = %f, cycle = %d", length, cycle);
    //         // mqtt_queue.call(&publish_message, client, length, cycle);
    //         angle_init = angle;
    //     }
    // }
}

float angle1 = 0;
float angle2 = 0;
float distanceBetween = 0;
float d1 = 0;
float d2 = 0;

void detect(int32_t speed, double factor) {
    int count = 0;
    int secondOption = 0;
    angle1 = 0;
    angle2 = 0;

    // make sure the car is in stable condition
    car.stop();
    ThisThread::sleep_for(800ms);

    // turn right
    car.Totalturn(baseSpeed - 20, -1);
    ThisThread::sleep_for(1500ms);
    angle1 -= 8;

    while (1) {
        // stop turning condition
        if (ping1.laserping_cm() < 20) {
            printf("laserping = %f", ping1.laserping_cm());
            // stop turning
            car.stop();
            break;
        }
        car.Totalturn(baseSpeed - 20, -1);
        angle1 -= 0.4;
        ThisThread::sleep_for(10ms);
    }
    printf("(turn right) angle is %f.\n", angle1);
    d1 = ping1;

    car.stop();
    ThisThread::sleep_for(300ms);

    car.Totalturn(baseSpeed - 20, 1);
    ThisThread::sleep_for(1000ms);
    angle1 += 4;

    // turn left
    while (1) {
        if (ping1.laserping_cm() < 20 && angle1 > 0) {
            printf("laserping_com = %f", ping1.laserping_cm());  // stop turning
            car.Totalturn(baseSpeed - 20, -1);
            ThisThread::sleep_for(500ms);
            break;
        }
        car.Totalturn(baseSpeed - 20, 1);
        angle2 += 0.4;
        angle1 += 0.4;
        ThisThread::sleep_for(10ms);
        count++;
    }
    d2 = ping1;

    // printf("%f\n", 2 * d1 * d2 * cos((angle2 + angle1) / 180 * PI));
    distanceBetween = sqrt(d1 * d1 + d2 * d2 -
                           2 * d1 * d2 * cos((angle2 + angle1) / 180 * PI));

    printf("d1 = %f, d2 = %f, angle is %f. distance is %f, count = %d\n", d1,
           d2, angle2 + angle1, distanceBetween, count);

    if (distanceBetween > 15.0) {
        int temp = count / 2;
        // turn right back to the middle
        car.Totalturn(baseSpeed - 20, -1);
        ThisThread::sleep_for(500ms);
        while (temp > 0) {
            car.Totalturn(baseSpeed - 20, -1);
            temp--;
            ThisThread::sleep_for(30ms);
        }
        car.stop();
        ThisThread::sleep_for(200ms);
        car.goStraight(baseSpeed - 20);
        ThisThread::sleep_for(1000ms);
        secondOption = 0;
        printf("finished\n");
    } else {
        secondOption = 1;
    }

    if (secondOption) {
        printf("*********************\n");
        printf("      scondOption\n");
        printf("*********************\n");
        count = 0;
        angle1 = 0;
        angle2 = 0;
        // make sure the car is in stable condition
        car.stop();
        ThisThread::sleep_for(200ms);

        // turn left
        car.Totalturn(baseSpeed - 20, 1);
        ThisThread::sleep_for(1500ms);
        angle1 -= 8;

        while (1) {
            // stop turning condition
            if (ping1.laserping_cm() < 20) {
                // stop turning
                car.stop();
                break;
            }
            car.Totalturn(baseSpeed - 20, 1);
            angle1 -= 0.4;
            ThisThread::sleep_for(10ms);
        }
        printf("(turn left) angle is %f.\n", angle1);
        d1 = ping1;

        car.stop();
        ThisThread::sleep_for(300ms);

        car.Totalturn(baseSpeed - 20, -1);
        ThisThread::sleep_for(1200ms);
        angle1 += 4;

        // turn right
        while (1) {
            if (ping1.laserping_cm() < 20 && angle1 > 0) {
                // stop turning
                car.Totalturn(baseSpeed - 20, 1);
                ThisThread::sleep_for(500ms);
                car.stop();
                break;
            }
            car.Totalturn(baseSpeed - 20, -1);
            angle2 += 0.4;
            angle1 += 0.4;
            ThisThread::sleep_for(10ms);
            count++;
        }
        d2 = ping1;

        // printf("%f\n", 2 * d1 * d2 * cos((angle2 + angle1) / 180 * PI));
        distanceBetween = sqrt(d1 * d1 + d2 * d2 -
                               2 * d1 * d2 * cos((angle2 + angle1) / 180 * PI));

        printf("d1 = %f, d2 = %f, angle is %f. distance is %f\n", d1, d2,
               angle2 + angle1, distanceBetween);

        if (distanceBetween > 15.0) {
            int temp = count / 2;
            // turn left back to the middle
            while (temp > 0) {
                car.Totalturn(baseSpeed - 20, 1);
                temp--;
                ThisThread::sleep_for(25ms);
            }
            car.stop();
            ThisThread::sleep_for(200ms);
            car.goStraight(baseSpeed - 20);
            ThisThread::sleep_for(1000ms);
        } else {
            printf("no option to go\n");
        }
    }
}

int main() {
    parallax_qti qti1(qti_pin);
    int index = 0;
    int temp[16] = {0};
    int pattern;
    int oldpattern[3] = {0, 0, 0};
    length = 0;
    angle_init = car.getAngle1() + car.getAngle0() / 2;
    angle_temp = angle_init;
    measure.start(callback(&measure_queue, &EventQueue::dispatch_forever));

    /*************** initial MQTT ***************/
    wifi = WiFiInterface::get_default_instance();
    if (!wifi) {
        printf("ERROR: No WiFiInterface found.\r\n");
        return -1;
    }

    printf("\nConnecting to %s...\r\n", MBED_CONF_APP_WIFI_SSID);
    int ret =
        wifi->connect(MBED_CONF_APP_WIFI_SSID, MBED_CONF_APP_WIFI_PASSWORD,
                      NSAPI_SECURITY_WPA_WPA2);
    if (ret != 0) {
        printf("\nConnection error: %d\r\n", ret);
        return -1;
    }

    NetworkInterface* net = wifi;
    MQTTNetwork mqttNetwork(net);
    MQTT::Client<MQTTNetwork, Countdown> client(mqttNetwork);

    // TODO: revise host to your IP
    const char* host = "172.20.10.9";
    const int port = 1883;
    printf("Connecting to TCP network...\r\n");
    printf("address is %s/%d\r\n", host, port);

    int rc = mqttNetwork.connect(host, port);  //(host, 1883);
    if (rc != 0) {
        printf("Connection error.");
        return -1;
    }
    printf("Successfully connected!\r\n");

    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 3;
    data.clientID.cstring = "Mbed";

    if ((rc = client.connect(data)) != 0) {
        printf("Fail to connect MQTT\r\n");
    }
    if (client.subscribe(topic, MQTT::QOS0, messageArrived) != 0) {
        printf("Fail to subscribe\r\n");
    }

    mqtt_thread.start(callback(&mqtt_queue, &EventQueue::dispatch_forever));
    // printf("test1");

    mqtt_queue.call(&publish_message, &client);
    mqtt_queue.call_every(7s, &length_measure, &client);
    /*************** initial MQTT ***************/

    car.goStraight(50);

    while (1) {
        pattern = (int)qti1;
        printf("%d\n", pattern);

        switch (pattern) {
            case 0b1000: {
                if (oldpattern[2] == 0b1100) {
                    car.Totalturn_new(baseSpeed + 200, 0.8);
                    ThisThread::sleep_for(30ms);
                } else if (oldpattern[1] == 0b1100) {
                    car.Totalturn_new(baseSpeed + 150, 0.8);
                } else if (oldpattern[0] == 0b1100) {
                    car.Totalturn_new(baseSpeed + 100, 0.8);
                    ThisThread::sleep_for(50ms);
                } else {
                    car.Totalturn_new(baseSpeed, 0.1);
                }
                break;
            }
            case 0b1100: {
                if (oldpattern[2] == 0b1000) {
                    car.Totalturn_new(baseSpeed + 200, -0.8);
                } else if (oldpattern[1] == 0b1000) {
                    car.Totalturn_new(baseSpeed + 200, -0.8);
                } else if (oldpattern[0] == 0b1000) {
                    car.Totalturn_new(baseSpeed + 200, -0.5);
                } else if (oldpattern[2] == 0b0110) {
                    car.Totalturn_new(baseSpeed + 200, -0.5);
                } else if (oldpattern[1] == 0b0110) {
                    car.Totalturn_new(baseSpeed + 100, -0.5);
                } else if (oldpattern[0] == 0b0110) {
                    car.Totalturn_new(baseSpeed, -0.5);
                } else {
                    car.turn(baseSpeed, 0.5);
                    ThisThread::sleep_for(50ms);
                }
                break;
            }
            case 0b1110:
                car.turn(baseSpeed, -0.5);
                break;
            case 0b0100:
                car.turn(baseSpeed, 0.7);
                break;
            case 0b0110:
                car.goStraight(baseSpeed - 20);
                break;
            case 0b0010:
                car.turn(baseSpeed, -0.7);
                break;
            case 0b0111:
                car.turn(baseSpeed, 0.5);
                break;
            case 0b0011: {
                if (oldpattern[2] == 0b0001) {
                    car.Totalturn_new(baseSpeed + 200, 0.8);
                } else if (oldpattern[1] == 0b0001) {
                    car.Totalturn_new(baseSpeed + 200, 0.8);
                } else if (oldpattern[0] == 0b0001) {
                    car.Totalturn_new(baseSpeed + 200, -0.5);
                } else if (oldpattern[2] == 0b0110) {
                    car.goStraight(-(baseSpeed + 200));
                } else if (oldpattern[1] == 0b0110) {
                    car.goStraight(-(baseSpeed + 120));
                } else if (oldpattern[0] == 0b0110) {
                    car.stop();
                } else {
                    car.turn(baseSpeed, -0.5);
                }
                break;
                //  car.turn(50, -0.5);
                //  break;
            }
            case 0b0001: {
                if (oldpattern[2] == 0b0011) {
                    car.Totalturn_new(baseSpeed + 200, -0.8);
                } else if (oldpattern[1] == 0b0011) {
                    car.Totalturn_new(baseSpeed + 100, -0.5);
                } else if (oldpattern[1] == 0b0011) {
                    car.Totalturn_new(baseSpeed + 20, -0.5);
                    ThisThread::sleep_for(50ms);
                } else {
                    car.Totalturn_new(baseSpeed, -0.1);
                }
                break;
            }
            case 0b1111: {
                if (oldpattern[0] == 0b1111 && oldpattern[1] == 0b1111 &&
                    oldpattern[2] == 0b1111) {
                    detect(baseSpeed, 0.8);
                    //  printf("first, cycleA = %d", cycleA);
                }
                // printf("at least\n");

                break;
            }
            case 0b0000: {
                car.goStraight(-baseSpeed);
                // printf("here\n");
                break;
            }
            case 0b1011: {
                car.Totalturn(baseSpeed, -0.5);
                break;
            }
            case 0b1010: {
                car.Totalturn(baseSpeed, -0.5);
                break;
            }
            case 0b1001: {
                car.goStraight(baseSpeed);
                break;
            }
            case 0b0101: {
                car.turn(baseSpeed, 0.5);
                break;
            }
            case 0b1101: {
                car.goStraight(baseSpeed);
                break;
            }
            default: {
                car.goStraight(-10);
            }
        }
        ThisThread::sleep_for(10ms);

        oldpattern[0] = oldpattern[1];
        oldpattern[1] = oldpattern[2];
        oldpattern[2] = pattern;
    }
}
