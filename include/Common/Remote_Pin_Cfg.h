#ifndef REMOTE_PIN_CFG_H
#define REMOTE_PIN_CFG_H

#include <Arduino.h>

// =========================================================================
// 1. Cấu hình Chân Analog cho Joysticks (A0 - A3)
// =========================================================================
#define REMOTE_PIN_JOY1_X       A0   // Joystick 1 - Trục X (Trái/Phải di chuyển)
#define REMOTE_PIN_JOY1_Y       A1   // Joystick 1 - Trục Y (Tiến/Lùi di chuyển)
#define REMOTE_PIN_JOY2_X       A2   // Joystick 2 - Trục X (Xoay tay cẩu/Kẹp)
#define REMOTE_PIN_JOY2_Y       A3   // Joystick 2 - Trục Y (Nâng/Hạ tay cẩu)

// =========================================================================
// 2. Cấu hình Chân Digital cho Nút bấm (Single Button)
// =========================================================================
#define REMOTE_PIN_BTN_1        2    // Nút bấm đơn dùng Pin Digital 2 (Dùng Pull-up)

// =========================================================================
// 3. Cấu hình Chân Cổng Serial/UART
// =========================================================================
// Serial1: Giao tiếp với ESP32 Bridge (TX1 = Pin 18, RX1 = Pin 19)
#define REMOTE_UART_ESP32       Serial3  
#define REMOTE_UART_ESP32_BAUD  115200

#endif // REMOTE_PIN_CFG_H