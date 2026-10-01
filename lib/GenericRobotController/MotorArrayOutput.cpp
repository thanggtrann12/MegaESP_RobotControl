/**
 * @file MotorArrayOutput.cpp
 * @brief Generic motor array output implementation.
 */

#include "MotorArrayOutput.h"

MotorArrayOutput::MotorArrayOutput(IMotor *const *motors, uint8_t motorCount)
    : _motors(motors), _motorCount(motorCount) {}

void MotorArrayOutput::begin()
{
    for (uint8_t index = 0; index < _motorCount; ++index)
    {
        _motors[index]->begin();
    }
}

void MotorArrayOutput::stop()
{
    for (uint8_t index = 0; index < _motorCount; ++index)
    {
        _motors[index]->stop();
    }
}

uint8_t MotorArrayOutput::getMotorCount() const
{
    return _motorCount;
}

IMotor *MotorArrayOutput::getMotor(uint8_t motorId)
{
    if (motorId == 0 || motorId > _motorCount)
    {
        return nullptr;
    }
    return _motors[motorId - 1];
}

void MotorArrayOutput::applyWheelSpeeds(const int16_t *speeds, uint8_t count, uint8_t pwmLimit)
{
    uint8_t appliedCount = min(count, _motorCount);

    int16_t maxMagnitude = 0;
    for (uint8_t index = 0; index < appliedCount; ++index)
    {
        maxMagnitude = max(maxMagnitude, static_cast<int16_t>(abs(speeds[index])));
    }

    for (uint8_t index = 0; index < appliedCount; ++index)
    {
        int16_t speed = speeds[index];
        if (maxMagnitude > pwmLimit && maxMagnitude > 0)
        {
            speed = static_cast<int16_t>(speed * static_cast<int32_t>(pwmLimit) / maxMagnitude);
        }
        _motors[index]->setSpeed(speed);
    }
}
