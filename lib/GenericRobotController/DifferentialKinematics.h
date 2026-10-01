#ifndef DIFFERENTIAL_KINEMATICS_H
#define DIFFERENTIAL_KINEMATICS_H

#include "IKinematics.h"

/**
 * @file DifferentialKinematics.h
 * @brief Differential-drive wheel mixing strategy implementation.
 */

// Differential mixing: throttle +/- rotation, strafe ignored.
// rightSideMask bit i (1-based wheel index i-1) marks that wheel as the
// "right" side; all unmarked wheels among wheelCount are treated as "left".
// This supports both a plain 2-wheel robot and a 4-wheel chassis emulating
// differential drive by duplicating left/right pairs.
class DifferentialKinematics : public IKinematics
{
public:
    /**
     * @brief Constructs differential kinematics with wheel topology metadata.
     * @param wheelCount Number of wheels used by this configuration.
     * @param rightSideMask Bit mask marking right-side wheel indices.
     */
    DifferentialKinematics(uint8_t wheelCount, uint8_t rightSideMask);
    /** @brief Gets the implemented kinematics mode id. */
    KinematicsMode getMode() const override;
    /** @brief Gets configured wheel count. */
    uint8_t getWheelCount() const override;
    /**
     * @brief Computes differential wheel speeds from throttle/rotation commands.
     * @param throttle Forward/backward command.
     * @param strafe Unused for differential mode.
     * @param rotation Turn command.
     * @param outSpeeds Output wheel speed buffer.
     */
    void computeWheelSpeeds(int16_t throttle,
                            int16_t strafe,
                            int16_t rotation,
                            int16_t *outSpeeds) const override;

private:
    uint8_t _wheelCount;
    uint8_t _rightSideMask;
};

#endif
