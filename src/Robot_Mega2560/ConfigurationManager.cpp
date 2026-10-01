#include "ConfigurationManager.h"
#include <EEPROM.h>
#include <string.h>

ConfigurationManager::ConfigurationManager(const CapabilityRegistry &capabilities, int address)
    : _capabilities(capabilities),
      _address(address),
      _active(CreateDefaultRobotConfig()),
    _staged(_active),
    _lastSaved(_active),
    _writesThisBoot(0),
    _hasSavedConfig(false)
{
}

bool ConfigurationManager::validate(const RobotConfig &config) const
{
    if (config.version != ROBOT_CONFIG_VERSION ||
        config.motorCount == 0 || config.motorCount > MAX_MOTOR_PORT ||
        !_capabilities.supportsDriver(config.driverType) ||
        !_capabilities.supportsKinematics(config.kinematicsMode) ||
        !IsRobotConfigCrcValid(config))
    {
        return false;
    }

    bool hasFrontLeft = false;
    bool hasRearLeft = false;
    bool hasFrontRight = false;
    bool hasRearRight = false;
    bool hasLeftMotor = false;
    bool hasRightMotor = false;
    bool seenRoles[static_cast<uint8_t>(MotorRole::INVALID)] = {false};
    for (uint8_t index = 0; index < config.motorCount; ++index)
    {
        const uint8_t roleIndex = static_cast<uint8_t>(config.motorRoles[index]);
        if (roleIndex >= static_cast<uint8_t>(MotorRole::INVALID))
        {
            return false;
        }
        if (roleIndex != static_cast<uint8_t>(MotorRole::UNBOUND) && seenRoles[roleIndex])
        {
            return false;
        }
        seenRoles[roleIndex] = true;

        switch (config.motorRoles[index])
        {
        case MotorRole::FRONT_LEFT:
            hasFrontLeft = true;
            break;
        case MotorRole::REAR_LEFT:
            hasRearLeft = true;
            break;
        case MotorRole::FRONT_RIGHT:
            hasFrontRight = true;
            break;
        case MotorRole::REAR_RIGHT:
            hasRearRight = true;
            break;
        case MotorRole::LEFT_MOTOR:
            hasLeftMotor = true;
            break;
        case MotorRole::RIGHT_MOTOR:
            hasRightMotor = true;
            break;
        default:
            break;
        }
    }

    if (config.kinematicsMode == KinematicsMode::MODE_2WD_DIFF)
    {
        return hasLeftMotor && hasRightMotor;
    }

    return hasFrontLeft && hasRearLeft && hasFrontRight && hasRearRight;
}

bool ConfigurationManager::load()
{
    RobotConfig candidate;
    EEPROM.get(_address, candidate);
    if (!validate(candidate))
    {
        resetToDefaults();
        return false;
    }

    _active = candidate;
    _staged = candidate;
    _lastSaved = candidate;
    _hasSavedConfig = true;
    return true;
}

bool ConfigurationManager::save()
{
    if (!validate(_active))
    {
        return false;
    }

    if (_hasSavedConfig && memcmp(&_active, &_lastSaved, sizeof(RobotConfig)) == 0)
    {
        return true;
    }
    if (_writesThisBoot >= MAX_EEPROM_WRITES_PER_BOOT)
    {
        return false;
    }

    EEPROM.put(_address, _active);
    _lastSaved = _active;
    _hasSavedConfig = true;
    ++_writesThisBoot;
    return true;
}

void ConfigurationManager::resetToDefaults()
{
    _active = CreateDefaultRobotConfig();
    _staged = _active;
    _hasSavedConfig = false;
}

void ConfigurationManager::beginStage()
{
    _staged = _active;
}

void ConfigurationManager::abortStage()
{
    _staged = _active;
}

bool ConfigurationManager::updateStaged(const RobotConfig &config)
{
    _staged = config;
    return true;
}

bool ConfigurationManager::apply()
{
    if (!validate(_staged))
    {
        return false;
    }

    _active = _staged;
    return true;
}