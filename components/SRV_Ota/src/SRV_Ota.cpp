#include "SRV_Ota.h"
#include "SRV_Ota_Cfg.h"
#include "Hmi_Cfg.h"
#include "hub_key.h"
#include <Ed25519.h>
#include <GenericLogger.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <mbedtls/base64.h>
#include <mbedtls/sha256.h>
#include <Update.h>
#include <Preferences.h>
#include <cstring>

ASSIGN_LOG_MACROS(HubOta)

namespace
{
    constexpr size_t SIGNATURE_SIZE = 64;
    constexpr size_t BASE64_SIGNATURE_SIZE = 4 * ((SIGNATURE_SIZE + 2) / 3);
    constexpr size_t NONCE_SIZE = 32;

    SemaphoreHandle_t checkinMutex = nullptr;
    TaskHandle_t checkinTaskHandle = nullptr;
    bool checkinPaused = false;
    bool updatePending = false;
    SrvOtaFile pendingOffer;
    bool updateAttempted = false; // one automatic attempt per boot, so a failing update cannot loop

    // The version installed by the last OTA is kept in NVS, otherwise the server would offer it forever.
    String installedFirmware()
    {
        Preferences prefs;
        String version = HUB_OTA_FIRMWARE_VERSION;
        if (prefs.begin("hubota", true))
        {
            // Ignore the stored version after a USB flash of a build with a different baked-in version.
            if (prefs.getString("base", "") == HUB_OTA_FIRMWARE_VERSION)
            {
                version = prefs.getString("fw", HUB_OTA_FIRMWARE_VERSION);
            }
            prefs.end();
        }
        return version;
    }

    void saveInstalledFirmware(const char *version)
    {
        Preferences prefs;
        if (version[0] != '\0' && prefs.begin("hubota", false))
        {
            prefs.putString("base", HUB_OTA_FIRMWARE_VERSION);
            prefs.putString("fw", version);
            prefs.end();
        }
    }

    // Copies the string value of "key" from a flat JSON object (no escape handling needed here).
    bool jsonString(const String &json, const char *key, char *out, size_t outSize)
    {
        const String needle = String("\"") + key + "\"";
        const int at = json.indexOf(needle);
        if (at < 0)
        {
            return false;
        }
        const int colon = json.indexOf(':', at + needle.length());
        const int start = colon < 0 ? -1 : json.indexOf('"', colon + 1);
        const int end = start < 0 ? -1 : json.indexOf('"', start + 1);
        if (end < 0 || static_cast<size_t>(end - start) > outSize)
        {
            return false;
        }
        json.substring(start + 1, end).toCharArray(out, outSize);
        return true;
    }

    bool jsonUint(const String &json, const char *key, uint32_t &out)
    {
        const String needle = String("\"") + key + "\"";
        const int at = json.indexOf(needle);
        const int colon = at < 0 ? -1 : json.indexOf(':', at + needle.length());
        if (colon < 0)
        {
            return false;
        }
        out = static_cast<uint32_t>(strtoul(json.c_str() + colon + 1, nullptr, 10));
        return out > 0;
    }

    String endpoint(const char *path)
    {
        return String(HUB_OTA_SERVER_BASE_URL) + path;
    }

    // The server's own "url" may point at a public HTTPS tunnel; files are fetched from the base URL instead.
    void setDownloadUrl(SrvOtaFile &file)
    {
        String path = String("/files/") + file.name;
        path.replace(" ", "%20");
        snprintf(file.url, sizeof(file.url), "%s", endpoint(path.c_str()).c_str());
    }

    // Reads "firmware":{"action":"update"|"rollback","version","name","size","sha256"} from the check-in answer.
    bool parseFirmwareOffer(const String &response, SrvOtaFile &offer, char *action, size_t actionSize)
    {
        const int fw = response.indexOf("\"firmware\"");
        if (fw < 0)
        {
            return false;
        }
        const String block = response.substring(fw);
        memset(&offer, 0, sizeof(offer));
        if (!jsonString(block, "action", action, actionSize) ||
            (strcmp(action, "update") != 0 && strcmp(action, "rollback") != 0))
        {
            return false;
        }
        if (!jsonString(block, "version", offer.version, sizeof(offer.version)) ||
            !jsonString(block, "name", offer.name, sizeof(offer.name)) ||
            !jsonString(block, "sha256", offer.sha256, sizeof(offer.sha256)) ||
            !jsonUint(block, "size", offer.size))
        {
            LOG_E("Hub check-in: incomplete firmware offer");
            return false;
        }
        if (strchr(offer.name, '/') != nullptr || strstr(offer.name, "..") != nullptr)
        {
            LOG_E("Hub check-in: rejected firmware file name");
            return false;
        }
        setDownloadUrl(offer);
        return true;
    }

    bool validNonce(const String &response, String &nonce)
    {
        const int key = response.indexOf("\"nonce\"");
        if (key < 0)
        {
            return false;
        }
        const int colon = response.indexOf(':', key);
        if (colon < 0)
        {
            return false;
        }
        const int start = response.indexOf('"', colon + 1);
        if (start < 0)
        {
            return false;
        }
        const int end = response.indexOf('"', start + 1);
        if (end < 0)
        {
            return false;
        }

        nonce = response.substring(start + 1, end);
        if (nonce.length() != NONCE_SIZE)
        {
            return false;
        }
        for (size_t i = 0; i < NONCE_SIZE; ++i)
        {
            const char c = nonce[i];
            if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')))
            {
                return false;
            }
        }
        return true;
    }

    bool serverAccepted(const String &response)
    {
        const int field = response.indexOf("\"ok\"");
        if (field < 0)
        {
            return false;
        }
        const int colon = response.indexOf(':', field);
        if (colon < 0)
        {
            return false;
        }
        int value = colon + 1;
        while (value < static_cast<int>(response.length()) &&
               (response[value] == ' ' || response[value] == '\t' ||
                response[value] == '\r' || response[value] == '\n'))
        {
            ++value;
        }
        return response.substring(value, value + 4) == "true";
    }

    bool keyIdValid()
    {
        if (strlen(OTA_KEY_ID) != 8)
        {
            return false;
        }
        for (size_t i = 0; i < 8; ++i)
        {
            const char c = OTA_KEY_ID[i];
            if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')))
            {
                return false;
            }
        }
        return true;
    }

    bool keyPairValid()
    {
        uint8_t derivedPublicKey[32];
        Ed25519::derivePublicKey(derivedPublicKey, OTA_PRIV);
        return memcmp(derivedPublicKey, OTA_PUB, sizeof(derivedPublicKey)) == 0;
    }

    // Challenge-response signed POST (see /api/hubs/challenge). `body` is signed together with the nonce.
    bool signedPost(const char *path, char *body, size_t bodyLength, const char *what, int &status, String &response)
    {
        WiFiClientSecure secureClient;
        WiFiClient plainClient;
        secureClient.setInsecure();
        WiFiClient &client = strncmp(HUB_OTA_SERVER_BASE_URL, "https://", 8) == 0 ? static_cast<WiFiClient &>(secureClient) : plainClient;
        HTTPClient http;
        http.setTimeout(HUB_OTA_HTTP_TIMEOUT_MS);

        if (!http.begin(client, endpoint("/api/hubs/challenge")))
        {
            LOG_E("Hub %s: cannot start challenge request", what);
            return false;
        }
        const int challengeStatus = http.GET();
        const String challengeResponse = http.getString();
        http.end();
        if (challengeStatus != HTTP_CODE_OK)
        {
            LOG_W("Hub %s: challenge failed (%d %s) rssi=%d", what, challengeStatus,
                  HTTPClient::errorToString(challengeStatus).c_str(), WiFi.RSSI());
            return false;
        }

        String nonce;
        if (!validNonce(challengeResponse, nonce))
        {
            LOG_E("Hub %s: invalid challenge response", what);
            return false;
        }

        String signedMessage;
        signedMessage.reserve(nonce.length() + bodyLength);
        signedMessage += nonce;
        signedMessage.concat(body, static_cast<unsigned int>(bodyLength));

        uint8_t signature[SIGNATURE_SIZE];
        Ed25519::sign(signature, OTA_PRIV, OTA_PUB, signedMessage.c_str(), signedMessage.length());
        unsigned char encodedSignature[BASE64_SIGNATURE_SIZE + 1];
        size_t encodedLength = 0;
        const int encodeStatus = mbedtls_base64_encode(
            encodedSignature, sizeof(encodedSignature), &encodedLength, signature, sizeof(signature));
        if (encodeStatus != 0 || encodedLength != BASE64_SIGNATURE_SIZE)
        {
            LOG_E("Hub %s: signature encoding failed (%d)", what, encodeStatus);
            return false;
        }
        encodedSignature[encodedLength] = '\0';

        if (!http.begin(client, endpoint(path)))
        {
            LOG_E("Hub %s: cannot start request", what);
            return false;
        }
        http.addHeader("Content-Type", "application/json");
        http.addHeader("X-Key-Id", OTA_KEY_ID);
        http.addHeader("X-Nonce", nonce);
        http.addHeader("X-Signature", reinterpret_cast<const char *>(encodedSignature));
        status = http.POST(reinterpret_cast<uint8_t *>(body), bodyLength);
        response = http.getString();
        http.end();
        return true;
    }

    bool checkIn()
    {
        const String mac = WiFi.macAddress();
        const String fwVersion = installedFirmware();
        char body[192];
        const int bodyLength = snprintf(
            body, sizeof(body),
            "{\"mac\":\"%s\",\"fw\":\"%s\",\"screen\":\"%s\",\"rssi\":%d,\"name\":\"%s\"}",
            mac.c_str(), fwVersion.c_str(), HUB_OTA_SCREEN_VERSION, WiFi.RSSI(), HUB_OTA_NAME);
        if (bodyLength <= 0 || static_cast<size_t>(bodyLength) >= sizeof(body))
        {
            LOG_E("Hub check-in: request body is too long");
            return false;
        }

        int checkinStatus = 0;
        String checkinResponse;
        if (!signedPost("/api/hubs/checkin", body, static_cast<size_t>(bodyLength), "check-in", checkinStatus, checkinResponse))
        {
            return false;
        }
        if (checkinStatus != HTTP_CODE_OK)
        {
            LOG_W("Hub check-in: server returned HTTP %d", checkinStatus);
            return false;
        }
        if (!serverAccepted(checkinResponse))
        {
            LOG_E("Hub check-in: invalid success response");
            return false;
        }

        LOG_I("Hub check-in accepted (fw %s)", fwVersion.c_str());
        char action[12];
        SrvOtaFile offer;
        if (!updateAttempted && parseFirmwareOffer(checkinResponse, offer, action, sizeof(action)))
        {
            if (fwVersion == offer.version)
            {
                LOG_I("Firmware %s already installed, ignoring %s", offer.version, action);
            }
            else
            {
                LOG_I("Server requests firmware %s -> %s (%s)", fwVersion.c_str(), offer.version, action);
                pendingOffer = offer;
                updatePending = true;
            }
        }
        return true;
    }

    void checkinTask(void *)
    {
        uint8_t waitingTicks = 0;
        for (;;)
        {
            uint32_t delayMs = HUB_OTA_CHECKIN_INTERVAL_MS;
            if (xSemaphoreTake(checkinMutex, portMAX_DELAY) == pdTRUE)
            {
                if (!checkinPaused)
                {
                    if (WiFi.status() != WL_CONNECTED)
                    {
                        // WiFi.begin() is asynchronous: poll until it is up, retrying the join every ~20 s
                        delayMs = 2000;
                        if (waitingTicks == 0)
                        {
                            LOG_I("Hub check-in: waiting for WiFi");
                        }
                        if (++waitingTicks >= 10)
                        {
                            waitingTicks = 0;
                            WiFi.begin(HMI_OTA_WIFI_SSID, HMI_OTA_WIFI_PASSWORD);
                        }
                    }
                    else
                    {
                        waitingTicks = 0;
                        checkIn();
                    }
                }
                xSemaphoreGive(checkinMutex);
            }
            if (updatePending)
            {
                // Outside the mutex: the update pauses check-in itself.
                updatePending = false;
                updateAttempted = true;
                if (SRV_Ota_InstallFirmware(pendingOffer, false))
                {
                    LOG_I("Firmware updated, restarting hub");
                    delay(500);
                    ESP.restart();
                }
                LOG_W("Automatic firmware update failed, check-in resumes");
                if (xSemaphoreTake(checkinMutex, portMAX_DELAY) == pdTRUE)
                {
                    checkinPaused = false;
                    xSemaphoreGive(checkinMutex);
                }
            }
            vTaskDelay(pdMS_TO_TICKS(delayMs));
        }
    }
}

void SRV_Ota_Begin(void)
{
    if (checkinTaskHandle != nullptr)
    {
        return;
    }
    if (!keyIdValid() || !keyPairValid())
    {
        LOG_E("Hub OTA check-in disabled: invalid key ID or key pair");
        return;
    }

    checkinMutex = xSemaphoreCreateMutex();
    if (checkinMutex == nullptr)
    {
        LOG_E("Hub OTA check-in disabled: cannot create mutex");
        return;
    }

    WiFi.begin(HMI_OTA_WIFI_SSID, HMI_OTA_WIFI_PASSWORD);
    const BaseType_t created = xTaskCreatePinnedToCore(
        checkinTask, "hub-ota-checkin", HUB_OTA_TASK_STACK_SIZE, nullptr, 1, &checkinTaskHandle, 0);
    if (created != pdPASS)
    {
        checkinTaskHandle = nullptr;
        vSemaphoreDelete(checkinMutex);
        checkinMutex = nullptr;
        LOG_E("Hub OTA check-in disabled: cannot create task");
    }
}

void SRV_Ota_Pause(void)
{
    if (checkinMutex != nullptr)
    {
        xSemaphoreTake(checkinMutex, portMAX_DELAY);
        checkinPaused = true;
        xSemaphoreGive(checkinMutex);
    }
}

bool SRV_Ota_GetLatest(const char *kind, SrvOtaFile &out)
{
    WiFiClientSecure secureClient;
    WiFiClient plainClient;
    secureClient.setInsecure();
    WiFiClient &client = strncmp(HUB_OTA_SERVER_BASE_URL, "https://", 8) == 0 ? static_cast<WiFiClient &>(secureClient) : plainClient;
    HTTPClient http;
    http.setTimeout(HUB_OTA_HTTP_TIMEOUT_MS);
    if (!http.begin(client, endpoint("/api/latest/") + kind))
    {
        return false;
    }
    const int status = http.GET();
    const String json = http.getString();
    http.end();
    if (status != HTTP_CODE_OK)
    {
        LOG_E("Latest %s: HTTP %d", kind, status);
        return false;
    }

    memset(&out, 0, sizeof(out));
    if (!jsonString(json, "name", out.name, sizeof(out.name)) ||
        !jsonString(json, "sha256", out.sha256, sizeof(out.sha256)) ||
        !jsonUint(json, "size", out.size))
    {
        LOG_E("Latest %s: unexpected response", kind);
        return false;
    }
    jsonString(json, "version", out.version, sizeof(out.version));

    setDownloadUrl(out);
    LOG_I("Latest %s: %s (%s, %u bytes)", kind, out.name, out.version, (unsigned)out.size);
    return true;
}

bool SRV_Ota_UpdateHubFirmware(bool dry)
{
    SRV_Ota_Pause();
    const uint32_t wifiStart = millis();
    while (WiFi.status() != WL_CONNECTED)
    {
        if (millis() - wifiStart > 20000)
        {
            LOG_E("Firmware update: WiFi not connected");
            return false;
        }
        delay(100);
    }

    SrvOtaFile latest;
    return SRV_Ota_GetLatest("hub-firmware", latest) && SRV_Ota_InstallFirmware(latest, dry);
}

namespace
{
    // Tells the server how the firmware install is going. state: downloading | done | failed.
    // Best effort: a failed report never aborts the update.
    void reportProgress(const SrvOtaFile &file, const char *state, int percent)
    {
        char body[224];
        const int length = snprintf(
            body, sizeof(body),
            "{\"mac\":\"%s\",\"name\":\"%s\",\"version\":\"%s\",\"state\":\"%s\",\"percent\":%d}",
            WiFi.macAddress().c_str(), HUB_OTA_NAME, file.version, state, percent);
        if (length <= 0 || static_cast<size_t>(length) >= sizeof(body))
        {
            return;
        }
        int status = 0;
        String response;
        if (!signedPost("/api/hubs/progress", body, static_cast<size_t>(length), "progress", status, response) ||
            status != HTTP_CODE_OK)
        {
            LOG_W("Progress report not accepted (HTTP %d)", status);
        }
    }

    bool installImpl(const SrvOtaFile &latest, bool dry);
    bool reportEnabled = false;
}

bool SRV_Ota_InstallFirmware(const SrvOtaFile &latest, bool dry)
{
    reportEnabled = !dry;
    if (reportEnabled)
    {
        reportProgress(latest, "downloading", 0);
    }
    const bool ok = installImpl(latest, dry);
    if (reportEnabled)
    {
        reportProgress(latest, ok ? "done" : "failed", ok ? 100 : -1);
    }
    reportEnabled = false;
    return ok;
}

namespace
{
bool installImpl(const SrvOtaFile &latest, bool dry)
{
    SRV_Ota_Pause();
    const uint32_t wifiStart = millis();
    while (WiFi.status() != WL_CONNECTED)
    {
        if (millis() - wifiStart > 20000)
        {
            LOG_E("Firmware update: WiFi not connected");
            return false;
        }
        delay(100);
    }

    WiFiClientSecure secureClient;
    WiFiClient plainClient;
    secureClient.setInsecure();
    const bool https = strncmp(latest.url, "https://", 8) == 0;
    HTTPClient http;
    http.setTimeout(HUB_OTA_HTTP_TIMEOUT_MS);
    if (!(https ? http.begin(secureClient, latest.url) : http.begin(plainClient, latest.url)))
    {
        LOG_E("Firmware update: bad URL");
        return false;
    }
    const int code = http.GET();
    const int size = http.getSize();
    if (code != HTTP_CODE_OK || size <= 0 || static_cast<uint32_t>(size) != latest.size)
    {
        LOG_E("Firmware update: download failed (HTTP %d, size %d, expected %u)", code, size, (unsigned)latest.size);
        http.end();
        return false;
    }
    if (!dry && !Update.begin(static_cast<size_t>(size)))
    {
        LOG_E("Firmware update: no room for %d bytes (%s)", size, Update.errorString());
        http.end();
        return false;
    }

    mbedtls_sha256_context sha;
    mbedtls_sha256_init(&sha);
    mbedtls_sha256_starts_ret(&sha, 0);
    WiFiClient *stream = http.getStreamPtr();
    uint8_t buffer[1024];
    int done = 0;
    int lastTen = -1;
    uint32_t lastData = millis();
    bool ok = true;
    while (ok && done < size)
    {
        const int available = stream->available();
        if (available <= 0)
        {
            if (millis() - lastData > HUB_OTA_HTTP_TIMEOUT_MS)
            {
                LOG_E("Firmware update: download stalled at %d/%d", done, size);
                ok = false;
            }
            delay(1);
            continue;
        }
        const size_t got = stream->readBytes(buffer, min(static_cast<size_t>(available), min(sizeof(buffer), static_cast<size_t>(size - done))));
        lastData = millis();
        mbedtls_sha256_update_ret(&sha, buffer, got);
        if (!dry && Update.write(buffer, got) != got)
        {
            LOG_E("Firmware update: flash write failed (%s)", Update.errorString());
            ok = false;
        }
        done += static_cast<int>(got);
        const int ten = static_cast<int>(static_cast<int64_t>(done) * 10 / size);
        if (ten != lastTen)
        {
            lastTen = ten;
            LOG_I("Firmware update: %d%%", ten * 10);
            if (reportEnabled && ten > 0 && ten < 10)
            {
                reportProgress(latest, "downloading", ten * 10);
            }
        }
    }
    http.end();

    uint8_t digest[32];
    mbedtls_sha256_finish_ret(&sha, digest);
    mbedtls_sha256_free(&sha);
    char hex[65];
    for (size_t i = 0; i < sizeof(digest); ++i)
    {
        snprintf(hex + 2 * i, 3, "%02x", digest[i]);
    }
    if (ok && strcasecmp(hex, latest.sha256) != 0)
    {
        LOG_E("Firmware update: sha256 mismatch (got %s)", hex);
        ok = false;
    }

    if (!dry)
    {
        // Update.end() is what activates the new image, so it only runs after the sha256 matched.
        if (!ok)
        {
            Update.abort();
        }
        else if (!Update.end(true))
        {
            LOG_E("Firmware update: image rejected (%s)", Update.errorString());
            ok = false;
        }
    }
    if (ok)
    {
        LOG_I("Firmware update: sha256 verified%s", dry ? " (dry run, nothing written)" : ", new image active");
        if (!dry)
        {
            saveInstalledFirmware(latest.version);
        }
    }
    return ok;
}
} // namespace
