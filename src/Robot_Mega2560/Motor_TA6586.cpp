#include "Motor_TA6586.h"

MotorTA6586::MotorTA6586(uint8_t forwardPin, uint8_t backwardPin)
    : _forwardPin(forwardPin), _backwardPin(backwardPin) {}

void MotorTA6586::begin()
{
    pinMode(_forwardPin, OUTPUT);
    pinMode(_backwardPin, OUTPUT);
    stop();
}

void MotorTA6586::setSpeed(int16_t speed)
{
    speed = constrain(speed, -255, 255);
    if (speed > 0)
    {
        analogWrite(_forwardPin, speed);
        analogWrite(_backwardPin, 0);
    }
    else if (speed < 0)
    {
        analogWrite(_forwardPin, 0);
        analogWrite(_backwardPin, -speed);
    }
    else
    {
        stop();
    }
}

void MotorTA6586::stop()
{
    analogWrite(_forwardPin, 0);
    analogWrite(_backwardPin, 0);
}