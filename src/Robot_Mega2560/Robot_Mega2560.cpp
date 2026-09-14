#include <Arduino.h>
#include <ComManager.h>
#include <GenericRobotController.h>
#include <GenericLogger.h>
#include <Motor_TA6586.h>
#include "Robot_Pin_Cfg.h"

static ComManager robotCom(ROBOT_UART_ESP8266);
static MotorTA6586 leftMotor(ROBOT_PIN_MOTOR_L_FI, ROBOT_PIN_MOTOR_L_BI);
static MotorTA6586 rightMotor(ROBOT_PIN_MOTOR_R_FI, ROBOT_PIN_MOTOR_R_BI);
static GenericRobotController robot(leftMotor, rightMotor);

ASSIGN_LOG_MACROS(RobotMega2560, Serial);

void setup() {
	Serial.begin(115200);
	robotCom.Init(ROBOT_UART_ESP8266_BAUD);
	robot.begin();
	robotCom.SendCommand("ESP_RESET");
	RobotMega2560_LogI("Mega 2560 ready");
}

void loop() {
	ControlPacket packet;
	if (robotCom.ReadPacket(packet)) {
		robot.handlePacket(packet);
	}
	robot.update();
}