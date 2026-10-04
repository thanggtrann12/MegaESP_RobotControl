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
    constexpr uint16_t FULL_ON_OFF = 0x1000; // bit 4 of the high byte
    constexpr uint32_t OSCILLATOR_HZ = 25000000UL;
}

void Pca9685::begin(uint16_t frequencyHz)
{
    _wire->begin();
    _wire->setClock(400000UL); // 12 motor channels are rewritten every control tick
    write(MODE2, MODE2_OUTDRV);
    write(MODE1, MODE1_SLEEP | MODE1_AUTO_INC); // prescale can only change while asleep
    write(PRESCALE, static_cast<uint8_t>((OSCILLATOR_HZ + 2048UL * frequencyHz) / (4096UL * frequencyHz) - 1));
    write(MODE1, MODE1_AUTO_INC);
    delay(1);
    write(MODE1, MODE1_AUTO_INC | MODE1_RESTART);
}

void Pca9685::setDuty(uint8_t channel, uint16_t ticks)
{
    if (channel > 15)
    {
        return;
    }

    uint16_t on = 0;
    uint16_t off = ticks;
    if (ticks == 0)
    {
        off = FULL_ON_OFF;
    }
    else if (ticks >= 4095)
    {
        on = FULL_ON_OFF;
        off = 0;
    }

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
