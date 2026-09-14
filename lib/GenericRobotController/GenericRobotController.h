#ifndef GENERIC_ROBOT_CONTROLLER_H
#define GENERIC_ROBOT_CONTROLLER_H

#include <Arduino.h>
#include "Std_Types.h"

class IMotor {
public:
    virtual ~IMotor() = default;
    virtual void begin() = 0;
    virtual void setSpeed(int16_t speed) = 0;
    virtual void stop() = 0;
};

class GenericRobotController {
public:
    GenericRobotController(IMotor& leftMotor, IMotor& rightMotor);
    void begin();
    void handlePacket(const ControlPacket& packet);
    void update();

private:
    static int16_t mapAxisToMotor(int16_t value);
    void stopMotors();
    void logPacket(const ControlPacket& packet);

    IMotor& _leftMotor;
    IMotor& _rightMotor;
    uint32_t _lastPacketTime;
    uint32_t _lastLogTime;
};

#endif