#ifndef ROBOT_H
#define ROBOT_H

#include <Arduino.h>
#include <Drive.h>
#include <MotorBank.h>
#include <Pca9685.h>
#include <RobotConfig.h>
#include <ServoOut.h>
#include <Std_Types.h>

namespace
{
    struct Command
    {
        int8_t throttle = 0;
        int8_t strafe = 0;
        int8_t rotation = 0;
    };

    inline bool roleUsedByChassis(Chassis chassis, WheelRole role)
    {
        switch (chassis)
        {
        case Chassis::TWO_WHEEL:
        case Chassis::TANK:
            return role == WheelRole::FRONT_LEFT || role == WheelRole::FRONT_RIGHT;

        case Chassis::OMNI_3:
            return role == WheelRole::FRONT_LEFT || role == WheelRole::FRONT_RIGHT ||
                   role == WheelRole::REAR_LEFT;

        case Chassis::OMNI_4:
        case Chassis::MECANUM:
        case Chassis::HOLONOMIC:
        case Chassis::XDRIVE:
            return role == WheelRole::FRONT_LEFT || role == WheelRole::FRONT_RIGHT ||
                   role == WheelRole::REAR_LEFT || role == WheelRole::REAR_RIGHT;

        case Chassis::SIX_WHEEL:
        case Chassis::CUSTOM:
            return role != WheelRole::NONE;

        default:
            return false;
        }
    }
}
enum class Source : uint8_t
{
    REMOTE = 0, // ESP-NOW remote through UART
    MANUAL = 1  // TJC touchscreen
};

// Wires motors, drive, config and the control source together.
class Robot
{
public:
    Robot(Pca9685 &motorPca, Pca9685 &servoPca) : _motors(motorPca), _servos(servoPca) {}

    void begin();
    void update(); // call from loop()

    // Control input
    void onRemote(const ControlPacket &packet);
    void setSource(Source source);
    Source source() const { return _source; }
    bool joy(int8_t throttle, int8_t strafe, int8_t rotation);
    bool runMotor(uint8_t slot, int16_t pwm); // manual test; stops by itself without a refresh
    bool brakeMotor(uint8_t slot);
    void stop();

    // Configuration
    RobotConfig &config() { return _config; }
    bool ready() const { return _drive.ready(); }
    void setMotor(uint8_t slot, WheelRole role, bool inverted);
    void apply();       // rebuild the drive from config; motors stop meanwhile
    void applyLimits(); // pwm limit and acceleration only; no stop
    void saveConfig();
    void resetConfig();
    void selectChassis(Chassis next);
    ServoOut &servos() { return _servos; }

    const Command &command() const { return _command; }
    bool moving() const { return _moving; }
    bool linkAlive() const { return millis() - _commandTime <= LINK_TIMEOUT_MS; }
    uint16_t getTimeout() const { return LINK_TIMEOUT_MS; }

private:
    static constexpr uint8_t NO_TEST = 0xFF;
    static constexpr uint16_t TICK_MS = 20;
    static constexpr uint16_t LINK_TIMEOUT_MS = 500;
    static constexpr uint16_t TEST_TIMEOUT_MS = 500;

    MotorBank _motors;
    ServoOut _servos;
    Drive _drive;
    RobotConfig _config = RobotConfig::defaults();
    Source _source = Source::REMOTE;

    Command _command;
    uint32_t _commandTime = 0;
    uint32_t _lastTick = 0;
    bool _moving = false;

    uint16_t _lastSequence = 0;
    bool _hasSequence = false;

    uint8_t _testSlot = NO_TEST;
    uint32_t _testUntil = 0;
};

#endif
