#include "EspNowLink.h"
#include <GenericLogger.h>
#include <esp_idf_version.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "EspNow_Cfg.h"
ASSIGN_LOG_MACROS(EspNowLink);

EspNowLink *EspNowLink::instance = nullptr;


namespace
{
    bool isExpectedSender(const uint8_t *senderMac)
    {
        return senderMac != nullptr &&
               memcmp(senderMac, ESPNOW_REMOTE_MAC, sizeof(ESPNOW_REMOTE_MAC)) == 0;
    }

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    void onReceive(const esp_now_recv_info_t *info, const uint8_t *data, int length)
    {
        EspNowLink::receive(info != nullptr ? info->src_addr : nullptr, data, length);
    }
#else
    void onReceive(const uint8_t *senderMac, const uint8_t *data, int length)
    {
        EspNowLink::receive(senderMac, data, length);
    }
#endif
}

EspNowLink::EspNowLink()
{
    instance = this;
}

EspNowLink::~EspNowLink()
{
    esp_now_unregister_recv_cb();

    if (instance == this)
    {
        instance = nullptr;
    }

    if (_queue != nullptr)
    {
        vQueueDelete(_queue);
        _queue = nullptr;
    }
}

bool EspNowLink::begin(Handler handler, void *context)
{
    _handler = handler;
    _context = context;

    if (_queue == nullptr)
    {
        _queue = xQueueCreate(ESPNOW_QUEUE_DEPTH, sizeof(ControlPacket));
        if (_queue == nullptr)
        {
            LOG_E("Cannot allocate the receive queue");
            return false;
        }
    }

    if (esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE) != ESP_OK)
    {
        LOG_W("Cannot set WiFi channel %u (is WiFi started?)", ESPNOW_CHANNEL);
    }

    LOG_I("Initializing ESP-NOW on channel %u", ESPNOW_CHANNEL);
    if (esp_now_init() != ESP_OK)
    {
        LOG_E("Failed to initialize ESP-NOW");
        return false;
    }

    if (esp_now_register_recv_cb(onReceive) != ESP_OK)
    {
        LOG_E("Failed to register ESP-NOW receive callback");
        esp_now_deinit();
        return false;
    }
    LOG_I("ESP-NOW initialized successfully");
    return true;
}

void EspNowLink::update()
{
    if (_queue == nullptr)
    {
        return;
    }

    ControlPacket packet;
    // At most one queue's worth per call, so a flood cannot starve the rest of loop().
    for (uint8_t n = 0; n < ESPNOW_QUEUE_DEPTH && xQueueReceive(_queue, &packet, 0) == pdTRUE; ++n)
    {
        if (packet.msgType == CONTROL_MESSAGE &&
            (packet.throttle != 0 || packet.strafe != 0 || packet.rotation != 0 || packet.buttons != 0))
        {
            LOG_D("Control: throttle=%d strafe=%d rotation=%d buttons=0x%02X seq=%u",
                  packet.throttle, packet.strafe, packet.rotation, packet.buttons, packet.sequenceNum);
        }
        if (_handler != nullptr)
        {
            _handler(packet, _context);
        }
    }
}

// Runs in the WiFi task: validate, enqueue, return. No logging and no Robot access here.
void EspNowLink::receive(const uint8_t *senderMac, const uint8_t *data, int length)
{
    EspNowLink *self = instance;
    if (self == nullptr || self->_queue == nullptr)
    {
        return;
    }

    if (!isExpectedSender(senderMac))
    {
        ++self->_rejected;
        return;
    }

    if (length != static_cast<int>(sizeof(ControlPacket)))
    {
        ++self->_rejected;
        return;
    }

    ControlPacket packet;
    memcpy(&packet, data, sizeof(packet));
    if (!IsControlPacketValid(packet) ||
        (packet.msgType != CONTROL_MESSAGE && packet.msgType != HEARTBEAT_MESSAGE))
    {
        ++self->_rejected;
        return;
    }

    if (xQueueSend(self->_queue, &packet, 0) != pdTRUE)
    {
        ++self->_dropped;
    }
}
