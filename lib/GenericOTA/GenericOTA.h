#ifndef GENERIC_OTA_H
#define GENERIC_OTA_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>

class GenericOTA {
public:
    static bool begin(const char* ssid,
                      const char* password,
                      const char* hostname,
                      WiFiMode_t wifiMode = WIFI_AP_STA);
    static void handle();
};

#endif
