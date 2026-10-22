#ifndef EEPROM_MAP_H
#define EEPROM_MAP_H

/**
 * @file Eeprom_Map.h
 * @brief Central map of the ESP32 EEPROM (512 bytes, EEPROM.begin(512)).
 *
 * Keep every persistent store registered here so addresses never collide.
 * The address constants themselves live in each component's cfg header
 * (Robot_Cfg.h, Gpio_Cfg.h) and are repeated below only as documentation.
 *
 *  Address  Size  Store
 *  -------  ----  -------------------------------------------------------
 *  0x10      93   RobotConfig (SWC_Robot)
 *  0x80      26   DigitalIo pin modes (CDD_Gpio)
 */

//--- RobotConfig store (SWC_Robot, address from Robot_Cfg.h) ------------------
// Layout: [0] version byte, [1..91] RobotConfig payload, [92] CRC-8.
// sizeof(RobotConfig) == 93 == 30 + 7 * sizeof(ChassisMap); CRC covers bytes 0..91.
constexpr int ROBOT_CONFIG_STORE_SIZE = 93;

//--- DigitalIo store (CDD_Gpio, address from Gpio_Cfg.h) ----------------------
// Layout: [0] STORE_VERSION, [1..24] 8 pins x 3 config bytes, [25] CRC-8.
constexpr int GPIO_EEPROM_STORE_SIZE = 26;

#endif // EEPROM_MAP_H
