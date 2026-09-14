#ifndef ROBOT_MOTOR_TA6586_H
#define ROBOT_MOTOR_TA6586_H

#include <Arduino.h>
#include "GenericRobotController.h"

class MotorTA6586 : public IMotor {
public:
    MotorTA6586(uint8_t forwardPin, uint8_t backwardPin);
    void begin() override;
    void setSpeed(int16_t speed) override;
    void stop() override;

private:
    uint8_t _forwardPin;
    uint8_t _backwardPin;
};

#endif