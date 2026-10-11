#ifndef COM_MANAGER_H
#define COM_MANAGER_H

#include "Std_Types.h"
#include <HardwareSerial.h>

/**
 * @file ComManager.h
 * @brief UART framed packet and command communication interface.
 */

/**
 * @class ComManager
 * @brief Handles UART communication for framed control packets and text commands.
 *
 * Binary packet frame format:
 * - Header: 0xAA 0xFF
 * - Payload: ControlPacket bytes
 * - Checksum: XOR of payload bytes
 * - Footer: 0x55
 */
class ComManager {
private:
    HardwareSerial* _serial;

    /**
     * @brief Computes XOR checksum for a byte buffer.
     * @param buffer Pointer to input bytes.
     * @param size Number of bytes to include.
     * @return XOR checksum value.
     */
    static uint8 CalculateChecksum(const uint8* buffer, size_t size);

public:
    /**
     * @brief Creates a communication manager bound to a hardware serial port.
     * @param port UART interface (for example Serial1 or Serial2).
     */
    ComManager(HardwareSerial& port);

    /**
     * @brief Initializes the UART port at the specified baudrate.
     * @param baudrate UART baudrate. Defaults to 115200.
     */
    void Init(uint32 baudrate = 115200);

    /**
     * @brief Sends one framed control packet over UART.
     * @param packet Control payload to transmit.
     */
    void SendPacket(const ControlPacket& packet);

    /**
     * @brief Sends an ASCII command terminated by a newline character.
     * @param command Null-terminated command string.
     */
    void SendCommand(const char* command);

    /**
     * @brief Reads and decodes one framed control packet from UART.
     * @param outPacket Output packet structure populated on success.
     * @return true if a valid packet is decoded and passes validation.
     * @return false if no complete valid packet is available.
     */
    bool ReadPacket(ControlPacket& outPacket);

    /**
     * @brief Reads one received ASCII log line, if available.
     * @param buffer Destination buffer for a null-terminated log string.
     * @param bufferSize Size of destination buffer in bytes.
     * @return true if a log line was copied to @p buffer.
     * @return false if no log line is ready or input arguments are invalid.
     */
    bool ReadLog(char* buffer, size_t bufferSize);
};

#endif // COM_MANAGER_H