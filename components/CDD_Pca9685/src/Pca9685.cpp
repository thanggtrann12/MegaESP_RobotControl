#include "Pca9685.h"

namespace
{
    constexpr uint8_t MODE1 = 0x00;
    constexpr uint8_t MODE2 = 0x01;
    constexpr uint8_t PRESCALE = 0xFE;
    constexpr uint8_t LED0_ON_L = 0x06;
    constexpr uint8_t MODE1_SLEEP = 0x10;
    constexpr uint8_t MODE1_AUTO_INC = 0x20;
    constexpr uint8_t MODE1_RESTART = 0x80;
    constexpr uint8_t MODE2_OUTDRV = 0x04;
    constexpr uint16_t FULL_ON_OFF = 0x1000; // Bit 4 của byte cao (kích hoạt chế độ ON/OFF hoàn toàn)
    constexpr uint32_t OSCILLATOR_HZ = 25000000UL;
    constexpr uint32_t I2C_SPEED = 400000UL; // 400kHz Fast Mode để tối ưu tốc độ truyền I2C
}

void Pca9685::begin(uint16_t frequencyHz)
{
    _wire->begin();
    _wire->setClock(I2C_SPEED); 

    // Đưa chip về trạng thái cấu hình
    write(MODE2, MODE2_OUTDRV);
    write(MODE1, MODE1_SLEEP | MODE1_AUTO_INC); // Phải đưa chip vào Sleep mới thay đổi được thanh ghi PRESCALE
    
    // Tính toán hệ số prescale dựa trên tần số yêu cầu (frequencyHz)
    uint32_t prescaleVal = (OSCILLATOR_HZ + 2048UL * frequencyHz) / (4096UL * frequencyHz) - 1;
    write(PRESCALE, static_cast<uint8_t>(prescaleVal));
    
    write(MODE1, MODE1_AUTO_INC); // Thoát chế độ sleep và bật Auto-Increment cho thanh ghi
    delay(1);
    write(MODE1, MODE1_AUTO_INC | MODE1_RESTART);

    // Reset lại cache khi khởi động lại chip
    for (uint8_t i = 0; i < 16; ++i)
    {
        _cachedTicks[i] = 0xFFFF;
    }
}

void Pca9685::setDuty(uint8_t channel, uint16_t ticks)
{
    if (channel > 15)
    {
        return;
    }

    // Giới hạn giá trị ticks trong khoảng cho phép của PCA9685 (0 - 4095)
    if (ticks > 4095)
    {
        ticks = 4095;
    }

    // Tối ưu hóa: Nếu giá trị ticks gửi vào trùng với giá trị đã cache, bỏ qua để tránh nghẽn bus I2C
    if (_cachedTicks[channel] == ticks)
    {
        return;
    }
    _cachedTicks[channel] = ticks;

    uint16_t on = 0;
    uint16_t off = ticks;
    if (ticks == 0)
    {
        off = FULL_ON_OFF; // Tắt hoàn toàn kênh PWM
    }
    else if (ticks == 4095)
    {
        on = FULL_ON_OFF;  // Bật tối đa hoàn toàn kênh PWM
        off = 0;
    }

    // Gửi chuỗi lệnh I2C tới các thanh ghi LEDX_ON_L đến LEDX_OFF_H nhờ tính năng Auto-Increment
    _wire->beginTransmission(_address);
    _wire->write(LED0_ON_L + 4 * channel);
    _wire->write(on & 0xFF);
    _wire->write(on >> 8);
    _wire->write(off & 0xFF);
    _wire->write(off >> 8);
    _wire->endTransmission();
}

void Pca9685::write(uint8_t reg, uint8_t value)
{
    _wire->beginTransmission(_address);
    _wire->write(reg);
    _wire->write(value);
    _wire->endTransmission();
}