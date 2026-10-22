/// @brief This is PUBLIC INTERFACE used for Hmi (TJC touchscreen over UART)

#ifndef SRV_HMI_H
#define SRV_HMI_H

#include <Arduino.h>

// Các hàm API công khai cho User Code
void SRV_Hmi_Begin(void);  // khởi động UART màn hình + bộ xử lý lệnh HMI
void SRV_Hmi_Update(void); // gọi trong loop(): đọc và xử lý lệnh từ màn hình

#endif // SRV_HMI_H
