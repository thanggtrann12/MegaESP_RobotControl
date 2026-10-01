#ifndef ROBOT_MOTOR_TA6586_H
#define ROBOT_MOTOR_TA6586_H

#include <Arduino.h>
#include <Wire.h>
#include "IMotor.h"

class MotorTA6586 : public IMotor
{
public:
    MotorTA6586(TwoWire &wire,
                uint8_t pcaAddress,
                uint8_t biChannel,
                uint8_t fiChannel,
                uint16_t pwmFrequency);
    void begin() override;
    void setSpeed(int16_t speed) override;
    void stop() override;

private:
    void setChannel(uint8_t channel, uint16_t value);
    void setPwmFrequency(uint16_t frequency);

    TwoWire *_wire;
    uint8_t _pcaAddress;
    uint8_t _biChannel;
    uint8_t _fiChannel;
    uint16_t _pwmFrequency;
};

#endif