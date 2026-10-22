#include "SRV_EspNow.h"
#include "EspNowLink.h"
#include <WiFi.h>

/// @brief Get the shared EspNowLink instance (created on first use)
static EspNowLink &link()
{
    static EspNowLink instance;
    return instance;
}

bool SRV_EspNow_Begin(SRV_EspNow_Handler handler, void *context)
{
    WiFi.mode(WIFI_STA); // ESP-NOW cần radio WiFi đã khởi động
    WiFi.disconnect();

    return link().begin(handler, context);
}

void SRV_EspNow_Update(void)
{
    link().update();
}
