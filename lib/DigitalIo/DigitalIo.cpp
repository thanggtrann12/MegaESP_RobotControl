#include "DigitalIo.h"
#include <EEPROM.h>
#include "RobotPins.h"

namespace
{
    constexpr uint8_t STORE_VERSION = 1;
    constexpr uint8_t PINS[DigitalIo::COUNT] = {IO_PIN_NUM_1, IO_PIN_NUM_2, IO_PIN_NUM_3, IO_PIN_NUM_4,
                                                IO_PIN_NUM_5, IO_PIN_NUM_6, IO_PIN_NUM_7, IO_PIN_NUM_8};
    uint8_t crc8(const uint8_t *data, size_t length)
    {
        uint8_t crc = 0;
        for (size_t i = 0; i < length; ++i)
        {
            crc ^= data[i];
            for (uint8_t bit = 0; bit < 8; ++bit)
                crc = (crc & 0x80) ? static_cast<uint8_t>((crc << 1) ^ 0x07) : static_cast<uint8_t>(crc << 1);
        }
        return crc;
    }
}
uint8_t DigitalIo::pin(uint8_t i) { return PINS[i]; }
int8_t DigitalIo::indexOfPin(uint8_t p)
{
    for (uint8_t i = 0; i < COUNT; ++i)
        if (PINS[i] == p)
            return i;
    return -1;
}
void DigitalIo::begin(int eepromAddress)
{
    _address = eepromAddress;
    load();
    for (uint8_t i = 0; i < COUNT; ++i)
        apply(i);
}
void DigitalIo::apply(uint8_t i)
{
    const Config &c = _config[i];
    switch (c.mode)
    {
    case OUT:
        IoPin::configure(PINS[i], IoPin::OUT);
        IoPin::write(PINS[i], (c.defaultHigh ^ c.invert) != 0);
        break;
    case PULLUP:
        IoPin::configure(PINS[i], IoPin::IN_PULLUP);
        break;
    default:
        IoPin::configure(PINS[i], IoPin::IN);
        break;
    }
}
bool DigitalIo::set(uint8_t i, const Config &config)
{
    if (i >= COUNT || config.mode > PULLUP || config.defaultHigh > 1 || config.invert > 1)
        return false;
    _config[i] = config;
    apply(i);
    save();
    return true;
}
void DigitalIo::reset()
{
    memset(_config, 0, sizeof(_config));
    for (uint8_t i = 0; i < COUNT; ++i)
        apply(i);
    save();
}
bool DigitalIo::read(uint8_t i) const { return (IoPin::read(PINS[i]) != 0) ^ (_config[i].invert != 0); }
void DigitalIo::write(uint8_t i, bool logical)
{
    if (_config[i].mode == OUT)
        IoPin::write(PINS[i], logical ^ (_config[i].invert != 0));
}
void DigitalIo::load()
{
    uint8_t version = EEPROM.read(_address);
    Config stored[COUNT];
    EEPROM.get(_address + 1, stored);
    const uint8_t crc = EEPROM.read(_address + 1 + sizeof(stored));
    if (version != STORE_VERSION || crc != crc8(reinterpret_cast<const uint8_t *>(stored), sizeof(stored)))
        return;
    for (uint8_t i = 0; i < COUNT; ++i)
        if (stored[i].mode > PULLUP || stored[i].defaultHigh > 1 || stored[i].invert > 1)
            return;
    memcpy(_config, stored, sizeof(_config));
}
void DigitalIo::save()
{
    EEPROM.update(_address, STORE_VERSION);
    EEPROM.put(_address + 1, _config);
    EEPROM.update(_address + 1 + sizeof(_config), crc8(reinterpret_cast<const uint8_t *>(_config), sizeof(_config)));
}