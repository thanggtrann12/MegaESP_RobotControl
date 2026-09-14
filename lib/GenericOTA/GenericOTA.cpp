#include "GenericOTA.h"
#include <GenericLogger.h>

ASSIGN_LOG_MACROS(GenericOTA, Serial);

namespace {
bool otaStarted = false;
uint32_t lastWifiAttempt = 0;
const char* otaHostname = nullptr;
}

bool GenericOTA::begin(const char* ssid,
                       const char* password,
                       const char* hostname,
                       WiFiMode_t wifiMode) {
    WiFi.mode(wifiMode);
    WiFi.setAutoReconnect(true);
    WiFi.begin(ssid, password);
    otaHostname = hostname;

    GenericOTA_LogI("WiFi connection started (non-blocking)");
    return true;
}

void GenericOTA::handle() {
    if (WiFi.status() != WL_CONNECTED) {
        if (millis() - lastWifiAttempt >= 10000) {
            lastWifiAttempt = millis();
            WiFi.reconnect();
            GenericOTA_LogW("WiFi unavailable, retrying in background");
        }
        return;
    }

    if (!otaStarted) {
        ArduinoOTA.setHostname(otaHostname);
        ArduinoOTA.onStart([]() {
            GenericOTA_LogI("Update started");
        });
        ArduinoOTA.onEnd([]() {
            GenericOTA_LogI("Update finished");
        });
        ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
            GenericOTA_LogD("Progress: %u%%", (progress * 100U) / total);
        });
        ArduinoOTA.onError([](ota_error_t error) {
            GenericOTA_LogE("Update error: %u", error);
        });
        ArduinoOTA.begin();
        otaStarted = true;

        GenericOTA_LogI("Ready as %s", otaHostname);
    }

    ArduinoOTA.handle();
}
