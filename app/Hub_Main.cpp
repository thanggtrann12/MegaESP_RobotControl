/**
 * @file Hub_Main.cpp
 * @brief ESP32 robot controller: remote commands arrive over ESP-NOW and HMI commands over UART.
 */

#include <Arduino.h>
#include <esp_idf_version.h>
#include "Hmi_Cfg.h"
#include "Pca9685_Cfg.h" // I2C_SDA_PIN, I2C_SCL_PIN, I2C_CLOCK_HZ for Wire.begin()
#include "CDD_Gpio.h"
#include "GenericLogger.h"
#include "SRV_EspNow.h"
#include "SRV_Hmi.h"
#include "SWC_Robot.h"
#include "UserCode.h"

/* NOTE: This module initializes and updates sub-modules for the ESP32 hub unit.
 *   - If you want to add more sub-modules, you should initialize and update them in the respective functions.
 *       - You should call `subModuleInit()` in the `setup()` or the first iteration of the `loop()` to initialize all sub-modules.
 *       - Put any code that depends on the sub-modules being initialized inside the `subModuleUpdate()` function.
 *   - The `isSubModuleInited` flag is used to track whether the sub-modules have been initialized.
 */

//======================================= Constants and Global Objects ========================//

constexpr uint32_t LOG_BAUD = 115200;

// ESP-NOW packets are delivered from loop() (SRV_EspNow_Update), never from the WiFi task.
static void onRemotePacket(const ControlPacket &packet, void *context)
{
    (void)context;
    SWC_Robot_OnRemote(packet);
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
}

void loop()
{
    if (!isSubModuleInited)
    {
        subModuleInit();
        userCodeInit();
    }
    else
    {
        subModuleUpdate();
        userCodeUpdate();
    }
}

//======================================== Sub-module Functions ==============================//
void subModuleInit()
{
    LOG_I("Initializing sub-modules");
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, I2C_CLOCK_HZ);

    CDD_Gpio_Begin();    // load saved IO modes and apply them
    SWC_Robot_Begin();   // init PCA9685 chips, load saved config, apply chassis
    SRV_Hmi_Begin();     // start the touchscreen UART and the command handler

    if (!SRV_EspNow_Begin(onRemotePacket, nullptr))
    {
        LOG_E("Failed to initialize ESP-NOW link");
    }

    // Mark sub-modules as initialized - DO NOT forget this step
    isSubModuleInited = true;
    LOG_I("Sub-modules initialized successfully");
}

void subModuleUpdate()
{
    SRV_EspNow_Update(); // first: hand received packets to the robot before it runs its control tick
    SRV_Hmi_Update();
    SWC_Robot_Update();
}