#include "HMIService.h"
#include <stdlib.h>
#include <string.h>

HMIService::HMIService(Stream &serial, GenericRobotController &robot, IOPinManager &ioPins, KinematicsRegistry &kinematicsRegistry)
    : _serial(serial), _robot(robot), _ioPins(ioPins), _kinematicsRegistry(kinematicsRegistry), _length(0), _terminatorCount(0), _lastHeartbeat(0)
{
    _buffer[0] = '\0';
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

    if (strncmp(_buffer, "CMD_MODE:", 9) == 0)
    {
        long mode;
        if (!parseInteger(_buffer + 9, mode) || mode < 0 || mode > 4)
        {
            sendMessage("CMD_MODE:ERR,VALUE");
            return;
        }
        IKinematics *kinematics = _kinematicsRegistry.find(static_cast<KinematicsMode>(mode));
        if (kinematics != nullptr && _robot.setKinematics(*kinematics))
        {
            sendMessage("CMD_MODE:OK");
        }
        else
        {
            sendMessage("CMD_MODE:ERR,UNSUPPORTED");
        }
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