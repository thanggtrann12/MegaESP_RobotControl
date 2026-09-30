#ifndef ROBOT_PIN_CFG_H
#define ROBOT_PIN_CFG_H

#include <Arduino.h>

// =========================================================================
// 1. Cấu hình Channel PCA9685 điều khiển TA6586
// =========================================================================
#define ROBOT_PCA9685_ADDRESS 0x40
#define ROBOT_PCA9685_FREQUENCY 1000

// Mỗi TA6586 dùng hai channel liền kề: Bi ở channel chẵn, Fi ở channel lẻ.
// Hai motor cùng bên nhận chung tốc độ.
#define ROBOT_MOTOR_L1_BI 0
#define ROBOT_MOTOR_L1_FI 1
#define ROBOT_MOTOR_L2_BI 2
#define ROBOT_MOTOR_L2_FI 3
#define ROBOT_MOTOR_R1_BI 4
#define ROBOT_MOTOR_R1_FI 5
#define ROBOT_MOTOR_R2_BI 6
#define ROBOT_MOTOR_R2_FI 7

// =========================================================================
// 2. Cấu hình Chân Servo / Cơ cấu Tay cẩu
// =========================================================================
// #define ROBOT_PIN_SERVO_ARM 6  // Servo Nâng/Hạ cẩu
// #define ROBOT_PIN_SERVO_GRIP 7 // Servo Kẹp/Nhả

// =========================================================================
// 3. Cấu hình Chân Cổng Serial/UART
// =========================================================================
// Serial3: Nhận lệnh từ ESP32 Receiver
// TX3 = Pin 14, RX3 = Pin 15
#define ROBOT_UART_ESP8266 Serial3
#define ROBOT_UART_ESP8266_BAUD 115200

#define ROBOT_UART_HMI Serial2
#define ROBOT_UART_HMI_BAUD 115200

static const uint8_t DIGITAL_PINS[8] = {39, 41, 43, 45, 47, 49, 51, 53};
static const uint8_t ANALOG_PINS[6] = {A0, A1, A2, A3, A4, A5};

#endif // ROBOT_PIN_CFG_H