#include <ComManager.h>

ComManager::ComManager(HardwareSerial& port) : _serial(&port) {}

void ComManager::Init(uint32 baudrate) {
    _serial->begin(baudrate);
}

uint8 ComManager::CalculateChecksum(const uint8* buffer, size_t size) {
    uint8 checksum = 0;
    for (size_t index = 0; index < size; index++) {
        checksum ^= buffer[index];
    }
    return checksum;
}

void ComManager::SendPacket(const ControlPacket& packet) {
    uint8 checksum = CalculateChecksum(reinterpret_cast<const uint8*>(&packet), sizeof(packet));
    _serial->write(0xAA);
    _serial->write(0xFF);
    _serial->write(reinterpret_cast<const uint8*>(&packet), sizeof(packet));
    _serial->write(checksum);
    _serial->write(0x55);
}

void ComManager::SendCommand(const char* command) {
    _serial->print(command);
    _serial->write('\n');
}

bool ComManager::ReadPacket(ControlPacket& outPacket) {
    constexpr size_t packetSize = sizeof(ControlPacket);
    constexpr size_t frameSize = 2 + packetSize + 1 + 1;
    static uint8 frame[frameSize];
    static uint8 frameIndex = 0;

    while (_serial->available()) {
        uint8 byte = _serial->read();
        if (frameIndex == 0) {
            if (byte == 0xAA) frame[frameIndex++] = byte;
            continue;
        }
        if (frameIndex == 1) {
            if (byte == 0xFF) frame[frameIndex++] = byte;
            else frameIndex = 0;
            continue;
        }

        frame[frameIndex++] = byte;
        if (frameIndex == frameSize) {
            frameIndex = 0;
            if (frame[frameSize - 1] == 0x55 &&
                CalculateChecksum(&frame[2], packetSize) == frame[2 + packetSize]) {
                memcpy(&outPacket, &frame[2], packetSize);
                return true;
            }
        }
    }
    return false;
}