#include "Hmi.h"
#include <IoPin.h>
#include <Wire.h>
#include <stdlib.h>
#include <string.h>
#include "RobotPins.h"
#include "GenericLogger.h"

ASSIGN_LOG_MACROS(HMI, Serial)

namespace
{
    // ---------------------------------------------------------------- hằng số
    constexpr const char *UI_PAGE = "Motor_sel"; // chứa c0..c5, b3..b8
    constexpr const char *MON_PAGE = "Monitor";

    constexpr uint16_t COLOR_GREEN = 2016;
    constexpr uint16_t COLOR_RED = 63488;
    constexpr uint16_t COLOR_GREY = 33808;

    constexpr const char *ROLE_SHORT[] = {"--", "FL", "FR", "RL", "RR", "ML", "MR"};
    constexpr uint8_t ROLE_SHORT_COUNT = sizeof(ROLE_SHORT) / sizeof(ROLE_SHORT[0]);

    // Thứ tự nhãn t0..t3 trên các trang chassis
    constexpr WheelRole LABEL_ORDER[4] = {WheelRole::FRONT_LEFT, WheelRole::FRONT_RIGHT,
                                          WheelRole::REAR_LEFT, WheelRole::REAR_RIGHT};

    constexpr uint8_t MAX_FIELDS = 6;
    constexpr uint8_t TERMINATOR_BYTE = 0xFF;
    constexpr uint8_t TERMINATOR_COUNT = 3;
    constexpr uint32_t HEARTBEAT_MS = 500;
    constexpr long NUMBER_LIMIT = 100000L; // mọi tham số HMI đều nhỏ hơn: chặn tràn khi parse

    constexpr uint8_t DIG_PINS[8] = {IO_PIN_NUM_1, IO_PIN_NUM_2, IO_PIN_NUM_3, IO_PIN_NUM_4,
                                    IO_PIN_NUM_5, IO_PIN_NUM_6, IO_PIN_NUM_7, IO_PIN_NUM_8};
    constexpr uint8_t ADC_PINS[6] = {ANALOG_PIN_NUM_1, ANALOG_PIN_NUM_2, ANALOG_PIN_NUM_3,
                                    ANALOG_PIN_NUM_4, ANALOG_PIN_NUM_5, ANALOG_PIN_NUM_6};

    constexpr uint8_t I2C_FIRST = 0x08;
    constexpr uint8_t I2C_LAST = 0x77;
    constexpr uint16_t I2C_TIMEOUT_US = 3000;

    // Chân đã dùng cho motor driver / servo / CS ... -> HMI không được đụng vào.
    // TODO: điền đúng các chân của bạn (giá trị 0xFF là placeholder, không khớp chân nào).
    // Nếu để trống, màn hình có thể kéo chân PWM/DIR của driver bằng lệnh IO.
    constexpr uint8_t RESERVED_PINS[] = {0xFF};

    // ---------------------------------------------------------------- parse
    /**
     * @brief So sánh hai chuỗi, trả về true nếu chúng giống nhau.
     */
    bool is(const char *a, const char *b) { return strcmp(a, b) == 0; }

    /**
     * @brief Chuyển chuỗi số nguyên (có thể có dấu) thành long, kiểm tra trong khoảng [low, high].
     *
     * Tự viết thay cho strtol: nhẹ hơn, không nhận khoảng trắng đầu chuỗi, và chặn tràn số.
     * @param text Chuỗi cần chuyển
     * @param low Giá trị nhỏ nhất
     * @param high Giá trị lớn nhất
     * @param value Chỉ được ghi khi trả về true
     * @return true nếu hợp lệ và nằm trong khoảng
     */
    bool number(const char *text, long low, long high, long &value)
    {
        if (text == nullptr || *text == '\0')
            return false;

        bool negative = false;

        if (*text == '-' || *text == '+')
        {
            negative = (*text == '-');
            ++text;
        }

        if (*text == '\0')
            return false;

        long parsed = 0;

        for (; *text != '\0'; ++text)
        {
            if (*text < '0' || *text > '9')
                return false;

            const long digit = *text - '0';

            if (parsed > (NUMBER_LIMIT - digit) / 10)
                return false;

            parsed = parsed * 10 + digit;
        }

        if (negative)
            parsed = -parsed;

        if (parsed < low || parsed > high)
            return false;

        value = parsed;
        return true;
    }

    bool isReserved(long pin)
    {
        for (uint8_t reserved : RESERVED_PINS)
        {
            if (pin == reserved)
            {
                return true;
            }
        }
        return false;
    }

    // Pins 0-1 là USB serial; 14-21 là các UART khác và I2C.
    bool ioPin(long pin)
    {
        return ((pin >= 2 && pin <= 13) || (pin >= 22 && pin <= 69)) && !isReserved(pin);
    }

    bool parsePin(const char *text, long &pin)
    {
        return number(text, 0, 69, pin) && ioPin(pin);
    }

    /**
     * @brief Tách chuỗi thành các trường, phân tách bằng dấu phẩy (sửa trực tiếp vào line).
     * @return Số lượng trường đã tách (tối đa MAX_FIELDS)
     */
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

    // ---------------------------------------------------------------- tiện ích định dạng
    /**
     * @brief Ghép tên component dạng "<prefix><index>", ví dụ ("tr", 3) -> "tr3".
     * @param buf Bộ đệm >= 4 byte
     * @param prefix Tối đa 2 ký tự
     * @param index 0..9
     */
    const char *indexed(char *buf, const char *prefix, uint8_t index)
    {
        uint8_t n = 0;
        while (*prefix != '\0' && n < 2)
        {
            buf[n++] = *prefix++;
        }
        buf[n++] = static_cast<char>('0' + index);
        buf[n] = '\0';
        return buf;
    }

    // Nhanh và nhẹ hơn snprintf("%ld")
    const char *num(char *buf, long value) { return ltoa(value, buf, 10); }

    const char *roleShort(WheelRole role)
    {
        const uint8_t index = static_cast<uint8_t>(role);
        return index < ROLE_SHORT_COUNT ? ROLE_SHORT[index] : "--";
    }

    /**
     * @brief Số bánh có nhãn trên trang chassis (t0..t3 theo LABEL_ORDER).
     */
    uint8_t labelCount(Chassis c)
    {
        switch (c)
        {
        case Chassis::TWO_WHEEL:
        case Chassis::TANK:
            return 2;
        case Chassis::HOLONOMIC:
        case Chassis::OMNI_3:
            return 3;
        default: // OMNI_4, MECANUM, XDRIVE, ...
            return 4;
        }
    }

    /**
     * @brief Tìm slot đang giữ một role.
     * @return slot 1..MOTOR_SLOTS, hoặc 0 nếu chưa gán
     */
    uint8_t slotOf(const WheelRole *roles, WheelRole role)
    {
        for (uint8_t s = 0; s < MOTOR_SLOTS; ++s)
        {
            if (roles[s] == role)
            {
                return s + 1;
            }
        }
        return 0;
    }

    // Hash 16-bit (FNV-1a rút gọn) để nhận biết trường monitor nào đã đổi
    uint16_t hashText(const char *s)
    {
        uint16_t h = 0x811C;
        while (*s != '\0')
        {
            h = static_cast<uint16_t>((h ^ static_cast<uint8_t>(*s++)) * 0x0193u);
        }
        return h;
    }

    // ---------------------------------------------------------------- gửi xuống màn hình
    void endMsg(Print &out)
    {
        for (uint8_t i = 0; i < TERMINATOR_COUNT; ++i)
        {
            out.write(TERMINATOR_BYTE);
        }
    }

    // "<page>.<comp><attr>" (page == nullptr: trang đang hiển thị)
    void beginAttr(Print &out, const char *page, const char *comp, const __FlashStringHelper *attr)
    {
        if (page != nullptr)
        {
            out.print(page);
            out.print('.');
        }
        out.print(comp);
        out.print(attr);
    }

    void sendTxt(Print &out, const char *page, const char *comp, const char *text)
    {
        beginAttr(out, page, comp, F(".txt=\""));
        out.print(text);
        out.print('"');
        endMsg(out);
    }

    void sendVal(Print &out, const char *page, const char *comp, long value)
    {
        beginAttr(out, page, comp, F(".val="));
        out.print(value);
        endMsg(out);
    }

    void sendCol(Print &out, const char *page, const char *comp, uint16_t color)
    {
        beginAttr(out, page, comp, F(".pco="));
        out.print(color);
        endMsg(out);
    }

    // Biến int global trong program.s: "FRONT_LEFT=3"
    void sendGlobal(Print &out, const char *name, long value)
    {
        out.print(name);
        out.print('=');
        out.print(value);
        endMsg(out);
    }
}

// =============================================================== command lookup table
// Đặt trong flash. JOY / MON / ADC (tần suất cao) đứng đầu để tìm thấy sớm nhất.
// minFields/maxFields đếm cả từ lệnh; sai số trường thì handle() trả ERR, handler không cần kiểm tra n.
const Hmi::CommandEntry Hmi::CMD_TABLE[] PROGMEM = {
    //  name       handler          min max quiet
    {"JOY",     &Hmi::handleJoy,     4, 4, true },
    {"MON",     &Hmi::handleMon,     1, 2, true },
    {"ADC",     &Hmi::handleAdc,     1, 1, true },
    {"RUN",     &Hmi::handleRun,     3, 3, false},
    {"SYNC",    &Hmi::handleSync,    1, 3, false},
    {"GET",     &Hmi::handleGet,     1, 1, false},
    {"CHASSIS", &Hmi::handleChassis, 2, 2, false},
    {"MOTOR",   &Hmi::handleMotor,   4, 4, false},
    {"MIX",     &Hmi::handleMix,     5, 5, false},
    {"PWM",     &Hmi::handlePwm,     2, 2, false},
    {"ACCEL",   &Hmi::handleAccel,   2, 2, false},
    {"SAVE",    &Hmi::handleSave,    1, 1, false},
    {"DEFAULT", &Hmi::handleDefault, 1, 1, false},
    {"CTRL",    &Hmi::handleCtrl,    2, 2, false},
    {"BRAKE",   &Hmi::handleBrake,   2, 2, false},
    {"STOP",    &Hmi::handleStop,    1, 1, false},
    {"SERVO",   &Hmi::handleServo,   3, 3, false},
    {"IO",      &Hmi::handleIo,      3, 4, false},
    {"AIN",     &Hmi::handleAin,     2, 2, false},
};

const uint8_t Hmi::CMD_TABLE_SIZE = sizeof(CMD_TABLE) / sizeof(CMD_TABLE[0]);

// =============================================================== line handling
/**
 * @brief Đọc UART từ màn hình, gom thành dòng lệnh và gửi heartbeat định kỳ.
 */
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
        if (byte == TERMINATOR_BYTE)
        {
            if (++_terminators == TERMINATOR_COUNT)
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
        else if (byte >= 32 && byte < 127)
        {
            if (_length < LINE_MAX - 1)
            {
                _line[_length++] = static_cast<char>(byte);
            }
            else
            {
                _overflow = true; // dòng quá dài: huỷ cả dòng, không xử lý bản cắt cụt
            }
        }
    }

    const uint32_t now = millis();
    if (now - _lastHeartbeat >= HEARTBEAT_MS)
    {
        _lastHeartbeat = now;
        _serial.print(F("SYS,OK"));
        endMsg(_serial);
    }
}

/**
 * @brief Hoàn tất một dòng lệnh: huỷ nếu quá dài, ngược lại đưa cho handle().
 */
void Hmi::finishLine()
{
    _terminators = 0;
    if (_overflow)
    {
        _overflow = false;
        _length = 0;
        LOG_D("Line too long, dropped");
        reply("LINE", false);
        return;
    }
    if (_length > 0)
    {
        _line[_length] = '\0';
        _length = 0;
        handle(_line);
    }
}

/**
 * @brief Tra bảng lệnh, kiểm tra số trường rồi gọi handler tương ứng.
 * @param line Dòng lệnh (bị sửa tại chỗ khi tách trường)
 */
void Hmi::handle(char *line)
{
    char *f[MAX_FIELDS] = {}; // khởi tạo nullptr: không bao giờ đọc con trỏ rác
    const uint8_t n = split(line, f);
    const char *command = f[0];

    for (uint8_t i = 0; i < CMD_TABLE_SIZE; ++i)
    {
        if (strcmp_P(command, CMD_TABLE[i].name) != 0)
        {
            continue;
        }

        CmdHandler handler;
        memcpy_P(&handler, &CMD_TABLE[i].handler, sizeof(handler));
        const bool quiet = pgm_read_byte(&CMD_TABLE[i].quiet) != 0;
        const uint8_t minFields = pgm_read_byte(&CMD_TABLE[i].minFields);
        const uint8_t maxFields = pgm_read_byte(&CMD_TABLE[i].maxFields);

        if (!quiet)
        {
            LOG_D("Command: %s (%u field(s))", command, n);
        }
        if (n < minFields || n > maxFields)
        {
            if (!quiet)
            {
                reply(command, false);
            }
            return;
        }

        const Result result = (this->*handler)(f, n);
        if (!quiet && result != Result::NONE)
        {
            reply(command, result == Result::OK);
        }
        return;
    }

    reply(command, false); // lệnh không tồn tại
}

void Hmi::reply(const char *command, bool ok)
{
    _serial.print(ok ? F("OK,") : F("ERR,"));
    _serial.print(command);
    endMsg(_serial);
}

// =============================================================== command handlers
Hmi::Result Hmi::handleGet(char **, uint8_t)
{
    sendConfig();
    return Result::NONE;
}

Hmi::Result Hmi::handleSync(char **f, uint8_t n)
{
    long a = 0;
    if (n == 1)
    {
        syncScreen();
        return Result::NONE;
    }
    if (n == 2 && is(f[1], "HOME"))
    {
        syncHome();
        return Result::NONE;
    }
    if (n == 2 && is(f[1], "I2C"))
    {
        syncI2C();
        return Result::NONE;
    }
    if (n == 3 && is(f[1], "CHASSIS") &&
        number(f[2], 0, static_cast<long>(Chassis::COUNT) - 1, a))
    {
        syncChassisLabels(static_cast<Chassis>(a));
        return Result::NONE;
    }
    return Result::ERR; // tham số sai
}

Hmi::Result Hmi::handleMon(char **f, uint8_t n)
{
    if (n == 2 && is(f[1], "ALL"))
    {
        memset(_monHash, 0, sizeof(_monHash)); // buộc gửi lại toàn bộ (vừa vào trang Monitor)
    }
    syncMonitor();
    return Result::NONE;
}

Hmi::Result Hmi::handleAdc(char **, uint8_t)
{
    syncADC();
    return Result::NONE;
}

Hmi::Result Hmi::handleChassis(char **f, uint8_t)
{
    long a = 0;
    if (!number(f[1], 0, static_cast<long>(Chassis::COUNT) - 1, a))
    {
        return Result::ERR;
    }
    const Chassis chassis = static_cast<Chassis>(a);
    if (chassis != _robot.config().chassis) // chọn lại chassis đang chạy: khỏi apply() và khỏi dừng motor
    {
        _robot.selectChassis(chassis);
    }
    return Result::OK;
}

Hmi::Result Hmi::handleMotor(char **f, uint8_t)
{
    long a = 0, b = 0, c = 0;
    if (!(number(f[1], 1, MOTOR_SLOTS, a) &&
          number(f[2], 0, static_cast<long>(WheelRole::COUNT) - 1, b) &&
          number(f[3], 0, 1, c)))
    {
        return Result::ERR;
    }

    const uint8_t slot = static_cast<uint8_t>(a - 1);
    const WheelRole role = static_cast<WheelRole>(b);
    const bool inverted = (c == 1);

    const RobotConfig &config = _robot.config();
    if (config.role[slot] == role && (((config.inverted >> slot) & 1) != 0) == inverted)
    {
        return Result::OK; // không đổi gì: khỏi apply() và khỏi dừng motor
    }

    LOG_D("MOTOR %ld role=%s inverted=%ld", a, wheelRoleToString(role), c);
    _robot.setMotor(slot, role, inverted);
    return Result::OK;
}

Hmi::Result Hmi::handleMix(char **f, uint8_t)
{
    long a = 0, b = 0, c = 0, d = 0;
    if (!(number(f[1], 1, WHEEL_COUNT, a) && number(f[2], -100, 100, b) &&
          number(f[3], -100, 100, c) && number(f[4], -100, 100, d)))
    {
        return Result::ERR;
    }

    Mix &mix = _robot.config().custom[a - 1];
    if (mix.throttle == b && mix.strafe == c && mix.rotation == d)
    {
        return Result::OK; // không đổi
    }
    mix.throttle = static_cast<int8_t>(b);
    mix.strafe = static_cast<int8_t>(c);
    mix.rotation = static_cast<int8_t>(d);
    _robot.apply();
    return Result::OK;
}

Hmi::Result Hmi::handlePwm(char **f, uint8_t)
{
    long a = 0;
    if (!number(f[1], 0, 255, a))
    {
        return Result::ERR;
    }
    _robot.config().pwmLimit = static_cast<uint8_t>(a);
    _robot.applyLimits();
    return Result::OK;
}

Hmi::Result Hmi::handleAccel(char **f, uint8_t)
{
    long a = 0;
    if (!number(f[1], 0, 255, a))
    {
        return Result::ERR;
    }
    _robot.config().accelStep = static_cast<uint8_t>(a);
    _robot.applyLimits();
    return Result::OK;
}

Hmi::Result Hmi::handleSave(char **, uint8_t)
{
    _robot.saveConfig();
    return Result::OK;
}

Hmi::Result Hmi::handleDefault(char **, uint8_t)
{
    _robot.resetConfig();
    return Result::OK;
}

Hmi::Result Hmi::handleCtrl(char **f, uint8_t)
{
    const bool manual = is(f[1], "MANUAL");
    if (!manual && !is(f[1], "REMOTE"))
    {
        return Result::ERR;
    }
    const Source source = manual ? Source::MANUAL : Source::REMOTE;
    if (source != _robot.source()) // setSource() dừng xe: bỏ qua khi không đổi mode
    {
        LOG_D("CTRL source: %s", f[1]);
        _robot.setSource(source);
    }
    return Result::OK;
}

// Stream liên tục (quiet): bị từ chối (vd đang ở REMOTE) cũng im lặng,
// để không spam "ERR,JOY" làm đầy buffer TX.
Hmi::Result Hmi::handleJoy(char **f, uint8_t)
{
    long a = 0, b = 0, c = 0;
    if (number(f[1], -100, 100, a) && number(f[2], -100, 100, b) && number(f[3], -100, 100, c))
    {
        _robot.joy(static_cast<int8_t>(a), static_cast<int8_t>(b), static_cast<int8_t>(c));
    }
    return Result::NONE;
}

Hmi::Result Hmi::handleRun(char **f, uint8_t)
{
    long a = 0, b = 0;
    return toResult(number(f[1], 1, MOTOR_SLOTS, a) && number(f[2], -255, 255, b) &&
                    _robot.runMotor(static_cast<uint8_t>(a - 1), static_cast<int16_t>(b)));
}

Hmi::Result Hmi::handleBrake(char **f, uint8_t)
{
    long a = 0;
    return toResult(number(f[1], 1, MOTOR_SLOTS, a) &&
                    _robot.brakeMotor(static_cast<uint8_t>(a - 1)));
}

Hmi::Result Hmi::handleStop(char **, uint8_t)
{
    _robot.stop();
    return Result::OK;
}

Hmi::Result Hmi::handleServo(char **f, uint8_t)
{
    long a = 0, b = 0;
    if (!(number(f[1], 0, 15, a) && number(f[2], 0, 180, b)))
    {
        return Result::ERR;
    }
    _robot.servos().write(static_cast<uint8_t>(a), static_cast<uint8_t>(b));
    return Result::OK;
}

Hmi::Result Hmi::handleIo(char **f, uint8_t n)
{
    long a = 0, b = 0;
    if (!parsePin(f[1], a))
    {
        return Result::ERR;
    }
    const uint8_t pin = static_cast<uint8_t>(a);

    if (n == 3)
    {
        if (is(f[2], "R"))
        {
            _serial.print(F("IO,"));
            _serial.print(pin);
            _serial.print(',');
            _serial.print(IoPin::read(pin) ? 1 : 0);
            endMsg(_serial);
            return Result::NONE; // trả giá trị thay cho OK
        }
        if (is(f[2], "OUT") || is(f[2], "IN") || is(f[2], "PULLUP"))
        {
            IoPin::configure(pin, is(f[2], "OUT") ? IoPin::OUT : (is(f[2], "IN") ? IoPin::IN : IoPin::IN_PULLUP));
            return Result::OK;
        }
    }
    else if (n == 4 && is(f[2], "W") && number(f[3], 0, 1, b))
    {
        IoPin::write(pin, b == 1);
        return Result::OK;
    }
    return Result::ERR;
}

Hmi::Result Hmi::handleAin(char **f, uint8_t)
{
    long a = 0;
    if (!number(f[1], 0, 15, a))
    {
        return Result::ERR;
    }
    _serial.print(F("AIN,"));
    _serial.print(static_cast<uint8_t>(a));
    _serial.print(',');
    _serial.print(IoPin::readAnalog(static_cast<uint8_t>(a)));
    endMsg(_serial);
    return Result::NONE; // trả giá trị thay cho OK
}

// =============================================================== config dump
/**
 * @brief Gửi cấu hình hiện tại (CFG, MAP, MIX) xuống màn hình.
 */
void Hmi::sendConfig()
{
    const RobotConfig &config = _robot.config();

    _serial.print(F("CFG,"));
    _serial.print(static_cast<uint8_t>(config.chassis));
    _serial.print(',');
    _serial.print(config.pwmLimit);
    _serial.print(',');
    _serial.print(config.accelStep);
    _serial.print(',');
    _serial.print(_robot.ready() ? 1 : 0);
    _serial.print(',');
    _serial.print(_robot.source() == Source::MANUAL ? F("MANUAL") : F("REMOTE"));
    endMsg(_serial);

    for (uint8_t slot = 0; slot < MOTOR_SLOTS; ++slot)
    {
        _serial.print(F("MAP,"));
        _serial.print(slot + 1);
        _serial.print(',');
        _serial.print(static_cast<uint8_t>(config.role[slot]));
        _serial.print(',');
        _serial.print((config.inverted >> slot) & 1);
        endMsg(_serial);
    }

    for (uint8_t wheel = 0; wheel < WHEEL_COUNT; ++wheel)
    {
        _serial.print(F("MIX,"));
        _serial.print(wheel + 1);
        _serial.print(',');
        _serial.print(config.custom[wheel].throttle);
        _serial.print(',');
        _serial.print(config.custom[wheel].strafe);
        _serial.print(',');
        _serial.print(config.custom[wheel].rotation);
        endMsg(_serial);
    }
}

// =============================================================== sync
/**
 * @brief Đồng bộ trang Motor_sel: checkbox đảo chiều, nhãn nút và 6 biến bánh trong program.s.
 */
void Hmi::syncScreen()
{
    const RobotConfig &cfg = _robot.config();
    long wheelMotor[6];
    computeWheelMotor(wheelMotor); // FL, FR, RL, RR, ML, MR -> slot 1..6, 0 = chưa gán
    char name[4];

    for (uint8_t s = 0; s < MOTOR_SLOTS; ++s)
    {
        // Role không thuộc chassis hiện tại coi như chưa gán
        const bool bound = cfg.role[s] != WheelRole::NONE &&
                           roleUsedByChassis(cfg.chassis, cfg.role[s]);

        sendVal(_serial, UI_PAGE, indexed(name, "c", s), (cfg.inverted >> s) & 1);                    // checkbox đảo chiều
        sendTxt(_serial, UI_PAGE, indexed(name, "b", s + 3), bound ? "BOUNDED" : "UNBOUND");          // nút nhãn
    }

    // Tên biến global trùng tên WheelRole: FRONT_LEFT, FRONT_RIGHT, ...
    for (uint8_t i = 0; i < 6; ++i)
    {
        sendGlobal(_serial, wheelRoleToString(static_cast<WheelRole>(i + 1)), wheelMotor[i]);
    }

    LOG_D("SYNC sent: FL=%ld FR=%ld RL=%ld RR=%ld ML=%ld MR=%ld",
          wheelMotor[0], wheelMotor[1], wheelMotor[2], wheelMotor[3], wheelMotor[4], wheelMotor[5]);
}

/**
 * @brief Với mỗi role (FL..MR), tìm slot đang giữ nó trong chassis hiện tại.
 * @param out out[role-1] = slot 1..6, 0 nếu chưa gán
 */
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
        // FL..MR là 1..6, trùng thứ tự out[0..5]
        const uint8_t role = static_cast<uint8_t>(cfg.role[s]);
        if (role >= 1 && role <= 6)
        {
            out[role - 1] = s + 1;
        }
    }
}

/**
 * @brief Gửi nhãn t0..tN (M0..M5 hoặc "--") của một trang chassis, đọc từ pool riêng của chassis đó.
 * @param chassis Chassis của trang đang hiển thị (có thể khác chassis đang chạy)
 */
void Hmi::syncChassisLabels(Chassis chassis)
{
    RobotConfig &cfg = _robot.config();
    // Chỉ chassis đang chạy mới có thay đổi chưa nằm trong saved[]
    if (chassis == cfg.chassis)
    {
        cfg.storeCurrent();
    }
    const ChassisMap &map = cfg.saved[static_cast<uint8_t>(chassis)];

    const uint8_t count = labelCount(chassis);
    char name[4];
    char text[3] = {'M', '\0', '\0'};

    for (uint8_t i = 0; i < count; ++i)
    {
        const uint8_t slot = slotOf(map.role, LABEL_ORDER[i]);
        if (slot == 0)
        {
            sendTxt(_serial, nullptr, indexed(name, "t", i), "--");
        }
        else
        {
            text[1] = static_cast<char>('0' + slot - 1); // M0..M5, khớp trang Motor_sel
            sendTxt(_serial, nullptr, indexed(name, "t", i), text); // nullptr = trang đang hiển thị
        }
    }

    LOG_D("labels page=%u running=%u count=%u",
          static_cast<uint8_t>(chassis), static_cast<uint8_t>(cfg.chassis), count);
}

/**
 * @brief Gửi (hoặc bỏ qua nếu không đổi) một trường của trang Monitor.
 *
 * Mỗi lần syncMonitor() phải gọi các trường theo cùng một thứ tự: vị trí gọi chính là chỉ số
 * trong _monHash. Chỉ gửi khi text hoặc màu đổi so với lần trước, nên khi xe đứng yên một lần
 * MON gần như không tốn băng thông và không chặn vòng điều khiển.
 */
void Hmi::monField(const char *comp, const char *text, int32_t color)
{
    if (_monSlot >= MON_FIELDS)
    {
        return; // quá số trường đã cấp: tăng MON_FIELDS
    }
    uint16_t h = hashText(text);
    if (color >= 0)
    {
        h = static_cast<uint16_t>(h * 31u) ^ static_cast<uint16_t>(color);
    }
    if (h == 0)
    {
        h = 1; // 0 nghĩa là "chưa gửi lần nào"
    }

    uint16_t &previous = _monHash[_monSlot++];
    if (h == previous)
    {
        return;
    }
    previous = h;

    sendTxt(_serial, MON_PAGE, comp, text);
    if (color >= 0)
    {
        sendCol(_serial, MON_PAGE, comp, static_cast<uint16_t>(color));
    }
}

/**
 * @brief Cập nhật trang Monitor (chỉ gửi các trường đã thay đổi).
 */
void Hmi::syncMonitor()
{
    const RobotConfig &cfg = _robot.config();
    const Robot::Command &cmd = _robot.command();
    char name[4], text[12];
    _monSlot = 0;

    monField("tsrc", _robot.source() == Source::MANUAL ? "MANUAL" : "REMOTE");
    const bool alive = _robot.linkAlive();
    monField("tlink", alive ? "Alive" : "Lost", alive ? COLOR_GREEN : COLOR_RED);
    monField("ttout", num(text, static_cast<long>(_robot.getTimeout())));

    monField("tthr", num(text, cmd.throttle));
    monField("tstr", num(text, cmd.strafe));
    monField("trot", num(text, cmd.rotation));

    monField("tchs", chassisToString(cfg.chassis));
    monField("tpwm", num(text, cfg.pwmLimit));
    monField("tacc", num(text, cfg.accelStep));

    for (uint8_t s = 0; s < MOTOR_SLOTS; ++s)
    {
        const bool used = cfg.role[s] != WheelRole::NONE &&
                          roleUsedByChassis(cfg.chassis, cfg.role[s]);
        const bool rev = (cfg.inverted >> s) & 1;

        monField(indexed(name, "tr", s), used ? roleShort(cfg.role[s]) : "--");
        monField(indexed(name, "tv", s), rev ? "ON" : "OFF", rev ? COLOR_GREEN : COLOR_GREY);
    }

    char bits[sizeof(DIG_PINS) + 1];
    for (uint8_t i = 0; i < sizeof(DIG_PINS); ++i)
    {
        bits[i] = IoPin::read(DIG_PINS[i]) ? '1' : '0';
    }
    bits[sizeof(DIG_PINS)] = '\0';
    monField("tdio", bits);

    char analog[24]; // 4 giá trị <= 1023 và 3 dấu cách: tối đa 19 ký tự
    uint8_t length = 0;
    for (uint8_t i = 0; i < 4; ++i)
    {
        if (i > 0)
        {
            analog[length++] = ' ';
        }
        length += strlen(num(analog + length, IoPin::readAnalog(i)));
    }
    monField("tain", analog);
}

/**
 * @brief Đẩy giá trị 6 kênh ADC lên trang Analog_test (thanh .val và chữ).
 */
void Hmi::syncADC()
{
    char name[4], text[8];
    for (uint8_t i = 0; i < sizeof(ADC_PINS) / sizeof(ADC_PINS[0]); ++i)
    {
        const uint16_t value = IoPin::readAnalog(ADC_PINS[i]); // đọc một lần dùng cho cả hai
        sendVal(_serial, "Analog_test", indexed(name, "h", i), value);   // slider/progress: .val là số
        sendTxt(_serial, "Analog_test", indexed(name, "ta", i), num(text, value));
    }
}

/**
 * @brief Đồng bộ trang Main: tên chassis và tiến độ gán motor.
 */
void Hmi::syncHome()
{
    const RobotConfig &cfg = _robot.config();

    const uint8_t need = labelCount(cfg.chassis);
    uint8_t bound = 0;
    for (uint8_t i = 0; i < need; ++i)
    {
        if (slotOf(cfg.role, LABEL_ORDER[i]) != 0)
        {
            ++bound;
        }
    }

    char text[16];
    sendTxt(_serial, "Main", "thchs", chassisToString(cfg.chassis));

    snprintf(text, sizeof(text), "%u/%u BOUND", bound, need);
    sendTxt(_serial, "Main", "thprof", text);

    sendTxt(_serial, "Main", "tsts", "OK");
}

/**
 * @brief Quét bus I2C và gửi tốc độ clock + danh sách địa chỉ lên trang I2c_test.
 */
void Hmi::syncI2C()
{
#if defined(WIRE_HAS_TIMEOUT)
    // Bus bị kẹt (SDA kéo thấp) sẽ không treo loop() nữa; chỉ cần đặt một lần
    static bool timeoutSet = false;
    if (!timeoutSet)
    {
        Wire.setWireTimeout(I2C_TIMEOUT_US, true);
        timeoutSet = true;
    }
#endif

    // Tốc độ clock thực tế, tính từ thanh ghi TWBR và prescaler
    const uint8_t prescaler = 1 << (2 * (TWSR & 0x03)); // 1, 4, 16, 64
    const uint32_t clockHz = F_CPU / (16UL + 2UL * TWBR * prescaler);

    char speed[16];
    snprintf(speed, sizeof(speed), "%lu kHz", static_cast<unsigned long>(clockHz / 1000UL));
    sendTxt(_serial, "I2c_test", "tbussp", speed);

    char list[96];
    uint8_t length = 0;
    uint8_t found = 0;
    uint8_t shown = 0;
    list[0] = '\0';

    for (uint8_t address = I2C_FIRST; address <= I2C_LAST; ++address)
    {
        Wire.beginTransmission(address);
        if (Wire.endTransmission() != 0)
        {
            continue;
        }
        ++found;
        if (length + 6 < static_cast<uint8_t>(sizeof(list))) // chừa chỗ cho "0xNN " và ký tự kết thúc
        {
            length += snprintf(list + length, sizeof(list) - length, "0x%02X ", address);
            ++shown;
        }
    }

    if (found == 0)
    {
        snprintf(list, sizeof(list), "No device");
    }
    else if (shown < found && length + 4 <= static_cast<uint8_t>(sizeof(list)))
    {
        snprintf(list + length, sizeof(list) - length, "...");
    }
    sendTxt(_serial, "I2c_test", "devlist", list);

    LOG_D("I2C scan: %u device(s), %lu Hz", found, static_cast<unsigned long>(clockHz));
}