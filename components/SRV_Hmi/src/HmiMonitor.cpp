#include "HmiInternal.h"
#include <IoPin.h>
#include <Wire.h>
#include "GenericLogger.h"

ASSIGN_LOG_MACROS(HMI)

// Screen sync, monitor/DIO pages, I2C scan.
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
    sendIfChanged(_serial, _monHash[slot], MON_PAGE, comp, text, color);
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
    sendIfChanged(_serial, _dioHash[slot], DIO_PAGE, comp, text, color);
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
