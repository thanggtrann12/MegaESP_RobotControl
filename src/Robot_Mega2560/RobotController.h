#ifndef ROBOT_CONTROLLER_H
#define ROBOT_CONTROLLER_H

#include <Arduino.h>
#include "Motor_TA6586.h"
#include "Robot_Pin_Cfg.h"
#include "Std_Types.h"

class RobotController {
public:
    RobotController();
    void begin();
    void handlePacket(const ControlPacket& packet);
    void update();

private:
    static int16_t mapAxisToMotor(int16_t value);
    void applyControl(const ControlPacket& packet);
    void stopMotors();
    void logPacket(const ControlPacket& packet);

    MotorTA6586 _leftMotor;
    MotorTA6586 _rightMotor;
    uint32_t _lastPacketTime;
    uint32_t _lastLogTime;
};

#endif
