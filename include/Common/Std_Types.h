#ifndef STD_TYPES_H
#define STD_TYPES_H

#include <Arduino.h>

/**
 * @file Std_Types.h
 * @brief Shared basic types, packets, enums, and helpers for control messaging.
 */

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef int16_t int16;

/**
 * @brief Standard control packet transmitted over UART and ESP-NOW.
 */
struct __attribute__((__packed__)) ControlPacket
{
    uint8_t msgType;
    int8_t throttle;
    int8_t strafe;
    int8_t rotation;
    uint8_t buttons;
    uint16_t sequenceNum;
    uint8_t crc8;
};

static_assert(sizeof(ControlPacket) == 8, "ControlPacket wire layout must be 8 bytes");

enum ControlMessageType : uint8_t
{
    CONTROL_MESSAGE = 0x01,
    HEARTBEAT_MESSAGE = 0x02
};

/**
 * @brief Supported kinematics modes for the drive controller.
 */
enum class KinematicsMode : uint8_t
{
    MODE_2WD_DIFF = 0,
    MODE_4WD_MECANUM = 1,
    MODE_OMNI_3WD = 2,
    MODE_OMNI_4WD = 3,
    MODE_CUSTOM = 4
};

/**
 * @brief Active command source used by the robot controller.
 */
enum class ControlSource : uint8_t
{
    REMOTE = 0,
    HMI_MANUAL = 1
};

/**
 * @brief Calculates CRC-8 for a control packet (excluding the crc8 field).
 * @param packet Packet to evaluate.
 * @return CRC-8 value.
 */
inline uint8_t CalculateControlPacketCrc(const ControlPacket &packet)
{
    const uint8_t *bytes = reinterpret_cast<const uint8_t *>(&packet);
    uint8_t crc = 0;

    for (size_t index = 0; index < sizeof(ControlPacket) - 1; ++index)
    {
        crc ^= bytes[index];
        for (uint8_t bit = 0; bit < 8; ++bit)
        {
            crc = (crc & 0x80) ? static_cast<uint8_t>((crc << 1) ^ 0x07)
                               : static_cast<uint8_t>(crc << 1);
        }
    }

    return crc;
}

/**
 * @brief Updates @p packet.crc8 with a newly calculated CRC value.
 * @param packet Packet to update.
 */
inline void UpdateControlPacketCrc(ControlPacket &packet)
{
    packet.crc8 = CalculateControlPacketCrc(packet);
}

/**
 * @brief Verifies whether the packet CRC matches packet payload data.
 * @param packet Packet to validate.
 * @return true if packet CRC is valid.
 * @return false otherwise.
 */
inline bool IsControlPacketValid(const ControlPacket &packet)
{
    return packet.crc8 == CalculateControlPacketCrc(packet);
}

/**
 * @brief Simple pairing packet used during link setup.
 */
struct __attribute__((__packed__)) PairPacket
{
    uint8 cmd;        // 0xA1: REQ, 0xA2: RESP
    uint8 macAddr[6]; // Địa chỉ MAC
};

#endif