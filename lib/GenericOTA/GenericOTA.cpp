#include "GenericOTA.h"

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

    Serial.println(F("[OTA] WiFi connection started (non-blocking)"));
    return true;
}

void GenericOTA::handle() {
    if (WiFi.status() != WL_CONNECTED) {
        if (millis() - lastWifiAttempt >= 10000) {
            lastWifiAttempt = millis();
            WiFi.reconnect();
            Serial.println(F("[OTA] WiFi unavailable, retrying in background"));
        }
        return;
    }

    if (!otaStarted) {
        ArduinoOTA.setHostname(otaHostname);
        ArduinoOTA.onStart([]() {
            Serial.println(F("[OTA] Update started"));
        });
        ArduinoOTA.onEnd([]() {
            Serial.println(F("[OTA] Update finished"));
        });
        ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
            Serial.printf("[OTA] Progress: %u%%\r", (progress * 100U) / total);
        });
        ArduinoOTA.onError([](ota_error_t error) {
            Serial.printf("[OTA] Error: %u\n", error);
        });
        ArduinoOTA.begin();
        otaStarted = true;

        Serial.print(F("[OTA] Ready as "));
        Serial.println(otaHostname);
    }

    ArduinoOTA.handle();
}
