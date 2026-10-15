#include "CDD_Pca9685.h"
#include <Wire.h>

// SERVO_PCA_ADDRESS (constexpr, 0x40) nằm trong Pca9685_Cfg.h
#include "Pca9685_Cfg.h"

// Tạo lazy (lần gọi đầu tiên) để tránh lỗi thứ tự khởi tạo static trên Arduino.
Pca9685 &CDD_Pca9685_GetServoPca()
{
    static Pca9685 pca(Wire, SERVO_PCA_ADDRESS);
    return pca;
}

ServoOut &CDD_Servo_GetInstance()
{
    static ServoOut servo(CDD_Pca9685_GetServoPca());
    return servo;
}

void CDD_Servo_Begin(void)
{
    CDD_Servo_GetInstance().begin();
}

void CDD_Servo_Write(uint8_t channel, uint8_t angle)
{
    if (channel >= CDD_SERVO_CHANNEL_COUNT)
    {
        return;
    }
    if (angle > 180)
    {
        angle = 180;
    }
    CDD_Servo_GetInstance().write(channel, angle);
}

void CDD_Servo_Release(uint8_t channel)
{
    if (channel >= CDD_SERVO_CHANNEL_COUNT)
    {
        return;
    }
    CDD_Servo_GetInstance().release(channel);
}

void CDD_Servo_ReleaseAll(void)
{
    for (uint8_t ch = 0; ch < CDD_SERVO_CHANNEL_COUNT; ++ch)
    {
        CDD_Servo_GetInstance().release(ch);
    }
}