#include "CapabilityRegistry.h"

CapabilityRegistry::CapabilityRegistry()
    : _kinematics{true, true, false, false, false},
      _drivers{true}
{
}

bool CapabilityRegistry::supportsKinematics(KinematicsMode mode) const
{
    const uint8_t index = static_cast<uint8_t>(mode);
    return index < sizeof(_kinematics) && _kinematics[index];
}

bool CapabilityRegistry::supportsDriver(RobotDriverType driver) const
{
    const uint8_t index = static_cast<uint8_t>(driver);
    return index < sizeof(_drivers) && _drivers[index];
}