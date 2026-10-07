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
    constexpr const char *DIO_PAGE = "Digital_test";

    constexpr uint16_t COLOR_GREEN = 2016;
    constexpr uint16_t COLOR_RED = 63488;
    constexpr uint16_t COLOR_GREY = 33808;

    constexpr const char *DIO_MODE_TXT[3] = {"INPUT", "OUTPUT", "PULLUP"};
    constexpr const char *ROLE_SHORT[] = {"--", "FL", "FR", "RL", "RR", "ML", "MR"};
    constexpr uint8_t ROLE_SHORT_COUNT = sizeof(ROLE_SHORT) / sizeof(ROLE_SHORT[0]);

    // Thứ tự nhãn t0..t5 trên các trang chassis (trang 4 bánh chỉ dùng t0..t3, six-wheel dùng đủ t0..t5)
    constexpr uint8_t LABEL_MAX = 6;
    constexpr PortRole LABEL_ORDER[LABEL_MAX] = {PortRole::DRIVE_FL, PortRole::DRIVE_FR,
                                                 PortRole::DRIVE_RL, PortRole::DRIVE_RR,
                                                 PortRole::DRIVE_ML, PortRole::DRIVE_MR};

    constexpr uint8_t MAX_FIELDS = 6;
    constexpr uint8_t TERMINATOR_BYTE = 0xFF;
    constexpr uint8_t TERMINATOR_COUNT = 3;
    constexpr uint32_t HEARTBEAT_MS = 500;
    constexpr uint8_t MAX_RX_PER_UPDATE = 64; // giới hạn byte đọc mỗi lần update(): màn hình flood không làm đói vòng điều khiển

    // Watchdog: dừng xe nếu lệnh lái dạng stream không còn đến.
    constexpr uint32_t JOY_TIMEOUT_MS = 400; // JOY phải được gửi lặp lại khi đang giữ joystick
    constexpr uint32_t RUN_TIMEOUT_MS = 0;   // 0 = tắt. Bật (vd 1000) nếu UI gửi lặp RUN khi giữ nút test motor
    constexpr long NUMBER_LIMIT = 100000L;   // mọi tham số HMI đều nhỏ hơn: chặn tràn khi parse

    // 8 chân IO đa dụng (IO_PIN_NUM_1..8) do DigitalIo quản lý: DigitalIo::pin(i) / indexOfPin(pin).
    // IoPin::readAnalog() nhận SỐ KÊNH (0..15, giống lệnh AIN). ANALOG_PIN_NUM_x là A0.. (số chân
    // 54.. trên Mega); analogRead() chấp nhận cả số kênh lẫn số chân, nên dùng thẳng được.
    constexpr uint8_t ADC_CHANNELS[6] = {ANALOG_PIN_NUM_1, ANALOG_PIN_NUM_2, ANALOG_PIN_NUM_3,
                                         ANALOG_PIN_NUM_4, ANALOG_PIN_NUM_5, ANALOG_PIN_NUM_6};

    constexpr uint8_t I2C_FIRST = 0x08;
    constexpr uint8_t I2C_LAST = 0x77;
    constexpr uint16_t I2C_TIMEOUT_US = 3000;
    constexpr uint8_t I2C_CHUNK = 8; // số địa chỉ quét mỗi lần update()

    // Chỉ số trường trên trang Monitor (khoá của _monHash). Thêm trường mới: thêm một hằng số ở đây,
    // không phụ thuộc thứ tự gọi trong syncMonitor().
    constexpr uint8_t MON_SRC = 0;
    constexpr uint8_t MON_LINK = 1;
    constexpr uint8_t MON_TOUT = 2;
    constexpr uint8_t MON_THR = 3;
    constexpr uint8_t MON_STR = 4;
    constexpr uint8_t MON_ROT = 5;
    constexpr uint8_t MON_CHS = 6;
    constexpr uint8_t MON_PWM = 7;
    constexpr uint8_t MON_ACC = 8;
    constexpr uint8_t MON_ROLE_BASE = 9;                          // + slot
    constexpr uint8_t MON_REV_BASE = MON_ROLE_BASE + MOTOR_SLOTS; // + slot
    constexpr uint8_t MON_DIO = MON_REV_BASE + MOTOR_SLOTS;
    constexpr uint8_t MON_AIN = MON_DIO + 1;
    constexpr uint8_t MON_SLOT_COUNT = MON_AIN + 1;

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

    /**
     * @brief Đổi số chân Arduino (từ lệnh IO) thành chỉ số GPIO 0..7 của DigitalIo.
     *
     * Danh sách trắng: chỉ 8 chân IO đa dụng mới điều khiển được từ màn hình. Motor chạy qua
     * PCA9685 (I2C), UART là Serial2/3, nên mọi chân khác mặc định KHÔNG đụng được.
     * @return true nếu pin là một trong 8 chân; index chỉ được ghi khi true
     */
    bool parseIoIndex(const char *text, uint8_t &index)
    {
        long pin = 0;
        if (!number(text, 0, 255, pin))
        {
            return false;
        }
        const int8_t found = DigitalIo::indexOfPin(static_cast<uint8_t>(pin));
        if (found < 0)
        {
            return false;
        }
        index = static_cast<uint8_t>(found);
        return true;
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

    const char *roleShort(PortRole role)
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
        default: // OMNI_4, MECANUM, XDRIVE, SIX_WHEEL, ...
            // Chassis có bánh giữa (ML/MR) -> 6 nhãn, ngược lại 4
            return (roleUsedByChassis(c, PortRole::DRIVE_ML) || roleUsedByChassis(c, PortRole::DRIVE_MR)) ? 6 : 4;
        }
    }

    /**
     * @brief Tìm slot đang giữ một role.
     * @return slot 1..MOTOR_SLOTS, hoặc 0 nếu chưa gán
     */
    uint8_t slotOf(const PortRole *roles, PortRole role)
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

    // Hash 32-bit FNV-1a để nhận biết trường monitor nào đã đổi (va chạm ~1/4 tỉ, thay cho 16-bit ~1/65536)
    constexpr uint32_t FNV_OFFSET = 2166136261UL;
    constexpr uint32_t FNV_PRIME = 16777619UL;

    uint32_t hashByte(uint32_t h, uint8_t b) { return (h ^ b) * FNV_PRIME; }

    uint32_t hashText(const char *s)
    {
        uint32_t h = FNV_OFFSET;
        while (*s != '\0')
        {
            h = hashByte(h, static_cast<uint8_t>(*s++));
        }
        return h;
    }

    // Trộn màu vào hash; byte 0xA5 làm dấu ngăn cách (text chỉ là ASCII nên không trùng)
    uint32_t hashColor(uint32_t h, uint16_t color)
    {
        h = hashByte(h, 0xA5);
        h = hashByte(h, static_cast<uint8_t>(color & 0xFF));
        return hashByte(h, static_cast<uint8_t>(color >> 8));
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

    // Biến int global trong program.s: "DRIVE_FL=3"
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
// clang-format off
const Hmi::CommandEntry Hmi::CMD_TABLE[] PROGMEM = {
    //  name                    handler                  minFields   maxFields      quiet
    {"JOY",                 &Hmi::handleJoy,                4,          4,          true},
    {"MON",                 &Hmi::handleMon,                1,          2,          true},
    {"DIO",                 &Hmi::handleDio,                1,          2,          true},
    {"DIOSEL",              &Hmi::handleDioSel,             2,          2,          false},
    {"DIOSET",              &Hmi::handleDioSet,             5,          5,          false},
    {"DIORST",              &Hmi::handleDioRst,             1,          1,          false},
    {"ADC",                 &Hmi::handleAdc,                1,          1,          true},
    {"RUN",                 &Hmi::handleRun,                3,          4,          false},
    {"SYNC",                &Hmi::handleSync,               1,          3,          false},
    {"GET",                 &Hmi::handleGet,                1,          1,          false},
    {"CHASSIS",             &Hmi::handleChassis,            2,          2,          false},
    {"MOTOR",               &Hmi::handleMotor,              4,          4,          false},
    {"MIX",                 &Hmi::handleMix,                5,          5,          false},
    {"PWM",                 &Hmi::handlePwm,                2,          2,          false},
    {"ACCEL",               &Hmi::handleAccel,              2,          2,          false},
    {"SAVE",                &Hmi::handleSave,               1,          1,          false},
    {"DEFAULT",             &Hmi::handleDefault,            1,          1,          false},
    {"CTRL",                &Hmi::handleCtrl,               2,          2,          false},
    {"BRAKE",               &Hmi::handleBrake,              2,          2,          false},
    {"STOP",                &Hmi::handleStop,               1,          1,          false},
    {"SERVO",               &Hmi::handleServo,              3,          3,          false},
    {"IO",                  &Hmi::handleIo,                 3,          4,          false},
    {"AIN",                 &Hmi::handleAin,                2,          2,          false},
    {"SYS",                 &Hmi::handleSys,                1,          2,          true},
};
// clang-format on
const uint8_t Hmi::CMD_TABLE_SIZE = sizeof(CMD_TABLE) / sizeof(CMD_TABLE[0]);

// =============================================================== line handling

/**
 * @brief Khởi tạo HMI, thiết lập UART và trạng thái ban đầu.
 *
 */
void Hmi::init()
{
    _serial.print("page Main");
    _serial.write(0xFF);
    _serial.write(0xFF);
    _serial.write(0xFF);
}

/**
 * @brief Đọc UART từ màn hình, gom thành dòng lệnh và gửi heartbeat định kỳ.
 */
void Hmi::update()
{
    uint8_t budget = MAX_RX_PER_UPDATE;
    while (budget > 0 && _serial.available())
    {
        --budget;
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
    checkWatchdog(now);
    stepI2CScan();

    if (now - _lastHeartbeat >= HEARTBEAT_MS)
    {
        _lastHeartbeat = now;
        _serial.print(F("SYS,OK"));
        endMsg(_serial);
    }
}

/**
 * @brief Bật (timeoutMs > 0) hoặc tắt (0) watchdog lái. Mỗi lệnh lái hợp lệ gọi lại để gia hạn.
 */
void Hmi::armDrive(uint32_t timeoutMs)
{
    if (timeoutMs == 0)
    {
        _driveArmed = false;
        return;
    }
    _driveDeadline = millis() + timeoutMs;
    _driveArmed = true;
}

/**
 * @brief Quá hạn mà không có lệnh lái mới (dây UART tuột, màn hình treo...) thì dừng xe.
 */
void Hmi::checkWatchdog(uint32_t now)
{
    if (_driveArmed && static_cast<int32_t>(now - _driveDeadline) >= 0)
    {
        _driveArmed = false;
        _robot.stop();
        LOG_D("Drive watchdog: stopped");
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
            for (uint8_t j = 0; j < n; ++j)
            {
                LOG_D("Field %u: %s", j, f[j]);
            }
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

    LOG_D("Unknown command: %s", command);
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
        startI2CScan();
        return Result::NONE;
    }
    if (n == 3 && is(f[1], "CHASSIS") &&
        number(f[2], 0, static_cast<long>(Chassis::COUNT) - 1, a))
    {
        const Chassis chassis = static_cast<Chassis>(a);
        RobotConfig &cfg = _robot.config();
        if (chassis == cfg.chassis)
        {
            cfg.storeCurrent(); // chỉ chassis đang chạy mới có thay đổi chưa nằm trong saved[]
        }
        syncChassisLabels(chassis); // chỉ đọc
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
    LOG_D("Selected chassis: %s", chassisToString(static_cast<Chassis>(a)));
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
          number(f[2], 0, static_cast<long>(PortRole::COUNT) - 1, b) &&
          number(f[3], 0, 1, c)))
    {
        return Result::ERR;
    }

    const uint8_t slot = static_cast<uint8_t>(a - 1);
    const PortRole role = static_cast<PortRole>(b);
    const bool inverted = (c == 1);

    const RobotConfig &config = _robot.config();
    if (config.role[slot] == role && (((config.inverted >> slot) & 1) != 0) == inverted)
    {
        return Result::OK; // không đổi gì: khỏi apply() và khỏi dừng motor
    }

    LOG_D("MOTOR %ld role=%s inverted=%ld", a, PortRoleToString(role), c);
    _robot.setMotor(slot, role, inverted);
    return Result::OK;
}

Hmi::Result Hmi::handleMix(char **f, uint8_t)
{
    long a = 0, b = 0, c = 0, d = 0;
    if (!(number(f[1], 1, MAX_PORT_COUNT, a) && number(f[2], -100, 100, b) &&
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
    RobotConfig &cfg = _robot.config();
    if (cfg.pwmLimit != a) // không đổi: khỏi applyLimits()
    {
        cfg.pwmLimit = static_cast<uint8_t>(a);
        _robot.applyLimits();
    }
    return Result::OK;
}

Hmi::Result Hmi::handleAccel(char **f, uint8_t)
{
    long a = 0;
    if (!number(f[1], 0, 255, a))
    {
        return Result::ERR;
    }
    RobotConfig &cfg = _robot.config();
    if (cfg.accelStep != a) // không đổi: khỏi applyLimits()
    {
        cfg.accelStep = static_cast<uint8_t>(a);
        _robot.applyLimits();
    }
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
        armDrive(0); // đổi mode: watchdog của mode cũ hết ý nghĩa
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

        // Chỉ canh khi đang lái tay và joystick lệch tâm; về 0 thì xe đã đứng, khỏi cần canh.
        if (_robot.source() == Source::MANUAL && (a != 0 || b != 0 || c != 0))
        {
            armDrive(JOY_TIMEOUT_MS);
        }
        else
        {
            armDrive(0);
        }
    }
    return Result::NONE;
}

Hmi::Result Hmi::handleRun(char **f, uint8_t argc)
{
    // Cú pháp mới: RUN, <1-6>, <0-1>, <0-255> -> Cần ít nhất 4 tham số (bao gồm cả command "RUN" ở f[0])
    long slot = 0, dir = 0, speed = 0;

    // Validate 3 tham số: Motor Slot (1-MOTOR_SLOTS), Direction (0-1), Speed (0-255)
    if (!(number(f[1], 1, MOTOR_SLOTS, slot) &&
          number(f[2], 0, 1, dir) &&
          number(f[3], 0, 255, speed)))
    {
        return Result::ERR;
    }

    // Chuyển đổi dir (0: Lùi, 1: Tiến) và speed (0-255) thành giá trị PWM có dấu (-255 đến 255)
    int16_t pwmValue = static_cast<int16_t>((dir == 1) ? speed : -speed);

    // Truyền giá trị đã tính toán vào hệ thống động cơ
    const bool ok = _robot.runMotor(static_cast<uint8_t>(slot - 1), pwmValue);

    if (ok)
    {
        // Giữ armDrive nếu động cơ đang quay (pwmValue != 0)
        armDrive(pwmValue != 0 ? RUN_TIMEOUT_MS : 0);
        LOG_D("RUN motor slot: %ld, dir: %ld, speed: %ld -> pwm: %d", slot, dir, speed, pwmValue);
    }
    else
    {
        LOG_E("Failed to run motor slot: %ld, dir: %ld, speed: %ld -> pwm: %d", slot, dir, speed, pwmValue);
    }
    return toResult(ok);
}

Hmi::Result Hmi::handleBrake(char **f, uint8_t)
{
    long a = 0;
    const bool ok = number(f[1], 1, MOTOR_SLOTS, a) &&
                    _robot.brakeMotor(static_cast<uint8_t>(a - 1));
    if (ok)
    {
        armDrive(0);
    }
    return toResult(ok);
}

Hmi::Result Hmi::handleStop(char **, uint8_t)
{
    armDrive(0);
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
    if (n == 2 && is(f[1], "ALL"))
    {
        LOG_D("DIO,ALL: resend all");
        memset(_dioHash, 0, sizeof(_dioHash));
    }
    uint8_t index = 0;
    long b = 0;
    if (!parseIoIndex(f[1], index))
    {
        return Result::ERR;
    }

    if (n == 3)
    {
        if (is(f[2], "R"))
        {
            _serial.print(F("IO,"));
            _serial.print(DigitalIo::pin(index));
            _serial.print(',');
            _serial.print(_io.read(index) ? 1 : 0); // giá trị logic, giống trang Digital_test
            endMsg(_serial);
            return Result::NONE; // trả giá trị thay cho OK
        }

        DigitalIo::Config config = _io.config(index); // giữ mức mặc định và cờ đảo
        if (is(f[2], "OUT"))
        {
            config.mode = DigitalIo::OUT;
        }
        else if (is(f[2], "IN"))
        {
            config.mode = DigitalIo::IN;
        }
        else if (is(f[2], "PULLUP"))
        {
            config.mode = DigitalIo::PULLUP;
        }
        else
        {
            return Result::ERR;
        }
        const bool ok = _io.set(index, config);
        if (ok)
        {
            syncDio();
        }
        return toResult(ok);
    }

    if (n == 4 && is(f[2], "W") && number(f[3], 0, 1, b))
    {
        // Chỉ ghi được khi chân đang là OUTPUT: ghi vào chân INPUT sẽ bật/tắt điện trở kéo lên ngoài ý muốn
        if (_io.config(index).mode != DigitalIo::OUT)
        {
            return Result::ERR;
        }
        _io.write(index, b == 1);
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

    for (uint8_t wheel = 0; wheel < MAX_PORT_COUNT; ++wheel)
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
 * @brief Đồng bộ trang Motor_sel: checkbox đảo chiều, nhãn nút (hiển thị Role) và 6 biến bánh trong program.s.
 */
void Hmi::syncScreen()
{
    const RobotConfig &cfg = _robot.config();
    long wheelMotor[6];
    computeWheelMotor(wheelMotor); // FL, FR, RL, RR, ML, MR -> slot 1..6, 0 = chưa gán
    char name[4];

    for (uint8_t s = 0; s < MOTOR_SLOTS; ++s)
    {
        // Kiểm tra xem Role của slot hiện tại có thuộc Chassis đang chạy hay không
        const bool bound = (cfg.role[s] != PortRole::NONE) &&
                           roleUsedByChassis(cfg.chassis, cfg.role[s]);

        // 1. Cập nhật checkbox đảo chiều (c0..c5)
        sendVal(_serial, UI_PAGE, indexed(name, "c", s), (cfg.inverted >> s) & 1);

        // 2. Cập nhật nhãn nút (b3..b8): Gửi tên Role (FL, FR, RL, RR...) thay vì "BOUNDED"
        const char *roleText = bound ? roleShort(cfg.role[s]) : "--";
        sendTxt(_serial, UI_PAGE, indexed(name, "b", s + 3), roleText);
    }

    // Tên biến global trùng tên PortRole: DRIVE_FL, DRIVE_FR, ...
    for (uint8_t i = 0; i < 6; ++i)
    {
        sendGlobal(_serial, PortRoleToString(static_cast<PortRole>(i + 1)), wheelMotor[i]);
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
    // Chỉ đọc: handleSync() đã gọi storeCurrent() nếu cần
    const RobotConfig &cfg = _robot.config();
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
            text[1] = static_cast<char>('0' + slot - 1);            // M0..M5, khớp trang Motor_sel
            sendTxt(_serial, nullptr, indexed(name, "t", i), text); // nullptr = trang đang hiển thị
        }
    }

    LOG_D("labels page=%s running=%s count=%u",
          chassisToString(chassis), chassisToString(cfg.chassis), count);
}

/**
 * @brief Gửi (hoặc bỏ qua nếu không đổi) một trường của trang Monitor.
 *
 * @param slot Chỉ số cố định của trường (MON_*), khoá của _monHash — không phụ thuộc thứ tự gọi.
 *
 * Chỉ gửi khi text hoặc màu đổi so với lần trước, nên khi xe đứng yên một lần
 * MON gần như không tốn băng thông và không chặn vòng điều khiển.
 */
void Hmi::monField(uint8_t slot, const char *comp, const char *text, int32_t color)
{
    if (slot >= MON_FIELDS)
    {
        return; // quá số trường đã cấp: tăng MON_FIELDS
    }
    uint32_t h = hashText(text);
    if (color >= 0)
    {
        h = hashColor(h, static_cast<uint16_t>(color));
    }
    if (h == 0)
    {
        h = 1; // 0 nghĩa là "chưa gửi lần nào"
    }

    uint32_t &previous = _monHash[slot];
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
 * @brief Gửi (hoặc bỏ qua nếu không đổi) một trường của trang Digital_test.
 *
 * @param slot Chỉ số cố định (0..DIO_FIELDS-1), khoá của _dioHash.
 */
void Hmi::dioField(uint8_t slot, const char *comp, const char *text, int32_t color)
{
    if (slot >= DIO_FIELDS)
    {
        return;
    }
    uint32_t h = hashText(text);
    if (color >= 0)
    {
        h = hashColor(h, static_cast<uint16_t>(color));
    }
    if (h == 0)
    {
        h = 1; // 0 nghĩa là "chưa gửi lần nào"
    }

    uint32_t &previous = _dioHash[slot];
    if (h == previous)
    {
        return;
    }
    previous = h;

    sendTxt(_serial, DIO_PAGE, comp, text);
    if (color >= 0)
    {
        sendCol(_serial, DIO_PAGE, comp, static_cast<uint16_t>(color));
    }
}

Hmi::Result Hmi::handleDio(char **f, uint8_t n)
{
    if (n == 2 && is(f[1], "ALL"))
    {
        memset(_dioHash, 0, sizeof(_dioHash)); // vừa vào trang: gửi lại tất cả
    }
    syncDio();
    return Result::NONE;
}

Hmi::Result Hmi::handleDioSel(char **f, uint8_t)
{
    long i = 0;
    if (!number(f[1], 0, DigitalIo::COUNT - 1, i))
    {
        return Result::ERR;
    }
    syncDioDetail(static_cast<uint8_t>(i));
    return Result::NONE;
}

Hmi::Result Hmi::handleDioSet(char **f, uint8_t)
{
    long i = 0, mode = 0, def = 0, inv = 0;
    if (!(number(f[1], 0, DigitalIo::COUNT - 1, i) && number(f[2], 0, 2, mode) &&
          number(f[3], 0, 1, def) && number(f[4], 0, 1, inv)))
    {
        LOG_D("DIOSET rejected: gpio=[%s] mode=[%s] def=[%s] inv=[%s]", f[1], f[2], f[3], f[4]);
        return Result::ERR;
    }
    const DigitalIo::Config config = {static_cast<uint8_t>(mode), static_cast<uint8_t>(def),
                                      static_cast<uint8_t>(inv)};
    LOG_D("DIO set: i=%ld mode=%ld def=%ld inv=%ld", i, mode, def, inv);
    const bool ok = _io.set(static_cast<uint8_t>(i), config);
    if (ok)
    {
        syncDio();                              // cập nhật ngay t0..t7 và t8..t15 (chỉ gửi ô đã đổi)
        syncDioDetail(static_cast<uint8_t>(i)); // khung PIN DETAILS khớp với giá trị Mega đã lưu
    }
    return toResult(ok);
}

Hmi::Result Hmi::handleDioRst(char **, uint8_t)
{
    _io.reset();
    memset(_dioHash, 0, sizeof(_dioHash)); // gửi lại toàn bộ
    syncDio();
    return Result::OK;
}

// 8 hàng: chế độ (t0..t7) và trạng thái (t8..t15); chỉ gửi trường đã đổi
void Hmi::syncDio()
{
    char name[4] = {'t', '\0', '\0', '\0'};
    for (uint8_t i = 0; i < DigitalIo::COUNT; ++i)
    {
        const bool on = _io.read(i);

        num(name + 1, i); // ghi chữ số ngay sau 't' -> "t0".."t7" (phải truyền `name`, không phải giá trị trả về)
        dioField(i, name, DIO_MODE_TXT[_io.config(i).mode]);

        num(name + 1, DigitalIo::COUNT + i); // "t8".."t15"
        dioField(DigitalIo::COUNT + i, name, on ? "ON" : "OFF", on ? COLOR_GREEN : COLOR_GREY);
    }
}

// Khung PIN DETAILS và các biến sửa (ed_mode, ed_def) trong program.s
void Hmi::syncDioDetail(uint8_t i)
{
    const DigitalIo::Config &d = _io.config(i);
    char text[8] = {'G', 'P', 'I', 'O', static_cast<char>('0' + i), '\0'};

    sendTxt(_serial, DIO_PAGE, "tpin", text);
    sendTxt(_serial, DIO_PAGE, "tmode", DIO_MODE_TXT[d.mode]);
    sendTxt(_serial, DIO_PAGE, "tdefv", d.defaultHigh ? "HIGH" : "LOW");
    sendVal(_serial, DIO_PAGE, "cinv", d.invert);
    sendGlobal(_serial, "ed_mode", d.mode);
    sendGlobal(_serial, "ed_def", d.defaultHigh);
}

/**
 * @brief Cập nhật trang Monitor (chỉ gửi các trường đã thay đổi).
 */
void Hmi::syncMonitor()
{
    static_assert(MON_SLOT_COUNT <= MON_FIELDS, "MON_FIELDS nhỏ hơn số trường Monitor: tăng MON_FIELDS");

    const RobotConfig &cfg = _robot.config();
    const Robot::Command &cmd = _robot.command();
    char name[4], text[12];

    monField(MON_SRC, "tsrc", _robot.source() == Source::MANUAL ? "MANUAL" : "REMOTE");
    const bool alive = _robot.linkAlive();
    monField(MON_LINK, "tlink", alive ? "Alive" : "Lost", alive ? COLOR_GREEN : COLOR_RED);
    monField(MON_TOUT, "ttout", num(text, static_cast<long>(_robot.getTimeout())));

    monField(MON_THR, "tthr", num(text, cmd.throttle));
    monField(MON_STR, "tstr", num(text, cmd.strafe));
    monField(MON_ROT, "trot", num(text, cmd.rotation));

    monField(MON_CHS, "tchs", chassisToString(cfg.chassis));
    monField(MON_PWM, "tpwm", num(text, cfg.pwmLimit));
    monField(MON_ACC, "tacc", num(text, cfg.accelStep));

    for (uint8_t s = 0; s < MOTOR_SLOTS; ++s)
    {
        const bool used = cfg.role[s] != PortRole::NONE &&
                          roleUsedByChassis(cfg.chassis, cfg.role[s]);
        const bool rev = (cfg.inverted >> s) & 1;

        monField(MON_ROLE_BASE + s, indexed(name, "tr", s), used ? roleShort(cfg.role[s]) : "--");
        monField(MON_REV_BASE + s, indexed(name, "tv", s), rev ? "ON" : "OFF", rev ? COLOR_GREEN : COLOR_GREY);
    }

    char bits[DigitalIo::COUNT + 1];
    for (uint8_t i = 0; i < DigitalIo::COUNT; ++i)
    {
        bits[i] = _io.read(i) ? '1' : '0'; // giá trị logic (đã tính cờ đảo)
    }
    bits[DigitalIo::COUNT] = '\0';
    monField(MON_DIO, "tdio", bits);

    // Dùng cùng bảng ADC_CHANNELS với trang Analog_test để hai trang luôn hiển thị cùng một kênh
    char analog[36]; // 6 giá trị <= 1023 (tối đa 4 ký tự/số) + 5 dấu cách + '\0' = tối đa 30 ký tự
    uint8_t length = 0;
    for (uint8_t i = 0; i < 6; ++i)
    {
        if (i > 0)
        {
            analog[length++] = ' ';
        }
        length += strlen(num(analog + length, IoPin::readAnalog(ADC_CHANNELS[i])));
    }
    monField(MON_AIN, "tain", analog);
}

/**
 * @brief Đẩy giá trị 6 kênh ADC lên trang Analog_test (thanh .val và chữ).
 */
void Hmi::syncADC()
{
    char name[4], text[8];
    for (uint8_t i = 0; i < sizeof(ADC_CHANNELS) / sizeof(ADC_CHANNELS[0]); ++i)
    {
        const uint16_t value = IoPin::readAnalog(ADC_CHANNELS[i]);     // đọc một lần dùng cho cả hai
        sendVal(_serial, "Analog_test", indexed(name, "h", i), value); // slider/progress: .val là số
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

    // "<bound>/<need> BOUND" ghép tay, không dùng snprintf (tiết kiệm flash)
    char *p = text;
    p += strlen(num(p, bound));
    *p++ = '/';
    p += strlen(num(p, need));
    strcpy(p, " BOUND");
    sendTxt(_serial, "Main", "thprof", text);

    // Kiểm tra trạng thái ready
    const bool isReady = _robot.ready();

    // 1. Cập nhật nội dung văn bản ("OK" hoặc "NOT READY")
    sendTxt(_serial, "Main", "tsts", isReady ? "OK" : "NOT READY");

    // 2. Cập nhật màu chữ (.pco): Xanh lá (COLOR_GREEN) nếu OK, Đỏ (COLOR_RED) nếu NOT READY
    sendCol(_serial, "Main", "tsts", isReady ? COLOR_GREEN : COLOR_RED);
}

/**
 * @brief Bắt đầu quét I2C: gửi tốc độ clock rồi để stepI2CScan() quét dần mỗi lần update().
 *
 * Quét theo từng đợt I2C_CHUNK địa chỉ để vòng điều khiển không bị chặn cả trăm ms
 * (đặc biệt khi bus kẹt và mỗi địa chỉ chờ hết timeout).
 */
void Hmi::startI2CScan()
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

    char speed[16];
#if defined(TWBR) && defined(TWSR) // thanh ghi TWI chỉ có trên AVR
    // Tốc độ clock thực tế, tính từ thanh ghi TWBR và prescaler
    const uint8_t prescaler = 1 << (2 * (TWSR & 0x03)); // 1, 4, 16, 64
    const uint32_t clockHz = F_CPU / (16UL + 2UL * TWBR * prescaler);
    ultoa(clockHz / 1000UL, speed, 10);
    strcat(speed, " kHz");
#else
    strcpy(speed, "n/a");
#endif
    sendTxt(_serial, "I2c_test", "tbussp", speed);

    _i2cNext = I2C_FIRST;
    _i2cFound = 0;
    _i2cShown = 0;
    _i2cLength = 0;
    _i2cList[0] = '\0';
    _i2cScanning = true; // bắt đầu quét lại từ đầu nếu đang quét dở
}

/**
 * @brief Quét tiếp tối đa I2C_CHUNK địa chỉ; quét xong thì gửi danh sách lên trang I2c_test.
 */
void Hmi::stepI2CScan()
{
    if (!_i2cScanning)
    {
        return;
    }

    static const char HEX_DIGITS[] = "0123456789ABCDEF";

    for (uint8_t n = 0; n < I2C_CHUNK && _i2cNext <= I2C_LAST; ++n, ++_i2cNext)
    {
        Wire.beginTransmission(_i2cNext);
        if (Wire.endTransmission() != 0)
        {
            continue;
        }
        ++_i2cFound;
        if (_i2cLength + 5 < I2C_LIST_MAX) // "0xNN " + ký tự kết thúc
        {
            _i2cList[_i2cLength++] = '0';
            _i2cList[_i2cLength++] = 'x';
            _i2cList[_i2cLength++] = HEX_DIGITS[_i2cNext >> 4];
            _i2cList[_i2cLength++] = HEX_DIGITS[_i2cNext & 0x0F];
            _i2cList[_i2cLength++] = ' ';
            _i2cList[_i2cLength] = '\0';
            ++_i2cShown;
        }
    }

    if (_i2cNext <= I2C_LAST)
    {
        return; // còn địa chỉ chưa quét: để lần update() sau
    }

    _i2cScanning = false;
    if (_i2cFound == 0)
    {
        strcpy(_i2cList, "No device");
    }
    else if (_i2cShown < _i2cFound && _i2cLength + 4 <= I2C_LIST_MAX)
    {
        strcpy(_i2cList + _i2cLength, "...");
    }
    sendTxt(_serial, "I2c_test", "devlist", _i2cList);

    LOG_D("I2C scan: %u device(s)", _i2cFound);
}

Hmi::Result Hmi::handleSys(char **, uint8_t)
{
    return Result::NONE; // bỏ qua: heartbeat "SYS,OK" dội ngược về
}