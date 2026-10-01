#ifndef GENERIC_OTA_H
#define GENERIC_OTA_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>

/**
 * @file GenericOTA.h
 * @brief OTA service bootstrap and periodic handler.
 */

/**
 * @class GenericOTA
 * @brief Static utility for non-blocking WiFi + ArduinoOTA lifecycle.
 */
class GenericOTA {
public:
    /**
     * @brief Starts WiFi connection and prepares OTA runtime state.
     * @param ssid WiFi SSID.
     * @param password WiFi password.
     * @param hostname OTA hostname.
     * @param wifiMode ESP8266 WiFi mode.
     * @return true when initialization started.
     */
    static bool begin(const char* ssid,
                      const char* password,
                      const char* hostname,
                      WiFiMode_t wifiMode = WIFI_AP_STA);
    /** @brief Runs OTA and WiFi maintenance in main loop. */
    static void handle();
};

#endif
