#include "PCA9685Motor.h"

namespace
{
    constexpr uint8_t PCA9685_MODE1 = 0x00;
    constexpr uint8_t PCA9685_MODE2 = 0x01;
    constexpr uint8_t PCA9685_PRESCALE = 0xFE;
    constexpr uint8_t PCA9685_LED0 = 0x06;
    constexpr uint8_t PCA9685_SLEEP = 0x10;
    constexpr uint8_t PCA9685_RESTART = 0x80;
    constexpr uint8_t PCA9685_AI = 0x20;
    constexpr uint8_t PCA9685_OUTDRV = 0x04;
}

PCA9685Motor::PCA9685Motor(TwoWire &wire,
                           uint8_t address,
                           uint8_t biChannel,
                           uint8_t fiChannel,
                           uint16_t pwmFrequency)
    : _wire(&wire),
      _address(address),
      _biChannel(biChannel),
      _fiChannel(fiChannel),
      _pwmFrequency(pwmFrequency) {}

void PCA9685Motor::begin()
{
    _wire->begin();
    _wire->beginTransmission(_address);
    _wire->write(PCA9685_MODE2);
    _wire->write(PCA9685_OUTDRV);
    _wire->endTransmission();
    _wire->beginTransmission(_address);
    _wire->write(PCA9685_MODE1);
    _wire->write(PCA9685_AI);
    _wire->endTransmission();
    setPwmFrequency(_pwmFrequency);
    stop();
}

void PCA9685Motor::setSpeed(int16_t speed)
{
    speed = constrain(speed, -255, 255);
    uint16_t pwm = static_cast<uint16_t>(abs(speed) * 4095L / 255L);
    if (speed > 0)
    {
        setChannel(_biChannel, 0);
        setChannel(_fiChannel, pwm);
    }
    else if (speed < 0)
    {
        setChannel(_biChannel, pwm);
        setChannel(_fiChannel, 0);
    }
    else
    {
        stop();
    }
}

void PCA9685Motor::stop()
{
    setChannel(_biChannel, 0);
    setChannel(_fiChannel, 0);
}

void PCA9685Motor::setChannel(uint8_t channel, uint16_t value)
{
    if (channel > 15)
    {
        return;
    }

    uint8_t registerAddress = PCA9685_LED0 + (4 * channel);
    _wire->beginTransmission(_address);
    _wire->write(registerAddress);
    if (value == 0)
    {
        _wire->write(0);
        _wire->write(0);
        _wire->write(0);
        _wire->write(0x10);
    }
    else if (value >= 4095)
    {
        _wire->write(0);
        _wire->write(0x10);
        _wire->write(0);
        _wire->write(0);
    }
    else
    {
        _wire->write(0);
        _wire->write(0);
        _wire->write(static_cast<uint8_t>(value & 0xFF));
        _wire->write(static_cast<uint8_t>((value >> 8) & 0x0F));
    }
    _wire->endTransmission();
}

void PCA9685Motor::setPwmFrequency(uint16_t frequency)
{
    if (frequency == 0)
    {
        return;
    }

    uint8_t prescale = static_cast<uint8_t>(25000000L / (4096L * frequency) - 1);
    _wire->beginTransmission(_address);
    _wire->write(PCA9685_MODE1);
    _wire->write(PCA9685_SLEEP | PCA9685_AI);
    _wire->endTransmission();
    _wire->beginTransmission(_address);
    _wire->write(PCA9685_PRESCALE);
    _wire->write(prescale);
    _wire->endTransmission();
    _wire->beginTransmission(_address);
    _wire->write(PCA9685_MODE1);
    _wire->write(PCA9685_AI);
    _wire->endTransmission();
    delay(1);
    _wire->beginTransmission(_address);
    _wire->write(PCA9685_MODE1);
    _wire->write(PCA9685_AI | PCA9685_RESTART);
    _wire->endTransmission();
}