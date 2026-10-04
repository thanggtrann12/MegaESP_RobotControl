#ifndef PCA9685_H
#define PCA9685_H

#include <Arduino.h>
#include <Wire.h>

// 16-channel I2C PWM chip. Motors and servos each get their own chip because one chip has one PWM frequency.
class Pca9685
{
public:
    Pca9685(TwoWire &wire, uint8_t address) : _wire(&wire), _address(address) {}

    void begin(uint16_t frequencyHz);
    void setDuty(uint8_t channel, uint16_t ticks); // 0 = off, 4095 = fully on

private:
    void write(uint8_t reg, uint8_t value);

    TwoWire *_wire;
    uint8_t _address;
};

#endif
