#include "Drive.h"
namespace
{
    // Positive rotation turns right.
    constexpr Mix LEFT = {100, 0, 100};
    constexpr Mix RIGHT = {100, 0, -100};
    constexpr Mix MEC_FL = {100, 100, 100};
    constexpr Mix MEC_FR = {100, -100, -100};
    constexpr Mix MEC_RL = {100, -100, 100};
    constexpr Mix MEC_RR = {100, 100, -100};
    constexpr Mix OFF = {0, 0, 0};

    // Every row follows the PortRole order (wheel index = role - 1): FL, FR, RL, RR, ML, MR.
    // Rows are picked with a switch on the enum, so adding or reordering a Chassis value can no
    // longer silently shift every preset by one row.
    constexpr Mix ROW_TWO_WHEEL[MAX_PORT_COUNT] = {LEFT, RIGHT, OFF, OFF, OFF, OFF};
    constexpr Mix ROW_TANK[MAX_PORT_COUNT] = {LEFT, RIGHT, LEFT, RIGHT, OFF, OFF};
    constexpr Mix ROW_MECANUM[MAX_PORT_COUNT] = {MEC_FL, MEC_FR, MEC_RL, MEC_RR, OFF, OFF};
    constexpr Mix ROW_SIX_WHEEL[MAX_PORT_COUNT] = {LEFT, RIGHT, LEFT, RIGHT, LEFT, RIGHT};
    constexpr Mix ROW_NONE[MAX_PORT_COUNT] = {OFF, OFF, OFF, OFF, OFF, OFF};

    // OMNI_3, HOLONOMIC, XDRIVE and INDEPENDENT have no preset yet: they select ROW_NONE, so
    // Drive::ready() stays false and the robot does not move until one is written.
    const Mix *presetRow(Chassis chassis)
    {
        switch (chassis)
        {
        case Chassis::TWO_WHEEL:
            return ROW_TWO_WHEEL;
        case Chassis::TANK:
            return ROW_TANK;
        case Chassis::OMNI_4: // X layout mixes exactly like mecanum
        case Chassis::MECANUM:
            return ROW_MECANUM;
        case Chassis::SIX_WHEEL:
            return ROW_SIX_WHEEL;
        default:
            return ROW_NONE;
        }
    }

    bool used(const Mix &mix)
    {
        return mix.throttle != 0 || mix.strafe != 0 || mix.rotation != 0;
    }
}

void Drive::assign(PortRole role, IMotor *motor, bool inverted)
{
    if (role == PortRole::NONE || role > PortRole::DRIVE_MR) // AUXILITARY is not a wheel: _wheels has MAX_PORT_COUNT entries
    {
        return;
    }
    Wheel &wheel = _wheels[static_cast<uint8_t>(role) - 1];
    wheel.motor = motor;
    wheel.inverted = inverted;
    wheel.last = 0;
    refreshReady();
}

void Drive::clearMotors()
{
    for (Wheel &wheel : _wheels)
    {
        wheel = Wheel();
    }
    refreshReady();
}

void Drive::setChassis(Chassis chassis, const Mix *custom)
{
    _chassis = chassis;
    const Mix *source = (chassis == Chassis::CUSTOM) ? custom : presetRow(chassis);
    if (source == nullptr) // CUSTOM without a table: no wheel is used, so ready() stays false
    {
        source = ROW_NONE;
    }
    for (uint8_t i = 0; i < MAX_PORT_COUNT; ++i)
    {
        _mix[i] = source[i];
    }
    refreshReady();
}

void Drive::refreshReady()
{
    bool any = false;
    for (uint8_t i = 0; i < MAX_PORT_COUNT; ++i)
    {
        if (used(_mix[i]))
        {
            if (_wheels[i].motor == nullptr)
            {
                _ready = false;
                return;
            }
            any = true;
        }
    }
    _ready = any;
}

bool Drive::ready() const
{
    return _ready; // cached: refreshed by assign/clearMotors/setChassis
}

void Drive::move(int8_t throttle, int8_t strafe, int8_t rotation)
{
    if (!ready())
    {
        stop();
        return;
    }

    const int32_t t = constrain(throttle, -100, 100);
    const int32_t s = constrain(strafe, -100, 100);
    const int32_t r = constrain(rotation, -100, 100);

    int16_t speed[MAX_PORT_COUNT] = {};
    int16_t peak = 0;
    for (uint8_t i = 0; i < MAX_PORT_COUNT; ++i)
    {
        const Mix &mix = _mix[i];
        if (!used(mix))
        {
            continue;
        }
        speed[i] = static_cast<int16_t>((mix.throttle * t + mix.strafe * s + mix.rotation * r) * 255L / 10000L);
        peak = max(peak, static_cast<int16_t>(abs(speed[i])));
    }

    for (uint8_t i = 0; i < MAX_PORT_COUNT; ++i)
    {
        Wheel &wheel = _wheels[i];
        if (!used(_mix[i]))
        {
            continue;
        }

        int16_t value = speed[i];
        if (peak > _pwmLimit)
        {
            value = static_cast<int16_t>(value * static_cast<int32_t>(_pwmLimit) / peak);
        }
        if (_accelStep > 0)
        {
            value = constrain(value, wheel.last - _accelStep, wheel.last + _accelStep);
        }
        wheel.last = value;
        wheel.motor->run(wheel.inverted ? -value : value);
    }
}

void Drive::stop()
{
    for (Wheel &wheel : _wheels)
    {
        if (wheel.motor != nullptr)
        {
            wheel.motor->stop();
        }
        wheel.last = 0;
    }
}
