## How to run this bbcar project?

> Prepare a black map with route colored in black. Your black route should have width between 1.75cm and 2cm. <a href="https://www.parallax.com/package/printable-tracks-for-line-following/">Ref</a>

### Import Libraries

<ol>
    <li> <a href="https://gitlab.larc-nthu.net/ee2405_2022/erpc_c.git">erpc</a> </li>
    <li> <a href="https://gitlab.larc-nthu.net/ee2405_2022/pwmin.git">PWMIn</a> </li>
    <li> <a href="https://gitlab.larc-nthu.net/ee2405_2022/bbcar.git">bbcar</a> </li>
    <li> <a href="https://github.com/ARMmbed/wifi-ism43362">WIFI (ISM43362)</a> </li>
    <li> <a href="https://gitlab.larc-nthu.net/ee2405_2022/paho_mqtt.git">MQTT</a> </li>
</ol>

### Environmental setup (Software)

<ol>
    <li> Mbed Studio </li>
    <li> Python 3.10.7 </li>
    <li> MQTT broker service </li>
</ol>

### Environmental setup (Hardware)

<ol>
    <li> basic component for bbcar </li>
    <li> QTI sensor </li>
    <li> laser ping (ultrasonic also works) </li>
    <li> B_L4S5I_IOT01A </li>
    <li> Portable charger </li>
</ol>

### Steps

<ol>
    <li>Replace <b>bbcar.h bbcar.cpp</b> with the one in this repo</li>
    <li>Run the program</li>
    <li>Run <b>mqtt_client.py</b> to receive the message from the car</li>
</ol>

### Results

<ol>
    <li>The car will track the route for normal situation</li>
    <li>The car will send info about the distance it had traveled through MQTT to the PC client side</li>
    <li>If the QTI sense a <b>0b1111</b> pattern, it will stop and start to check if the distance between the obstacles is enough to pass. If it's not enough, it will see if the other route is available. If it's available, then it will choose the track and go on. If it's not, then it will print "no option to go"</li>
</ol>

### Future Plan

#### May take advantage of Xbee to manipulate the car remotely @ PC side.

---

## Prerequisite

### <a href="https://randomnerdtutorials.com/what-is-mqtt-and-how-it-works/">MQTT<a> (Message Queue Telemetry Transport)

> Client and server side share the same WIFI

#### MQTT Broker

```
$ brew install mosquitto
```

<ol>
    <li>Open Port 1883</li>
    <li>Set the listener at port 1883</li>
    <li>Allow all connections without authentication</li>
    <li>Start the mosquitto service</li>
    <li>Create <b>mbed_app.json</b> and connet the board to the WIFI</li>
</ol>
    
```
listener 1883
allow_anonymous true
```
    
#### MQTT Client
```
$ python3 -m pip install paho-mqtt
```
<ol>
    <li>Run mqtt_client.py</li>
</ol>
