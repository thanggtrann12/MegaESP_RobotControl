/// @brief This is PUBLIC INTERFACE used for Pca9685 (servo chip) / ServoOut

#ifndef CDD_PCA9685_H
#define CDD_PCA9685_H

#include <Arduino.h>
#include "Pca9685.h"
#include "ServoOut.h"
#include "Pca9685_Cfg.h" // I2C_SDA_PIN, I2C_SCL_PIN, I2C_CLOCK_HZ, *_PCA_ADDRESS

// Số kênh PWM của PCA9685
#define CDD_SERVO_CHANNEL_COUNT 16

// Instance PCA9685 dành cho servo (địa chỉ SERVO_PCA_ADDRESS, chạy 50 Hz).
// Dùng để chia sẻ đúng 1 object cho module khác (ví dụ Robot), tránh tạo 2 object cho cùng 1 chip.
Pca9685 &CDD_Pca9685_GetServoPca();

// Instance ServoOut gắn trên chip servo ở trên
ServoOut &CDD_Servo_GetInstance();

// Các hàm API công khai cho User Code
void CDD_Servo_Begin(void);                           // khởi tạo chip ở 50 Hz (Robot::begin() đã làm việc này)
void CDD_Servo_Write(uint8_t channel, uint8_t angle); // angle: 0 đến 180 độ, channel: 0 đến 15
void CDD_Servo_Release(uint8_t channel);              // nhả servo (ngừng cấp xung)
void CDD_Servo_ReleaseAll(void);

#endif // CDD_PCA9685_H