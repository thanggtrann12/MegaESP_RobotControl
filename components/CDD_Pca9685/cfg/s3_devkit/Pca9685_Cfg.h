#ifndef PCA9685_CFG_H
#define PCA9685_CFG_H

#include <Arduino.h>

#define I2C_SDA_PIN 8 // ESP32-S3 default I2C pins
#define I2C_SCL_PIN 9

constexpr uint32_t I2C_CLOCK_HZ = 400000UL;

constexpr uint8_t MOTOR_PCA_ADDRESS = 0x40;
constexpr uint8_t SERVO_PCA_ADDRESS = 0x41; // own chip: servos need 50 Hz, motors 1 kHz

#endif // PCA9685_CFG_H
