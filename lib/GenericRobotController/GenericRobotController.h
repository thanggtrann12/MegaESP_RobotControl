#ifndef GENERIC_ROBOT_CONTROLLER_H
#define GENERIC_ROBOT_CONTROLLER_H

#include <Arduino.h>
#include "Std_Types.h"
#include "IMotor.h"
#include "IMotorOutput.h"
#include "IKinematics.h"

// Topology-agnostic: owns control state, command arbitration, and the
// watchdog. Motor count/layout and motion mixing are fully delegated to
// the injected IMotorOutput and IKinematics implementations.
class GenericRobotController
{
public:
    GenericRobotController(IMotorOutput &motorOutput, IKinematics &kinematics);
    void begin();
    void handlePacket(const ControlPacket &packet);
    void update();
    bool setKinematics(IKinematics &kinematics);
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
    void stopMotors();

    IMotorOutput &_motorOutput;
    IKinematics *_kinematics;
    uint32_t _lastPacketTime;
    uint32_t _lastLogTime;
    uint16_t _lastSequenceNumber;
    bool _hasSequenceNumber;
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