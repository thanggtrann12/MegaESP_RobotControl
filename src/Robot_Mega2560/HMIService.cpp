/**
 * @file HMIService.cpp
 * @brief HMI command parsing and command dispatch implementation.
 */

#include "HMIService.h"
#include <stdlib.h>
#include <string.h>

HMIService::HMIService(Stream &serial,
                       GenericRobotController &robot,
                       IOPinManager &ioPins,
                       KinematicsRegistry &kinematicsRegistry,
                       const CapabilityRegistry &capabilities)
    : _serial(serial),
      _robot(robot),
      _ioPins(ioPins),
      _kinematicsRegistry(kinematicsRegistry),
      _capabilities(capabilities),
      _length(0),
      _terminatorCount(0),
      _lastHeartbeat(0),
      _motorBindHandler(nullptr),
      _motorBindContext(nullptr),
      _configHandler(nullptr),
      _configContext(nullptr)
{
    _buffer[0] = '\0';
}

void HMIService::setMotorBindHandler(MotorBindHandler handler, void *context)
{
    _motorBindHandler = handler;
    _motorBindContext = context;
}

void HMIService::setConfigHandler(ConfigHandler handler, void *context)
{
    _configHandler = handler;
    _configContext = context;
}

void HMIService::update()
{
    while (_serial.available())
    {
        int incoming = _serial.read();
        if (incoming < 0)
        {
            break;
        }

        uint8_t byte = static_cast<uint8_t>(incoming);
        if (byte == 0xFF)
        {
            if (++_terminatorCount == 3)
            {
                processCommand();
                _length = 0;
                _buffer[0] = '\0';
                _terminatorCount = 0;
            }
            continue;
        }

        _terminatorCount = 0;
        if (byte == '\r' || byte == '\n')
        {
            processCommand();
            _length = 0;
            _buffer[0] = '\0';
            continue;
        }

        if (byte >= 32 && byte <= 126 && _length < sizeof(_buffer) - 1)
        {
            _buffer[_length++] = static_cast<char>(byte);
            _buffer[_length] = '\0';
        }
    }

    if (millis() - _lastHeartbeat >= 500)
    {
        _lastHeartbeat = millis();
        sendMessage("CMD_SYS:OK");
    }
}

void HMIService::processCommand()
{
    if (_length == 0)
    {
        return;
    }

    if (strcmp(_buffer, "CMD_CTRL:MANUAL") == 0)
    {
        _robot.setControlSource(ControlSource::HMI_MANUAL);
        sendMessage("CMD_CTRL:OK,MANUAL");
        return;
    }
    if (strcmp(_buffer, "CMD_CTRL:REMOTE") == 0)
    {
        _robot.setControlSource(ControlSource::REMOTE);
        sendMessage("CMD_CTRL:OK,REMOTE");
        return;
    }
    if (strcmp(_buffer, "CMD_SYS:OK") == 0)
    {
        return;
    }

    if (strcmp(_buffer, "CMD_CAP:GET") == 0)
    {
        sendCapabilities();
        return;
    }

    if (strcmp(_buffer, "CMD_CFG:BEGIN") == 0 ||
        strcmp(_buffer, "CMD_CFG:VALIDATE") == 0 ||
        strcmp(_buffer, "CMD_CFG:APPLY") == 0 ||
        strcmp(_buffer, "CMD_CFG:ABORT") == 0 ||
        strcmp(_buffer, "CMD_CFG:SAVE") == 0)
    {
        HmiConfigCommand command = HmiConfigCommand::BEGIN;
        const char *suffix = _buffer + 8;
        if (strcmp(suffix, "VALIDATE") == 0)
            command = HmiConfigCommand::VALIDATE;
        else if (strcmp(suffix, "APPLY") == 0)
            command = HmiConfigCommand::APPLY;
        else if (strcmp(suffix, "ABORT") == 0)
            command = HmiConfigCommand::ABORT;
        else if (strcmp(suffix, "SAVE") == 0)
            command = HmiConfigCommand::SAVE;

        if (_configHandler != nullptr && _configHandler(_configContext, command, 0, 0))
            sendMessage("CMD_CFG:OK");
        else
            sendMessage("CMD_CFG:ERR");
        return;
    }

    if (strncmp(_buffer, "CMD_CFG:MODE,", 13) == 0 ||
        strncmp(_buffer, "CMD_CFG:PWM,", 12) == 0 ||
        strncmp(_buffer, "CMD_CFG:PROFILE,", 16) == 0)
    {
        const bool isMode = strncmp(_buffer, "CMD_CFG:MODE,", 13) == 0;
        const bool isProfile = strncmp(_buffer, "CMD_CFG:PROFILE,", 16) == 0;
        const char *valueText = _buffer + (isMode ? 13 : (isProfile ? 16 : 12));
        long value;
        const long maximum = isMode ? 4 : (isProfile ? 1 : 255);
        if (!parseInteger(valueText, value) || value < 0 || value > maximum ||
            _configHandler == nullptr ||
            !_configHandler(_configContext,
                            isMode ? HmiConfigCommand::SET_MODE : (isProfile ? HmiConfigCommand::SET_PROFILE : HmiConfigCommand::SET_PWM),
                            static_cast<uint8_t>(value), 0))
        {
            sendMessage("CMD_CFG:ERR,VALUE");
        }
        else
        {
            sendMessage("CMD_CFG:OK");
        }
        return;
    }

    if (strncmp(_buffer, "CMD_CFG:MOTOR,", 14) == 0)
    {
        char *slotText = _buffer + 14;
        char *roleText = strchr(slotText, ',');
        long slot;
        long role;
        if (roleText == nullptr)
        {
            sendMessage("CMD_CFG:ERR,FORMAT");
            return;
        }
        *roleText++ = '\0';
        if (!parseInteger(slotText, slot) || !parseInteger(roleText, role) ||
            slot < 1 || slot > MAX_MOTOR_PORT ||
            role < 0 || role > static_cast<long>(MotorRole::AUXILIARY) ||
            _configHandler == nullptr ||
            !_configHandler(_configContext, HmiConfigCommand::SET_MOTOR,
                            static_cast<uint8_t>(slot - 1), static_cast<uint8_t>(role)))
        {
            sendMessage("CMD_CFG:ERR,VALUE");
        }
        else
        {
            sendMessage("CMD_CFG:OK");
        }
        return;
    }

    if (strncmp(_buffer, "CMD_CFG:INVERT,", 15) == 0)
    {
        char *slotText = _buffer + 15;
        char *invertText = strchr(slotText, ',');
        long slot;
        long invert;
        if (invertText == nullptr)
        {
            sendMessage("CMD_CFG:ERR,FORMAT");
            return;
        }
        *invertText++ = '\0';
        if (!parseInteger(slotText, slot) || !parseInteger(invertText, invert) ||
            slot < 1 || slot > MAX_MOTOR_PORT || invert < 0 || invert > 1 ||
            _configHandler == nullptr ||
            !_configHandler(_configContext, HmiConfigCommand::SET_INVERT,
                            static_cast<uint8_t>(slot - 1), static_cast<uint8_t>(invert)))
        {
            sendMessage("CMD_CFG:ERR,VALUE");
        }
        else
        {
            sendMessage("CMD_CFG:OK");
        }
        return;
    }

    if (strncmp(_buffer, "CMD_MODE:", 9) == 0)
    {
        long mode;
        if (!parseInteger(_buffer + 9, mode) || mode < 0 || mode > 4)
        {
            sendMessage("CMD_MODE:ERR,VALUE");
            return;
        }
        bool applied = false;
        if (_configHandler != nullptr)
        {
            applied = _configHandler(_configContext,
                                     HmiConfigCommand::SET_MODE,
                                     static_cast<uint8_t>(mode),
                                     0) &&
                      _configHandler(_configContext, HmiConfigCommand::APPLY, 0, 0);
        }
        else
        {
            IKinematics *kinematics = _kinematicsRegistry.find(static_cast<KinematicsMode>(mode));
            applied = kinematics != nullptr && _robot.setKinematics(*kinematics);
        }
        if (applied)
        {
            sendMessage("CMD_MODE:OK");
        }
        else
        {
            sendMessage("CMD_MODE:ERR,UNSUPPORTED");
        }
        return;
    }

    if (strncmp(_buffer, "CMD_MAP:", 8) == 0)
    {
        char *slotText = _buffer + 8;
        char *roleText = strchr(slotText, ',');
        if (roleText == nullptr)
        {
            sendMessage("CMD_MAP:ERR,FORMAT");
            return;
        }
        *roleText++ = '\0';

        long slotIndex;
        MotorRole roleId;
        if (!parseInteger(slotText, slotIndex) || slotIndex < 1 || slotIndex > MAX_MOTOR_PORT)
        {
            sendMessage("CMD_MAP:ERR,SLOT");
            return;
        }
        if (!parseMotorRole(roleText, roleId))
        {
            sendMessage("CMD_MAP:ERR,ROLE");
            return;
        }
        if (_motorBindHandler == nullptr)
        {
            sendMessage("CMD_MAP:ERR,UNSUPPORTED");
            return;
        }

        _motorBindHandler(_motorBindContext, static_cast<uint8_t>(slotIndex - 1), roleId);
        sendMessage("CMD_MAP:OK");
        return;
    }

    if (strncmp(_buffer, "CMD_JOY:", 8) == 0)
    {
        int8_t throttle;
        int8_t strafe;
        int8_t rotation;
        if (!parseMotion(_buffer + 8, throttle, strafe, rotation))
        {
            sendMessage("CMD_JOY:ERR,VALUE");
        }
        else if (_robot.getControlSource() != ControlSource::HMI_MANUAL)
        {
            sendMessage("CMD_JOY:ERR,REMOTE_MODE");
        }
        else
        {
            _robot.setHmiMotion(throttle, strafe, rotation);
        }
        return;
    }

    if (strncmp(_buffer, "CMD_M:", 6) == 0)
    {
        char *motorText = _buffer + 6;
        char *direction = strchr(motorText, ',');
        if (direction == nullptr)
        {
            sendMessage("CMD_M:ERR,FORMAT");
            return;
        }
        *direction++ = '\0';
        char *pwmText = strchr(direction, ',');
        if (pwmText == nullptr)
        {
            sendMessage("CMD_M:ERR,FORMAT");
            return;
        }
        *pwmText++ = '\0';
        long motorId;
        long pwm;
        if (!parseInteger(motorText, motorId) || !parseInteger(pwmText, pwm) ||
            motorId < 1 || motorId > 4 || pwm < 0 || pwm > 255)
        {
            sendMessage("CMD_M:ERR,VALUE");
            return;
        }
        if (_robot.getControlSource() != ControlSource::HMI_MANUAL)
        {
            sendMessage("CMD_M:ERR,REMOTE_MODE");
            return;
        }
        if (strcmp(direction, "STOP") == 0)
        {
            _robot.releaseManualMotor(static_cast<uint8_t>(motorId));
            sendMessage("CMD_M:OK,STOP");
        }
        else if (strcmp(direction, "FWD") == 0 || strcmp(direction, "REV") == 0)
        {
            int16_t speed = static_cast<int16_t>(pwm);
            if (strcmp(direction, "REV") == 0)
            {
                speed = -speed;
            }
            if (_robot.setManualMotor(static_cast<uint8_t>(motorId), speed))
            {
                sendMessage("CMD_M:OK,RUNNING");
            }
            else
            {
                sendMessage("CMD_M:ERR,MOTOR");
            }
        }
        else
        {
            sendMessage("CMD_M:ERR,DIRECTION");
        }
        return;
    }

    if (strncmp(_buffer, "CMD_LIMIT:PWM,", 14) == 0)
    {
        long limit;
        if (!parseInteger(_buffer + 14, limit) || limit < 0 || limit > 255)
        {
            sendMessage("CMD_LIMIT:ERR,VALUE");
        }
        else
        {
            _robot.setPwmLimit(static_cast<uint8_t>(limit));
            sendMessage("CMD_LIMIT:OK");
        }
        return;
    }

    if (strncmp(_buffer, "CMD_SIM:TOGGLE_IO,", 18) == 0 ||
        strncmp(_buffer, "CMD_SIM:RESET_IO,", 17) == 0)
    {
        bool toggle = strncmp(_buffer, "CMD_SIM:TOGGLE_IO,", 18) == 0;
        const char *pinText = _buffer + (toggle ? 18 : 17);
        uint8_t pin;
        if (!parsePin(pinText, pin))
        {
            sendMessage("CMD_SIM:ERR,PIN");
            return;
        }
        bool success = toggle ? _ioPins.toggleSimulation(pin)
                              : _ioPins.resetSimulation(pin);
        if (!success)
        {
            sendMessage("CMD_SIM:ERR,UNCONFIGURED_PIN");
        }
        else
        {
            sendIoStatus(pin);
        }
        return;
    }

    sendMessage("CMD_ERR:UNKNOWN_COMMAND");
}

void HMIService::sendMessage(const char *message)
{
    _serial.print(message);
    _serial.write(0xFF);
    _serial.write(0xFF);
    _serial.write(0xFF);
}

void HMIService::sendIoStatus(uint8_t pin)
{
    bool isAnalog;
    bool isSimulated;
    int16_t value;
    if (!_ioPins.getStatus(pin, isAnalog, value, isSimulated))
    {
        return;
    }
    _serial.print("IO_STATUS:");
    _serial.print(pin);
    _serial.print(isAnalog ? ",ANALOG," : ",DIGITAL,");
    _serial.print(value);
    _serial.print(isSimulated ? ",SIM" : ",REAL");
    _serial.write(0xFF);
    _serial.write(0xFF);
    _serial.write(0xFF);
}

bool HMIService::parseInteger(const char *text, long &value) const
{
    if (text == nullptr || *text == '\0')
    {
        return false;
    }
    char *end;
    value = strtol(text, &end, 10);
    return end != text && *end == '\0';
}

bool HMIService::parsePin(const char *text, uint8_t &pin) const
{
    if (text != nullptr && text[0] == 'A' && text[1] >= '0' && text[1] <= '5' && text[2] == '\0')
    {
        pin = ANALOG_PINS[text[1] - '0'];
        return true;
    }

    long numericPin;
    if (!parseInteger(text, numericPin) || numericPin < 0 || numericPin > 69)
    {
        return false;
    }
    pin = static_cast<uint8_t>(numericPin);
    return true;
}

bool HMIService::parseMotorRole(const char *text, MotorRole &role) const
{
    long roleValue;
    if (!parseInteger(text, roleValue) || roleValue < 0 || roleValue > static_cast<long>(MotorRole::AUXILIARY))
    {
        return false;
    }
    role = static_cast<MotorRole>(roleValue);
    return true;
}

bool HMIService::parseMotion(const char *text, int8_t &throttle, int8_t &strafe, int8_t &rotation) const
{
    float values[3];
    const char *cursor = text;
    for (uint8_t index = 0; index < 3; ++index)
    {
        bool negative = false;
        if (*cursor == '-' || *cursor == '+')
        {
            negative = *cursor == '-';
            ++cursor;
        }

        bool hasDigit = false;
        float value = 0.0f;
        while (*cursor >= '0' && *cursor <= '9')
        {
            hasDigit = true;
            value = value * 10.0f + (*cursor++ - '0');
        }

        if (*cursor == '.')
        {
            ++cursor;
            float fraction = 0.1f;
            while (*cursor >= '0' && *cursor <= '9')
            {
                hasDigit = true;
                value += (*cursor++ - '0') * fraction;
                fraction *= 0.1f;
            }
        }

        if (!hasDigit)
        {
            return false;
        }
        values[index] = negative ? -value : value;
        if (values[index] < -1.0f || values[index] > 1.0f)
        {
            return false;
        }
        if (index < 2)
        {
            if (*cursor != ',')
            {
                return false;
            }
            ++cursor;
        }
        else if (*cursor != '\0')
        {
            return false;
        }
    }
    throttle = static_cast<int8_t>(values[0] * 100.0f);
    strafe = static_cast<int8_t>(values[1] * 100.0f);
    rotation = static_cast<int8_t>(values[2] * 100.0f);
    return true;
}

void HMIService::sendCapabilities()
{
    _serial.print("CAP:KIN,");
    for (uint8_t mode = 0; mode <= static_cast<uint8_t>(KinematicsMode::MODE_CUSTOM); ++mode)
    {
        if (mode > 0)
        {
            _serial.print(',');
        }
        _serial.print(_capabilities.supportsKinematics(static_cast<KinematicsMode>(mode)) ? 1 : 0);
    }
    _serial.print(",DRV,");
    _serial.print(_capabilities.supportsDriver(RobotDriverType::TA6586_PCA9685) ? 1 : 0);
    _serial.write(0xFF);
    _serial.write(0xFF);
    _serial.write(0xFF);
}