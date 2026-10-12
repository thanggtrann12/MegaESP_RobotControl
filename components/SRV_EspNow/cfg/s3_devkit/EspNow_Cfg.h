#ifndef ESPNOW_CFG_H
#define ESPNOW_CFG_H

#include <Arduino.h>

constexpr uint8_t ESPNOW_CHANNEL = 1;
constexpr uint8_t ESPNOW_QUEUE_DEPTH = 8;

// Replace with the MAC address of the paired remote.
constexpr uint8_t ESPNOW_REMOTE_MAC[6] =
{
    0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
};

#endif // ESPNOW_CFG_H