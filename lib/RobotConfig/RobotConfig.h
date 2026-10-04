#ifndef ROBOT_CONFIG_H
#define ROBOT_CONFIG_H

#include <Arduino.h>
#include <Drive.h>

constexpr uint8_t ROBOT_CONFIG_VERSION = 4;

// Everything the HMI can change; one copy lives in RAM and one in EEPROM.
struct __attribute__((__packed__)) RobotConfig
{
    uint8_t version;
    Chassis chassis;
    uint8_t pwmLimit;
    uint8_t accelStep;
    WheelRole role[MOTOR_SLOTS]; // which wheel each motor slot drives
    uint8_t inverted;            // bit n set = motor slot n runs reversed
    Mix custom[WHEEL_COUNT];     // mixing table used when chassis is CUSTOM
    uint8_t crc;

    static RobotConfig defaults();
    bool isValid() const;
    bool load(int address); // returns false and keeps current values if EEPROM is empty or corrupt
    void save(int address);
};

static_assert(sizeof(RobotConfig) == 30, "RobotConfig layout changed: bump ROBOT_CONFIG_VERSION");

#endif
