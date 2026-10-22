#include "SRV_Hmi.h"
#include "Hmi.h"
#include "Hmi_Cfg.h"
#include "SWC_Robot.h"
#include "CDD_Gpio.h"

/// @brief Get the shared Hmi instance (created on first use, after Robot and DigitalIo exist)
static Hmi &hmi()
{
    static Hmi instance(HMI_UART, SWC_Robot_GetInstance(), CDD_Gpio_GetInstance());
    return instance;
}

void SRV_Hmi_Begin(void)
{
    HMI_UART.begin(HMI_UART_BAUD, SERIAL_8N1, HMI_UART_RX, HMI_UART_TX);
    hmi().init();
}

void SRV_Hmi_Update(void)
{
    hmi().update();
}
