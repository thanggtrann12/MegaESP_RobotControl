#ifndef ROBOT_CFG_H
#define ROBOT_CFG_H

#include <Arduino.h>

constexpr int ROBOT_CONFIG_EEPROM_ADDRESS = 0x10; // RobotConfig is stored here (100 bytes)
constexpr uint16_t ROBOT_MOTOR_PWM_HZ = 1000;     // TA6586 inputs are driven at 1 kHz

constexpr uint16_t ROBOT_TICK_MS = 20;            // control loop period
constexpr uint16_t ROBOT_LINK_TIMEOUT_MS = 500;   // no remote packet for this long -> stop
constexpr uint16_t ROBOT_TEST_TIMEOUT_MS = 500;   // single-motor test stops by itself after this

#endif // ROBOT_CFG_H
