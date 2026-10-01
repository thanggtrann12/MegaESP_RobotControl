/**
 * @file IOPinManager.cpp
 * @brief Pin monitoring and simulation logic implementation.
 */

#include "IOPinManager.h"

IOPinManager::IOPinManager()
{
    for (uint8_t pin = 0; pin < PIN_STORAGE_SIZE; ++pin)
    {
        _isSimulated[pin] = false;
        _simulatedValue[pin] = 0;
    }
}

void IOPinManager::begin()
{
    for (uint8_t index = 0; index < 8; ++index)
    {
        pinMode(DIGITAL_PINS[index], INPUT);
    }
}

bool IOPinManager::toggleSimulation(uint8_t pin)
{
    if (pin >= PIN_STORAGE_SIZE || (!isDigitalPin(pin) && !isAnalogPin(pin)))
    {
        return false;
    }

    if (!_isSimulated[pin])
    {
        if (isDigitalPin(pin))
        {
            _simulatedValue[pin] = digitalRead(pin) == HIGH ? LOW : HIGH;
        }
        else
        {
            _simulatedValue[pin] = analogRead(pin) > 512 ? 0 : 1023;
        }
        _isSimulated[pin] = true;
    }
    else if (isDigitalPin(pin))
    {
        _simulatedValue[pin] = _simulatedValue[pin] == HIGH ? LOW : HIGH;
    }
    else
    {
        _simulatedValue[pin] = _simulatedValue[pin] >= 512 ? 0 : 1023;
    }

    return true;
}

bool IOPinManager::resetSimulation(uint8_t pin)
{
    if (pin >= PIN_STORAGE_SIZE || (!isDigitalPin(pin) && !isAnalogPin(pin)))
    {
        return false;
    }
    _isSimulated[pin] = false;
    return true;
}

bool IOPinManager::readDigital(uint8_t pin, uint8_t &value) const
{
    if (pin >= PIN_STORAGE_SIZE || !isDigitalPin(pin))
    {
        return false;
    }
    value = _isSimulated[pin] ? static_cast<uint8_t>(_simulatedValue[pin])
                              : static_cast<uint8_t>(digitalRead(pin));
    return true;
}

bool IOPinManager::readAnalog(uint8_t pin, uint16_t &value) const
{
    if (pin >= PIN_STORAGE_SIZE || !isAnalogPin(pin))
    {
        return false;
    }
    value = _isSimulated[pin] ? static_cast<uint16_t>(_simulatedValue[pin])
                              : static_cast<uint16_t>(analogRead(pin));
    return true;
}

bool IOPinManager::getStatus(uint8_t pin, bool &isAnalog, int16_t &value, bool &isSimulated) const
{
    if (pin >= PIN_STORAGE_SIZE)
    {
        return false;
    }
    isAnalog = isAnalogPin(pin);
    if (!isAnalog && !isDigitalPin(pin))
    {
        return false;
    }
    isSimulated = _isSimulated[pin];
    if (isSimulated)
    {
        value = _simulatedValue[pin];
        return true;
    }
    value = isAnalog ? analogRead(pin) : digitalRead(pin);
    return true;
}

bool IOPinManager::isDigitalPin(uint8_t pin) const
{
    for (uint8_t index = 0; index < 8; ++index)
    {
        if (DIGITAL_PINS[index] == pin)
        {
            return true;
        }
    }
    return false;
}

bool IOPinManager::isAnalogPin(uint8_t pin) const
{
    for (uint8_t index = 0; index < 6; ++index)
    {
        if (ANALOG_PINS[index] == pin)
        {
            return true;
        }
    }
    return false;
}