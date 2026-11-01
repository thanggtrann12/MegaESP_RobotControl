/**
 * @file Remote_Mega2560.cpp
 * @brief Main firmware entry for remote-side Mega2560 controller.
 */

#include <ControlPacket.h>
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
    // CONTROL is sent on change; HEARTBEAT keeps the link alive while the sticks are idle.
    // A periodic CONTROL refresh re-syncs the hub if a change (e.g. back to zero) was lost.
    constexpr uint32_t CONTROL_MIN_GAP_MS = 20;
    constexpr uint32_t HEARTBEAT_MS = 100;
    constexpr uint32_t CONTROL_REFRESH_MS = 200;

    ControlPacket echoedPacket;
    static uint32 lastEchoLog = 0;
    static uint32_t lastSend = 0;
    static uint32_t lastControlSend = 0;
    static ControlPacket lastControl = {};
    int16 joy1X;
    int16 joy1Y;
    int16 joy2X;
    joysticks.GetProcessedValues(joy1X, joy1Y, joy2X);

    ControlPacket current = {};
    current.msgType = CONTROL_MESSAGE;
    current.throttle = static_cast<int8_t>(map(joy1Y, 0, 1023, -100, 100));
    current.strafe = static_cast<int8_t>(map(joy2X, 0, 1023, -100, 100));
    current.rotation = static_cast<int8_t>(map(joy1X, 0, 1023, -100, 100));
    current.buttons = static_cast<uint8_t>(buttons.GetState() & 0xFF);

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

    const uint32_t now = millis();
    const bool changed = current.throttle != lastControl.throttle ||
                         current.strafe != lastControl.strafe ||
                         current.rotation != lastControl.rotation ||
                         current.buttons != lastControl.buttons;
    const bool sendControl = (changed && now - lastSend >= CONTROL_MIN_GAP_MS) ||
                             now - lastControlSend >= CONTROL_REFRESH_MS;
    const bool sendHeartbeat = !sendControl && now - lastSend >= HEARTBEAT_MS;
    if (!sendControl && !sendHeartbeat)
    {
        return;
    }

    packet = current;
    if (sendHeartbeat)
    {
        packet.msgType = HEARTBEAT_MESSAGE;
        packet.throttle = 0;
        packet.strafe = 0;
        packet.rotation = 0;
        packet.buttons = 0;
    }
    else
    {
        lastControl = current;
        lastControlSend = now;
    }
    packet.sequenceNum = sequenceNumber++;
    UpdateControlPacketCrc(packet);
    lastSend = now;
    comBridge.SendPacket(packet);
}
