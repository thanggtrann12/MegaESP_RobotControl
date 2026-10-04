#include "Drive.h"

static_assert(static_cast<uint8_t>(WheelRole::COUNT) == WHEEL_COUNT + 1, "WHEEL_COUNT must match WheelRole");

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

    // Columns: FRONT_LEFT, FRONT_RIGHT, MID_LEFT, MID_RIGHT, REAR_LEFT, REAR_RIGHT.
    constexpr Mix PRESETS[static_cast<uint8_t>(Chassis::CUSTOM)][WHEEL_COUNT] = {
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

void Drive::assign(WheelRole role, IMotor *motor, bool inverted)
{
    if (role == WheelRole::NONE || role >= WheelRole::COUNT)
    {
        return;
    }
    Wheel &wheel = _wheels[static_cast<uint8_t>(role) - 1];
    wheel.motor = motor;
    wheel.inverted = inverted;
    wheel.last = 0;
}

void Drive::clearMotors()
{
    for (Wheel &wheel : _wheels)
    {
        wheel = Wheel();
    }
}

void Drive::setChassis(Chassis chassis, const Mix *custom)
{
    for (uint8_t i = 0; i < WHEEL_COUNT; ++i)
    {
        if (chassis == Chassis::CUSTOM)
        {
            _mix[i] = custom[i];
        }
        else if (chassis < Chassis::CUSTOM)
        {
            _mix[i] = PRESETS[static_cast<uint8_t>(chassis)][i];
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
    for (uint8_t i = 0; i < WHEEL_COUNT; ++i)
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

    int16_t speed[WHEEL_COUNT];
    int16_t peak = 0;
    for (uint8_t i = 0; i < WHEEL_COUNT; ++i)
    {
        const Mix &mix = _mix[i];
        speed[i] = static_cast<int16_t>((mix.throttle * t + mix.strafe * s + mix.rotation * r) * 255L / 10000L);
        peak = max(peak, static_cast<int16_t>(abs(speed[i])));
    }

    for (uint8_t i = 0; i < WHEEL_COUNT; ++i)
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
