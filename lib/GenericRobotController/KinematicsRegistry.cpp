#include "KinematicsRegistry.h"

KinematicsRegistry::KinematicsRegistry() : _count(0)
{
    for (uint8_t index = 0; index < MAX_ENTRIES; ++index)
    {
        _entries[index] = nullptr;
    }
}

bool KinematicsRegistry::add(IKinematics &kinematics)
{
    if (_count >= MAX_ENTRIES)
    {
        return false;
    }
    _entries[_count++] = &kinematics;
    return true;
}

IKinematics *KinematicsRegistry::find(KinematicsMode mode) const
{
    for (uint8_t index = 0; index < _count; ++index)
    {
        if (_entries[index]->getMode() == mode)
        {
            return _entries[index];
        }
    }
    return nullptr;
}
