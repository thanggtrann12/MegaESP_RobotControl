#ifndef IMOTOR_H
#define IMOTOR_H

#include <Arduino.h>

class IMotor
{
public:
    virtual ~IMotor() = default;
    virtual void begin() = 0;
    virtual void setSpeed(int16_t speed) = 0;
    virtual void stop() = 0;
};

#endif
