#ifndef IKINEMATICS_H
#define IKINEMATICS_H

#include <Arduino.h>
#include "Std_Types.h"

// Maximum wheel count any IKinematics/IMotorOutput pair can use on this hardware.
constexpr uint8_t MAX_DRIVE_WHEELS = 4;

// Pure motion-mixing strategy: turns a motion intent into per-wheel speeds.
// Implementations do not own motors or apply PWM limits.
class IKinematics
{
public:
    virtual ~IKinematics() = default;
    virtual KinematicsMode getMode() const = 0;
    virtual uint8_t getWheelCount() const = 0;
    virtual void computeWheelSpeeds(int16_t throttle,
                                    int16_t strafe,
                                    int16_t rotation,
                                    int16_t *outSpeeds) const = 0;
};

#endif
