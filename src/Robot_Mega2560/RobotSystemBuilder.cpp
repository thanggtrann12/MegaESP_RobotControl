/**
 * @file RobotSystemBuilder.cpp
 * @brief Robot system builder implementation.
 */

#include "RobotSystemBuilder.h"
#include <EEPROM.h>

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
}

RobotSystemBuilder::RobotSystemBuilder()
    : _robotCom(ROBOT_UART_ESP8266),
      _driveMotors{nullptr, nullptr, nullptr, nullptr},
      _motorOutput(_driveMotors, 4),
            _differentialKinematics(),
      _robot(_motorOutput, _mecanumKinematics),
      _hmiService(ROBOT_UART_HMI, _robot, _ioPins, _kinematicsRegistry),
      _isBuilt(false),
      _kinematicsRegistered(false),
      _activeMode(KinematicsMode::MODE_4WD_MECANUM)
{
        _hmiService.setMotorBindHandler(OnMotorBindCommand, this);
}

void RobotSystemBuilder::build(KinematicsMode mode)
{
    _motorManager.initTA6586Array(Wire, ROBOT_PCA9685_ADDRESS, ROBOT_PCA9685_FREQUENCY);

    if (!loadMotorConfigFromEEPROM())
    {
        _motorManager.setDefaultDriveBinding();
        saveMotorConfigToEEPROM();
    }

    if (!_motorManager.buildDriveArray(_driveMotors, 4))
    {
        _motorManager.setDefaultDriveBinding();
        _motorManager.buildDriveArray(_driveMotors, 4);
    }
    _motorOutput.setMotors(_driveMotors, 4);

    if (!_kinematicsRegistered)
    {
        _kinematicsRegistry.add(_mecanumKinematics);
        _kinematicsRegistry.add(_differentialKinematics);
        _kinematicsRegistered = true;
    }

    _activeMode = mode;
    IKinematics *targetKinematics = _kinematicsRegistry.find(_activeMode);
    if (targetKinematics != nullptr)
    {
        _robot.setKinematics(*targetKinematics);
    }

    _isBuilt = true;
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

bool RobotSystemBuilder::loadMotorConfigFromEEPROM()
{
    const uint8_t motorCount = _motorManager.getMotorCount();
    if (motorCount == 0)
    {
        return false;
    }

    MotorBindingConfig config;
    EEPROM.get(EEPROM_MOTOR_CFG_ADDR, config);

    uint8_t calculatedCrc = 0;
    for (uint8_t index = 0; index < motorCount; ++index)
    {
        calculatedCrc ^= static_cast<uint8_t>(config.motor[index].role);
    }

    if (config.crc != calculatedCrc || static_cast<uint8_t>(config.motor[0].role) == 0xFF)
    {
        return false;
    }

    for (uint8_t index = 0; index < motorCount; ++index)
    {
        _motorManager.bindMotorRole(index, config.motor[index].role);
    }

    return _motorManager.isFullyBound();
}

bool RobotSystemBuilder::saveMotorConfigToEEPROM()
{
    const uint8_t motorCount = _motorManager.getMotorCount();
    if (motorCount == 0)
    {
        return false;
    }

    MotorBindingConfig config;
    for (uint8_t index = 0; index < 6; ++index)
    {
        config.motor[index].role = MotorRole::UNBOUND;
    }

    uint8_t calculatedCrc = 0;
    for (uint8_t index = 0; index < motorCount; ++index)
    {
        config.motor[index].role = _motorManager.getMotorRole(index);
        calculatedCrc ^= static_cast<uint8_t>(config.motor[index].role);
    }
    config.crc = calculatedCrc;

    EEPROM.put(EEPROM_MOTOR_CFG_ADDR, config);
    return true;
}

void RobotSystemBuilder::handleTJCMotorBindCommand(uint8_t slotIndex, MotorRole roleId)
{
    if (!_motorManager.bindMotorRole(slotIndex, roleId))
    {
        return;
    }

    if (!_motorManager.isFullyBound())
    {
        return;
    }

    if (_motorManager.buildDriveArray(_driveMotors, 4))
    {
        _motorOutput.setMotors(_driveMotors, 4);
    }

    saveMotorConfigToEEPROM();
}
