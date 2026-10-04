#ifndef DRIVE_H
#define DRIVE_H

#include <Arduino.h>
#include <Motor.h>

// Order matches the TJC chassis list.
// TANK is four-wheel skid steer; OMNI_4 is the X layout, which mixes exactly like mecanum.
enum class Chassis : uint8_t
{
    TWO_WHEEL = 0,
    TANK,
    OMNI_4,
    MECANUM,
    SIX_WHEEL,
    CUSTOM,
    COUNT
};

// Where a motor sits on the chassis.
enum class WheelRole : uint8_t
{
    NONE = 0,
    FRONT_LEFT,
    FRONT_RIGHT,
    MID_LEFT,
    MID_RIGHT,
    REAR_LEFT,
    REAR_RIGHT,
    COUNT
};

constexpr uint8_t WHEEL_COUNT = 6;

// Share of each command a wheel receives, in percent. All zero means the chassis does not use that wheel.
struct Mix
{
    int8_t throttle;
    int8_t strafe;
    int8_t rotation;
};

// Turns throttle/strafe/rotation (-100..100) into per-wheel motor commands for the selected chassis.
class Drive
{
public:
    void assign(WheelRole role, IMotor *motor, bool inverted);
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

    Wheel _wheels[WHEEL_COUNT];
    Mix _mix[WHEEL_COUNT] = {};
    uint8_t _pwmLimit = 255;
    uint8_t _accelStep = 0;
};

#endif
