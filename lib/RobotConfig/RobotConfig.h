#ifndef ROBOT_CONFIG_H
#define ROBOT_CONFIG_H

#include <Arduino.h>
#include <Drive.h>

constexpr uint8_t ROBOT_CONFIG_VERSION = 5; // layout đổi: bump version

constexpr uint8_t CHASSIS_COUNT = static_cast<uint8_t>(Chassis::COUNT);

// Gán motor của riêng một chassis
struct __attribute__((__packed__)) ChassisMap
{
    PortRole role[MOTOR_SLOTS];
    uint8_t inverted;
};

// Everything the HMI can change; one copy lives in RAM and one in EEPROM.
struct __attribute__((__packed__)) RobotConfig
{
    uint8_t version;
    Chassis chassis;
    uint8_t pwmLimit;
    uint8_t accelStep;
    PortRole role[MOTOR_SLOTS];      // bản đang dùng của chassis hiện tại
    uint8_t inverted;                // bit n set = motor slot n runs reversed
    Mix custom[MAX_PORT_COUNT];      // mixing table used when chassis is CUSTOM
    ChassisMap saved[CHASSIS_COUNT]; // mỗi chassis một pool
    uint8_t crc;

    static RobotConfig defaults();
    bool isValid() const;
    bool load(int address);
    void save(int address);

    void storeCurrent(); // cất role/inverted đang dùng vào saved[chassis]
    void loadCurrent();  // nạp saved[chassis] vào role/inverted
};

static_assert(sizeof(RobotConfig) == 30 + sizeof(ChassisMap) * CHASSIS_COUNT,
              "RobotConfig layout changed: bump ROBOT_CONFIG_VERSION");

#endif