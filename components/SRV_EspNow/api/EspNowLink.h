#ifndef ESPNOWLINK_H
#define ESPNOWLINK_H

#include <Arduino.h>
#include "ControlPacket.h"

#if defined(ARDUINO_ARCH_ESP32)
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#endif

// Receives ControlPackets from the remote over ESP-NOW.
//
// The ESP-NOW receive callback runs in the WiFi task, not in loop(). The callback only validates the
// frame and pushes it on a queue; update() (called from loop()) pops packets and hands them to the
// handler, so the handler runs in the same context as the rest of the firmware and needs no locking.
class EspNowLink
{
public:
    using Handler = void (*)(const ControlPacket &packet, void *context);

    EspNowLink();
    ~EspNowLink();

    // handler is called from update() once per valid CONTROL / HEARTBEAT packet, oldest first.
    bool begin(Handler handler, void *context);
    void update();

    uint32_t rejectedCount() const { return _rejected; } // wrong length, bad CRC or unknown type
    uint32_t droppedCount() const { return _dropped; }   // queue was full

    // Called by the ESP-NOW callback (WiFi task); not for application code.
    static void receive(const uint8_t *data, int length);

private:
    static EspNowLink *instance;

    Handler _handler = nullptr;
    void *_context = nullptr;
    QueueHandle_t _queue = nullptr;
    volatile uint32_t _rejected = 0;
    volatile uint32_t _dropped = 0;
};

#endif // ESPNOWLINK_H
