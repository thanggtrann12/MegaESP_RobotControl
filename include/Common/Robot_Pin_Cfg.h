#ifndef ROBOT_PIN_CFG_H
#define ROBOT_PIN_CFG_H

#include <Arduino.h>

// =========================================================================
// 1. Cấu hình Chân Điều khiển IC Động cơ TA6586 (Cần chân PWM)
// =========================================================================
// Động cơ Trái (Left Motor)
#define ROBOT_PIN_MOTOR_L_FI    2    // Forward Input (PWM)
#define ROBOT_PIN_MOTOR_L_BI    3    // Backward Input (PWM)

// Động cơ Phải (Right Motor)
#define ROBOT_PIN_MOTOR_R_FI    4    // Forward Input (PWM)
#define ROBOT_PIN_MOTOR_R_BI    5    // Backward Input (PWM)

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