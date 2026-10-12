#ifndef DRIVE_H
#define DRIVE_H

#include <Arduino.h>
#include <Motor.h>
#include "Chassis.h"


// Turns throttle/strafe/rotation (-100..100) into per-wheel motor commands for the selected chassis.
class Drive
{
public:
    void assign(PortRole role, IMotor *motor, bool inverted);
    void clearMotors();
    void setChassis(Chassis chassis, const Mix *custom); // custom is only read for Chassis::CUSTOM
    void setPwmLimit(uint8_t limit) { _pwmLimit = limit; }
    void setAccelStep(uint8_t step) { _accelStep = step; } // max wheel speed change per move(), 0 = off

    bool ready() const; // every wheel the chassis uses has a motor
    void move(int8_t throttle, int8_t strafe, int8_t rotation);
    void stop();

private:
    struct Wheel
    {
        IMotor *motor = nullptr;
        bool inverted = false;
        int16_t last = 0;
    };
    Chassis _chassis = Chassis::MECANUM;
    Wheel _wheels[MAX_PORT_COUNT];
    Mix _mix[MAX_PORT_COUNT] = {};
    uint8_t _pwmLimit = 255;
    uint8_t _accelStep = 0;
};

#endif
