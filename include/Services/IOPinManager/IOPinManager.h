#ifndef IO_PIN_MANAGER_H
#define IO_PIN_MANAGER_H

#include <Arduino.h>
#include "Robot_Pin_Cfg.h"

/**
 * @file IOPinManager.h
 * @brief Input pin readback and simulation manager for robot diagnostics.
 */

/**
 * @class IOPinManager
 * @brief Handles digital/analog input reads and optional simulated values.
 */
class IOPinManager
{
public:
    /** @brief Constructs the manager with simulation disabled for all pins. */
    IOPinManager();
    /** @brief Configures known digital input pins. */
    void begin();
    /**
     * @brief Toggles simulation state/value for a configured pin.
     * @param pin Target pin number.
     * @return true on success.
     * @return false when pin is out of range or not configured.
     */
    bool toggleSimulation(uint8_t pin);
    /**
     * @brief Disables simulation for one configured pin.
     * @param pin Target pin number.
     * @return true on success.
     * @return false when pin is out of range or not configured.
     */
    bool resetSimulation(uint8_t pin);
    /**
     * @brief Reads a digital pin state.
     * @param pin Target digital pin.
     * @param value Output state value.
     * @return true when read succeeded.
     * @return false when pin is invalid or not digital.
     */
    bool readDigital(uint8_t pin, uint8_t &value) const;
    /**
     * @brief Reads an analog pin value.
     * @param pin Target analog pin.
     * @param value Output ADC value.
     * @return true when read succeeded.
     * @return false when pin is invalid or not analog.
     */
    bool readAnalog(uint8_t pin, uint16_t &value) const;
    /**
     * @brief Gets pin status including type, value, and simulation state.
     * @param pin Target pin.
     * @param isAnalog Output true for analog pins.
     * @param value Output pin value.
     * @param isSimulated Output simulation flag.
     * @return true when pin is configured and status is available.
     * @return false otherwise.
     */
    bool getStatus(uint8_t pin, bool &isAnalog, int16_t &value, bool &isSimulated) const;

private:
    static constexpr uint8_t PIN_STORAGE_SIZE = 70;
    /**
     * @brief Checks whether pin is part of configured digital pin list.
     * @param pin Pin number.
     * @return true if pin is configured as digital input.
     */
    bool isDigitalPin(uint8_t pin) const;
    /**
     * @brief Checks whether pin is part of configured analog pin list.
     * @param pin Pin number.
     * @return true if pin is configured as analog input.
     */
    bool isAnalogPin(uint8_t pin) const;

    bool _isSimulated[PIN_STORAGE_SIZE];
    int16_t _simulatedValue[PIN_STORAGE_SIZE];
};

#endif