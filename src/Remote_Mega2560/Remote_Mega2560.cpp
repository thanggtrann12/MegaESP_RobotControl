#include <Std_Types.h>
#include <ComManager.h>
#include <Swc_Joystick.h>
#include <Swc_Button.h>
#include "Remote_Pin_Cfg.h"

static ComManager   comBridge(REMOTE_UART_ESP32);
static Swc_Joystick joysticks(
    REMOTE_PIN_JOY1_X,
    REMOTE_PIN_JOY1_Y,
    REMOTE_PIN_JOY2_X);
static Swc_Button   buttons;

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        ;
    }

    buttons.Init();
    joysticks.Init();
    comBridge.Init(REMOTE_UART_ESP32_BAUD);
    comBridge.SendCommand("ESP_RESET");

    Serial.println(F("[REMOTE] Mega 2560 started"));
    Serial.println(F("[REMOTE] Serial3 bridge ready at 115200 baud"));
}

void loop() {
    static uint32 lastExec = 0;
    char espLog[96];
    ControlPacket echoedPacket;

    if (comBridge.ReadPacket(echoedPacket)) {
        Serial.print(F("[ESP ECHO OK] J1("));
        Serial.print(echoedPacket.joy1_x);
        Serial.print(F(", "));
        Serial.print(echoedPacket.joy1_y);
        Serial.print(F(") J2("));
        Serial.print(echoedPacket.joy2_x);
        Serial.print(F(", "));
        Serial.print(echoedPacket.joy2_y);
        Serial.print(F(") buttons=0x"));
        Serial.println(echoedPacket.buttons, HEX);
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