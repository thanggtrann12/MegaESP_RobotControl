#ifndef CRC_H
#define CRC_H

#include <cstdint>
#include <cstddef>

/**
 * @file Crc.h
 * @brief Shared CRC-8: polynomial 0x07, init 0x00, byte-wise, no reflection.
 *
 * Single implementation used by:
 *  - ControlPacket (ESP-NOW / UART control messages)
 *  - RobotConfig EEPROM store (SWC_Robot)
 *  - DigitalIo EEPROM store (CDD_Gpio)
 *
 * Header-only and Arduino-free so host unit tests can include it directly.
 */
inline uint8_t crc8(const uint8_t *data, size_t length)
{
    uint8_t crc = 0;
    for (size_t i = 0; i < length; ++i)
    {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; ++bit)
        {
            crc = (crc & 0x80) ? static_cast<uint8_t>((crc << 1) ^ 0x07)
                               : static_cast<uint8_t>(crc << 1);
        }
    }
    return crc;
}

#endif // CRC_H
