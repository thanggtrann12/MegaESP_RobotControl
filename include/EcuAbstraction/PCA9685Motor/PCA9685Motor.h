#ifndef PCA9685_MOTOR_H
#define PCA9685_MOTOR_H

#include <Arduino.h>
#include <Wire.h>
#include "GenericRobotController.h"

class PCA9685Motor : public IMotor {
public:
    PCA9685Motor(TwoWire& wire,
                 uint8_t address,
                 uint8_t enableChannel,
                 uint8_t input1Channel,
                 uint8_t input2Channel,
                 uint16_t pwmFrequency);
    void begin() override;
    void setSpeed(int16_t speed) override;
    void stop() override;

private:
    void setChannel(uint8_t channel, uint16_t value);
    void setPwmFrequency(uint16_t frequency);

    TwoWire* _wire;
    uint8_t _address;
    uint8_t _enableChannel;
    uint8_t _input1Channel;
    uint8_t _input2Channel;
    uint16_t _pwmFrequency;
};

#endif