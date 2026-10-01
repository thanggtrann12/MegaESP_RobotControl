#include "MecanumKinematics.h"

KinematicsMode MecanumKinematics::getMode() const
{
    return KinematicsMode::MODE_4WD_MECANUM;
}

uint8_t MecanumKinematics::getWheelCount() const
{
    return 4;
}

void MecanumKinematics::computeWheelSpeeds(int16_t throttle,
                                           int16_t strafe,
                                           int16_t rotation,
                                           int16_t *outSpeeds) const
{
    outSpeeds[0] = throttle + strafe + rotation; // Front-left
    outSpeeds[1] = throttle - strafe + rotation; // Rear-left
    outSpeeds[2] = throttle - strafe - rotation; // Front-right
    outSpeeds[3] = throttle + strafe - rotation; // Rear-right
}
