/**
 * @file Hub_Main.cpp
 * @brief ESP32 robot controller: remote commands arrive over ESP-NOW and HMI commands over UART.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <esp_idf_version.h>
#include "Hmi.h"
#include "Robot.h"
#include "Robot_Cfg.h"
#include "Hmi_Cfg.h"
#include "Pca9685_Cfg.h"
#include "Gpio_Cfg.h"
#include "GenericLogger.h"
#include "EspNowLink.h"

/* NOTE: This module initializes and updates sub-modules for the ESP32 hub unit.
 *   - If you want to add more sub-modules, you should initialize and update them in the respective functions.
 *       - You should call `subModuleInit()` in the `setup()` or the first iteration of the `loop()` to initialize all sub-modules.
 *       - Put any code that depends on the sub-modules being initialized inside the `subModuleUpdate()` function.
 *   - The `isSubModuleInited` flag is used to track whether the sub-modules have been initialized.
 */

//======================================= Constants and Global Objects ========================//
static Pca9685 motorPca(Wire, MOTOR_PCA_ADDRESS);
static Pca9685 servoPca(Wire, SERVO_PCA_ADDRESS);
static Robot robot(motorPca, servoPca);
static DigitalIo digitalIo;
static Hmi hmi(HMI_UART, robot, digitalIo);
static EspNowLink espLink;

constexpr uint32_t LOG_BAUD = 115200;

// ESP-NOW packets are delivered from loop() (EspNowLink::update), never from the WiFi task.
static void onRemotePacket(const ControlPacket &packet, void *context)
{
    static_cast<Robot *>(context)->onRemote(packet);
}

//======================================== Logging Macros ====================================//
INIT_LOG(Serial);
ASSIGN_LOG_MACROS(Esp32Hub);

//======================================== Variables =========================================//
bool isSubModuleInited = false;

//======================================== Function Prototypes ===============================//
void subModuleInit();
void subModuleUpdate();

//======================================== Setup and Loop Functions ==========================//
void setup()
{
    Serial.begin(LOG_BAUD);
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    LOG_I("Setup complete");
    isSubModuleInited = false;
}

void loop()
{
    if (!isSubModuleInited)
    {
        subModuleInit();
    }
    else
    {
        subModuleUpdate();
    }
}

//======================================== Sub-module Functions ==============================//
void subModuleInit()
{
    LOG_I("Initializing sub-modules");
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, I2C_CLOCK_HZ);
    HMI_UART.begin(HMI_UART_BAUD, SERIAL_8N1, HMI_UART_RX, HMI_UART_TX);

    digitalIo.begin(GPIO_EEPROM_ADDRESS); // load saved IO modes and apply them
    robot.begin();                        // init PCA9685 chips, load saved config, apply chassis

    if (!espLink.begin(onRemotePacket, &robot))
    {
        LOG_E("Failed to initialize ESP-NOW link");
    }

    hmi.init();

    // Mark sub-modules as initialized - DO NOT forget this step
    isSubModuleInited = true;
    LOG_I("Sub-modules initialized successfully");
}

void subModuleUpdate()
{
    espLink.update(); // first: hand received packets to the robot before it runs its control tick
    hmi.update();
    robot.update();
}