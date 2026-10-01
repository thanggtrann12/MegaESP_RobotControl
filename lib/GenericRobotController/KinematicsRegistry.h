#ifndef KINEMATICS_REGISTRY_H
#define KINEMATICS_REGISTRY_H

#include "IKinematics.h"

/**
 * @file KinematicsRegistry.h
 * @brief Registry mapping kinematics modes to kinematics implementations.
 */

// Composition-layer lookup table mapping a KinematicsMode to a pre-built
// IKinematics instance. GenericRobotController itself has no knowledge of
// this registry or of any concrete kinematics class.
class KinematicsRegistry
{
public:
    /** @brief Constructs an empty registry. */
    KinematicsRegistry();
    /**
     * @brief Adds one kinematics implementation to the registry.
     * @param kinematics Kinematics instance to register.
     * @return true when inserted.
     * @return false when registry capacity is full.
     */
    bool add(IKinematics &kinematics);
    /**
     * @brief Finds registered kinematics by mode.
     * @param mode Mode to search.
     * @return Matching kinematics pointer or nullptr.
     */
    IKinematics *find(KinematicsMode mode) const;

private:
    static constexpr uint8_t MAX_ENTRIES = 4;
    IKinematics *_entries[MAX_ENTRIES];
    uint8_t _count;
};

#endif
