// Host test for ControlPacket CRC and the EspNowLink receive queue.
#include <stdio.h>
#include "EspNowLink.h"
#include "EspNow_Cfg.h"

static int failures = 0;
#define CHECK(cond)                                                          \
    do                                                                       \
    {                                                                        \
        if (!(cond))                                                         \
        {                                                                    \
            printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);           \
            ++failures;                                                      \
        }                                                                    \
    } while (0)

static ControlPacket make(uint8_t type, int8_t t, int8_t s, int8_t r, uint16_t seq)
{
    ControlPacket p = {};
    p.msgType = type;
    p.throttle = t;
    p.strafe = s;
    p.rotation = r;
    p.sequenceNum = seq;
    UpdateControlPacketCrc(p);
    return p;
}

static void feed(const ControlPacket &p) { EspNowLink::receive(reinterpret_cast<const uint8_t *>(&p), sizeof(p)); }

struct Sink
{
    ControlPacket got[32];
    int count = 0;
};
static void onPacket(const ControlPacket &p, void *ctx)
{
    Sink *s = static_cast<Sink *>(ctx);
    if (s->count < 32)
    {
        s->got[s->count++] = p;
    }
}

static void crc()
{
    ControlPacket p = make(CONTROL_MESSAGE, 10, -20, 30, 7);
    CHECK(IsControlPacketValid(p));
    for (size_t i = 0; i + 1 < sizeof(p); ++i) // flip each payload byte: CRC must notice
    {
        ControlPacket q = p;
        reinterpret_cast<uint8_t *>(&q)[i] ^= 0x01;
        CHECK(!IsControlPacketValid(q));
    }
}

static void delivery()
{
    EspNowLink link;
    Sink sink;
    CHECK(link.begin(onPacket, &sink));

    feed(make(CONTROL_MESSAGE, 50, 0, -10, 1));
    feed(make(HEARTBEAT_MESSAGE, 0, 0, 0, 2));
    CHECK(sink.count == 0); // nothing is delivered from the receive callback itself
    link.update();
    CHECK(sink.count == 2);
    CHECK(sink.got[0].throttle == 50 && sink.got[0].rotation == -10 && sink.got[0].sequenceNum == 1);
    CHECK(sink.got[1].msgType == HEARTBEAT_MESSAGE && sink.got[1].sequenceNum == 2);
    CHECK(link.rejectedCount() == 0 && link.droppedCount() == 0);
}

static void rejectsBadFrames()
{
    EspNowLink link;
    Sink sink;
    link.begin(onPacket, &sink);

    ControlPacket good = make(CONTROL_MESSAGE, 1, 2, 3, 1);
    EspNowLink::receive(reinterpret_cast<const uint8_t *>(&good), sizeof(good) - 1); // wrong length
    ControlPacket bad = good;
    bad.throttle = 99; // payload changed after the CRC was computed
    feed(bad);
    feed(make(0x7F, 1, 2, 3, 2)); // valid CRC but unknown message type
    link.update();
    CHECK(sink.count == 0);
    CHECK(link.rejectedCount() == 3);
}

static void overflowKeepsOrderAndCountsDrops()
{
    EspNowLink link;
    Sink sink;
    link.begin(onPacket, &sink);

    for (uint16_t i = 1; i <= 12; ++i)
    {
        feed(make(CONTROL_MESSAGE, 1, 0, 0, i));
    }
    CHECK(link.droppedCount() == 12 - ESPNOW_QUEUE_DEPTH);
    link.update();
    CHECK(sink.count == ESPNOW_QUEUE_DEPTH);
    for (int i = 0; i < sink.count; ++i)
    {
        CHECK(sink.got[i].sequenceNum == i + 1); // oldest first
    }
}

int main()
{
    crc();
    delivery();
    rejectsBadFrames();
    overflowKeepsOrderAndCountsDrops();
    printf(failures == 0 ? "test_espnow: all passed\n" : "test_espnow: %d FAILED\n", failures);
    return failures == 0 ? 0 : 1;
}
