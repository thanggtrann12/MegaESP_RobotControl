#ifndef CHASSIS_H
#define CHASSIS_H

#include <Arduino.h>

// Order matches the TJC chassis list.
// TANK is four-wheel skid steer; OMNI_4 is the X layout, which mixes exactly like mecanum.
constexpr uint8_t MAX_PORT_COUNT = 6;

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
enum class PortRole : uint8_t
{
    NONE = 0,
    DRIVE_FL,
    DRIVE_FR,
    DRIVE_RL,
    DRIVE_RR,
    DRIVE_ML,
    DRIVE_MR,

    AUXILITARY,
    COUNT
};

inline const char *PortRoleToString(PortRole role)
{
    switch (role)
    {
    case PortRole::NONE:
        return "NONE";
    case PortRole::DRIVE_FL:
        return "DRIVE_FL";
    case PortRole::DRIVE_FR:
        return "DRIVE_FR";
    case PortRole::DRIVE_ML:
        return "DRIVE_ML";
    case PortRole::DRIVE_MR:
        return "DRIVE_MR";
    case PortRole::DRIVE_RL:
        return "DRIVE_RL";
    case PortRole::DRIVE_RR:
        return "DRIVE_RR";
    case PortRole::AUXILITARY:
        return "AUXILITARY";
    default:
        return "UNKNOWN";
    }
}

inline const char *chassisToString(Chassis chassis)
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

inline uint8_t chassisToWheelCount(Chassis chassis)
{
    switch (chassis)
    {
    case Chassis::TWO_WHEEL:
        return 2;
    case Chassis::TANK:
        return 4;
    case Chassis::OMNI_3:
        return 3;
    case Chassis::OMNI_4:
        return 4;
    case Chassis::MECANUM:
        return 4;
    case Chassis::SIX_WHEEL:
        return 6;
    case Chassis::HOLONOMIC:
        return 4;
    case Chassis::XDRIVE:
        return 4;
    case Chassis::INDEPENDENT:
        return 6;
    case Chassis::CUSTOM:
        return MAX_PORT_COUNT;
    default:
        return 0;
    }
}

// Share of each command a wheel receives, in percent. All zero means the chassis does not use that wheel.
struct Mix
{
    int8_t throttle;
    int8_t strafe;
    int8_t rotation;
};

// Which wheel roles a chassis actually uses. Keep in sync with the presets in Drive.cpp.
// NOTE: TANK is documented above as four-wheel, but only FL/FR are accepted here and in
// Hmi::labelCount(). Decide which one is right and change all three places together.
inline bool roleUsedByChassis(Chassis chassis, PortRole role)
{
    switch (chassis)
    {
    case Chassis::TWO_WHEEL:
    case Chassis::TANK:
        return role == PortRole::DRIVE_FL || role == PortRole::DRIVE_FR;

    case Chassis::OMNI_3:
        return role == PortRole::DRIVE_FL || role == PortRole::DRIVE_FR ||
               role == PortRole::DRIVE_RL;

    case Chassis::OMNI_4:
    case Chassis::MECANUM:
    case Chassis::HOLONOMIC:
    case Chassis::XDRIVE:
        return role == PortRole::DRIVE_FL || role == PortRole::DRIVE_FR ||
               role == PortRole::DRIVE_RL || role == PortRole::DRIVE_RR;

    case Chassis::SIX_WHEEL:
    case Chassis::CUSTOM:
        return role != PortRole::NONE;

    default:
        return false;
    }
}

#endif
