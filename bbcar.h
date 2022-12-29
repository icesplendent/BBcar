#ifndef BBCAR_H
#define BBCAR_H
#include "parallax_laserping.h"
#include "parallax_ping.h"
#include "parallax_qti.h"
#include "parallax_servo.h"

class BBCar {
   public:
    BBCar(PwmOut &pinc_servo0, PwmIn &pinf_servo0, PwmOut &pinc_servo1,
          PwmIn &pinf_servo1, Ticker &servo_control_ticker,
          Ticker &servo_feedback_ticker);
    parallax_servo servo0;
    parallax_servo servo1;

    void controlWheel();
    void stop();
    void goStraight(double speed);

    // turn left/right with a factor of speed
    void turn(double speed, double factor);

    // turn totally left/right with a factor of speed
    void Totalturn(double speed, double factor);

    // turn totally left/right with a factor of speed
    void Totalturn_new(double speed, double factor);

    // limit the value by max and min
    float clamp(float value, float max, float min);
    int turn2speed(float turn);

    // feedbackservo
    void feedbackWheel();
    void goCertainDistance(float distance);
    int checkDistance(int errorAngle);

    // get Angles
    float getAngle0();
    float getAngle1();
};

#endif
