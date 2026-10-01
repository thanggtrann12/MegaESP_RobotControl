#ifndef KINEMATICS_REGISTRY_H
#define KINEMATICS_REGISTRY_H

#include "IKinematics.h"

// Composition-layer lookup table mapping a KinematicsMode to a pre-built
// IKinematics instance. GenericRobotController itself has no knowledge of
// this registry or of any concrete kinematics class.
class KinematicsRegistry
{
public:
    KinematicsRegistry();
    bool add(IKinematics &kinematics);
    IKinematics *find(KinematicsMode mode) const;

private:
    static constexpr uint8_t MAX_ENTRIES = 4;
    IKinematics *_entries[MAX_ENTRIES];
    uint8_t _count;
};

#endif
