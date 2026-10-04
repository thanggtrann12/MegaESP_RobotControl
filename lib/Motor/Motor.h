#ifndef MOTOR_H
#define MOTOR_H

#include <Arduino.h>

constexpr uint8_t MOTOR_SLOTS = 6; // M0..M5

// A motor only knows how to run, brake or release; it has no idea what role it plays.
class IMotor
{
public:
    virtual ~IMotor() = default;
    virtual void run(int16_t pwm) = 0; // -255..255, sign selects direction
    virtual void brake() = 0;
    virtual void stop() = 0; // release (coast)
};

#endif
