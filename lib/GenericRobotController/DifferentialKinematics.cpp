/**
 * @file DifferentialKinematics.cpp
 * @brief Differential kinematics implementation.
 */

#include "DifferentialKinematics.h"

DifferentialKinematics::DifferentialKinematics(uint8_t wheelCount, uint8_t rightSideMask)
    : _wheelCount(wheelCount > MAX_DRIVE_WHEELS ? MAX_DRIVE_WHEELS : wheelCount),
      _rightSideMask(rightSideMask) {}

KinematicsMode DifferentialKinematics::getMode() const
{
    return KinematicsMode::MODE_2WD_DIFF;
}

uint8_t DifferentialKinematics::getWheelCount() const
{
    return _wheelCount;
}

void DifferentialKinematics::computeWheelSpeeds(int16_t throttle,
                                                int16_t /*strafe*/,
                                                int16_t rotation,
                                                int16_t *outSpeeds) const
{
    for (uint8_t index = 0; index < _wheelCount; ++index)
    {
        bool isRightSide = (_rightSideMask & (1 << index)) != 0;
        outSpeeds[index] = isRightSide ? (throttle - rotation) : (throttle + rotation);
    }
}
