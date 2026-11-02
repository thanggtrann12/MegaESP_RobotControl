#ifndef HMI_CFG_H
#define HMI_CFG_H

#include <Arduino.h>
#include "wifi_secret.h" // HMI_OTA_WIFI_SSID / HMI_OTA_WIFI_PASSWORD (git-ignored, see wifi_secret.example.h)

#define HMI_UART Serial1 // TJC touchscreen (UART1)
#define HMI_UART_TX 17
#define HMI_UART_RX 18

constexpr uint32_t HMI_UART_BAUD = 115200;

// Screen firmware (.tft) update over WiFi, started by the TFTOTA command.
// The hub joins the router only for the update, downloads the file and streams it to the screen.
constexpr uint32_t HMI_OTA_WIFI_TIMEOUT_MS = 15000;
constexpr uint32_t HMI_OTA_ACK_TIMEOUT_MS = 5000;     // screen answers 0x05 after each block
constexpr uint32_t HMI_OTA_DOWNLOAD_TIMEOUT_MS = 10000; // no data from the server for this long -> abort

#endif // HMI_CFG_H
