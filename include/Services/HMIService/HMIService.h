#ifndef HMI_SERVICE_H
#define HMI_SERVICE_H

#include <Arduino.h>
#include "GenericRobotController.h"
#include "IOPinManager.h"
#include "KinematicsRegistry.h"

/**
 * @file HMIService.h
 * @brief HMI command parser and command dispatcher for runtime control.
 */

/**
 * @class HMIService
 * @brief Processes serial HMI commands and applies them to robot services.
 */
class HMIService
{
public:
    /**
     * @brief Constructs an HMI service bound to robot and IO components.
     * @param serial Stream used for HMI communication.
     * @param robot Robot controller target.
     * @param ioPins IO pin manager target.
     * @param kinematicsRegistry Registry used to switch kinematics mode.
     */
    HMIService(Stream &serial, GenericRobotController &robot, IOPinManager &ioPins, KinematicsRegistry &kinematicsRegistry);
    /** @brief Polls stream, parses commands, and sends heartbeat responses. */
    void update();

private:
    /** @brief Parses and executes one complete buffered command. */
    void processCommand();
    /**
     * @brief Sends one protocol message to the HMI stream.
     * @param message Null-terminated response text.
     */
    void sendMessage(const char *message);
    /**
     * @brief Sends formatted IO status response for one pin.
     * @param pin Pin identifier to report.
     */
    void sendIoStatus(uint8_t pin);
    /**
     * @brief Parses decimal integer text.
     * @param text Input C-string.
     * @param value Parsed output value.
     * @return true on success.
     * @return false on parse failure.
     */
    bool parseInteger(const char *text, long &value) const;
    /**
     * @brief Parses and validates a pin number.
     * @param text Input C-string.
     * @param pin Parsed pin number.
     * @return true when valid.
     * @return false otherwise.
     */
    bool parsePin(const char *text, uint8_t &pin) const;
    /**
     * @brief Parses HMI motion tuple values.
     * @param text Input C-string containing motion values.
     * @param throttle Parsed throttle.
     * @param strafe Parsed strafe.
     * @param rotation Parsed rotation.
     * @return true when command format and ranges are valid.
     * @return false otherwise.
     */
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