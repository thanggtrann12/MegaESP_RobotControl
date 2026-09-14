#include "ComManager.h"

namespace {
char receivedLog[96];
size_t receivedLogLength = 0;
bool receivedLogReady = false;
}

ComManager::ComManager(HardwareSerial& port) : _serial(&port) {}

void ComManager::Init(uint32 baudrate) {
    _serial->begin(baudrate);
}

uint8 ComManager::CalculateChecksum(const uint8* buffer, size_t size) {
    uint8 crc = 0;
    for (size_t i = 0; i < size; i++) {
        crc ^= buffer[i];
    }
    return crc;
}

void ComManager::SendPacket(const ControlPacket& packet) {
    uint8 checksum = CalculateChecksum((const uint8*)&packet, sizeof(ControlPacket));

    _serial->write(0xAA);
    _serial->write(0xFF);
    _serial->write((const uint8*)&packet, sizeof(ControlPacket));
    _serial->write(checksum);
    _serial->write(0x55);
}

void ComManager::SendCommand(const char* command) {
    _serial->print(command);
    _serial->write('\n');
}

bool ComManager::ReadPacket(ControlPacket& outPacket) {
    constexpr size_t PACKET_SIZE = sizeof(ControlPacket);
    constexpr size_t FRAME_SIZE = 2 + PACKET_SIZE + 1 + 1;

    static uint8 rxBuffer[FRAME_SIZE];
    static uint8 rxIndex = 0;

    while (_serial->available()) {
        uint8 byte = _serial->read();

        if (rxIndex == 0) {
            if (byte == 0xAA) {
                rxBuffer[rxIndex++] = byte;
            } else if (byte == '\n' || byte == '\r') {
                if (receivedLogLength > 0) {
                    receivedLog[receivedLogLength] = '\0';
                    receivedLogReady = true;
                    receivedLogLength = 0;
                }
            } else if (byte >= 32 && byte <= 126 &&
                       receivedLogLength < sizeof(receivedLog) - 1) {
                receivedLog[receivedLogLength++] = static_cast<char>(byte);
            }
            continue;
        }

        if (rxIndex == 1) {
            if (byte == 0xFF) {
                rxBuffer[rxIndex++] = byte;
            } else {
                rxIndex = 0;
            }
            continue;
        }

        rxBuffer[rxIndex++] = byte;
        if (rxIndex >= FRAME_SIZE) {
            rxIndex = 0;

            if (rxBuffer[FRAME_SIZE - 1] == 0x55) {
                uint8 calculatedCrc = CalculateChecksum(&rxBuffer[2], PACKET_SIZE);
                uint8 receivedCrc = rxBuffer[2 + PACKET_SIZE];

                if (calculatedCrc == receivedCrc) {
                    memcpy(&outPacket, &rxBuffer[2], PACKET_SIZE);
                    return true;
                }
            }
        }
    }

    return false;
}

bool ComManager::ReadLog(char* buffer, size_t bufferSize) {
    if (!receivedLogReady || buffer == nullptr || bufferSize == 0) {
        return false;
    }

    strncpy(buffer, receivedLog, bufferSize - 1);
    buffer[bufferSize - 1] = '\0';
    receivedLogReady = false;
    return true;
}