#ifndef ROBOT_CONFIG_H
#define ROBOT_CONFIG_H

#include <Arduino.h>
#include "Robot_Pin_Cfg.h"
#include "Std_Types.h"

constexpr uint8_t ROBOT_CONFIG_VERSION = 2;

struct __attribute__((__packed__)) RobotConfig
{
    uint8_t version;
    KinematicsMode kinematicsMode;
    RobotDriverType driverType;
    uint8_t motorCount;
    uint8_t pwmLimit;
    MotorRole motorRoles[MAX_MOTOR_PORT];
    uint8_t motorInverted[MAX_MOTOR_PORT];
    MotionProfile motionProfile;
    uint16_t wheelBaseMm;
    uint16_t trackWidthMm;
    uint8_t crc;
};

static_assert(sizeof(RobotConfig) == 23, "RobotConfig wire layout changed");

inline uint8_t CalculateRobotConfigCrc(const RobotConfig &config)
{
    const uint8_t *bytes = reinterpret_cast<const uint8_t *>(&config);
    uint8_t crc = 0;

    for (size_t index = 0; index < sizeof(RobotConfig) - 1; ++index)
    {
        crc ^= bytes[index];
        for (uint8_t bit = 0; bit < 8; ++bit)
        {
            crc = (crc & 0x80) ? static_cast<uint8_t>((crc << 1) ^ 0x07)
                               : static_cast<uint8_t>(crc << 1);
        }
    }
    return crc;
}

inline void UpdateRobotConfigCrc(RobotConfig &config)
{
    config.crc = CalculateRobotConfigCrc(config);
}

inline bool IsRobotConfigCrcValid(const RobotConfig &config)
{
    return config.crc == CalculateRobotConfigCrc(config);
}

inline RobotConfig CreateDefaultRobotConfig()
{
    RobotConfig config{};
    config.version = ROBOT_CONFIG_VERSION;
    config.kinematicsMode = KinematicsMode::MODE_4WD_MECANUM;
    config.driverType = RobotDriverType::TA6586_PCA9685;
    config.motorCount = MAX_MOTOR_PORT;
    config.pwmLimit = 255;
    config.motorRoles[0] = MotorRole::FRONT_LEFT;
    config.motorRoles[1] = MotorRole::REAR_LEFT;
    config.motorRoles[2] = MotorRole::FRONT_RIGHT;
    config.motorRoles[3] = MotorRole::REAR_RIGHT;
    config.motionProfile = MotionProfile::DIRECT;
    config.wheelBaseMm = 200;
    config.trackWidthMm = 200;
    UpdateRobotConfigCrc(config);
    return config;
}

#endif