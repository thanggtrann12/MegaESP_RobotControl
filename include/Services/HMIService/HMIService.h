#ifndef HMI_SERVICE_H
#define HMI_SERVICE_H

#include <Arduino.h>
#include "GenericRobotController.h"
#include "IOPinManager.h"
#include "KinematicsRegistry.h"

class HMIService
{
public:
    HMIService(Stream &serial, GenericRobotController &robot, IOPinManager &ioPins, KinematicsRegistry &kinematicsRegistry);
    void update();

private:
    void processCommand();
    void sendMessage(const char *message);
    void sendIoStatus(uint8_t pin);
    bool parseInteger(const char *text, long &value) const;
    bool parsePin(const char *text, uint8_t &pin) const;
    bool parseMotion(const char *text, int8_t &throttle, int8_t &strafe, int8_t &rotation) const;

    Stream &_serial;
    GenericRobotController &_robot;
    IOPinManager &_ioPins;
    KinematicsRegistry &_kinematicsRegistry;
    char _buffer[96];
    size_t _length;
    uint8_t _terminatorCount;
    uint32_t _lastHeartbeat;
};

#endif