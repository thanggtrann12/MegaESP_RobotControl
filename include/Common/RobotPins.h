#ifndef ROBOT_PINS_H
#define ROBOT_PINS_H

#include <Arduino.h>
/*=============================================================================
=======               DEFINES & MACROS FOR GENERAL PURPOSE              =======
=============================================================================*/

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

#define HMI_UART Serial1 // TJC touchscreen (UART1)
#define HMI_UART_TX 17
#define HMI_UART_RX 18

#define I2C_SDA_PIN 8 // ESP32-S3 default I2C pins
#define I2C_SCL_PIN 9
// #define REMOTE_UART Serial3 // ESP8266 receiver (ESP-NOW)
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