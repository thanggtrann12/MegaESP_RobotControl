#ifndef ESPNOWLINK_H
#define ESPNOWLINK_H

#include <Arduino.h>
#include "ControlPacket.h"

#if defined(ARDUINO_ARCH_ESP32)
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#endif

class EspNowLink
{
public:
    using Handler = void (*)(const ControlPacket &packet, void *context);

    EspNowLink();
    ~EspNowLink();

    bool begin(Handler handler, void *context);
    void update();

    uint32_t rejectedCount() const { return _rejected; }
    uint32_t droppedCount() const { return _dropped; }

    static void receive(const uint8_t *senderMac, const uint8_t *data, int length);

private:
    static EspNowLink *instance;

    Handler _handler = nullptr;
    void *_context = nullptr;
    QueueHandle_t _queue = nullptr;
    volatile uint32_t _rejected = 0;
    volatile uint32_t _dropped = 0;

    bool _initialized = false;
    bool _callbackRegistered = false;
};

#endif // ESPNOWLINK_H