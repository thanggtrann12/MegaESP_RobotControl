#ifndef IO_PIN_H
#define IO_PIN_H

#include <Arduino.h>

// A pin is just an input or an output; nothing else lives here.
namespace IoPin
{
    enum Mode : uint8_t
    {
        IN = 0,
        OUT = 1,
        IN_PULLUP = 2
    };

    inline void configure(uint8_t pin, Mode mode)
    {
        pinMode(pin, mode == OUT ? OUTPUT : (mode == IN_PULLUP ? INPUT_PULLUP : INPUT));
    }

    inline void write(uint8_t pin, bool high) { digitalWrite(pin, high ? HIGH : LOW); }
    inline bool read(uint8_t pin) { return digitalRead(pin) == HIGH; }
    inline uint16_t readAnalog(uint8_t channel) { return analogRead(channel); }
}

#endif
