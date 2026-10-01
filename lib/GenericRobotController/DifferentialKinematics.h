#ifndef DIFFERENTIAL_KINEMATICS_H
#define DIFFERENTIAL_KINEMATICS_H

#include "IKinematics.h"

// Differential mixing: throttle +/- rotation, strafe ignored.
// rightSideMask bit i (1-based wheel index i-1) marks that wheel as the
// "right" side; all unmarked wheels among wheelCount are treated as "left".
// This supports both a plain 2-wheel robot and a 4-wheel chassis emulating
// differential drive by duplicating left/right pairs.
class DifferentialKinematics : public IKinematics
{
public:
    DifferentialKinematics(uint8_t wheelCount, uint8_t rightSideMask);
    KinematicsMode getMode() const override;
    uint8_t getWheelCount() const override;
    void computeWheelSpeeds(int16_t throttle,
                            int16_t strafe,
                            int16_t rotation,
                            int16_t *outSpeeds) const override;

private:
    uint8_t _wheelCount;
    uint8_t _rightSideMask;
};

#endif
