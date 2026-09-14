#ifndef STD_TYPES_H
#define STD_TYPES_H

#include <Arduino.h>

typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef int16_t  int16;

// Gói tin chuẩn gửi qua UART & ESP-NOW
struct __attribute__((__packed__)) ControlPacket {
    int16 joy1_x;
    int16 joy1_y;
    int16 joy2_x;
    int16 joy2_y;
    uint16 buttons;
};

// Gói tin Ghép nối (Pairing)
struct __attribute__((__packed__)) PairPacket {
    uint8 cmd;        // 0xA1: REQ, 0xA2: RESP
    uint8 macAddr[6]; // Địa chỉ MAC
};

#endif