#include "HmiInternal.h"
#include "Hmi_Cfg.h"
#include "SRV_Ota.h"
#include <mbedtls/sha256.h>
#include "GenericLogger.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

ASSIGN_LOG_MACROS(HMI)

// Screen firmware update: download the .tft over HTTPS and stream it to the TJC screen with the
// "whmi-wri" protocol (4096-byte blocks, the screen answers 0x05 after each one).
// The update is blocking by design: the robot is stopped first and the hub restarts afterwards,
// which also puts WiFi back on the ESP-NOW channel.

namespace
{
    constexpr size_t BLOCK_SIZE = 4096;
    constexpr uint8_t SCREEN_ACK = 0x05;

    bool waitAck(Stream &serial, uint32_t timeoutMs)
    {
        const uint32_t start = millis();
        while (millis() - start < timeoutMs)
        {
            while (serial.available())
            {
                if (serial.read() == SCREEN_ACK)
                {
                    return true;
                }
            }
            delay(1);
        }
        return false;
    }

    bool connectWifi()
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            return true;
        }
        WiFi.mode(WIFI_STA);
        WiFi.begin(HMI_OTA_WIFI_SSID, HMI_OTA_WIFI_PASSWORD);
        const uint32_t start = millis();
        while (WiFi.status() != WL_CONNECTED)
        {
            if (millis() - start > HMI_OTA_WIFI_TIMEOUT_MS)
            {
                return false;
            }
            delay(100);
        }
        return true;
    }
}

Hmi::Result Hmi::handleFwOta(char **f, uint8_t n)
{
    const bool dry = n >= 2 && is(f[1], "DRY");
    armDrive(0);
    _robot.stop();
    if (!dry)
    {
        reply("FWOTA", true);
    }
    const bool ok = SRV_Ota_UpdateHubFirmware(dry);
    LOG_I("Firmware update %s, restarting hub", ok ? "finished" : "FAILED");
    Serial.flush();
    delay(500);
    ESP.restart();
    return Result::NONE;
}

Hmi::Result Hmi::handleTftOta(char **f, uint8_t n)
{
    // TFTOTA[,<url>[,DRY]]: "-" or no URL = newest "screen" file on the OTA server (sha256-checked)
    const char *url = (n >= 2 && strcmp(f[1], "-") != 0) ? f[1] : nullptr;
    const bool dry = n >= 3 && is(f[2], "DRY");

    armDrive(0);
    _robot.stop();
    SRV_Ota_Pause();
    if (!dry)
    {
        reply("TFTOTA", true); // tells the screen the update has started
    }

    bool ok = false;
    SrvOtaFile latest;
    if (url != nullptr)
    {
        ok = runTftOta(url, dry, nullptr);
    }
    else if (!connectWifi())
    {
        LOG_E("Screen update: WiFi connection failed");
    }
    else if (!SRV_Ota_GetLatest("screen", latest))
    {
        LOG_E("Screen update: cannot get the latest screen firmware from the server");
    }
    else
    {
        // Download once to verify the sha256, then again to stream to the screen: the screen is
        // never written with a file that failed the check (the hub cannot buffer 6 MB).
        ok = runTftOta(latest.url, true, latest.sha256);
        if (ok && !dry)
        {
            ok = runTftOta(latest.url, false, nullptr);
        }
    }
    LOG_I("Screen update %s, restarting hub", ok ? "finished" : "FAILED");
    Serial.flush();
    delay(ok && !dry ? 3000 : 500); // the screen reboots into the new firmware on its own
    ESP.restart();
    return Result::NONE;
}

bool Hmi::runTftOta(const char *url, bool dry, const char *expectedSha256)
{
    LOG_I("Screen update%s: %s", dry ? " (dry run)" : "", url);
    if (!connectWifi())
    {
        LOG_E("Screen update: WiFi connection failed");
        return false;
    }

    WiFiClientSecure secureClient;
    WiFiClient plainClient;
    secureClient.setInsecure(); // no certificate pinning on the hub
    HTTPClient http;
    http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    http.setTimeout(HMI_OTA_DOWNLOAD_TIMEOUT_MS);
    const bool https = strncmp(url, "https://", 8) == 0;
    if (!(https ? http.begin(secureClient, url) : http.begin(plainClient, url)))
    {
        LOG_E("Screen update: bad URL");
        return false;
    }
    const int code = http.GET();
    const int size = http.getSize();
    if (code != HTTP_CODE_OK || size <= 0)
    {
        LOG_E("Screen update: download failed (HTTP %d, size %d)", code, size);
        http.end();
        return false;
    }
    LOG_I("Screen update: %d bytes", size);

    uint8_t *block = static_cast<uint8_t *>(malloc(BLOCK_SIZE));
    if (block == nullptr)
    {
        LOG_E("Screen update: out of memory");
        http.end();
        return false;
    }

    bool ok = true;
    if (!dry)
    {
        while (_serial.available())
        {
            _serial.read(); // drop stale replies so only the real 0x05 counts
        }
        endMsg(_serial);
        {
            Frame frame;
            frame.add("whmi-wri ").add(static_cast<long>(size)).add(',').add(static_cast<long>(HMI_UART_BAUD)).add(",0").send(_serial);
        }
        ok = waitAck(_serial, HMI_OTA_ACK_TIMEOUT_MS);
        if (!ok)
        {
            LOG_E("Screen update: screen did not answer whmi-wri");
        }
    }

    WiFiClient *stream = http.getStreamPtr();
    int sent = 0;
    int lastPercent = -1;
    uint32_t checksum = 0;
    mbedtls_sha256_context sha;
    mbedtls_sha256_init(&sha);
    mbedtls_sha256_starts_ret(&sha, 0);
    const uint32_t startMs = millis();
    while (ok && sent < size)
    {
        const size_t want = min(static_cast<size_t>(size - sent), BLOCK_SIZE);
        size_t got = 0;
        uint32_t lastData = millis();
        while (got < want)
        {
            const int available = stream->available();
            if (available > 0)
            {
                got += stream->readBytes(block + got, min(static_cast<size_t>(available), want - got));
                lastData = millis();
            }
            else if (millis() - lastData > HMI_OTA_DOWNLOAD_TIMEOUT_MS)
            {
                break;
            }
            else
            {
                delay(1);
            }
        }
        if (got < want)
        {
            LOG_E("Screen update: download stalled at %d/%d", sent, size);
            ok = false;
            break;
        }

        if (dry)
        {
            mbedtls_sha256_update_ret(&sha, block, got);
            for (size_t i = 0; i < got; ++i)
            {
                checksum += block[i];
            }
        }
        else
        {
            _serial.write(block, got);
            _serial.flush();
            if (!waitAck(_serial, HMI_OTA_ACK_TIMEOUT_MS))
            {
                LOG_E("Screen update: no ack from the screen at %d/%d", sent, size);
                ok = false;
                break;
            }
        }
        sent += static_cast<int>(got);

        const int percent = static_cast<int>(static_cast<int64_t>(sent) * 100 / size);
        if (percent / 10 != lastPercent / 10)
        {
            lastPercent = percent;
            LOG_I("Screen update: %d%%", percent);
        }
    }
    uint8_t digest[32];
    mbedtls_sha256_finish_ret(&sha, digest);
    mbedtls_sha256_free(&sha);
    if (ok && dry)
    {
        LOG_I("Screen update (dry run): %d bytes in %u ms, byte sum 0x%08X", sent,
              (unsigned)(millis() - startMs), (unsigned)checksum);
        if (expectedSha256 != nullptr)
        {
            char hex[65];
            for (size_t i = 0; i < sizeof(digest); ++i)
            {
                snprintf(hex + 2 * i, 3, "%02x", digest[i]);
            }
            ok = strcasecmp(hex, expectedSha256) == 0;
            if (ok)
            {
                LOG_I("Screen update: sha256 verified");
            }
            else
            {
                LOG_E("Screen update: sha256 mismatch (got %s)", hex);
            }
        }
    }

    free(block);
    http.end();
    return ok;
}
