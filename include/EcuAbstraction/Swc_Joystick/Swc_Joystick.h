#ifndef SWC_JOYSTICK_H
#define SWC_JOYSTICK_H

#include "Std_Types.h"

class Swc_Joystick {
private:
    uint8 _pinJ1X, _pinJ1Y, _pinJ2X;
    float _filteredVal[3];
    static constexpr float ALPHA = 0.6f;
    static const int16 CENTER_VAL = 512;
    static const int16 DEADZONE = 30;

    int16 ApplyDeadzone(int16 rawVal);

public:
    Swc_Joystick(uint8 j1x = A8, uint8 j1y = A9, uint8 j2x = A10);
    void Init();
    void Update();
    void GetProcessedValues(int16& j1x, int16& j1y, int16& j2x);
};

#endif