#ifndef ROBOT_SYSTEM_BUILDER_H
#define ROBOT_SYSTEM_BUILDER_H

#include <Arduino.h>

#include "ComManager.h"
#include "ConfigurationManager.h"
#include "CapabilityRegistry.h"
#include "DifferentialKinematics.h"
#include "GenericRobotController.h"
#include "HMIService.h"
#include "IOPinManager.h"
#include "KinematicsRegistry.h"
#include "MecanumKinematics.h"
#include "MotorArrayOutput.h"
#include "MotorManager.h"
#include "Robot_Pin_Cfg.h"
#include "Std_Types.h"

/**
 * @file RobotSystemBuilder.h
 * @brief Robot system composition layer with EEPROM motor binding support.
 */

#ifndef EEPROM_ROBOT_CONFIG_ADDR
#define EEPROM_ROBOT_CONFIG_ADDR 0x10
#endif

class RobotSystemBuilder
{
public:
    RobotSystemBuilder();

    /**
     * @brief Builds motor binding and selects the active kinematics mode.
     * @param mode Initial kinematics mode.
     */
    void build(KinematicsMode mode = KinematicsMode::MODE_4WD_MECANUM);

    /** @brief Initializes serial links, services, and controller runtime. */
    void begin();

    /** @brief Runs one system update cycle. */
    void update();

    bool handleHMIConfigCommand(HmiConfigCommand command, uint8_t valueA, uint8_t valueB);

    /**
     * @brief Handles one HMI binding command and persists when complete.
     * @param slotIndex Physical motor slot index.
     * @param roleId Logical role to bind.
     */
    void handleTJCMotorBindCommand(uint8_t slotIndex, MotorRole roleId);

    MotorManager &getMotorManager() { return _motorManager; }
    GenericRobotController &getRobotController() { return _robot; }
    HMIService &getHMIService() { return _hmiService; }

private:
    bool applyConfiguration(const RobotConfig &config, bool persist);
    bool bindMotorConfiguration(const RobotConfig &config);

    CapabilityRegistry _capabilities;
    ConfigurationManager _configurationManager;
    ComManager _robotCom;
    MotorManager _motorManager;
    IMotor *_driveMotors[4];
    MotorArrayOutput _motorOutput;

    MecanumKinematics _mecanumKinematics;
    DifferentialKinematics _differentialKinematics;
    KinematicsRegistry _kinematicsRegistry;
    GenericRobotController _robot;
    IOPinManager _ioPins;
    HMIService _hmiService;

    bool _isBuilt;
    bool _kinematicsRegistered;
};

#endif
