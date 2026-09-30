#ifndef GENERIC_ROBOT_CONTROLLER_H
#define GENERIC_ROBOT_CONTROLLER_H

#include <Arduino.h>
#include "Std_Types.h"

class IMotor
{
public:
    virtual ~IMotor() = default;
    virtual void begin() = 0;
    virtual void setSpeed(int16_t speed) = 0;
    virtual void stop() = 0;
};

class GenericRobotController
{
public:
    GenericRobotController(IMotor &leftMotor1,
                           IMotor &leftMotor2,
                           IMotor &rightMotor1,
                           IMotor &rightMotor2);
    void begin();
    void handlePacket(const ControlPacket &packet);
    void update();

private:
    static int16_t mapAxisToMotor(int16_t value);
    void stopMotors();
    void logPacket(const ControlPacket &packet);

    IMotor &_leftMotor1;
    IMotor &_leftMotor2;
    IMotor &_rightMotor1;
    IMotor &_rightMotor2;
    uint32_t _lastPacketTime;
    uint32_t _lastLogTime;
    uint16_t _lastSequenceNumber;
    bool _hasSequenceNumber;
};

#endif