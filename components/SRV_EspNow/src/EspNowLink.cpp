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
    static uint32_t lastRx = 0, maxGap = 0, count = 0, lastPrint = 0;
    if (_queue == nullptr)
    {
        return;
    }

    ControlPacket packet;
    // At most one queue's worth per call, so a flood cannot starve the rest of loop().
    for (uint8_t n = 0; n < ESPNOW_QUEUE_DEPTH && xQueueReceive(_queue, &packet, 0) == pdTRUE; ++n)
    {
        uint32_t now = micros();
        if (lastRx)
        {
            uint32_t gap = now - lastRx;
            if (gap > maxGap)
                maxGap = gap;
        }
        lastRx = now;
        ++count;
        if (packet.msgType == CONTROL_MESSAGE)
        {
            // Log only when the command changes, at most every 200 ms; per-packet logs slow loop().
            static ControlPacket lastLogged = {};
            static uint32_t lastLogMs = 0;
            const bool changed = packet.throttle != lastLogged.throttle || packet.strafe != lastLogged.strafe ||
                                 packet.rotation != lastLogged.rotation || packet.buttons != lastLogged.buttons;
            if (changed && millis() - lastLogMs >= 200)
            {
                lastLogMs = millis();
                lastLogged = packet;
                LOG_D("Control: throttle=%d strafe=%d rotation=%d buttons=0x%02X seq=%u",
                      packet.throttle, packet.strafe, packet.rotation, packet.buttons, packet.sequenceNum);
            }
        }
        if (_handler != nullptr)
        {
            _handler(packet, _context);
        }
    }
    if (millis() - lastPrint >= 1000)
    {
        lastPrint = millis();
        // Quiet while healthy: INFO only when the link is silent or the reject/drop counters grow.
        static uint32_t lastRejected = 0, lastDropped = 0;
        const bool anomaly = count == 0 || _rejected != lastRejected || _dropped != lastDropped;
        if (anomaly)
        {
            LOG_I("rx=%u/s maxGap=%u us rejected=%u dropped=%u",
                  (unsigned)count, (unsigned)maxGap, (unsigned)_rejected, (unsigned)_dropped);
        }
        else
        {
            LOG_D("rx=%u/s maxGap=%u us", (unsigned)count, (unsigned)maxGap);
        }
        const bool rejectedGrew = _rejected != lastRejected;
        lastRejected = _rejected;
        lastDropped = _dropped;
        if (rejectedGrew)
        {
            LOG_W("rejected by: mac=%u length=%u packet(crc/type)=%u | last sender %02X:%02X:%02X:%02X:%02X:%02X len=%d",
                  (unsigned)_badMac, (unsigned)_badLength, (unsigned)_badPacket,
                  _lastBadMac[0], _lastBadMac[1], _lastBadMac[2], _lastBadMac[3], _lastBadMac[4],
                  _lastBadMac[5], _lastBadLength);
        }
        count = 0;
        maxGap = 0;
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
        ++self->_badMac;
        self->_lastBadLength = length;
        for (uint8_t i = 0; i < 6 && senderMac != nullptr; ++i)
        {
            self->_lastBadMac[i] = senderMac[i];
        }
        return;
    }

    if (length != static_cast<int>(sizeof(ControlPacket)))
    {
        ++self->_rejected;
        ++self->_badLength;
        self->_lastBadLength = length;
        return;
    }

    ControlPacket packet;
    memcpy(&packet, data, sizeof(packet));
    if (!IsControlPacketValid(packet) ||
        (packet.msgType != CONTROL_MESSAGE && packet.msgType != HEARTBEAT_MESSAGE))
    {
        ++self->_rejected;
        ++self->_badPacket;
        return;
    }

    if (xQueueSend(self->_queue, &packet, 0) != pdTRUE)
    {
        ++self->_dropped;
    }
}
