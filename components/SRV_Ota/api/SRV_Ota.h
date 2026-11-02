#ifndef SRV_OTA_H
#define SRV_OTA_H

#include <Arduino.h>

void SRV_Ota_Begin(void);
void SRV_Ota_Pause(void);

struct SrvOtaFile
{
    char version[24];
    char name[64];
    char url[192];   // download URL built from the configured server base URL
    char sha256[65]; // lowercase hex
    uint32_t size;
};

// Asks the server for the newest file of a kind ("screen" or "hub-firmware"). Needs WiFi.
bool SRV_Ota_GetLatest(const char *kind, SrvOtaFile &out);

// Downloads the newest "hub-firmware" file, checks its sha256 and writes it to the next OTA
// partition. dry = download and verify only. The caller restarts the hub on success.
bool SRV_Ota_UpdateHubFirmware(bool dry);

// Same, for a specific file (e.g. the "firmware" offer in a check-in answer, update or rollback).
bool SRV_Ota_InstallFirmware(const SrvOtaFile &file, bool dry);

#endif // SRV_OTA_H
