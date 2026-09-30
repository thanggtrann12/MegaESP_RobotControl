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
      _hasSequenceNumber(false) {}

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

    int16_t throttle = mapAxisToMotor(packet.throttle);
    int16_t strafe = mapAxisToMotor(packet.strafe);
    int16_t rotation = mapAxisToMotor(packet.rotation);

    // Standard mecanum mix; correct individual motor polarity separately if needed.
    int16_t frontLeft = throttle + strafe + rotation;
    int16_t rearLeft = throttle - strafe + rotation;
    int16_t frontRight = throttle - strafe - rotation;
    int16_t rearRight = throttle + strafe - rotation;

    // Normalize PWM nếu vượt quá 255
    int16_t maxMagnitude = max(max(abs(frontLeft), abs(rearLeft)),
                               max(abs(frontRight), abs(rearRight)));
    if (maxMagnitude > 255)
    {
        frontLeft = static_cast<int16_t>(frontLeft * 255L / maxMagnitude);
        rearLeft = static_cast<int16_t>(rearLeft * 255L / maxMagnitude);
        frontRight = static_cast<int16_t>(frontRight * 255L / maxMagnitude);
        rearRight = static_cast<int16_t>(rearRight * 255L / maxMagnitude);
    }

    _leftMotor1.setSpeed(frontLeft);
    _leftMotor2.setSpeed(rearLeft);
    _rightMotor1.setSpeed(frontRight);
    _rightMotor2.setSpeed(rearRight);
    if (millis() - _lastLogTime >= 500)
    {
        _lastLogTime = millis();
        GenericRobotController_LogI("motion=%s %s throttle=%d strafe=%d rotation=%d FL=%d RL=%d FR=%d RR=%d",
                                    GetTranslationDirection(packet.throttle, packet.strafe),
                                    GetRotationDirection(packet.rotation),
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