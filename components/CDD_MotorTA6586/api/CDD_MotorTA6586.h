/// @brief This is PUBLIC INTERFACE used for MotorTA6586 / MotorBank

#ifndef CDD_MOTOR_TA6586_H
#define CDD_MOTOR_TA6586_H

#include <Arduino.h>
#include "MotorBank.h"
#include "Motor.h"

// Macro định nghĩa các slot motor (0 đến 5 tương ứng M0 đến M5)
#define CDD_MOTOR_SLOT_1 0
#define CDD_MOTOR_SLOT_2 1
#define CDD_MOTOR_SLOT_3 2
#define CDD_MOTOR_SLOT_4 3
#define CDD_MOTOR_SLOT_5 4
#define CDD_MOTOR_SLOT_6 5

// Khai báo instance quản lý motor chung toàn cục qua extern
MotorBank &CDD_Motor_GetBankInstance();

// Các hàm API công khai cho User Code
void CDD_Motor_Begin(uint16_t pwmHz = 1000);
void CDD_Motor_Run(uint8_t slotIndex, int16_t pwm); // pwm: -255 đến 255
void CDD_Motor_Brake(uint8_t slotIndex);
void CDD_Motor_Stop(uint8_t slotIndex);
void CDD_Motor_StopAll(void);

#endif // CDD_MOTOR_TA6586_H