#ifndef CAPABILITY_REGISTRY_H
#define CAPABILITY_REGISTRY_H

#include <Arduino.h>
#include "Std_Types.h"

class CapabilityRegistry
{
public:
    CapabilityRegistry();

    bool supportsKinematics(KinematicsMode mode) const;
    bool supportsDriver(RobotDriverType driver) const;

private:
    bool _kinematics[5];
    bool _drivers[1];
};

#endif