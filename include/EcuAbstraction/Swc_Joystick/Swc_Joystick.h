#ifndef SWC_JOYSTICK_H
#define SWC_JOYSTICK_H

#include "Std_Types.h"

/**
 * @file Swc_Joystick.h
 * @brief Joystick abstraction with low-pass filtering and deadzone handling.
 */

/**
 * @class Swc_Joystick
 * @brief Reads joystick analog channels and provides processed axis values.
 */
class Swc_Joystick
{
private:
    uint8 _pinJ1X, _pinJ1Y, _pinJ2X;
    float _filteredVal[3];
    static constexpr float ALPHA = 0.6f;
    static const int16 CENTER_VAL = 512;
    static const int16 DEADZONE = 50;

    /**
     * @brief Applies center deadzone to an analog value.
     * @param rawVal Raw analog value.
     * @return Value snapped to center when inside deadzone.
     */
    int16 ApplyDeadzone(int16 rawVal);

public:
    /**
     * @brief Constructs joystick abstraction with optional custom analog pins.
     * @param j1x Analog pin for joystick 1 X axis.
     * @param j1y Analog pin for joystick 1 Y axis.
     * @param j2x Analog pin for joystick 2 X axis.
     */
    Swc_Joystick(uint8 j1x = A8, uint8 j1y = A9, uint8 j2x = A10);
    /** @brief Initializes configured analog pins as inputs. */
    void Init();
    /** @brief Reads channels and updates filtered axis values. */
    void Update();
    /**
     * @brief Returns processed joystick axis values.
     * @param j1x Output joystick 1 X value.
     * @param j1y Output joystick 1 Y value.
     * @param j2x Output joystick 2 X value.
     */
    void GetProcessedValues(int16 &j1x, int16 &j1y, int16 &j2x);
};

#endif