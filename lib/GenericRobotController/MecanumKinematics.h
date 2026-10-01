#ifndef MECANUM_KINEMATICS_H
#define MECANUM_KINEMATICS_H

#include "IKinematics.h"

// Standard 4-wheel mecanum mix. Wheel order expected by computeWheelSpeeds
// is [0]=FrontLeft, [1]=RearLeft, [2]=FrontRight, [3]=RearRight; the
// IMotorOutput wired to this kinematics must use the same physical order.
class MecanumKinematics : public IKinematics
{
public:
    KinematicsMode getMode() const override;
    uint8_t getWheelCount() const override;
    void computeWheelSpeeds(int16_t throttle,
                            int16_t strafe,
                            int16_t rotation,
                            int16_t *outSpeeds) const override;
};

#endif
