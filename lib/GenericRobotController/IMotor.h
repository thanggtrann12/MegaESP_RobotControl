#ifndef IMOTOR_H
#define IMOTOR_H

#include <Arduino.h>

/**
 * @file IMotor.h
 * @brief Abstract motor output interface.
 */

class IMotor
{
public:
    virtual ~IMotor() = default;
    /** @brief Initializes underlying motor hardware. */
    virtual void begin() = 0;
    /**
     * @brief Sets signed motor speed command.
     * @param speed Motor speed in implementation-specific range.
     */
    virtual void setSpeed(int16_t speed) = 0;
    /** @brief Sets whether signed speed commands are inverted. */
    virtual void setInverted(bool inverted) = 0;
    /** @brief Stops motor output. */
    virtual void stop() = 0;
};

#endif
