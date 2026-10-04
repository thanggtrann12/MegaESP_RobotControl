#ifndef ROBOT_PINS_H
#define ROBOT_PINS_H

#include <Arduino.h>

#define HMI_UART Serial2    // TJC touchscreen
#define REMOTE_UART Serial3 // ESP8266 receiver (ESP-NOW)
constexpr uint32_t UART_BAUD = 115200;

constexpr uint8_t MOTOR_PCA_ADDRESS = 0x40;
constexpr uint16_t MOTOR_PWM_HZ = 1000;
constexpr uint8_t SERVO_PCA_ADDRESS = 0x41; // own chip: servos need 50 Hz, motors 1 kHz

constexpr int CONFIG_EEPROM_ADDRESS = 0x10;

#endif
