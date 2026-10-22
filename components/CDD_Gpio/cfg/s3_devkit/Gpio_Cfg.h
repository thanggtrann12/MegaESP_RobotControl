#ifndef GPIO_CFG_H
#define GPIO_CFG_H

#include <Arduino.h>

// General purpose IO managed by DigitalIo (8 pins)
#define IO_PIN_NUM_1 38 // green pins
#define IO_PIN_NUM_2 47
#define IO_PIN_NUM_3 48
#define IO_PIN_NUM_4 19 // yellow pins
#define IO_PIN_NUM_5 39
#define IO_PIN_NUM_6 40
#define IO_PIN_NUM_7 41
#define IO_PIN_NUM_8 42

#define ANALOG_PIN_NUM_1 1
#define ANALOG_PIN_NUM_2 2
#define ANALOG_PIN_NUM_3 4
#define ANALOG_PIN_NUM_4 5
#define ANALOG_PIN_NUM_5 6
#define ANALOG_PIN_NUM_6 7

// 4 encoders, two channels each
#define ENCODER_PIN_NUM_1A 10
#define ENCODER_PIN_NUM_1B 11
#define ENCODER_PIN_NUM_2A 12
#define ENCODER_PIN_NUM_2B 13
#define ENCODER_PIN_NUM_3A 14
#define ENCODER_PIN_NUM_3B 15
#define ENCODER_PIN_NUM_4A 16
#define ENCODER_PIN_NUM_4B 21

// EEPROM: DigitalIo needs DigitalIo::STORE_SIZE bytes. Robot config lives at ROBOT_CONFIG_EEPROM_ADDRESS
// (see Robot_Cfg.h) and is 93 bytes long, so keep this one clear of it.
constexpr int GPIO_EEPROM_ADDRESS = 0x80;

#endif // GPIO_CFG_H
