#ifndef ROBOT_MOTOR_TA6586_H
#define ROBOT_MOTOR_TA6586_H

#include <Arduino.h>
#include <Wire.h>
#include "IMotor.h"

/**
 * @file Motor_TA6586.h
 * @brief TA6586 motor driver implementation over PCA9685 PWM expander.
 */

/**
 * @class MotorTA6586
 * @brief Controls one TA6586 motor channel pair through PCA9685.
 */
class MotorTA6586 : public IMotor
{
public:
    /**
     * @brief Constructs a motor output bound to PCA9685 channels.
     * @param wire I2C bus instance.
     * @param pcaAddress I2C address of PCA9685.
     * @param biChannel PCA9685 channel for reverse direction.
     * @param fiChannel PCA9685 channel for forward direction.
     * @param pwmFrequency PCA9685 PWM frequency in Hz.
     */
    MotorTA6586(TwoWire &wire,
                uint8_t pcaAddress,
                uint8_t biChannel,
                uint8_t fiChannel,
                uint16_t pwmFrequency);
    /** @brief Initializes I2C and PCA9685 state for this motor. */
    void begin() override;
    /**
     * @brief Sets signed motor speed in range [-255, 255].
     * @param speed Signed speed command.
     */
    void setSpeed(int16_t speed) override;
    /** @brief Stops motor output on both direction channels. */
    void stop() override;

private:
    /**
     * @brief Writes PWM value to one PCA9685 channel.
     * @param channel PCA9685 channel index.
     * @param value PWM value in range [0, 4095].
     */
    void setChannel(uint8_t channel, uint16_t value);
    /**
     * @brief Configures PCA9685 PWM frequency.
     * @param frequency PWM frequency in Hz.
     */
    void setPwmFrequency(uint16_t frequency);

    TwoWire *_wire;
    uint8_t _pcaAddress;
    uint8_t _biChannel;
    uint8_t _fiChannel;
    uint16_t _pwmFrequency;
};

#endif