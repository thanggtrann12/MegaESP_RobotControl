#include <Std_Types.h>
#include <ComManager.h>
#include <GenericLogger.h>
#include <Swc_Joystick.h>
#include <Swc_Button.h>
#include "Remote_Pin_Cfg.h"

static ComManager   comBridge(REMOTE_UART_ESP32);
static Swc_Joystick joysticks(
    REMOTE_PIN_JOY1_X,
    REMOTE_PIN_JOY1_Y,
    REMOTE_PIN_JOY2_X);
static Swc_Button   buttons;

ASSIGN_LOG_MACROS(RemoteMega2560, Serial);

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        ;
    }

    buttons.Init();
    joysticks.Init();
    comBridge.Init(REMOTE_UART_ESP32_BAUD);
    comBridge.SendCommand("ESP_RESET");

    RemoteMega2560_LogI("Mega 2560 started");
    RemoteMega2560_LogI("Serial3 bridge ready at 115200 baud");
}

void loop() {
    static uint32 lastExec = 0;
    static uint32 lastEchoLog = 0;
    ControlPacket echoedPacket;

    if (comBridge.ReadPacket(echoedPacket) && millis() - lastEchoLog >= 500) {
        lastEchoLog = millis();
        RemoteMega2560_LogD("ESP ECHO OK J1(%d, %d) J2(%d, %d) buttons=0x%04X",
                            echoedPacket.joy1_x,
                            echoedPacket.joy1_y,
                            echoedPacket.joy2_x,
                            echoedPacket.joy2_y,
                            echoedPacket.buttons);
    }

    if (millis() - lastExec >= 20) { // Chu kỳ 50Hz chuẩn
        lastExec = millis();

        buttons.Update();
        joysticks.Update();

        ControlPacket packet;
        joysticks.GetProcessedValues(packet.joy1_x, packet.joy1_y, packet.joy2_x);
        packet.joy2_y = 512;
        packet.buttons = buttons.GetState();

        // Gửi gói tin sang ESP32
        comBridge.SendPacket(packet);
    }
}