#ifndef MOTOR_TA6586_H
#define MOTOR_TA6586_H

#include <Pca9685.h>
#include "Motor.h"

// One TA6586 driver: two inputs fed by two PCA9685 channels.
class MotorTA6586 : public IMotor
{
public:
    MotorTA6586(Pca9685 &pca, uint8_t biChannel, uint8_t fiChannel)
        : _pca(&pca), _biChannel(biChannel), _fiChannel(fiChannel) {}

    void run(int16_t pwm) override;
    void brake() override;
    void stop() override;

private:
    Pca9685 *_pca;
    uint8_t _biChannel;
    uint8_t _fiChannel;
};

#endif
