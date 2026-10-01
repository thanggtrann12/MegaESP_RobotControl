#include "DifferentialKinematics.h"

KinematicsMode DifferentialKinematics::getMode() const {
    return KinematicsMode::MODE_2WD_DIFF;
}

uint8_t DifferentialKinematics::getWheelCount() const {
    return 4;
}

void DifferentialKinematics::computeWheelSpeeds(int16_t throttle,
                                                int16_t /*strafe*/,
                                                int16_t rotation,
                                                int16_t *outSpeeds) const {
    if (outSpeeds == nullptr) return;

    const int16_t left = static_cast<int16_t>(throttle + rotation);
    const int16_t right = static_cast<int16_t>(throttle - rotation);

    outSpeeds[0] = left;   // Front-left
    outSpeeds[1] = left;   // Rear-left
    outSpeeds[2] = right;  // Front-right
    outSpeeds[3] = right;  // Rear-right
}