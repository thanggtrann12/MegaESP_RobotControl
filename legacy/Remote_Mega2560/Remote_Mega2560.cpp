/**
 * @file Remote_Mega2560.cpp
 * @brief Main firmware entry for remote-side Mega2560 controller.
 */

#include <Std_Types.h>
#include <ComManager.h>
#include <GenericLogger.h>
#include <Swc_Joystick.h>
#include <Swc_Button.h>
#include "Remote_Pin_Cfg.h"

//======================================= Constants and Global Objects ========================//
static ComManager comBridge(REMOTE_UART_ESP32);
static Swc_Joystick joysticks(
    REMOTE_PIN_JOY1_X,
    REMOTE_PIN_JOY1_Y,
    REMOTE_PIN_JOY2_X);
static Swc_Button buttons;
static uint16_t sequenceNumber = 0;
ControlPacket packet;

//======================================== Logging Macros ====================================//
INIT_LOG(Serial);
ASSIGN_LOG_MACROS(RemoteMega2560);

//======================================== Function Prototypes ===============================//
void subModuleInit();
void subModuleUpdate();
void updatePacket(ControlPacket &packet);

//======================================== Setup and Loop Functions ==========================//
void setup()
{
    Serial.begin(115200);
    subModuleInit();
}

void loop()
{
    subModuleUpdate();
}

void subModuleInit()
{
    LOG_I("Initializing sub-modules...");
    LOG_I("Initializing buttons...");
    buttons.Init();
    LOG_I("Buttons initialized");
    joysticks.Init();
    LOG_I("Joysticks initialized");
    comBridge.Init(REMOTE_UART_ESP32_BAUD);
    comBridge.SendCommand("ESP_RESET");
    LOG_I("Communication bridge initialized");
    LOG_I("Sub-modules initialized successfully");
}

void subModuleUpdate()
{
    buttons.Update();
    joysticks.Update();

    updatePacket(packet);
}

void updatePacket(ControlPacket &packet)
{
    ControlPacket echoedPacket;
    static uint32 lastEchoLog = 0;
    int16 joy1X;
    int16 joy1Y;
    int16 joy2X;
    joysticks.GetProcessedValues(joy1X, joy1Y, joy2X);

    memset(&packet, 0, sizeof(ControlPacket));
    packet.msgType = CONTROL_MESSAGE;
    packet.throttle = static_cast<int8_t>(map(joy1Y, 0, 1023, -100, 100));
    packet.strafe = static_cast<int8_t>(map(joy2X, 0, 1023, -100, 100));
    packet.rotation = static_cast<int8_t>(map(joy1X, 0, 1023, -100, 100));
    packet.buttons = static_cast<uint8_t>(buttons.GetState() & 0xFF);
    packet.sequenceNum = sequenceNumber++;
    UpdateControlPacketCrc(packet);

    if (comBridge.ReadPacket(echoedPacket) && millis() - lastEchoLog >= 500)
    {
        lastEchoLog = millis();
        LOG_D("ESP ECHO OK type=%u throttle=%d strafe=%d rotation=%d buttons=0x%02X seq=%u",
              echoedPacket.msgType,
              echoedPacket.throttle,
              echoedPacket.strafe,
              echoedPacket.rotation,
              echoedPacket.buttons,
              echoedPacket.sequenceNum);
    }
    static uint32_t lastSend = 0;
    if (millis() - lastSend >= 20)
    { // 50 Hz
        lastSend = millis();
        comBridge.SendPacket(packet);
    }
}
