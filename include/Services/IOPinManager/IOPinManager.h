#ifndef IO_PIN_MANAGER_H
#define IO_PIN_MANAGER_H

#include <Arduino.h>
#include "Robot_Pin_Cfg.h"

class IOPinManager
{
public:
    IOPinManager();
    void begin();
    bool toggleSimulation(uint8_t pin);
    bool resetSimulation(uint8_t pin);
    bool readDigital(uint8_t pin, uint8_t &value) const;
    bool readAnalog(uint8_t pin, uint16_t &value) const;
    bool getStatus(uint8_t pin, bool &isAnalog, int16_t &value, bool &isSimulated) const;

private:
    static constexpr uint8_t PIN_STORAGE_SIZE = 70;
    bool isDigitalPin(uint8_t pin) const;
    bool isAnalogPin(uint8_t pin) const;

    bool _isSimulated[PIN_STORAGE_SIZE];
    int16_t _simulatedValue[PIN_STORAGE_SIZE];
};

#endif