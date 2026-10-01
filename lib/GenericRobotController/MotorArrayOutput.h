#ifndef MOTOR_ARRAY_OUTPUT_H
#define MOTOR_ARRAY_OUTPUT_H

#include <Arduino.h>
#include "IMotorOutput.h"

/**
 * @file MotorArrayOutput.h
 * @brief Generic array-backed motor output implementation.
 */

// Generic N-motor output: applies proportional scaling so no wheel exceeds
// the PWM limit while preserving the commanded direction of motion.
class MotorArrayOutput : public IMotorOutput
{
public:
    /**
     * @brief Constructs motor output from motor pointer array.
     * @param motors Array of motor pointers.
     * @param motorCount Number of elements in @p motors.
     */
    MotorArrayOutput(IMotor *const *motors, uint8_t motorCount);
    /**
     * @brief Replaces the underlying motor pointer array at runtime.
     * @param motors Array of motor pointers.
     * @param motorCount Number of motors in @p motors.
     */
    void setMotors(IMotor *const *motors, uint8_t motorCount);
    /** @brief Initializes all configured motors. */
    void begin() override;
    /** @brief Stops all configured motors. */
    void stop() override;
    /** @brief Returns total configured motor count. */
    uint8_t getMotorCount() const override;
    /**
     * @brief Gets motor by 1-based index.
     * @param motorId 1-based motor identifier.
     * @return Motor pointer or nullptr if out of range.
     */
    IMotor *getMotor(uint8_t motorId) override; // 1-based index
    /**
     * @brief Applies wheel speeds with proportional scaling to pwm limit.
     * @param speeds Wheel speed array.
     * @param count Number of values in @p speeds.
     * @param pwmLimit Maximum absolute output value.
     */
    void applyWheelSpeeds(const int16_t *speeds, uint8_t count, uint8_t pwmLimit) override;

private:
    IMotor **_motors;
    uint8_t _motorCount;
};

#endif
