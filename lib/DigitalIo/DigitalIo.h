#ifndef DIGITAL_IO_H
#define DIGITAL_IO_H
#include <Arduino.h>
#include <IoPin.h>
#include "RobotPins.h"

class DigitalIo
{
public:
    static constexpr uint8_t COUNT = 8;
    enum Mode : uint8_t
    {
        IN = 0,
        OUT = 1,
        PULLUP = 2
    };
    struct Config
    {
        uint8_t mode;
        uint8_t defaultHigh;
        uint8_t invert;
    };
    void begin(int eepromAddress);
    bool set(uint8_t i, const Config &config);
    void reset();
    const Config &config(uint8_t i) const { return _config[i]; }
    bool read(uint8_t i) const;
    void write(uint8_t i, bool logical);
    static uint8_t pin(uint8_t i);
    static int8_t indexOfPin(uint8_t pin);
    static constexpr size_t STORE_SIZE = 2 + COUNT * sizeof(Config);

private:
    void apply(uint8_t i);
    void load();
    void save();
    Config _config[COUNT] = {};
    int _address = 0;
};
#endif