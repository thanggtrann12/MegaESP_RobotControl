/**
 * @file GenericOTA.cpp
 * @brief WiFi reconnect and ArduinoOTA runtime implementation.
 */

#include "GenericOTA.h"
#include <GenericLogger.h>

ASSIGN_LOG_MACROS(GenericOTA, Serial);

namespace
{
    bool otaStarted = false;
    uint32_t lastWifiAttempt = 0;
    const char *otaHostname = nullptr;
}

bool GenericOTA::begin(const char *ssid,
                       const char *password,
                       const char *hostname,
                       WiFiMode_t wifiMode)
{
    WiFi.mode(wifiMode);
    WiFi.setAutoReconnect(true);
    WiFi.begin(ssid, password);
    otaHostname = hostname;

    LOG_I("WiFi connection started (non-blocking)");
    return true;
}

void GenericOTA::handle()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        if (millis() - lastWifiAttempt >= 10000)
        {
            lastWifiAttempt = millis();
            WiFi.reconnect();
            LOG_W("WiFi unavailable, retrying in background");
        }
        return;
    }

    if (!otaStarted)
    {
        ArduinoOTA.setHostname(otaHostname);
        ArduinoOTA.onStart([]()
                           { LOG_I("Update started"); });
        ArduinoOTA.onEnd([]()
                         { LOG_I("Update finished"); });
        ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                              { LOG_D("Progress: %u%%", (progress * 100U) / total); });
        ArduinoOTA.onError([](ota_error_t error)
                           { LOG_E("Update error: %u", error); });
        ArduinoOTA.begin();
        otaStarted = true;

        LOG_I("Ready as %s", otaHostname);
    }

    ArduinoOTA.handle();
}
