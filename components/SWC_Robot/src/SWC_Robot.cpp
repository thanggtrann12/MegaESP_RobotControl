#include "SWC_Robot.h"
#include "CDD_MotorTA6586.h"
#include "CDD_Pca9685.h"

// Tạo lazy: Robot nhận đúng 2 object Pca9685 do CDD sở hữu (không tạo object thứ hai cho cùng chip).
Robot &SWC_Robot_GetInstance()
{
    static Robot robot(CDD_Motor_GetBankInstance().pca(), CDD_Pca9685_GetServoPca());
    return robot;
}

void SWC_Robot_Begin(void)
{
    SWC_Robot_GetInstance().begin();
}

void SWC_Robot_Update(void)
{
    SWC_Robot_GetInstance().update();
}

void SWC_Robot_OnRemote(const ControlPacket &packet)
{
    SWC_Robot_GetInstance().onRemote(packet);
}

void SWC_Robot_SetSource(Source source)
{
    SWC_Robot_GetInstance().setSource(source);
}

bool SWC_Robot_Joy(int8_t throttle, int8_t strafe, int8_t rotation)
{
    return SWC_Robot_GetInstance().joy(throttle, strafe, rotation);
}

bool SWC_Robot_RunMotor(uint8_t slotIndex, int16_t pwm)
{
    return SWC_Robot_GetInstance().runMotor(slotIndex, pwm);
}

bool SWC_Robot_BrakeMotor(uint8_t slotIndex)
{
    return SWC_Robot_GetInstance().brakeMotor(slotIndex);
}

void SWC_Robot_Stop(void)
{
    SWC_Robot_GetInstance().stop();
}

bool SWC_Robot_IsReady(void)
{
    return SWC_Robot_GetInstance().ready();
}

bool SWC_Robot_IsMoving(void)
{
    return SWC_Robot_GetInstance().moving();
}

bool SWC_Robot_IsLinkAlive(void)
{
    return SWC_Robot_GetInstance().linkAlive();
}