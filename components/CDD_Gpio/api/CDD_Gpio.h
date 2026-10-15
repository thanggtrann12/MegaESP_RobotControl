/// @brief This is PUBLIC INTERFACE used for DigitalIo

#ifndef CDD_GPIO_H
#define CDD_GPIO_H

#include <Arduino.h>
#include "DigitalIo.h"
#include "IoPin.h"

// Sử dụng inline cho các hằng số global trong header để tránh multiple definition
const uint8_t GPIO_HIGH = 1;
const uint8_t GPIO_LOW = 0;

#define CDD_GPIO_PIN_1 0
#define CDD_GPIO_PIN_2 1
#define CDD_GPIO_PIN_3 2
#define CDD_GPIO_PIN_4 3
#define CDD_GPIO_PIN_5 4
#define CDD_GPIO_PIN_6 5
#define CDD_GPIO_PIN_7 6
#define CDD_GPIO_PIN_8 7

// Dùng extern cho instance toàn cục (chỉ khai báo, không khởi tạo ở đây)
DigitalIo &CDD_Gpio_GetInstance();

uint8_t CDD_Gpio_ReadPin(uint8_t pinIndex);
void CDD_Gpio_WritePin(uint8_t pinIndex, uint8_t value);
void CDD_Gpio_SetPinMode(uint8_t pinIndex, uint8_t mode);
void CDD_Gpio_Begin(int eepromAddress = GPIO_EEPROM_ADDRESS);

#endif