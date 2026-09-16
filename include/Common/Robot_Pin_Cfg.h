#ifndef ROBOT_PIN_CFG_H
#define ROBOT_PIN_CFG_H

#include <Arduino.h>

// =========================================================================
// 1. Cấu hình Channel PCA9685 điều khiển L298N
// =========================================================================
#define ROBOT_PCA9685_ADDRESS   0x40
#define ROBOT_PCA9685_FREQUENCY 1000

// Động cơ trái: ENA, IN1, IN2
#define ROBOT_MOTOR_L_EN        0
#define ROBOT_MOTOR_L_IN1       1
#define ROBOT_MOTOR_L_IN2       2

// Động cơ phải: IN3, IN4, ENB
#define ROBOT_MOTOR_R_IN1       4
#define ROBOT_MOTOR_R_IN2       5
#define ROBOT_MOTOR_R_EN        6

// =========================================================================
// 2. Cấu hình Chân Servo / Cơ cấu Tay cẩu
// =========================================================================
#define ROBOT_PIN_SERVO_ARM     6    // Servo Nâng/Hạ cẩu
#define ROBOT_PIN_SERVO_GRIP    7    // Servo Kẹp/Nhả

// =========================================================================
// 3. Cấu hình Chân Cổng Serial/UART
// =========================================================================
// Serial3: Nhận lệnh từ ESP32 Receiver
// TX3 = Pin 14, RX3 = Pin 15
#define ROBOT_UART_ESP8266        Serial3
#define ROBOT_UART_ESP8266_BAUD   115200

#endif // ROBOT_PIN_CFG_H