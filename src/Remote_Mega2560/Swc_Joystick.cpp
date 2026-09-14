#include "Swc_Joystick.h"

Swc_Joystick::Swc_Joystick(uint8 j1x, uint8 j1y, uint8 j2x)
    : _pinJ1X(j1x), _pinJ1Y(j1y), _pinJ2X(j2x) {
    for (int i = 0; i < 3; i++) _filteredVal[i] = 512.0f;
}

void Swc_Joystick::Init() {
    pinMode(_pinJ1X, INPUT);
    pinMode(_pinJ1Y, INPUT);
    pinMode(_pinJ2X, INPUT);
}

int16 Swc_Joystick::ApplyDeadzone(int16 rawVal) {
    int16 diff = rawVal - CENTER_VAL;
    if (abs(diff) < DEADZONE) return CENTER_VAL;
    return rawVal;
}

void Swc_Joystick::Update() {
    uint16 raw[3] = {
        (uint16)analogRead(_pinJ1X),
        (uint16)analogRead(_pinJ1Y),
        (uint16)analogRead(_pinJ2X)
    };

    for (int i = 0; i < 3; i++) {
        _filteredVal[i] = (ALPHA * raw[i]) + ((1.0f - ALPHA) * _filteredVal[i]);
    }
}

void Swc_Joystick::GetProcessedValues(int16& j1x, int16& j1y, int16& j2x) {
    j1x = ApplyDeadzone((int16)_filteredVal[0]);
    j1y = ApplyDeadzone((int16)_filteredVal[1]);
    j2x = ApplyDeadzone((int16)_filteredVal[2]);
}