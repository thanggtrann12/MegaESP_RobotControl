#include <Arduino.h>
#include <ComManager.h>
#include <GenericRobotController.h>
#include <GenericLogger.h>
#include <PCA9685Motor.h>
#include "Robot_Pin_Cfg.h"

static ComManager robotCom(ROBOT_UART_ESP8266);
static PCA9685Motor leftMotor(Wire,
							  ROBOT_PCA9685_ADDRESS,
							  ROBOT_MOTOR_L_EN,
							  ROBOT_MOTOR_L_IN1,
							  ROBOT_MOTOR_L_IN2,
							  ROBOT_PCA9685_FREQUENCY);
static PCA9685Motor rightMotor(Wire,
							   ROBOT_PCA9685_ADDRESS,
							   ROBOT_MOTOR_R_EN,
							   ROBOT_MOTOR_R_IN1,
							   ROBOT_MOTOR_R_IN2,
							   ROBOT_PCA9685_FREQUENCY);
static GenericRobotController robot(leftMotor, rightMotor);

ASSIGN_LOG_MACROS(RobotMega2560, Serial);

void setup() {
	Serial.begin(115200); // Debug on USB Serial only; Serial2 conflicts with the ESP8266 UART link
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