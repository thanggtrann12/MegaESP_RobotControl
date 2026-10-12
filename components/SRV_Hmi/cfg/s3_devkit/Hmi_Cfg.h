#ifndef HMI_CFG_H
#define HMI_CFG_H

#include <Arduino.h>

#define HMI_UART Serial1 // TJC touchscreen (UART1)
#define HMI_UART_TX 17
#define HMI_UART_RX 18

constexpr uint32_t HMI_UART_BAUD = 115200;

#endif // HMI_CFG_H
