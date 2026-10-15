/// @brief This is PUBLIC INTERFACE used for Robot

#ifndef SWC_ROBOT_H
#define SWC_ROBOT_H

#include <Arduino.h>
#include "Robot.h"

// Robot dùng chung PCA9685 với CDD_Motor (0x41) và CDD_Servo (0x40),
// nên User Code có thể gọi CDD_Motor_* / CDD_Servo_* song song trên cùng chip.
Robot &SWC_Robot_GetInstance();

// Các hàm API công khai cho User Code
void SWC_Robot_Begin(void);  // khởi tạo 2 chip PCA9685, nạp config EEPROM, dựng chassis
void SWC_Robot_Update(void); // gọi trong loop()

void SWC_Robot_SetSource(Source source);                             // REMOTE / MANUAL
bool SWC_Robot_Joy(int8_t throttle, int8_t strafe, int8_t rotation); // chỉ nhận khi source = MANUAL
bool SWC_Robot_RunMotor(uint8_t slotIndex, int16_t pwm);             // test 1 motor (MANUAL), tự dừng sau timeout
bool SWC_Robot_BrakeMotor(uint8_t slotIndex);
void SWC_Robot_Stop(void); // dừng Drive và TẤT CẢ motor slot

bool SWC_Robot_IsReady(void);
bool SWC_Robot_IsMoving(void);
bool SWC_Robot_IsLinkAlive(void);

#endif // SWC_ROBOT_H