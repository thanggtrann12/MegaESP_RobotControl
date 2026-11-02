#ifndef SRV_OTA_CFG_H
#define SRV_OTA_CFG_H

#define HUB_OTA_SERVER_BASE_URL "http://192.168.42.235:8000" // http:// or https://
#define HUB_OTA_NAME "hub-lab"
// Version baked into this build; override with build_flags = -DHUB_OTA_FIRMWARE_VERSION=\"1.2.0\".
// A version installed later over OTA is stored in NVS and reported instead, until a USB flash changes this value.
#ifndef HUB_OTA_FIRMWARE_VERSION
#define HUB_OTA_FIRMWARE_VERSION "1.0.7"
#endif
#define HUB_OTA_SCREEN_VERSION "1.0.0"

constexpr uint32_t HUB_OTA_CHECKIN_INTERVAL_MS = 30000;
constexpr uint32_t HUB_OTA_HTTP_TIMEOUT_MS = 10000;
constexpr uint32_t HUB_OTA_TASK_STACK_SIZE = 12288;

#endif // SRV_OTA_CFG_H6