#include "CDD_Gpio.h"

// Khởi tạo biến toàn cục DUY NHẤT tại đây (dùng static để giới hạn trong file này)
static DigitalIo g_digitalIoInstance;

/// @brief Get the instance of the DigitalIo object used by CDD_Gpio
/// @return Reference to the DigitalIo instance
DigitalIo &CDD_Gpio_GetInstance()
{
    return g_digitalIoInstance;
}

/// @brief Read the value of a GPIO pin
/// @param pinIndex Index of the pin to read
/// @return GPIO_HIGH if the pin is high, GPIO_LOW if the pin is low
uint8_t CDD_Gpio_ReadPin(uint8_t pinIndex)
{
    return g_digitalIoInstance.read(pinIndex) ? GPIO_HIGH : GPIO_LOW;
}

/// @brief Write a value to a GPIO pin
/// @param pinIndex Index of the pin to write to
/// @param value Value to write (GPIO_HIGH or GPIO_LOW)
void CDD_Gpio_WritePin(uint8_t pinIndex, uint8_t value)
{
    g_digitalIoInstance.write(pinIndex, value);
}

/// @brief Set the mode of a GPIO pin
/// @param pinIndex Index of the pin to configure
/// @param mode Mode to set (e.g., IoPin::INPUT, IoPin::OUTPUT)
void CDD_Gpio_SetPinMode(uint8_t pinIndex, uint8_t mode)
{
    IoPin::configure(g_digitalIoInstance.pin(pinIndex), static_cast<IoPin::Mode>(mode));
}

/// @brief Initialize the CDD_Gpio system
/// @param eepromAddress EEPROM address for storing pin configurations
void CDD_Gpio_Begin(int eepromAddress)
{
    g_digitalIoInstance.begin(eepromAddress);
}