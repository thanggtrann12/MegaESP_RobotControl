#include "MotorTA6586.h"

namespace
{
    constexpr uint16_t FULL_ON = 4095;
}

void MotorTA6586::run(int16_t pwm)
{
    pwm = constrain(pwm, -255, 255);
    const uint16_t duty = static_cast<uint16_t>(abs(pwm) * 4095L / 255L);
    if (pwm > 0)
    {
        _pca->setDuty(_biChannel, 0);
        _pca->setDuty(_fiChannel, duty);
    }
    else if (pwm < 0)
    {
        _pca->setDuty(_biChannel, duty);
        _pca->setDuty(_fiChannel, 0);
    }
    else
    {
        stop();
    }
}

// Both inputs high shorts the motor terminals.
void MotorTA6586::brake()
{
    _pca->setDuty(_biChannel, FULL_ON);
    _pca->setDuty(_fiChannel, FULL_ON);
}

void MotorTA6586::stop()
{
    _pca->setDuty(_biChannel, 0);
    _pca->setDuty(_fiChannel, 0);
}
