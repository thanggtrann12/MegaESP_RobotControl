#ifndef PCA9685_H
#define PCA9685_H

#include <Arduino.h>
#include <Wire.h>
#include "Pwm.h"

// Lớp điều khiển chip PWM 16 kênh qua I2C, kế thừa từ giao diện IPwmSource (HAL_Pwm).
class Pca9685 : public IPwmSource
{
public:
    Pca9685(TwoWire &wire, uint8_t address) 
        : _wire(&wire), _address(address) {}

    void begin(uint16_t frequencyHz) override;
    void setDuty(uint8_t channel, uint16_t ticks) override; // 0 = tắt, 4095 = sáng/chạy tối đa

private:
    void write(uint8_t reg, uint8_t value);

    TwoWire *_wire;
    uint8_t _address;
    uint16_t _cachedTicks[16] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 
                                 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 
                                 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 
                                 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}; // Cache duty cycle để bỏ qua các lệnh ghi trùng lặp
};

#endif