#ifndef ESPNOW_CFG_H
#define ESPNOW_CFG_H

#include <Arduino.h>

// ESP-NOW only works when both ends sit on the same WiFi channel. The remote must be set to this
// channel too (an ESP8266 bridge that joins a router follows the router's channel instead).
constexpr uint8_t ESPNOW_CHANNEL = 1;

// Packets received by the WiFi task wait here until loop() calls EspNowLink::update().
// 50 Hz remote + a few heartbeats: 8 slots is ~150 ms of backlog.
constexpr uint8_t ESPNOW_QUEUE_DEPTH = 8;

#endif // ESPNOW_CFG_H
