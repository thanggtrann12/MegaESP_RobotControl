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
    OMNI_3,
    OMNI_4,
    MECANUM,
    SIX_WHEEL,
    HOLONOMIC,
    XDRIVE,
    INDEPENDENT,
    CUSTOM,
    COUNT
};

// Where a motor sits on the chassis.
enum class WheelRole : uint8_t
{
    NONE = 0,
    FRONT_LEFT,
    FRONT_RIGHT,
    REAR_LEFT,
    REAR_RIGHT,
    MID_LEFT,
    MID_RIGHT,
    COUNT
};

inline const char *wheelRoleToString(WheelRole role)
{
    switch (role)
    {
    case WheelRole::NONE:
        return "NONE";
    case WheelRole::FRONT_LEFT:
        return "FRONT_LEFT";
    case WheelRole::FRONT_RIGHT:
        return "FRONT_RIGHT";
    case WheelRole::MID_LEFT:
        return "MID_LEFT";
    case WheelRole::MID_RIGHT:
        return "MID_RIGHT";
    case WheelRole::REAR_LEFT:
        return "REAR_LEFT";
    case WheelRole::REAR_RIGHT:
        return "REAR_RIGHT";
    default:
        return "UNKNOWN";
    }
}

inline const char* chassisToString(Chassis chassis)
{
    switch (chassis)
    {
    case Chassis::TWO_WHEEL:
        return "TWO_WHEEL";
    case Chassis::TANK:
        return "TANK";
    case Chassis::OMNI_3:
        return "OMNI_3";
    case Chassis::OMNI_4:
        return "OMNI_4";
    case Chassis::MECANUM:
        return "MECANUM";
    case Chassis::SIX_WHEEL:
        return "SIX_WHEEL";
    case Chassis::HOLONOMIC:
        return "HOLONOMIC";
    case Chassis::XDRIVE:
        return "XDRIVE";
    case Chassis::INDEPENDENT:
        return "INDEPENDENT";
    case Chassis::CUSTOM:
        return "CUSTOM";
    default:
        return "UNKNOWN";
    }
}

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
