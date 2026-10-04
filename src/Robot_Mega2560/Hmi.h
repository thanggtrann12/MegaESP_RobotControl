#ifndef HMI_H
#define HMI_H

#include <Arduino.h>
#include "Robot.h"

// TJC touchscreen protocol over UART: "NAME,arg,arg" ending in 0xFF 0xFF 0xFF (or a newline).
// Slots (motors M1..M6) are 1-based. Every command is answered with "OK,NAME" or "ERR,NAME".
//
//   GET                              CFG, MAP and MIX lines describing the current setup
//   CHASSIS,<0-5>                    two-wheel, tank, omni-4, mecanum, six-wheel, custom
//   MOTOR,<1-6>,<role 0-6>,<0|1>     slot -> wheel role (0 = unused), reversed flag
//   MIX,<role 1-6>,<t>,<s>,<r>       custom chassis: percent of each command, -100..100
//   PWM,<0-255>  ACCEL,<0-255>       output limit, max speed change per 20 ms (0 = off)
//   SAVE  DEFAULT                    store to EEPROM, restore factory setup
//   CTRL,<REMOTE|MANUAL>             who drives the robot
//   JOY,<t>,<s>,<r>                  manual drive, -100..100 (no reply on success)
//   RUN,<1-6>,<-255..255>  BRAKE,<1-6>  STOP     single-motor test
//   SERVO,<0-15>,<0-180>
//   IO,<pin>,<IN|OUT|PULLUP>  IO,<pin>,W,<0|1>  IO,<pin>,R  AIN,<0-15>
class Hmi
{
public:
    Hmi(Stream &serial, Robot &robot) : _serial(serial), _robot(robot) {}

    void update();

private:
    void finishLine();
    void handle(char *line);
    void reply(const char *command, bool ok);
    void sendConfig();
    void endMessage();
    void syncScreen();
    void sendComponentVal(const char *component, long value);
    void sendComponentTxt(const char *component, const char *text);
    void sendGlobal(const char *name, long value);

    void syncChassisLabels(Chassis chassis);
    void computeWheelMotor(long out[6]);

    void syncMonitor();
    void monTxt(const char *comp, const char *text);
    void monCol(const char *comp, uint16_t color);

    void syncHome();
    void syncI2C();
    Stream &_serial;
    Robot &_robot;
    char _line[48];
    uint8_t _length = 0;
    uint8_t _terminators = 0;
    uint32_t _lastHeartbeat = 0;
};

#endif
