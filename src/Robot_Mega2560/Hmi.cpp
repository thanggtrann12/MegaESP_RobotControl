#include "Hmi.h"
#include <IoPin.h>
#include <stdlib.h>
#include <string.h>

namespace
{
    constexpr uint8_t MAX_FIELDS = 6;

    bool is(const char *a, const char *b) { return strcmp(a, b) == 0; }

    bool number(const char *text, long low, long high, long &value)
    {
        if (text == nullptr || *text == '\0')
        {
            return false;
        }
        char *end;
        value = strtol(text, &end, 10);
        return *end == '\0' && value >= low && value <= high;
    }

    // Pins 0-1 are the USB serial; 14-21 are the other UARTs and I2C.
    bool ioPin(long pin) { return (pin >= 2 && pin <= 13) || (pin >= 22 && pin <= 69); }

    uint8_t split(char *line, char *fields[])
    {
        uint8_t count = 0;
        char *cursor = line;
        while (count < MAX_FIELDS)
        {
            fields[count++] = cursor;
            cursor = strchr(cursor, ',');
            if (cursor == nullptr)
            {
                break;
            }
            *cursor++ = '\0';
        }
        return count;
    }
}

void Hmi::update()
{
    while (_serial.available())
    {
        const int incoming = _serial.read();
        if (incoming < 0)
        {
            break;
        }

        const uint8_t byte = static_cast<uint8_t>(incoming);
        if (byte == 0xFF)
        {
            if (++_terminators == 3)
            {
                finishLine();
            }
            continue;
        }

        _terminators = 0;
        if (byte == '\r' || byte == '\n')
        {
            finishLine();
        }
        else if (byte >= 32 && byte < 127 && _length < sizeof(_line) - 1)
        {
            _line[_length++] = static_cast<char>(byte);
        }
    }

    if (millis() - _lastHeartbeat >= 500)
    {
        _lastHeartbeat = millis();
        _serial.print("SYS,OK");
        endMessage();
    }
}

void Hmi::finishLine()
{
    _terminators = 0;
    if (_length > 0)
    {
        _line[_length] = '\0';
        _length = 0;
        handle(_line);
    }
}

void Hmi::handle(char *line)
{
    char *f[MAX_FIELDS];
    const uint8_t n = split(line, f);
    const char *command = f[0];
    RobotConfig &config = _robot.config();
    long a, b, c, d;
    bool ok = false;

    if (is(command, "GET"))
    {
        sendConfig();
        return;
    }
    else if (is(command, "CHASSIS"))
    {
        ok = n == 2 && number(f[1], 0, static_cast<long>(Chassis::COUNT) - 1, a);
        if (ok)
        {
            config.chassis = static_cast<Chassis>(a);
            _robot.apply();
        }
    }
    else if (is(command, "MOTOR"))
    {
        ok = n == 4 && number(f[1], 1, MOTOR_SLOTS, a) && number(f[2], 0, WHEEL_COUNT, b) && number(f[3], 0, 1, c);
        if (ok)
        {
            _robot.setMotor(static_cast<uint8_t>(a - 1), static_cast<WheelRole>(b), c == 1);
        }
    }
    else if (is(command, "MIX"))
    {
        ok = n == 5 && number(f[1], 1, WHEEL_COUNT, a) && number(f[2], -100, 100, b) &&
             number(f[3], -100, 100, c) && number(f[4], -100, 100, d);
        if (ok)
        {
            Mix &mix = config.custom[a - 1];
            mix.throttle = static_cast<int8_t>(b);
            mix.strafe = static_cast<int8_t>(c);
            mix.rotation = static_cast<int8_t>(d);
            _robot.apply();
        }
    }
    else if (is(command, "PWM"))
    {
        ok = n == 2 && number(f[1], 0, 255, a);
        if (ok)
        {
            config.pwmLimit = static_cast<uint8_t>(a);
            _robot.applyLimits();
        }
    }
    else if (is(command, "ACCEL"))
    {
        ok = n == 2 && number(f[1], 0, 255, a);
        if (ok)
        {
            config.accelStep = static_cast<uint8_t>(a);
            _robot.applyLimits();
        }
    }
    else if (is(command, "SAVE"))
    {
        ok = true;
        _robot.saveConfig();
    }
    else if (is(command, "DEFAULT"))
    {
        ok = true;
        _robot.resetConfig();
    }
    else if (is(command, "CTRL"))
    {
        ok = n == 2 && (is(f[1], "REMOTE") || is(f[1], "MANUAL"));
        if (ok)
        {
            _robot.setSource(is(f[1], "MANUAL") ? Source::MANUAL : Source::REMOTE);
        }
    }
    else if (is(command, "JOY"))
    {
        ok = n == 4 && number(f[1], -100, 100, a) && number(f[2], -100, 100, b) && number(f[3], -100, 100, c) &&
             _robot.joy(static_cast<int8_t>(a), static_cast<int8_t>(b), static_cast<int8_t>(c));
        if (ok)
        {
            return; // streamed continuously; no reply
        }
    }
    else if (is(command, "RUN"))
    {
        ok = n == 3 && number(f[1], 1, MOTOR_SLOTS, a) && number(f[2], -255, 255, b) &&
             _robot.runMotor(static_cast<uint8_t>(a - 1), static_cast<int16_t>(b));
    }
    else if (is(command, "BRAKE"))
    {
        ok = n == 2 && number(f[1], 1, MOTOR_SLOTS, a) && _robot.brakeMotor(static_cast<uint8_t>(a - 1));
    }
    else if (is(command, "STOP"))
    {
        ok = true;
        _robot.stop();
    }
    else if (is(command, "SERVO"))
    {
        ok = n == 3 && number(f[1], 0, 15, a) && number(f[2], 0, 180, b);
        if (ok)
        {
            _robot.servos().write(static_cast<uint8_t>(a), static_cast<uint8_t>(b));
        }
    }
    else if (is(command, "IO"))
    {
        if (n == 3 && number(f[1], 0, 69, a) && ioPin(a) && is(f[2], "R"))
        {
            _serial.print("IO,");
            _serial.print(static_cast<uint8_t>(a));
            _serial.print(',');
            _serial.print(IoPin::read(static_cast<uint8_t>(a)) ? 1 : 0);
            endMessage();
            return;
        }
        else if (n == 3 && number(f[1], 0, 69, a) && ioPin(a) &&
                 (is(f[2], "IN") || is(f[2], "OUT") || is(f[2], "PULLUP")))
        {
            ok = true;
            IoPin::configure(static_cast<uint8_t>(a), is(f[2], "OUT") ? IoPin::OUT : (is(f[2], "IN") ? IoPin::IN : IoPin::IN_PULLUP));
        }
        else if (n == 4 && number(f[1], 0, 69, a) && ioPin(a) && is(f[2], "W") && number(f[3], 0, 1, b))
        {
            ok = true;
            IoPin::write(static_cast<uint8_t>(a), b == 1);
        }
    }
    else if (is(command, "AIN"))
    {
        if (n == 2 && number(f[1], 0, 15, a))
        {
            _serial.print("AIN,");
            _serial.print(static_cast<uint8_t>(a));
            _serial.print(',');
            _serial.print(IoPin::readAnalog(static_cast<uint8_t>(a)));
            endMessage();
            return;
        }
    }

    reply(command, ok);
}

void Hmi::reply(const char *command, bool ok)
{
    _serial.print(ok ? "OK," : "ERR,");
    _serial.print(command);
    endMessage();
}

void Hmi::sendConfig()
{
    const RobotConfig &config = _robot.config();

    _serial.print("CFG,");
    _serial.print(static_cast<uint8_t>(config.chassis));
    _serial.print(',');
    _serial.print(config.pwmLimit);
    _serial.print(',');
    _serial.print(config.accelStep);
    _serial.print(',');
    _serial.print(_robot.ready() ? 1 : 0);
    _serial.print(',');
    _serial.print(_robot.source() == Source::MANUAL ? "MANUAL" : "REMOTE");
    endMessage();

    for (uint8_t slot = 0; slot < MOTOR_SLOTS; ++slot)
    {
        _serial.print("MAP,");
        _serial.print(slot + 1);
        _serial.print(',');
        _serial.print(static_cast<uint8_t>(config.role[slot]));
        _serial.print(',');
        _serial.print((config.inverted >> slot) & 1);
        endMessage();
    }

    for (uint8_t wheel = 0; wheel < WHEEL_COUNT; ++wheel)
    {
        _serial.print("MIX,");
        _serial.print(wheel + 1);
        _serial.print(',');
        _serial.print(config.custom[wheel].throttle);
        _serial.print(',');
        _serial.print(config.custom[wheel].strafe);
        _serial.print(',');
        _serial.print(config.custom[wheel].rotation);
        endMessage();
    }
}

void Hmi::endMessage()
{
    for (uint8_t i = 0; i < 3; ++i)
    {
        _serial.write(0xFF);
    }
}
