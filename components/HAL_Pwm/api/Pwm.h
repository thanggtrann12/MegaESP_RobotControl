#ifndef PWM_H
#define PWM_H

#include <Arduino.h>

class IPwmSource {
public:
    virtual ~IPwmSource() = default;
    virtual void begin(uint16_t frequencyHz) = 0;
    virtual void setDuty(uint8_t channel, uint16_t ticks) = 0; // 0 = off, 4095 = fully on
};

#endif