#ifndef ROBOT_PINS_H
#define ROBOT_PINS_H

#include <Arduino.h>
/*=============================================================================
=======               DEFINES & MACROS FOR GENERAL PURPOSE              =======
=============================================================================*/

#define IO_PIN_NUM_1 39
#define IO_PIN_NUM_2 41
#define IO_PIN_NUM_3 43
#define IO_PIN_NUM_4 45
#define IO_PIN_NUM_5 47
#define IO_PIN_NUM_6 49
#define IO_PIN_NUM_7 51
#define IO_PIN_NUM_8 53

#define ANALOG_PIN_NUM_1 A0
#define ANALOG_PIN_NUM_2 A1
#define ANALOG_PIN_NUM_3 A2
#define ANALOG_PIN_NUM_4 A3
#define ANALOG_PIN_NUM_5 A4
#define ANALOG_PIN_NUM_6 A5

#define HMI_UART Serial2    // TJC touchscreen
#define REMOTE_UART Serial3 // ESP8266 receiver (ESP-NOW)
/*=============================================================================
=======                     CONSTANTS  &  TYPES                         =======
=============================================================================*/
constexpr uint32_t UART_BAUD = 115200;

constexpr uint8_t MOTOR_PCA_ADDRESS = 0x40;
constexpr uint16_t MOTOR_PWM_HZ = 1000;
constexpr uint8_t SERVO_PCA_ADDRESS = 0x41; // own chip: servos need 50 Hz, motors 1 kHz

constexpr int CONFIG_EEPROM_ADDRESS = 0x10;
/*=============================================================================
=======                VARIABLES & MESSAGES & RESSOURCEN                =======
=============================================================================*/
#endif // ROBOT_PINS_H