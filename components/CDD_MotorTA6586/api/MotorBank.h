#ifndef MOTOR_BANK_H
#define MOTOR_BANK_H

#include "MotorTA6586.h"

// Motor service: M0..M5 on one PCA9685, slot n uses channels 2n and 2n+1.
class MotorBank
{
public:
    explicit MotorBank(Pca9685 &pca)
        : _pca(&pca),
          _motors{MotorTA6586(pca, 0, 1), MotorTA6586(pca, 2, 3), MotorTA6586(pca, 4, 5),
                  MotorTA6586(pca, 6, 7), MotorTA6586(pca, 8, 9), MotorTA6586(pca, 10, 11)} {}

    void begin(uint16_t pwmHz)
    {
        _pca->begin(pwmHz);
        stopAll();
    }

    IMotor *motor(uint8_t slot) { return &_motors[slot]; } // slot 0..5

    void stopAll()
    {
        for (MotorTA6586 &motor : _motors)
        {
            motor.stop();
        }
    }

private:
    Pca9685 *_pca;
    MotorTA6586 _motors[MOTOR_SLOTS];
};

#endif
