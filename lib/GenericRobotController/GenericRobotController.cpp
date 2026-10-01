/**
 * @file GenericRobotController.cpp
 * @brief Core robot controller implementation.
 */

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

GenericRobotController::GenericRobotController(IMotorOutput &motorOutput, IKinematics &kinematics)
    : _motorOutput(motorOutput),
      _kinematics(&kinematics),
      _lastPacketTime(0),
      _lastLogTime(0),
      _lastSequenceNumber(0),
      _hasSequenceNumber(false),
      _controlSource(ControlSource::REMOTE),
      _pwmLimit(255),
    _motionProfile(MotionProfile::DIRECT),
      _hmiMotionActive(false),
      _hmiThrottle(0),
      _hmiStrafe(0),
      _hmiRotation(0),
      _hmiMotionTime(0),
      _manualOverrideActive(false),
      _manualMotorId(0),
      _manualCommandTime(0) {}

bool GenericRobotController::setKinematics(IKinematics &kinematics)
{
    if (kinematics.getWheelCount() != _motorOutput.getMotorCount())
    {
        return false;
    }
    _kinematics = &kinematics;
    stopMotors();
    return true;
}

KinematicsMode GenericRobotController::getKinematicsMode() const
{
    return _kinematics->getMode();
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

    IMotor *motor = _motorOutput.getMotor(motorId);
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

void GenericRobotController::setMotionProfile(MotionProfile profile)
{
    _motionProfile = profile;
    for (uint8_t index = 0; index < MAX_DRIVE_WHEELS; ++index)
    {
        _lastWheelSpeeds[index] = 0;
    }
    stopMotors();
}

uint8_t GenericRobotController::getPwmLimit() const
{
    return _pwmLimit;
}

void GenericRobotController::begin()
{
    _motorOutput.begin();
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

    int16_t speeds[MAX_DRIVE_WHEELS] = {0};
    uint8_t wheelCount = _kinematics->getWheelCount();
    _kinematics->computeWheelSpeeds(throttle, strafe, rotation, speeds);

    if (_motionProfile == MotionProfile::LIMITED_ACCELERATION)
    {
        constexpr int16_t MAX_STEP_PER_UPDATE = 32;
        for (uint8_t index = 0; index < wheelCount; ++index)
        {
            const int16_t delta = speeds[index] - _lastWheelSpeeds[index];
            if (delta > MAX_STEP_PER_UPDATE)
                speeds[index] = _lastWheelSpeeds[index] + MAX_STEP_PER_UPDATE;
            else if (delta < -MAX_STEP_PER_UPDATE)
                speeds[index] = _lastWheelSpeeds[index] - MAX_STEP_PER_UPDATE;
            _lastWheelSpeeds[index] = speeds[index];
        }
    }
    else
    {
        for (uint8_t index = 0; index < wheelCount; ++index)
        {
            _lastWheelSpeeds[index] = speeds[index];
        }
    }
    _motorOutput.applyWheelSpeeds(speeds, wheelCount, _pwmLimit);

    if (millis() - _lastLogTime >= 500)
    {
        _lastLogTime = millis();
        GenericRobotController_LogI("mode=%u motion=%s %s throttle=%d strafe=%d rotation=%d",
                                    static_cast<uint8_t>(_kinematics->getMode()),
                                    GetTranslationDirection(throttleValue, strafeValue),
                                    GetRotationDirection(rotationValue),
                                    throttle,
                                    strafe,
                                    rotation);
        for (uint8_t index = 0; index < wheelCount; ++index)
        {
            GenericRobotController_LogD("wheel[%u]=%d", index, speeds[index]);
        }
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
    for (uint8_t index = 0; index < MAX_DRIVE_WHEELS; ++index)
    {
        _lastWheelSpeeds[index] = 0;
    }
    _motorOutput.stop();
}