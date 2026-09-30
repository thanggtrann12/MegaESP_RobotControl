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
    bool setKinematicsMode(KinematicsMode mode);
    KinematicsMode getKinematicsMode() const;
    void setControlSource(ControlSource source);
    ControlSource getControlSource() const;
    void setHmiMotion(int8_t throttle, int8_t strafe, int8_t rotation);
    bool setManualMotor(uint8_t motorId, int16_t speed);
    void releaseManualMotor(uint8_t motorId);
    void setPwmLimit(uint8_t limit);
    uint8_t getPwmLimit() const;

private:
    static int16_t mapAxisToMotor(int16_t value);
    void applyMotion(int8_t throttle, int8_t strafe, int8_t rotation);
    IMotor *getMotor(uint8_t motorId);
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
    KinematicsMode _kinematicsMode;
    ControlSource _controlSource;
    uint8_t _pwmLimit;
    bool _hmiMotionActive;
    int8_t _hmiThrottle;
    int8_t _hmiStrafe;
    int8_t _hmiRotation;
    uint32_t _hmiMotionTime;
    bool _manualOverrideActive;
    uint8_t _manualMotorId;
    uint32_t _manualCommandTime;
};

#endif