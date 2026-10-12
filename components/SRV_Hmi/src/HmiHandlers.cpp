#include "HmiInternal.h"
#include <IoPin.h>
#include "GenericLogger.h"

ASSIGN_LOG_MACROS(HMI)

// Command handlers (one per CMD_TABLE entry, except SYS).
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

    memset(_dioHash, 0, sizeof(_dioHash));
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
        else if (is(f[2], "PULL_UP"))
        {
            config.mode = DigitalIo::PULL_UP;
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
