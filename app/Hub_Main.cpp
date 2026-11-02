/**
 * @file Hub_Main.cpp
 * @brief ESP32 robot controller: remote commands arrive over ESP-NOW and HMI commands over UART.
 */

#include <Arduino.h>
#include <WiFi.h>
#include "Hmi_Cfg.h"
#include "Pca9685_Cfg.h" // I2C_SDA_PIN, I2C_SCL_PIN, I2C_CLOCK_HZ for Wire.begin()
#include "Gpio_Cfg.h"    // SIGNAL_LED_PIN
#include "CDD_Gpio.h"
#include "GenericLogger.h"
#include "SRV_EspNow.h"
#include "SRV_Hmi.h"
#include "SRV_Ota.h"
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
    // Heartbeat-rate blink (<= every 100 ms) for any packet, so it also blinks while CONTROL replaces heartbeats
    static bool ledOn = false;
    static uint32_t lastToggleMs = 0;
    const uint32_t now = millis();
    if (now - lastToggleMs >= 100)
    {
        lastToggleMs = now;
        ledOn = !ledOn;
        digitalWrite(SIGNAL_LED_PIN, ledOn ? HIGH : LOW);
    }
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
// HMI commands typed on the USB serial port (e.g. TFTOTA from the flash tool), one per line.
static void pollSerialCommand()
{
    static char line[96];
    static uint8_t length = 0;
    while (Serial.available())
    {
        const char c = static_cast<char>(Serial.read());
        if (c == '\n' || c == '\r')
        {
            if (length > 0)
            {
                line[length] = '\0';
                length = 0;
                SRV_Hmi_ExecLine(line);
            }
        }
        else if (length < sizeof(line) - 1)
        {
            line[length++] = c;
        }
    }
}
void subModuleInit()
{
    LOG_I("Initializing sub-modules");
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, I2C_CLOCK_HZ);

    CDD_Gpio_Begin();  // load saved IO modes and apply them
    pinMode(SIGNAL_LED_PIN, OUTPUT);
    digitalWrite(SIGNAL_LED_PIN, LOW);
    SWC_Robot_Begin(); // init PCA9685 chips, load saved config, apply chassis
    SRV_Hmi_Begin();   // start the touchscreen UART and the command handler

    if (!SRV_EspNow_Begin(onRemotePacket, nullptr))
    {
        LOG_E("Failed to initialize ESP-NOW link");
    }
    SRV_Ota_Begin();
    LOG_I("Hub MAC: %s (must equal robotMac in the bridge)", WiFi.macAddress().c_str());

    // Mark sub-modules as initialized - DO NOT forget this step
    isSubModuleInited = true;
    LOG_I("Sub-modules initialized successfully");
}

void subModuleUpdate()
{
    pollSerialCommand();
    SRV_EspNow_Update(); // first: hand received packets to the robot before it runs its control tick
    SRV_Hmi_Update();
    SWC_Robot_Update();

    // Link lost: blink slowly (500 ms) so it differs from the fast blink driven by packets.
    static uint32_t lastSlowBlink = 0;
    static bool slowOn = false;
    if (!SWC_Robot_IsLinkAlive() && millis() - lastSlowBlink >= 500)
    {
        lastSlowBlink = millis();
        slowOn = !slowOn;
        digitalWrite(SIGNAL_LED_PIN, slowOn ? HIGH : LOW);
    }
}