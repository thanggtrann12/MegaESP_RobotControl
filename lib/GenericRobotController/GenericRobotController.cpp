#include "GenericRobotController.h"
#include <GenericLogger.h>

ASSIGN_LOG_MACROS(GenericRobotController, Serial);

namespace
{
    const char *GetTranslationDirection(int8_t throttle, int8_t strafe)
    {
        constexpr int8_t DEADZONE = 8;
        bool forward = throttle > DEADZONE;
        bool backward = throttle < -DEADZONE;
        bool left = strafe < -DEADZONE;
        bool right = strafe > DEADZONE;

        if (forward && left)
            return "FORWARD-LEFT";
        if (forward && right)
            return "FORWARD-RIGHT";
        if (backward && left)
            return "BACKWARD-LEFT";
        if (backward && right)
            return "BACKWARD-RIGHT";
        if (forward)
            return "FORWARD";
        if (backward)
            return "BACKWARD";
        if (left)
            return "STRAFE-LEFT";
        if (right)
            return "STRAFE-RIGHT";
        return "STOP";
    }

    const char *GetRotationDirection(int8_t rotation)
    {
        constexpr int8_t DEADZONE = 8;
        if (rotation > DEADZONE)
            return "TURN-RIGHT";
        if (rotation < -DEADZONE)
            return "TURN-LEFT";
        return "NO-TURN";
    }
}

GenericRobotController::GenericRobotController(IMotor &leftMotor1,
                                               IMotor &leftMotor2,
                                               IMotor &rightMotor1,
                                               IMotor &rightMotor2)
    : _leftMotor1(leftMotor1),
      _leftMotor2(leftMotor2),
      _rightMotor1(rightMotor1),
      _rightMotor2(rightMotor2),
      _lastPacketTime(0),
      _lastLogTime(0),
      _lastSequenceNumber(0),
      _hasSequenceNumber(false),
      _kinematicsMode(KinematicsMode::MODE_4WD_MECANUM),
      _controlSource(ControlSource::REMOTE),
      _pwmLimit(255),
      _hmiMotionActive(false),
      _hmiThrottle(0),
      _hmiStrafe(0),
      _hmiRotation(0),
      _hmiMotionTime(0),
      _manualOverrideActive(false),
      _manualMotorId(0),
      _manualCommandTime(0) {}

bool GenericRobotController::setKinematicsMode(KinematicsMode mode)
{
    if (mode != KinematicsMode::MODE_2WD_DIFF &&
        mode != KinematicsMode::MODE_4WD_MECANUM)
    {
        return false;
    }
    _kinematicsMode = mode;
    stopMotors();
    return true;
}

KinematicsMode GenericRobotController::getKinematicsMode() const
{
    return _kinematicsMode;
}

void GenericRobotController::setControlSource(ControlSource source)
{
    _controlSource = source;
    _hmiMotionActive = false;
    _manualOverrideActive = false;
    _manualMotorId = 0;
    stopMotors();
}

ControlSource GenericRobotController::getControlSource() const
{
    return _controlSource;
}

void GenericRobotController::setHmiMotion(int8_t throttle, int8_t strafe, int8_t rotation)
{
    if (_controlSource != ControlSource::HMI_MANUAL || _manualOverrideActive)
    {
        return;
    }
    _hmiThrottle = constrain(throttle, -100, 100);
    _hmiStrafe = constrain(strafe, -100, 100);
    _hmiRotation = constrain(rotation, -100, 100);
    _hmiMotionTime = millis();
    _hmiMotionActive = true;
}

bool GenericRobotController::setManualMotor(uint8_t motorId, int16_t speed)
{
    if (_controlSource != ControlSource::HMI_MANUAL)
    {
        return false;
    }

    IMotor *motor = getMotor(motorId);
    if (motor == nullptr)
    {
        return false;
    }

    _hmiMotionActive = false;
    _manualOverrideActive = true;
    _manualMotorId = motorId;
    _manualCommandTime = millis();
    stopMotors();
    motor->setSpeed(constrain(speed, -_pwmLimit, _pwmLimit));
    return true;
}

void GenericRobotController::releaseManualMotor(uint8_t motorId)
{
    if (_manualOverrideActive && (motorId == 0 || motorId == _manualMotorId))
    {
        stopMotors();
        _manualOverrideActive = false;
        _manualMotorId = 0;
    }
}

void GenericRobotController::setPwmLimit(uint8_t limit)
{
    _pwmLimit = limit;
}

uint8_t GenericRobotController::getPwmLimit() const
{
    return _pwmLimit;
}

void GenericRobotController::begin()
{
    _leftMotor1.begin();
    _leftMotor2.begin();
    _rightMotor1.begin();
    _rightMotor2.begin();
}

void GenericRobotController::handlePacket(const ControlPacket &packet)
{
    if (!IsControlPacketValid(packet) ||
        (packet.msgType != CONTROL_MESSAGE && packet.msgType != HEARTBEAT_MESSAGE))
    {
        return;
    }

    if (_hasSequenceNumber &&
        static_cast<int16_t>(packet.sequenceNum - _lastSequenceNumber) <= 0)
    {
        return;
    }

    _lastSequenceNumber = packet.sequenceNum;
    _hasSequenceNumber = true;
    _lastPacketTime = millis();

    if (packet.msgType == HEARTBEAT_MESSAGE)
    {
        return;
    }

    if (_controlSource != ControlSource::REMOTE || _manualOverrideActive ||
        (_hmiMotionActive && millis() - _hmiMotionTime <= 500))
    {
        return;
    }
    _hmiMotionActive = false;

    applyMotion(packet.throttle, packet.strafe, packet.rotation);
}

void GenericRobotController::applyMotion(int8_t throttleValue, int8_t strafeValue, int8_t rotationValue)
{
    int16_t throttle = mapAxisToMotor(throttleValue);
    int16_t strafe = mapAxisToMotor(strafeValue);
    int16_t rotation = mapAxisToMotor(rotationValue);

    int16_t frontLeft;
    int16_t rearLeft;
    int16_t frontRight;
    int16_t rearRight;
    if (_kinematicsMode == KinematicsMode::MODE_2WD_DIFF)
    {
        frontLeft = throttle + rotation;
        rearLeft = frontLeft;
        frontRight = throttle - rotation;
        rearRight = frontRight;
    }
    else
    {
        frontLeft = throttle + strafe + rotation;
        rearLeft = throttle - strafe + rotation;
        frontRight = throttle - strafe - rotation;
        rearRight = throttle + strafe - rotation;
    }

    // Normalize PWM nếu vượt quá 255
    int16_t maxMagnitude = max(max(abs(frontLeft), abs(rearLeft)),
                               max(abs(frontRight), abs(rearRight)));
    if (maxMagnitude > _pwmLimit)
    {
        frontLeft = static_cast<int16_t>(frontLeft * _pwmLimit / maxMagnitude);
        rearLeft = static_cast<int16_t>(rearLeft * _pwmLimit / maxMagnitude);
        frontRight = static_cast<int16_t>(frontRight * _pwmLimit / maxMagnitude);
        rearRight = static_cast<int16_t>(rearRight * _pwmLimit / maxMagnitude);
    }

    _leftMotor1.setSpeed(frontLeft);
    _leftMotor2.setSpeed(rearLeft);
    _rightMotor1.setSpeed(frontRight);
    _rightMotor2.setSpeed(rearRight);
    if (millis() - _lastLogTime >= 500)
    {
        _lastLogTime = millis();
        GenericRobotController_LogI("motion=%s %s throttle=%d strafe=%d rotation=%d FL=%d RL=%d FR=%d RR=%d",
                                    GetTranslationDirection(throttleValue, strafeValue),
                                    GetRotationDirection(rotationValue),
                                    throttle,
                                    strafe,
                                    rotation,
                                    frontLeft,
                                    rearLeft,
                                    frontRight,
                                    rearRight);
    }
}

void GenericRobotController::update()
{
    if (_manualOverrideActive)
    {
        if (millis() - _manualCommandTime > 500)
        {
            releaseManualMotor(_manualMotorId);
        }
        return;
    }

    if (_hmiMotionActive)
    {
        if (millis() - _hmiMotionTime > 500)
        {
            _hmiMotionActive = false;
            stopMotors();
            return;
        }
        applyMotion(_hmiThrottle, _hmiStrafe, _hmiRotation);
        _lastPacketTime = millis();
        return;
    }

    if (millis() - _lastPacketTime > 500)
    {
        stopMotors();
    }
}

int16_t GenericRobotController::mapAxisToMotor(int16_t value)
{
    return static_cast<int16_t>((constrain(value, -100, 100) * 255L) / 100L);
}

void GenericRobotController::stopMotors()
{
    _leftMotor1.stop();
    _leftMotor2.stop();
    _rightMotor1.stop();
    _rightMotor2.stop();
}

void GenericRobotController::logPacket(const ControlPacket &packet)
{
    GenericRobotController_LogD("type=%u throttle=%d strafe=%d rotation=%d buttons=0x%02X seq=%u",
                                packet.msgType,
                                packet.throttle,
                                packet.strafe,
                                packet.rotation,
                                packet.buttons,
                                packet.sequenceNum);
}

IMotor *GenericRobotController::getMotor(uint8_t motorId)
{
    switch (motorId)
    {
    case 1:
        return &_leftMotor1;
    case 2:
        return &_leftMotor2;
    case 3:
        return &_rightMotor1;
    case 4:
        return &_rightMotor2;
    default:
        return nullptr;
    }
}