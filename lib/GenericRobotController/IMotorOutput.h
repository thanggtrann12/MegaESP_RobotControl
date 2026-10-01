#ifndef IMOTOR_OUTPUT_H
#define IMOTOR_OUTPUT_H

#include <Arduino.h>
#include "IMotor.h"

/**
 * @file IMotorOutput.h
 * @brief Abstract drive base motor output interface.
 */

// Owns the physical motors for a drive base and applies wheel speed
// commands to them, independent of motor count or layout.
class IMotorOutput
{
public:
    virtual ~IMotorOutput() = default;
    /** @brief Initializes all motors in the output set. */
    virtual void begin() = 0;
    /** @brief Stops all motors in the output set. */
    virtual void stop() = 0;
    /**
     * @brief Returns number of motors managed by this output.
     * @return Motor count.
     */
    virtual uint8_t getMotorCount() const = 0;
    /**
     * @brief Gets a motor by 1-based index.
     * @param motorId 1-based motor identifier.
     * @return Pointer to motor, or nullptr if invalid id.
     */
    virtual IMotor *getMotor(uint8_t motorId) = 0; // 1-based index
    /**
     * @brief Applies wheel speeds with optional saturation limit.
     * @param speeds Input wheel speed array.
     * @param count Number of wheel values in @p speeds.
     * @param pwmLimit Maximum absolute speed after scaling.
     */
    virtual void applyWheelSpeeds(const int16_t *speeds, uint8_t count, uint8_t pwmLimit) = 0;
};

#endif
