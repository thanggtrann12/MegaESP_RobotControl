#include "Drive.h"
#include "RobotConfig.h"
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

    // Columns: DRIVE_FL, DRIVE_FR, DRIVE_ML, DRIVE_MR, DRIVE_RL, DRIVE_RR.
    constexpr Mix PRESETS[static_cast<uint8_t>(Chassis::CUSTOM)][MAX_PORT_COUNT] = {
        {LEFT, RIGHT, OFF, OFF, OFF, OFF},          // TWO_WHEEL
        {LEFT, RIGHT, OFF, OFF, LEFT, RIGHT},       // TANK
        {MEC_FL, MEC_FR, OFF, OFF, MEC_RL, MEC_RR}, // OMNI_4
        {MEC_FL, MEC_FR, OFF, OFF, MEC_RL, MEC_RR}, // MECANUM
        {LEFT, RIGHT, LEFT, RIGHT, LEFT, RIGHT},    // SIX_WHEEL
    };

    bool used(const Mix &mix)
    {
        return mix.throttle != 0 || mix.strafe != 0 || mix.rotation != 0;
    }
}

void Drive::assign(PortRole role, IMotor *motor, bool inverted)
{
    if (role == PortRole::NONE || role >= PortRole::COUNT)
    {
        return;
    }
    Wheel &wheel = _wheels[static_cast<uint8_t>(role) - 1];
    wheel.motor = motor;
    wheel.inverted = inverted;
    wheel.last = 0;
    _wheelCount += 1;
}

void Drive::clearMotors()
{
    for (Wheel &wheel : _wheels)
    {
        wheel = Wheel();
    }
    _wheelCount = 0;
}

void Drive::setChassis(Chassis chassis, const Mix *custom)
{
    _chassis = chassis;
    for (uint8_t i = 0; i < MAX_PORT_COUNT; ++i)
    {
        if (_chassis == Chassis::CUSTOM)
        {
            _mix[i] = custom[i];
        }
        else if (_chassis < Chassis::CUSTOM)
        {
            _mix[i] = PRESETS[static_cast<uint8_t>(_chassis)][i];
        }
        else
        {
            _mix[i] = OFF;
        }
    }
}

bool Drive::ready() const
{
    bool any = false;
    for (uint8_t i = 0; i < _wheelCount; ++i)
    {
        if (used(_mix[i]))
        {
            if (_wheels[i].motor == nullptr)
            {
                return false;
            }
            any = true;
        }
    }
    return any;
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

    int16_t speed[MAX_PORT_COUNT];
    int16_t peak = 0;
    for (uint8_t i = 0; i < _wheelCount; ++i)
    {
        const Mix &mix = _mix[i];
        speed[i] = static_cast<int16_t>((mix.throttle * t + mix.strafe * s + mix.rotation * r) * 255L / 10000L);
        peak = max(peak, static_cast<int16_t>(abs(speed[i])));
    }

    for (uint8_t i = 0; i < _wheelCount; ++i)
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
