#ifndef COM_MANAGER_H
#define COM_MANAGER_H

#include "Std_Types.h"
#include <HardwareSerial.h>

class ComManager {
private:
    HardwareSerial* _serial;
    
    // Hàm nội bộ tính Checksum XOR cho buffer
    static uint8 CalculateChecksum(const uint8* buffer, size_t size);

public:
    // Khởi tạo truyền vào Serial Port (Serial1, Serial2...)
    ComManager(HardwareSerial& port);
    
    // Khởi tạo Baudrate cho cổng Serial
    void Init(uint32 baudrate = 115200);
    
    // Gửi gói tin ControlPacket qua UART (Có đóng khung 0xAA 0xFF ... Checksum 0x55)
    void SendPacket(const ControlPacket& packet);

    // Gửi lệnh ASCII kết thúc bằng newline qua UART
    void SendCommand(const char* command);
    
    // Nhận và giải mã gói tin từ UART. Trả về true nếu đọc thành công 1 gói hợp lệ.
    bool ReadPacket(ControlPacket& outPacket);

    // Đọc một dòng log ASCII nhận được từ thiết bị bên kia.
    bool ReadLog(char* buffer, size_t bufferSize);
};

#endif // COM_MANAGER_H