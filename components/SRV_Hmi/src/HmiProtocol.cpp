#include "HmiInternal.h"
#include "GenericLogger.h"

ASSIGN_LOG_MACROS(HMI)

// Line protocol: command table, framing, dispatch, drive watchdog.

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
    {"RUN",                 &Hmi::handleRun,                4,          4,          false},
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
    // The TJC screen boots slower than the MCU: wait, flush its parser with a bare terminator,
    // then send the command twice so at least one copy lands after the screen is ready.
    // Non-blocking: update() performs the two attempts (500 ms, then 300 ms later).
    _initAttempts = 2;
    _initDeadline = millis() + 500;
}

void Hmi::stepInit(uint32_t now)
{
    if (_initAttempts == 0 || static_cast<int32_t>(now - _initDeadline) < 0)
    {
        return;
    }
    --_initAttempts;
    endMsg(_serial);
    Frame frame;
    frame.add("page Main").send(_serial);
    _initDeadline = now + 300;
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
            if (_length < HMI_LINE_MAX - 1)
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
    stepInit(now);
    stepI2CScan();

    if (now - _lastHeartbeat >= HEARTBEAT_MS)
    {
        _lastHeartbeat = now;
        Frame frame;
        frame.add("SYS,OK").send(_serial);
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
    Frame frame;
    frame.add(ok ? "OK," : "ERR,").add(command).send(_serial);
}

Hmi::Result Hmi::handleSys(char **, uint8_t)
{
    return Result::NONE; // bỏ qua: heartbeat "SYS,OK" dội ngược về
}
