#ifndef MECANUM_KINEMATICS_H
#define MECANUM_KINEMATICS_H

#include "IKinematics.h"

/**
 * @file MecanumKinematics.h
 * @brief 4-wheel mecanum wheel speed mixing implementation.
 */

// Standard 4-wheel mecanum mix. Wheel order expected by computeWheelSpeeds
// is [0]=FrontLeft, [1]=RearLeft, [2]=FrontRight, [3]=RearRight; the
// IMotorOutput wired to this kinematics must use the same physical order.
class MecanumKinematics : public IKinematics
{
public:
    /** @brief Gets the implemented kinematics mode id. */
    KinematicsMode getMode() const override;
    /** @brief Returns fixed wheel count for mecanum layout. */
    uint8_t getWheelCount() const override;
    /**
     * @brief Computes standard mecanum wheel speeds.
     * @param throttle Forward/backward command.
     * @param strafe Left/right command.
     * @param rotation Rotation command.
     * @param outSpeeds Output wheel speed buffer in fixed wheel order.
     */
    void computeWheelSpeeds(int16_t throttle,
                            int16_t strafe,
                            int16_t rotation,
                            int16_t *outSpeeds) const override;
};

#endif
