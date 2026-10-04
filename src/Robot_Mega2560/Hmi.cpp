#include "Hmi.h"
#include <IoPin.h>
#include <stdlib.h>
#include <string.h>
#include "GenericLogger.h"

namespace
{
    // Trang chứa c0..c5, b3..b8
    constexpr const char *UI_PAGE = "Motor_sel";
    constexpr long CURRENT_WHEEL_NONE = 0;

    constexpr const char *MON_PAGE = "Monitor";
    constexpr uint16_t COLOR_GREEN = 2016;
    constexpr uint16_t COLOR_RED = 63488;
    constexpr uint16_t COLOR_GREY = 33808;
    constexpr const char *CHASSIS_NAMES[] = {
        "TWO WHEEL", "TANK", "OMNI 3", "OMNI 4", "MECANUM",
        "SIX WHEEL", "HOLONOMIC", "XDRIVE", "CUSTOM"};
    constexpr const char *ROLE_SHORT[] = {"--", "FL", "FR", "RL", "RR", "ML", "MR"};

    // Thứ tự mảng này = thứ tự mảng wheelMotor trong syncScreen
    constexpr const char *WHEEL_VARS[6] = {
        "FRONT_LEFT", "FRONT_RIGHT", "REAR_LEFT", "REAR_RIGHT", "MID_LEFT", "MID_RIGHT"};

    uint8_t labelRoles(Chassis c, WheelRole out[4])
    {
        switch (c)
        {
        case Chassis::TWO_WHEEL:
        case Chassis::TANK:
            out[0] = WheelRole::FRONT_LEFT;
            out[1] = WheelRole::FRONT_RIGHT;
            return 2;
        case Chassis::HOLONOMIC:
        case Chassis::OMNI_3:
            out[0] = WheelRole::FRONT_LEFT;
            out[1] = WheelRole::FRONT_RIGHT;
            out[2] = WheelRole::REAR_LEFT;
            return 3;
        default: // OMNI_4, MECANUM, XDRIVE
            out[0] = WheelRole::FRONT_LEFT;
            out[1] = WheelRole::FRONT_RIGHT;
            out[2] = WheelRole::REAR_LEFT;
            out[3] = WheelRole::REAR_RIGHT;
            return 4;
        }
    }
}

ASSIGN_LOG_MACROS(HMI, Serial)
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
    LOG_D("Handling command: %s", command);
    if (is(command, "GET"))
    {
        LOG_D("Command: GET");
        sendConfig();
        return;
    }
    else if (is(command, "SYNC"))
    {
        if (n == 2 && is(f[1], "HOME"))
        {
            LOG_D("Command: SYNC HOME");
            syncHome();
        }
        else if (n == 2 && is(f[1], "I2C"))
        {
            LOG_D("Command: SYNC I2C");
            syncI2C();
        }
        else if (n == 3 && is(f[1], "CHASSIS") && number(f[2], 0, static_cast<long>(Chassis::COUNT) - 1, a))
        {
            LOG_D("Command: SYNC CHASSIS %ld", a);
            syncChassisLabels(static_cast<Chassis>(a));
        }
        else
        {
            LOG_D("Command: SYNC SCREEN");
            syncScreen();
        }
        return;
    }
    else if (is(command, "MON"))
    {
        LOG_D("Command: MON");
        syncMonitor();
        return;
    }

    else if (is(command, "CHASSIS"))
    {
        ok = n == 2 && number(f[1], 0, static_cast<long>(Chassis::COUNT) - 1, a);
        if (ok)
        {
            LOG_D("Handling command: %s, chassis: %ld", command, a);
            _robot.selectChassis(static_cast<Chassis>(a));
        }
    }
    else if (is(command, "MOTOR"))
    {
        LOG_D("Handling command: %s, motor: %s, role: %s, inverted: %s", command, f[1], wheelRoleToString(static_cast<WheelRole>(atoi(f[2]))), f[3]);
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
        LOG_D("Handling command: %s", command);
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
            LOG_D("Handling command: %s, source: %s", command, f[1]);
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

// Biến int global trong program.s: "FRONT_LEFT=3"
void Hmi::sendGlobal(const char *name, long value)
{
    _serial.print(name);
    _serial.print('=');
    _serial.print(value);
    endMessage();
}

// Component number/checkbox: "Motor_sel.c0.val=1"
void Hmi::sendComponentVal(const char *component, long value)
{
    _serial.print(UI_PAGE);
    _serial.print('.');
    _serial.print(component);
    _serial.print(".val=");
    _serial.print(value);
    endMessage();
}

// Component text/button: Motor_sel.b3.txt="BOUNDED"
void Hmi::sendComponentTxt(const char *component, const char *text)
{
    _serial.print(UI_PAGE);
    _serial.print('.');
    _serial.print(component);
    _serial.print(".txt=\"");
    _serial.print(text);
    _serial.print('"');
    endMessage();
}

void Hmi::syncScreen()
{
    const RobotConfig &cfg = _robot.config();
    long wheelMotor[6];
    computeWheelMotor(wheelMotor); // FL, FR, RL, RR, ML, MR -> slot 1..6, 0 = chưa gán
    char name[8];

    for (uint8_t s = 0; s < MOTOR_SLOTS; ++s)
    {
        // Role hiệu lực: role không thuộc chassis hiện tại coi như chưa gán
        const bool bound = cfg.role[s] != WheelRole::NONE &&
                           roleUsedByChassis(cfg.chassis, cfg.role[s]);

        // Checkbox đảo chiều c0..c5
        snprintf(name, sizeof(name), "c%u", s);
        sendComponentVal(name, (cfg.inverted >> s) & 1);

        // Nút nhãn b3..b8
        snprintf(name, sizeof(name), "b%u", s + 3);
        sendComponentTxt(name, bound ? "BOUNDED" : "UNBOUND");
    }

    for (uint8_t i = 0; i < 6; ++i)
    {
        sendGlobal(WHEEL_VARS[i], wheelMotor[i]);
    }

    LOG_D("SYNC sent: FL=%ld FR=%ld RL=%ld RR=%ld ML=%ld MR=%ld",
          wheelMotor[0], wheelMotor[1], wheelMotor[2], wheelMotor[3], wheelMotor[4], wheelMotor[5]);
}

void Hmi::computeWheelMotor(long out[6])
{
    const RobotConfig &cfg = _robot.config();
    for (uint8_t i = 0; i < 6; ++i)
    {
        out[i] = 0;
    }
    for (uint8_t s = 0; s < MOTOR_SLOTS; ++s)
    {
        if (!roleUsedByChassis(cfg.chassis, cfg.role[s]))
        {
            continue;
        }
        switch (cfg.role[s])
        {
        case WheelRole::FRONT_LEFT:
            out[0] = s + 1;
            break;
        case WheelRole::FRONT_RIGHT:
            out[1] = s + 1;
            break;
        case WheelRole::REAR_LEFT:
            out[2] = s + 1;
            break;
        case WheelRole::REAR_RIGHT:
            out[3] = s + 1;
            break;
        case WheelRole::MID_LEFT:
            out[4] = s + 1;
            break;
        case WheelRole::MID_RIGHT:
            out[5] = s + 1;
            break;
        default:
            break;
        }
    }
}

void Hmi::syncChassisLabels(Chassis chassis)
{
    RobotConfig &cfg = _robot.config();
    cfg.storeCurrent(); // pool đang dùng đã nằm trong saved[]
    const ChassisMap &map = cfg.saved[static_cast<uint8_t>(chassis)];

    WheelRole roles[4];
    const uint8_t count = labelRoles(chassis, roles);

    for (uint8_t i = 0; i < count; ++i)
    {
        long slot = 0;
        for (uint8_t s = 0; s < MOTOR_SLOTS; ++s)
        {
            if (map.role[s] == roles[i])
            {
                slot = s + 1;
                break;
            }
        }
        _serial.print('t');
        _serial.print(i);
        _serial.print(".txt=\"");
        if (slot == 0)
        {
            _serial.print("--");
        }
        else
        {
            _serial.print('M');
            _serial.print(slot);
        }
        _serial.print('"');
        endMessage();
    }
    LOG_D("labels page=%u running=%u count=%u", static_cast<uint8_t>(chassis), static_cast<uint8_t>(cfg.chassis), count);
    for (uint8_t s = 0; s < MOTOR_SLOTS; ++s)
    {
        LOG_D("  pool slot %u role=%s", s + 1, wheelRoleToString(map.role[s]));
    }
}

void Hmi::monTxt(const char *comp, const char *text)
{
    _serial.print(MON_PAGE);
    _serial.print('.');
    _serial.print(comp);
    _serial.print(".txt=\"");
    _serial.print(text);
    _serial.print('"');
    endMessage();
}

void Hmi::monCol(const char *comp, uint16_t color)
{
    _serial.print(MON_PAGE);
    _serial.print('.');
    _serial.print(comp);
    _serial.print(".pco=");
    _serial.print(color);
    endMessage();
}

void Hmi::syncMonitor()
{
    const RobotConfig &cfg = _robot.config();
    const Command &cmd = _robot.command();
    char name[8], text[24];

    monTxt("tsrc", _robot.source() == Source::MANUAL ? "MANUAL" : "REMOTE");
    const bool alive = _robot.linkAlive();
    monTxt("tlink", alive ? "Alive" : "Lost");
    monCol("tlink", alive ? COLOR_GREEN : COLOR_RED);
    snprintf(text, sizeof(text), "%lu", static_cast<unsigned long>(_robot.getTimeout()));
    monTxt("ttout", text);

    snprintf(text, sizeof(text), "%d", cmd.throttle);
    monTxt("tthr", text);
    snprintf(text, sizeof(text), "%d", cmd.strafe);
    monTxt("tstr", text);
    snprintf(text, sizeof(text), "%d", cmd.rotation);
    monTxt("trot", text);

    monTxt("tchs", CHASSIS_NAMES[static_cast<uint8_t>(cfg.chassis)]);
    snprintf(text, sizeof(text), "%u", cfg.pwmLimit);
    monTxt("tpwm", text);
    snprintf(text, sizeof(text), "%u", cfg.accelStep);
    monTxt("tacc", text);

    for (uint8_t s = 0; s < MOTOR_SLOTS; ++s)
    {
        const bool used = cfg.role[s] != WheelRole::NONE &&
                          roleUsedByChassis(cfg.chassis, cfg.role[s]);
        const bool rev = (cfg.inverted >> s) & 1;

        snprintf(name, sizeof(name), "tr%u", s);
        monTxt(name, used ? ROLE_SHORT[static_cast<uint8_t>(cfg.role[s])] : "--");

        snprintf(name, sizeof(name), "tv%u", s);
        monTxt(name, rev ? "ON" : "OFF");
        monCol(name, rev ? COLOR_GREEN : COLOR_GREY);
    }

    // Digital IO: 8 chân, đổi danh sách cho đúng chân bạn dùng
    static const uint8_t DIG_PINS[8] = {22, 23, 24, 25, 26, 27, 28, 29};
    char bits[9];
    for (uint8_t i = 0; i < 8; ++i)
    {
        bits[i] = IoPin::read(DIG_PINS[i]) ? '1' : '0';
    }
    bits[8] = '\0';
    monTxt("tdio", bits);

    snprintf(text, sizeof(text), "%u %u %u %u",
             IoPin::readAnalog(0), IoPin::readAnalog(1),
             IoPin::readAnalog(2), IoPin::readAnalog(3));
    monTxt("tain", text);
}

void Hmi::syncHome()
{
    const RobotConfig &cfg = _robot.config();

    WheelRole roles[4];
    const uint8_t need = labelRoles(cfg.chassis, roles);
    uint8_t bound = 0;
    for (uint8_t i = 0; i < need; ++i)
    {
        for (uint8_t s = 0; s < MOTOR_SLOTS; ++s)
        {
            if (cfg.role[s] == roles[i])
            {
                ++bound;
                break;
            }
        }
    }

    char text[16];
    _serial.print("Main.thchs.txt=\"");
    _serial.print(CHASSIS_NAMES[static_cast<uint8_t>(cfg.chassis)]);
    _serial.print('"');
    endMessage();

    snprintf(text, sizeof(text), "%u/%u BOUND", bound, need);
    _serial.print("Main.thprof.txt=\"");
    _serial.print(text);
    _serial.print('"');
    endMessage();

    snprintf(text, sizeof(text), "OK", bound, need);
    _serial.print("Main.tsts.txt=\"");
    _serial.print(text);
    _serial.print('"');
    endMessage();
}

void Hmi::syncI2C()
{
    // Tốc độ clock thực tế, tính từ thanh ghi TWBR và prescaler
    const uint8_t prescaler = 1 << (2 * (TWSR & 0x03)); // 1, 4, 16, 64
    const uint32_t clockHz = F_CPU / (16UL + 2UL * TWBR * prescaler);

    char speed[16];
    snprintf(speed, sizeof(speed), "%lu kHz", static_cast<unsigned long>(clockHz / 1000UL));
    _serial.print("I2c_test.tbussp.txt=\"");
    _serial.print(speed);
    _serial.print('"');
    endMessage();

    // Quét địa chỉ 7 bit hợp lệ: 0x08..0x77
    char list[96];
    uint8_t length = 0;
    uint8_t found = 0;
    list[0] = '\0';

    for (uint8_t address = 0x08; address <= 0x77; ++address)
    {
        Wire.beginTransmission(address);
        if (Wire.endTransmission() != 0)
        {
            continue;
        }
        ++found;
        if (length + 6 < sizeof(list)) // chừa chỗ cho "0xNN " và ký tự kết thúc
        {
            length += snprintf(list + length, sizeof(list) - length, "0x%02X ", address);
        }
    }

    if (found == 0)
    {
        snprintf(list, sizeof(list), "No device");
    }

    _serial.print("I2c_test.devlist.txt=\"");
    _serial.print(list);
    _serial.print('"');
    endMessage();

    LOG_D("I2C scan: %u device(s), %lu Hz", found, static_cast<unsigned long>(clockHz));
}