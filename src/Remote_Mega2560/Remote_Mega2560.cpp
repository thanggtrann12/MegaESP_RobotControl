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

static ComManager comBridge(REMOTE_UART_ESP32);
static Swc_Joystick joysticks(
    REMOTE_PIN_JOY1_X,
    REMOTE_PIN_JOY1_Y,
    REMOTE_PIN_JOY2_X);
static Swc_Button buttons;
static uint16_t sequenceNumber = 0;

ASSIGN_LOG_MACROS(RemoteMega2560, Serial);

void setup()
{
    Serial.begin(115200);
    while (!Serial)
    {
        ;
    }

    buttons.Init();
    joysticks.Init();
    comBridge.Init(REMOTE_UART_ESP32_BAUD);
    comBridge.SendCommand("ESP_RESET");

    RemoteMega2560_LogI("Mega 2560 started");
    RemoteMega2560_LogI("Serial3 bridge ready at 115200 baud");
}

void loop()
{
    static uint32 lastExec = 0;
    static uint32 lastEchoLog = 0;
    ControlPacket echoedPacket;

    if (comBridge.ReadPacket(echoedPacket) && millis() - lastEchoLog >= 500)
    {
        lastEchoLog = millis();
        RemoteMega2560_LogD("ESP ECHO OK type=%u throttle=%d strafe=%d rotation=%d buttons=0x%02X seq=%u",
                            echoedPacket.msgType,
                            echoedPacket.throttle,
                            echoedPacket.strafe,
                            echoedPacket.rotation,
                            echoedPacket.buttons,
                            echoedPacket.sequenceNum);
    }

    if (millis() - lastExec >= 20)
    { // Chu kỳ 50Hz chuẩn
        lastExec = millis();

        buttons.Update();
        joysticks.Update();

        ControlPacket packet;
        int16 joy1X;
        int16 joy1Y;
        int16 joy2X;
        joysticks.GetProcessedValues(joy1X, joy1Y, joy2X);

        packet.msgType = CONTROL_MESSAGE;
        packet.throttle = static_cast<int8_t>(map(joy1Y, 0, 1023, -100, 100));
        packet.strafe = static_cast<int8_t>(map(joy2X, 0, 1023, -100, 100));
        packet.rotation = static_cast<int8_t>(map(joy1X, 0, 1023, -100, 100));
        packet.buttons = static_cast<uint8_t>(buttons.GetState() & 0xFF);
        packet.sequenceNum = sequenceNumber++;
        UpdateControlPacketCrc(packet);

        // Gửi gói tin sang ESP32
        comBridge.SendPacket(packet);
    }
}