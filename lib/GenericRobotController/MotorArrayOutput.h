#ifndef MOTOR_ARRAY_OUTPUT_H
#define MOTOR_ARRAY_OUTPUT_H

#include <Arduino.h>
#include "IMotorOutput.h"

// Generic N-motor output: applies proportional scaling so no wheel exceeds
// the PWM limit while preserving the commanded direction of motion.
class MotorArrayOutput : public IMotorOutput
{
public:
    MotorArrayOutput(IMotor *const *motors, uint8_t motorCount);
    void begin() override;
    void stop() override;
    uint8_t getMotorCount() const override;
    IMotor *getMotor(uint8_t motorId) override; // 1-based index
    void applyWheelSpeeds(const int16_t *speeds, uint8_t count, uint8_t pwmLimit) override;

private:
    IMotor *const *_motors;
    uint8_t _motorCount;
};

#endif
