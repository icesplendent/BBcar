#include "bbcar.h"

BBCar::BBCar(PwmOut &pinc_servo0, PwmIn &pinf_servo0, PwmOut &pinc_servo1,
             PwmIn &pinf_servo1, Ticker &servo_control_ticker,
             Ticker &servo_feedback_ticker)
    : servo0(pinc_servo0, pinf_servo0), servo1(pinc_servo1, pinf_servo1) {
    servo0.set_speed(0);
    servo1.set_speed(0);
    servo_control_ticker.attach(callback(this, &BBCar::controlWheel), 20ms);
    servo_feedback_ticker.attach(callback(this, &BBCar::feedbackWheel), 5ms);
}

void BBCar::controlWheel() {
    servo0.control();
    servo1.control();
    servo0.control360();
    servo1.control360();
}

void BBCar::stop() {
    servo0.set_factor(1);
    servo1.set_factor(1);
    servo0.set_speed(0);
    servo1.set_speed(0);
    servo0.flag_control360 = 0;
    servo1.flag_control360 = 0;
}

void BBCar::goStraight(double speed) {
    servo0.set_factor(1);
    servo1.set_factor(1);
    servo0.set_speed(speed);
    servo1.set_speed(-speed);
}

/*	speed : speed value of servo
    factor: control the speed value with 0~1
            control left/right turn with +/-
*/
void BBCar::Totalturn(double speed, double factor) {
    if (factor > 0) {
        servo0.set_factor(factor);
        servo1.set_factor(1);
        servo0.set_speed(-speed - 5);
        servo1.set_speed(-speed);
    } else if (factor < 0) {
        servo0.set_factor(1);
        servo1.set_factor(-factor);
        servo0.set_speed(speed);
        servo1.set_speed(speed + 5);
    }
}

void BBCar::Totalturn_new(double speed, double factor) {
    if (factor > 0) {
        servo0.set_factor(factor);
        servo1.set_factor(1);
        servo0.set_speed(-speed);
        servo1.set_speed(-speed);
    } else if (factor < 0) {
        servo0.set_factor(1);
        servo1.set_factor(-factor);
        servo0.set_speed(speed);
        servo1.set_speed(speed);
    }
}

void BBCar::turn(double speed, double factor) {
    if (factor > 0) {
        servo0.set_factor(factor);
        servo1.set_factor(1);
    } else if (factor < 0) {
        servo0.set_factor(1);
        servo1.set_factor(-factor);
    }
    servo0.set_speed(speed);
    servo1.set_speed(-speed);
}

float BBCar::clamp(float value, float max, float min) {
    if (value > max)
        return max;
    else if (value < min)
        return min;
    else
        return value;
}

int BBCar::turn2speed(float turn) { return 25 + abs(25 * turn); }

void BBCar::feedbackWheel() {
    servo0.feedback360();
    servo1.feedback360();
}

void BBCar::goCertainDistance(float distance) {
    servo0.targetAngle = (int)(distance * 360 / (6.5 * 3.14)) + servo0.angle;
    servo1.targetAngle = (int)(-distance * 360 / (6.5 * 3.14)) + servo1.angle;

    servo0.flag_control360 = 1;
    servo1.flag_control360 = 1;
}

int BBCar::checkDistance(int errorAngle) {
    if (abs(servo0.targetAngle - servo0.angle) > errorAngle)
        return 1;
    else
        return 0;
}

float BBCar::getAngle0() { return servo0.angle; }

float BBCar::getAngle1() { return servo1.angle; }