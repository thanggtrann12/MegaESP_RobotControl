/**
 * @file RobotSystemBuilder.cpp
 * @brief Robot system builder implementation.
 */

#include "RobotSystemBuilder.h"

namespace
{
    void OnMotorBindCommand(void *context, uint8_t slotIndex, MotorRole roleId)
    {
        if (context == nullptr)
        {
            return;
        }

        static_cast<RobotSystemBuilder *>(context)->handleTJCMotorBindCommand(slotIndex, roleId);
    }

    bool OnConfigCommand(void *context, HmiConfigCommand command, uint8_t valueA, uint8_t valueB)
    {
        if (context == nullptr)
        {
            return false;
        }

        return static_cast<RobotSystemBuilder *>(context)->handleHMIConfigCommand(command, valueA, valueB);
    }
}

RobotSystemBuilder::RobotSystemBuilder()
    : _capabilities(),
      _configurationManager(_capabilities, EEPROM_ROBOT_CONFIG_ADDR),
      _robotCom(ROBOT_UART_ESP8266),
      _driveMotors{nullptr, nullptr, nullptr, nullptr},
      _motorOutput(_driveMotors, 4),
      _differentialKinematics(),
      _robot(_motorOutput, _mecanumKinematics),
      _hmiService(ROBOT_UART_HMI, _robot, _ioPins, _kinematicsRegistry, _capabilities),
      _isBuilt(false),
      _kinematicsRegistered(false)
{
    _hmiService.setMotorBindHandler(OnMotorBindCommand, this);
    _hmiService.setConfigHandler(OnConfigCommand, this);
}

void RobotSystemBuilder::build(KinematicsMode mode)
{
    _motorManager.initTA6586Array(Wire, ROBOT_PCA9685_ADDRESS, ROBOT_PCA9685_FREQUENCY);

    const bool loaded = _configurationManager.load();
    if (!loaded)
    {
        RobotConfig defaults = _configurationManager.getActive();
        defaults.kinematicsMode = mode;
        UpdateRobotConfigCrc(defaults);
        _configurationManager.updateStaged(defaults);
        _configurationManager.apply();
        _configurationManager.save();
    }

    if (!_kinematicsRegistered)
    {
        _kinematicsRegistry.add(_mecanumKinematics);
        _kinematicsRegistry.add(_differentialKinematics);
        _kinematicsRegistered = true;
    }

    _isBuilt = applyConfiguration(_configurationManager.getActive(), false);
}

void RobotSystemBuilder::begin()
{
    if (!_isBuilt)
    {
        build();
    }

    _robotCom.Init(ROBOT_UART_ESP8266_BAUD);
    ROBOT_UART_HMI.begin(ROBOT_UART_HMI_BAUD);
    _ioPins.begin();
    _robot.begin();
    _robotCom.SendCommand("ESP_RESET");
}

void RobotSystemBuilder::update()
{
    if (!_isBuilt)
    {
        return;
    }

    _hmiService.update();

    ControlPacket packet;
    if (_robotCom.ReadPacket(packet))
    {
        _robot.handlePacket(packet);
    }

    _robot.update();
}

void RobotSystemBuilder::handleTJCMotorBindCommand(uint8_t slotIndex, MotorRole roleId)
{
    if (slotIndex >= MAX_MOTOR_PORT)
    {
        return;
    }

    RobotConfig candidate = _configurationManager.getActive();
    candidate.motorRoles[slotIndex] = roleId;
    UpdateRobotConfigCrc(candidate);
    if (!applyConfiguration(candidate, true))
    {
        return;
    }
}

bool RobotSystemBuilder::applyConfiguration(const RobotConfig &config, bool persist)
{
    if (!_configurationManager.validate(config))
    {
        return false;
    }

    const RobotConfig previous = _configurationManager.getActive();
    const uint8_t outputCount = config.kinematicsMode == KinematicsMode::MODE_2WD_DIFF ? 2 : 4;
    if (!bindMotorConfiguration(config))
    {
        bindMotorConfiguration(previous);
        return false;
    }

    if (!_motorManager.buildDriveArray(_driveMotors, outputCount, config.kinematicsMode))
    {
        bindMotorConfiguration(previous);
        _motorManager.buildDriveArray(_driveMotors, 4, previous.kinematicsMode);
        return false;
    }

    _robot.setControlSource(_robot.getControlSource());
    _motorOutput.setMotors(_driveMotors, outputCount);
    _robot.setPwmLimit(config.pwmLimit);
    _robot.setMotionProfile(config.motionProfile);

    IKinematics *targetKinematics = _kinematicsRegistry.find(config.kinematicsMode);
    if (targetKinematics == nullptr || !_robot.setKinematics(*targetKinematics))
    {
        bindMotorConfiguration(previous);
        _motorOutput.setMotors(_driveMotors, previous.kinematicsMode == KinematicsMode::MODE_2WD_DIFF ? 2 : 4);
        _robot.setPwmLimit(previous.pwmLimit);
        _robot.setMotionProfile(previous.motionProfile);
        IKinematics *previousKinematics = _kinematicsRegistry.find(previous.kinematicsMode);
        if (previousKinematics != nullptr)
        {
            _robot.setKinematics(*previousKinematics);
        }
        return false;
    }

    _configurationManager.updateStaged(config);
    if (!_configurationManager.apply())
    {
        return false;
    }

    return !persist || _configurationManager.save();
}

bool RobotSystemBuilder::bindMotorConfiguration(const RobotConfig &config)
{
    if (config.motorCount > _motorManager.getMotorCount())
    {
        return false;
    }

    for (uint8_t index = 0; index < _motorManager.getMotorCount(); ++index)
    {
        const bool configured = index < config.motorCount;
        const MotorRole role = configured ? config.motorRoles[index] : MotorRole::UNBOUND;
        const bool inverted = configured && config.motorInverted[index] != 0;
        if (!_motorManager.bindMotorRole(index, role, inverted))
        {
            return false;
        }
    }

    return true;
}

bool RobotSystemBuilder::handleHMIConfigCommand(HmiConfigCommand command, uint8_t valueA, uint8_t valueB)
{
    RobotConfig candidate;
    switch (command)
    {
    case HmiConfigCommand::BEGIN:
        _configurationManager.beginStage();
        return true;
    case HmiConfigCommand::ABORT:
        _configurationManager.abortStage();
        return true;
    case HmiConfigCommand::VALIDATE:
        return _configurationManager.validateStaged();
    case HmiConfigCommand::APPLY:
        return applyConfiguration(_configurationManager.getStaged(), false);
    case HmiConfigCommand::SAVE:
        return _configurationManager.save();
    case HmiConfigCommand::SET_MODE:
        candidate = _configurationManager.getStaged();
        candidate.kinematicsMode = static_cast<KinematicsMode>(valueA);
        UpdateRobotConfigCrc(candidate);
        return _configurationManager.updateStaged(candidate);
    case HmiConfigCommand::SET_PWM:
        candidate = _configurationManager.getStaged();
        candidate.pwmLimit = valueA;
        UpdateRobotConfigCrc(candidate);
        return _configurationManager.updateStaged(candidate);
    case HmiConfigCommand::SET_PROFILE:
        if (valueA > static_cast<uint8_t>(MotionProfile::LIMITED_ACCELERATION))
        {
            return false;
        }
        candidate = _configurationManager.getStaged();
        candidate.motionProfile = static_cast<MotionProfile>(valueA);
        UpdateRobotConfigCrc(candidate);
        return _configurationManager.updateStaged(candidate);
    case HmiConfigCommand::SET_INVERT:
        if (valueA >= MAX_MOTOR_PORT || valueB > 1)
        {
            return false;
        }
        candidate = _configurationManager.getStaged();
        candidate.motorInverted[valueA] = valueB;
        UpdateRobotConfigCrc(candidate);
        return _configurationManager.updateStaged(candidate);
    case HmiConfigCommand::SET_MOTOR:
        if (valueA >= MAX_MOTOR_PORT)
        {
            return false;
        }
        candidate = _configurationManager.getStaged();
        candidate.motorRoles[valueA] = static_cast<MotorRole>(valueB);
        UpdateRobotConfigCrc(candidate);
        return _configurationManager.updateStaged(candidate);
    }
    return false;
}
