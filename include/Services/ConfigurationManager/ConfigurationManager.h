#ifndef CONFIGURATION_MANAGER_H
#define CONFIGURATION_MANAGER_H

#include <Arduino.h>
#include "CapabilityRegistry.h"
#include "RobotConfig.h"

class ConfigurationManager
{
public:
    static constexpr uint8_t MAX_EEPROM_WRITES_PER_BOOT = 8;

    ConfigurationManager(const CapabilityRegistry &capabilities, int address);

    bool load();
    bool save();
    void resetToDefaults();

    void beginStage();
    void abortStage();
    bool updateStaged(const RobotConfig &config);
    bool validate(const RobotConfig &config) const;
    bool validateStaged() const { return validate(_staged); }
    bool apply();

    const RobotConfig &getActive() const { return _active; }
    const RobotConfig &getStaged() const { return _staged; }

private:
    const CapabilityRegistry &_capabilities;
    int _address;
    RobotConfig _active;
    RobotConfig _staged;
    RobotConfig _lastSaved;
    uint8_t _writesThisBoot;
    bool _hasSavedConfig;
};

#endif