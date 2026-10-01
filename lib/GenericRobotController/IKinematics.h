#ifndef IKINEMATICS_H
#define IKINEMATICS_H

#include <Arduino.h>
#include "Std_Types.h"

/**
 * @file IKinematics.h
 * @brief Kinematics interface for converting motion intent to wheel speeds.
 */

// Maximum wheel count any IKinematics/IMotorOutput pair can use on this hardware.
constexpr uint8_t MAX_DRIVE_WHEELS = 4;

// Pure motion-mixing strategy: turns a motion intent into per-wheel speeds.
// Implementations do not own motors or apply PWM limits.
class IKinematics
{
public:
    virtual ~IKinematics() = default;
    /**
     * @brief Returns the mode identifier implemented by this kinematics.
     * @return Kinematics mode enum value.
     */
    virtual KinematicsMode getMode() const = 0;
    /**
     * @brief Returns the number of wheels expected by this mixer.
     * @return Wheel count.
     */
    virtual uint8_t getWheelCount() const = 0;
    /**
     * @brief Computes per-wheel speed commands.
     * @param throttle Forward/backward command in motor domain.
     * @param strafe Left/right command in motor domain.
     * @param rotation Rotation command in motor domain.
     * @param outSpeeds Output array receiving wheel speeds.
     */
    virtual void computeWheelSpeeds(int16_t throttle,
                                    int16_t strafe,
                                    int16_t rotation,
                                    int16_t *outSpeeds) const = 0;
};

#endif
