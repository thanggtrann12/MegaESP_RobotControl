#include <Arduino.h>
#include <ComManager.h>
#include <GenericRobotController.h>
#include <GenericLogger.h>
#include <PCA9685Motor.h>
#include "Robot_Pin_Cfg.h"

static ComManager robotCom(ROBOT_UART_ESP8266);
static PCA9685Motor leftMotor1(Wire, ROBOT_PCA9685_ADDRESS,
							   ROBOT_MOTOR_L1_BI, ROBOT_MOTOR_L1_FI,
							   ROBOT_PCA9685_FREQUENCY);
static PCA9685Motor leftMotor2(Wire, ROBOT_PCA9685_ADDRESS,
							   ROBOT_MOTOR_L2_BI, ROBOT_MOTOR_L2_FI,
							   ROBOT_PCA9685_FREQUENCY);
static PCA9685Motor rightMotor1(Wire, ROBOT_PCA9685_ADDRESS,
								ROBOT_MOTOR_R1_BI, ROBOT_MOTOR_R1_FI,
								ROBOT_PCA9685_FREQUENCY);
static PCA9685Motor rightMotor2(Wire, ROBOT_PCA9685_ADDRESS,
								ROBOT_MOTOR_R2_BI, ROBOT_MOTOR_R2_FI,
								ROBOT_PCA9685_FREQUENCY);
static GenericRobotController robot(leftMotor1, leftMotor2, rightMotor1, rightMotor2);

ASSIGN_LOG_MACROS(RobotMega2560, Serial);

void setup()
{
	Serial.begin(115200); // Debug on USB Serial only; Serial2 conflicts with the ESP8266 UART link
	robotCom.Init(ROBOT_UART_ESP8266_BAUD);
	robot.begin();
	robotCom.SendCommand("ESP_RESET");
	RobotMega2560_LogI("Mega 2560 ready");
}

void loop()
{
	ControlPacket packet;
	if (robotCom.ReadPacket(packet))
	{
		robot.handlePacket(packet);
	}
	robot.update();
}