#ifndef HMI_H
#define HMI_H

#include <Arduino.h>
#include "Robot.h"

// TJC touchscreen protocol over UART: "NAME,arg,arg" ending in 0xFF 0xFF 0xFF (or a newline).
// Slots (motors M1..M6) are 1-based. Commands are answered with "OK,NAME" or "ERR,NAME",
// except the stream / push commands marked (no reply).
//
//   GET                              CFG, MAP and MIX lines describing the current setup (no reply)
//   CHASSIS,<0-8>                    two-wheel, tank, omni-3, omni-4, mecanum, six-wheel, holonomic, x-drive, custom
//   MOTOR,<1-6>,<role 0-6>,<0|1>     slot -> wheel role (0 = unused), reversed flag
//   MIX,<role 1-6>,<t>,<s>,<r>       custom chassis: percent of each command, -100..100
//   PWM,<0-255>  ACCEL,<0-255>       output limit, max speed change per 20 ms (0 = off)
//   SAVE  DEFAULT                    store to EEPROM, restore factory setup
//   CTRL,<REMOTE|MANUAL>             who drives the robot
//   JOY,<t>,<s>,<r>                  manual drive, -100..100 (no reply, never logged)
//   RUN,<1-6>,<-255..255>  BRAKE,<1-6>  STOP     single-motor test
//   SERVO,<0-15>,<0-180>
//   IO,<pin>,<IN|OUT|PULLUP>  IO,<pin>,W,<0|1>   OK / ERR
//   IO,<pin>,R   AIN,<0-15>          reply with the value instead of OK (no OK/ERR)
//   SYNC | SYNC,HOME | SYNC,I2C | SYNC,CHASSIS,<0-8>   push UI state to the screen (no reply)
//   MON  MON,ALL  ADC                push monitor / analog test pages (no reply).
//                                    MON only sends fields that changed; MON,ALL resends everything
//                                    (send it once when the Monitor page opens).
// A line longer than the buffer is dropped and answered with "ERR,LINE".
class Hmi
{
public:
    Hmi(Stream &serial, Robot &robot) : _serial(serial), _robot(robot) {}

    void update();

private:
    // OK / ERR are answered by handle(); NONE means the handler already answered (or must stay silent).
    enum class Result : uint8_t
    {
        OK,
        ERR,
        NONE
    };

    using CmdHandler = Result (Hmi::*)(char **f, uint8_t n);

    // One row of the command lookup table. Lives in flash (PROGMEM): the name is stored inline
    // so it costs no RAM, and the field-count check is done once in handle() instead of in
    // every handler.
    struct CommandEntry
    {
        char name[8]; // longest command ("CHASSIS", "DEFAULT") is 7 chars + '\0'
        CmdHandler handler;
        uint8_t minFields; // counts the command word itself
        uint8_t maxFields;
        bool quiet; // stream command: not logged and never answered with OK/ERR
    };

    static const CommandEntry CMD_TABLE[];
    static const uint8_t CMD_TABLE_SIZE;

    static constexpr uint8_t LINE_MAX = 48;
    static constexpr uint8_t MON_FIELDS = 24; // number of monitor fields cached for change detection

    static Result toResult(bool ok) { return ok ? Result::OK : Result::ERR; }

    // Line handling
    void finishLine();
    void handle(char *line);
    void reply(const char *command, bool ok);
    void sendConfig();

    // Command handlers (field count already validated against CMD_TABLE)
    Result handleGet(char **f, uint8_t n);
    Result handleSync(char **f, uint8_t n);
    Result handleMon(char **f, uint8_t n);
    Result handleAdc(char **f, uint8_t n);
    Result handleChassis(char **f, uint8_t n);
    Result handleMotor(char **f, uint8_t n);
    Result handleMix(char **f, uint8_t n);
    Result handlePwm(char **f, uint8_t n);
    Result handleAccel(char **f, uint8_t n);
    Result handleSave(char **f, uint8_t n);
    Result handleDefault(char **f, uint8_t n);
    Result handleCtrl(char **f, uint8_t n);
    Result handleJoy(char **f, uint8_t n);
    Result handleRun(char **f, uint8_t n);
    Result handleBrake(char **f, uint8_t n);
    Result handleStop(char **f, uint8_t n);
    Result handleServo(char **f, uint8_t n);
    Result handleIo(char **f, uint8_t n);
    Result handleAin(char **f, uint8_t n);

    // Screen sync
    void syncScreen();
    void syncChassisLabels(Chassis chassis);
    void computeWheelMotor(long out[6]);
    void syncMonitor();
    void monField(const char *comp, const char *text, int32_t color = -1);
    void syncHome();
    void syncI2C();
    void syncADC();

    Stream &_serial;
    Robot &_robot;
    char _line[LINE_MAX];
    uint8_t _length = 0;
    uint8_t _terminators = 0;
    bool _overflow = false; // current line exceeded _line: drop it at the terminator
    uint32_t _lastHeartbeat = 0;

    uint16_t _monHash[MON_FIELDS] = {}; // hash of the last text/colour sent for each monitor field
    uint8_t _monSlot = 0;               // field index while syncMonitor() runs
};

#endif