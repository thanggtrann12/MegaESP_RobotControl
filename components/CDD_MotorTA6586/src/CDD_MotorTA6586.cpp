#include "CDD_MotorTA6586.h"
#include <Pca9685.h>

#include <Wire.h>

#include "Pca9685_Cfg.h"

// Instance PCA9685 + MotorBank được tạo lazy (lần gọi đầu tiên) để tránh
// lỗi thứ tự khởi tạo static trên Arduino (Wire chưa sẵn sàng trước setup()).
MotorBank &CDD_Motor_GetBankInstance()
{
    static Pca9685 pca(Wire, MOTOR_PCA_ADDRESS);
    static MotorBank bank(pca);
    return bank;
}

void CDD_Motor_Begin(uint16_t pwmHz)
{
    CDD_Motor_GetBankInstance().begin(pwmHz);
}

void CDD_Motor_Run(uint8_t slotIndex, int16_t pwm)
{
    if (slotIndex >= MOTOR_SLOTS)
    {
        return;
    }
    CDD_Motor_GetBankInstance().motor(slotIndex)->run(pwm);
}

void CDD_Motor_Brake(uint8_t slotIndex)
{
    if (slotIndex >= MOTOR_SLOTS)
    {
        return;
    }
    CDD_Motor_GetBankInstance().motor(slotIndex)->brake();
}

void CDD_Motor_Stop(uint8_t slotIndex)
{
    if (slotIndex >= MOTOR_SLOTS)
    {
        return;
    }
    CDD_Motor_GetBankInstance().motor(slotIndex)->stop();
}

void CDD_Motor_StopAll(void)
{
    CDD_Motor_GetBankInstance().stopAll();
}