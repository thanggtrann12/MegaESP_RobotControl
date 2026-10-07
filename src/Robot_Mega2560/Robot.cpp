#include "Robot.h"
#include "RobotPins.h"
#include "GenericLogger.h"
ASSIGN_LOG_MACROS(Robot, Serial);

void Robot::begin()
{
    _motors.begin(MOTOR_PWM_HZ);
    _servos.begin();
    _config.load(CONFIG_EEPROM_ADDRESS); // keeps defaults when EEPROM is empty or corrupt
    apply();
}

void Robot::update()
{
    const uint32_t now = millis();

    if (_testSlot != NO_TEST)
    {
        if (static_cast<int32_t>(now - _testUntil) >= 0)
        {
            _motors.stopAll();
            _testSlot = NO_TEST;
        }
        return;
    }

    if (now - _lastTick < TICK_MS)
    {
        return;
    }
    _lastTick = now;

    if (now - _commandTime > LINK_TIMEOUT_MS)
    {
        _command = Command();
        _hasSequence = false; // a restarted remote begins again at sequence 0
        if (_moving)
        {
            _drive.stop();
            _moving = false;
        }
        return;
    }

    _drive.move(_command.throttle, _command.strafe, _command.rotation);
    _moving = true;
}

void Robot::onRemote(const ControlPacket &packet)
{
    if (_source != Source::REMOTE ||
        (packet.msgType != CONTROL_MESSAGE && packet.msgType != HEARTBEAT_MESSAGE))
    {
        return;
    }
    if (_hasSequence && static_cast<int16_t>(packet.sequenceNum - _lastSequence) <= 0)
    {
        return;
    }
    _lastSequence = packet.sequenceNum;
    _hasSequence = true;
    _commandTime = millis(); // a heartbeat keeps the link alive and the last command running
    if (packet.msgType == CONTROL_MESSAGE)
    {
        _command.throttle = packet.throttle;
        _command.strafe = packet.strafe;
        _command.rotation = packet.rotation;

        // LOGI("Received control command: throttle=%d, strafe=%d, rotation=%d", _command.throttle, _command.strafe, _command.rotation);
    }
}

void Robot::setSource(Source source)
{
    stop();
    _source = source;
    _hasSequence = false;
}

bool Robot::joy(int8_t throttle, int8_t strafe, int8_t rotation)
{
    if (_source != Source::MANUAL)
    {
        return false;
    }
    _command.throttle = throttle;
    _command.strafe = strafe;
    _command.rotation = rotation;
    _commandTime = millis();
    return true;
}

bool Robot::runMotor(uint8_t slot, int16_t pwm)
{
    if (_source != Source::MANUAL || slot >= MOTOR_SLOTS)
    {
        LOG_E("Failed to run motor: slot=%d, pwm=%d", slot, pwm);
        return false;
    }
    if (_testSlot != slot)
    {
        stop();
    }

    pwm = constrain(pwm, -_config.pwmLimit, _config.pwmLimit);
    const bool inverted = (_config.inverted >> slot) & 1;
    _testSlot = slot;
    _testUntil = millis() + TEST_TIMEOUT_MS;
    _motors.motor(slot)->run(inverted ? -pwm : pwm);
    return true;
}

bool Robot::brakeMotor(uint8_t slot)
{
    if (_source != Source::MANUAL || slot >= MOTOR_SLOTS)
    {
        return false;
    }
    stop();
    _motors.motor(slot)->brake();
    return true;
}

void Robot::stop()
{
    _drive.stop();
    _motors.stopAll();
    _command = Command();
    _moving = false;
    _testSlot = NO_TEST;
}

void Robot::setMotor(uint8_t slot, PortRole role, bool inverted)
{
    if (slot >= MOTOR_SLOTS)
    {
        return;
    }

    if (role != PortRole::NONE)
    {
        for (uint8_t i = 0; i < MOTOR_SLOTS; ++i)
        {
            if (i != slot && _config.role[i] == role)
            {
                _config.role[i] = PortRole::NONE; // one motor per wheel
            }
        }
    }
    _config.role[slot] = role;
    if (inverted)
    {
        _config.inverted |= 1 << slot;
    }
    else
    {
        _config.inverted &= ~(1 << slot);
    }
    apply();
}

void Robot::apply()
{
    stop();
    for (uint8_t s = 0; s < MOTOR_SLOTS; ++s)
    {
        if (!roleUsedByChassis(_config.chassis, _config.role[s]))
        {
            _config.role[s] = PortRole::NONE;
        }
    }
    _drive.clearMotors();
    for (uint8_t slot = 0; slot < MOTOR_SLOTS; ++slot)
    {
        _drive.assign(_config.role[slot], _motors.motor(slot), (_config.inverted >> slot) & 1);
    }
    _drive.setChassis(_config.chassis, _config.custom);
    applyLimits();
}

void Robot::applyLimits()
{
    _drive.setPwmLimit(_config.pwmLimit);
    _drive.setAccelStep(_config.accelStep);
}

void Robot::saveConfig()
{
    _config.storeCurrent();
    _config.save(CONFIG_EEPROM_ADDRESS);
}

void Robot::resetConfig()
{
    _config = RobotConfig::defaults();
    apply();
}

void Robot::selectChassis(Chassis next)
{
    _config.storeCurrent(); // cất pool chassis cũ
    _config.chassis = next;
    _config.loadCurrent(); // nạp pool chassis mới
    apply();
}