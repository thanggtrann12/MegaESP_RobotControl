/**
 * @file Robot_Mega2560.cpp
 * @brief Main firmware entry for robot-side Mega2560 controller.
 */

#include <Arduino.h>
#include <RobotSystemBuilder.h>
#include "Robot_Pin_Cfg.h"

static RobotSystemBuilder robotSystem;

void setup()
{
	Serial.begin(115200);
	robotSystem.build(KinematicsMode::MODE_4WD_MECANUM);
	robotSystem.begin();
}

void loop()
{
	robotSystem.update();
}