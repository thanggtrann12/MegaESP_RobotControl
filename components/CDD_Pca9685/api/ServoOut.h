#ifndef SERVO_OUT_H
#define SERVO_OUT_H

#include <Pca9685.h>

// Hobby servos on a PCA9685 running at 50 Hz.
class ServoOut
{
public:
    explicit ServoOut(Pca9685 &pca) : _pca(&pca) {}

    void begin() { _pca->begin(50); }

    // Pulse 0.5..2.5 ms inside a 20 ms period of 4096 ticks.
    void write(uint8_t channel, uint8_t angle)
    {
        const uint32_t pulseUs = 500UL + constrain(angle, 0, 180) * 2000UL / 180;
        _pca->setDuty(channel, static_cast<uint16_t>(pulseUs * 4096UL / 20000UL));
    }

    void release(uint8_t channel) { _pca->setDuty(channel, 0); }

private:
    Pca9685 *_pca;
};

#endif
