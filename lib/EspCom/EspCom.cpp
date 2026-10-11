#include "EspCom.h"
#include <GenericLogger.h>
#include <esp_now.h>
#include "Std_Types.h"
ASSIGN_LOG_MACROS(EspCom);

EspCom *EspCom::instance = nullptr;
EspCom::EspCom()
{
    LOG_I("This module used to handle ESP communication between controler and this hub");
    instance = this;
}

bool EspCom::begin()
{
    LOG_I("Initializing ESP-NOW");
    if (esp_now_init() != ESP_OK)
    {
        LOG_E("Failed to initialize ESP-NOW");
        return false;
    }

    LOG_I("Registering ESP-NOW receive callback");
    if (esp_now_register_recv_cb(EspCom::OnControlPacketReceived) != ESP_OK)
    {
        LOG_E("Failed to register ESP-NOW receive callback");
        return false;
    }
    LOG_I("ESP-NOW initialized successfully");
    return true;
}

void EspCom::update()
{
    // Currently, there is no periodic update logic for ESP-NOW in this module.
}

void EspCom::OnControlPacketReceived(const uint8_t *macAddress, const uint8_t *incomingData, int length)
{
    if (length != sizeof(ControlPacket))
    {
        // RobotESP32_LogE("Invalid ESP-NOW length: %u", length);
        return;
    }

    ControlPacket packet;
    memcpy(&packet, incomingData, sizeof(packet));
    if (!IsControlPacketValid(packet) ||
        (packet.msgType != CONTROL_MESSAGE && packet.msgType != HEARTBEAT_MESSAGE))
    {
        // RobotESP32_LogW("Discarded invalid control packet");
        return;
    }

    // Heartbeats arrive continuously; only log real control commands that are not idle.
    if (packet.msgType == CONTROL_MESSAGE &&
        (packet.throttle != 0 || packet.strafe != 0 || packet.rotation != 0 || packet.buttons != 0))
    {
        LOG_I("Control: throttle=%d strafe=%d rotation=%d buttons=0x%02X seq=%u",
              packet.throttle,
              packet.strafe,
              packet.rotation,
              packet.buttons,
              packet.sequenceNum);
    }
}