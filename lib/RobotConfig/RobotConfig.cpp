#include "RobotConfig.h"
#include <EEPROM.h>

namespace
{
    uint8_t crc8(const RobotConfig &config)
    {
        const uint8_t *bytes = reinterpret_cast<const uint8_t *>(&config);
        uint8_t crc = 0;
        for (size_t i = 0; i < sizeof(RobotConfig) - 1; ++i)
        {
            crc ^= bytes[i];
            for (uint8_t bit = 0; bit < 8; ++bit)
            {
                crc = (crc & 0x80) ? static_cast<uint8_t>((crc << 1) ^ 0x07) : static_cast<uint8_t>(crc << 1);
            }
        }
        return crc;
    }

    bool rolesValid(const WheelRole *role, uint8_t inverted)
    {
        if ((inverted & ~((1 << MOTOR_SLOTS) - 1)) != 0)
        {
            return false;
        }
        uint8_t seen = 0;
        for (uint8_t i = 0; i < MOTOR_SLOTS; ++i)
        {
            if (role[i] >= WheelRole::COUNT)
            {
                return false;
            }
            const uint8_t bit = 1 << static_cast<uint8_t>(role[i]);
            if (role[i] != WheelRole::NONE && (seen & bit))
            {
                return false;
            }
            seen |= bit;
        }
        return true;
    }
}

RobotConfig RobotConfig::defaults()
{
    RobotConfig config{}; // saved[] toàn NONE, inverted = 0
    config.version = ROBOT_CONFIG_VERSION;
    config.chassis = Chassis::MECANUM;
    config.pwmLimit = 255;

    // Mặc định chỉ cho MECANUM (giữ hành vi cũ), chassis khác để trống
    ChassisMap &mecanum = config.saved[static_cast<uint8_t>(Chassis::MECANUM)];
    mecanum.role[0] = WheelRole::FRONT_LEFT;
    mecanum.role[1] = WheelRole::REAR_LEFT;
    mecanum.role[2] = WheelRole::FRONT_RIGHT;
    mecanum.role[3] = WheelRole::REAR_RIGHT;

    config.loadCurrent();
    return config;
}

bool RobotConfig::isValid() const
{
    if (version != ROBOT_CONFIG_VERSION || chassis >= Chassis::COUNT || !rolesValid(role, inverted))
    {
        return false;
    }
    for (uint8_t c = 0; c < CHASSIS_COUNT; ++c)
    {
        if (!rolesValid(saved[c].role, saved[c].inverted))
        {
            return false;
        }
    }
    for (uint8_t i = 0; i < WHEEL_COUNT; ++i)
    {
        if (custom[i].throttle < -100 || custom[i].throttle > 100 ||
            custom[i].strafe < -100 || custom[i].strafe > 100 ||
            custom[i].rotation < -100 || custom[i].rotation > 100)
        {
            return false;
        }
    }
    return true;
}

bool RobotConfig::load(int address)
{
    RobotConfig stored;
    EEPROM.get(address, stored);
    if (stored.crc != crc8(stored) || !stored.isValid())
    {
        return false;
    }
    *this = stored;
    return true;
}

void RobotConfig::save(int address)
{
    crc = crc8(*this);
    EEPROM.put(address, *this); // put() only rewrites bytes that changed
}

void RobotConfig::storeCurrent()
{
    ChassisMap &slot = saved[static_cast<uint8_t>(chassis)];
    memcpy(slot.role, role, sizeof(slot.role));
    slot.inverted = inverted;
}

void RobotConfig::loadCurrent()
{
    const ChassisMap &slot = saved[static_cast<uint8_t>(chassis)];
    memcpy(role, slot.role, sizeof(role));
    inverted = slot.inverted;
}