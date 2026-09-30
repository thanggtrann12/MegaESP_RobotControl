#include <Arduino.h>
#include <ComManager.h>
#include <GenericRobotController.h>
#include <GenericLogger.h>
#include <HMIService.h>
#include <IOPinManager.h>
#include <Motor_TA6586.h>
#include "Robot_Pin_Cfg.h"

static ComManager robotCom(ROBOT_UART_ESP8266);
static MotorTA6586 leftMotor1(Wire, ROBOT_PCA9685_ADDRESS,
							  ROBOT_MOTOR_L1_BI, ROBOT_MOTOR_L1_FI,
							  ROBOT_PCA9685_FREQUENCY);
static MotorTA6586 leftMotor2(Wire, ROBOT_PCA9685_ADDRESS,
							  ROBOT_MOTOR_L2_BI, ROBOT_MOTOR_L2_FI,
							  ROBOT_PCA9685_FREQUENCY);
static MotorTA6586 rightMotor1(Wire, ROBOT_PCA9685_ADDRESS,
							   ROBOT_MOTOR_R1_BI, ROBOT_MOTOR_R1_FI,
							   ROBOT_PCA9685_FREQUENCY);
static MotorTA6586 rightMotor2(Wire, ROBOT_PCA9685_ADDRESS,
							   ROBOT_MOTOR_R2_BI, ROBOT_MOTOR_R2_FI,
							   ROBOT_PCA9685_FREQUENCY);
static GenericRobotController robot(leftMotor1, leftMotor2, rightMotor1, rightMotor2);
static IOPinManager ioPins;
static HMIService hmiService(ROBOT_UART_HMI, robot, ioPins);

ASSIGN_LOG_MACROS(RobotMega2560, Serial);

void setup()
{
	Serial.begin(115200); // Debug output uses USB Serial.
	robotCom.Init(ROBOT_UART_ESP8266_BAUD);
	ROBOT_UART_HMI.begin(ROBOT_UART_HMI_BAUD);
	ioPins.begin();
	robot.begin();
	robotCom.SendCommand("ESP_RESET");
	RobotMega2560_LogI("Mega 2560 ready");
}

void loop()
{
	hmiService.update();

	ControlPacket packet;
	if (robotCom.ReadPacket(packet))
	{
		robot.handlePacket(packet);
	}
	robot.update();
}