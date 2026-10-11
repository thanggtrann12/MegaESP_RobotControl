#ifndef SWC_BUTTON_H
#define SWC_BUTTON_H

#include "Std_Types.h"

/**
 * @file Swc_Button.h
 * @brief Debounced button input abstraction for the remote controller.
 */

/**
 * @class Swc_Button
 * @brief Reads and debounces button ports and exposes packed state bits.
 */
class Swc_Button {
private:
    uint16 _debouncedState;
    uint16 _lastRawState;
    uint32 _lastDebounceTime;
    static const uint32 DEBOUNCE_DELAY_MS = 20;

    /**
     * @brief Reads raw GPIO inputs and packs them into bitmask format.
     * @return Raw button state mask.
     */
    uint16 ReadRawPorts();

public:
    /** @brief Constructs button abstraction with reset state. */
    Swc_Button();
    /** @brief Initializes GPIO mode for configured button pins. */
    void Init();
    /** @brief Executes one debounce update cycle. */
    void Update();
    /**
     * @brief Gets the current debounced button bitmask.
     * @return Debounced button state.
     */
    inline uint16 GetState() const { return _debouncedState; }
};

#endif