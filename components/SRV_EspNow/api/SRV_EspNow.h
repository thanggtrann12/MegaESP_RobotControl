/// @brief This is PUBLIC INTERFACE used for EspNowLink (ESP-NOW remote link)

#ifndef SRV_ESPNOW_H
#define SRV_ESPNOW_H

#include <Arduino.h>
#include "ControlPacket.h"

// Handler được gọi từ loop() (packet đã qua queue, không bao giờ chạy trong WiFi task).
typedef void (*SRV_EspNow_Handler)(const ControlPacket &packet, void *context);

// Các hàm API công khai cho User Code
bool SRV_EspNow_Begin(SRV_EspNow_Handler handler, void *context); // khởi động WiFi STA + ESP-NOW
void SRV_EspNow_Update(void); // gọi trong loop(): chuyển packet đã nhận cho handler

#endif // SRV_ESPNOW_H
