#ifndef ESPCOM_H
#define ESPCOM_H

#include <Arduino.h>

class EspCom
{
public:
    EspCom();
    bool begin();
    void update();

private:
    static void OnControlPacketReceived(const uint8_t *macAddress, const uint8_t *incomingData, int length);

protected:
    static EspCom *instance;
};

#endif // ESPCOM_H