#ifndef IMOTOR_OUTPUT_H
#define IMOTOR_OUTPUT_H

#include <Arduino.h>
#include "IMotor.h"

// Owns the physical motors for a drive base and applies wheel speed
// commands to them, independent of motor count or layout.
class IMotorOutput
{
public:
    virtual ~IMotorOutput() = default;
    virtual void begin() = 0;
    virtual void stop() = 0;
    virtual uint8_t getMotorCount() const = 0;
    virtual IMotor *getMotor(uint8_t motorId) = 0; // 1-based index
    virtual void applyWheelSpeeds(const int16_t *speeds, uint8_t count, uint8_t pwmLimit) = 0;
};

#endif
