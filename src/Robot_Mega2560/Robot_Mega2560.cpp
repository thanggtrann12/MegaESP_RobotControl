#include <Arduino.h>
#include <ComManager.h>
#include "RobotController.h"

static ComManager robotCom(ROBOT_UART_ESP8266);
static RobotController robot;

void setup() {
	Serial.begin(115200);
	robotCom.Init(ROBOT_UART_ESP8266_BAUD);
	robot.begin();
	robotCom.SendCommand("ESP_RESET");
	Serial.println(F("[ROBOT] Mega 2560 ready"));
}

void loop() {
	ControlPacket packet;
	if (robotCom.ReadPacket(packet)) {
		robot.handlePacket(packet);
	}
	robot.update();
}