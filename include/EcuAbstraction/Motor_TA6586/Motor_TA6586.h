#ifndef ROBOT_MOTOR_TA6586_H
#define ROBOT_MOTOR_TA6586_H

#include <Arduino.h>

class MotorTA6586 {
public:
    MotorTA6586(uint8_t forwardPin, uint8_t backwardPin);
    void begin();
    void setSpeed(int16_t speed);
    void stop();

private:
    uint8_t _forwardPin;
    uint8_t _backwardPin;
};

#endif