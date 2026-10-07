/**
 * @file Robot_Mega2560.cpp
 * @brief Robot controller: remote and HMI come in over UART, motors and servos go out over PCA9685.
 */

#include <Arduino.h>
#include <ComManager.h>
#include "Hmi.h"
#include "Robot.h"
#include "RobotPins.h"
#include "GenericLogger.h"

static Pca9685 motorPca(Wire, MOTOR_PCA_ADDRESS);
static Pca9685 servoPca(Wire, SERVO_PCA_ADDRESS);
static Robot robot(motorPca, servoPca);
DigitalIo digitalIo;
static Hmi hmi(HMI_UART, robot, digitalIo);
static ComManager remote(REMOTE_UART);

ASSIGN_LOG_MACROS(RobotMega2560, Serial);

void setup()
{
    Serial.begin(UART_BAUD);
    HMI_UART.begin(UART_BAUD);
    hmi.init();
    remote.Init(UART_BAUD);
    robot.begin();
    remote.SendCommand("ESP_RESET");
    LOG_I("Setup complete");
}

void loop()
{
    hmi.update();
    ControlPacket packet;
    if (remote.ReadPacket(packet))
    {
        robot.onRemote(packet);
    }

    robot.update();
}
