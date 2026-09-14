#ifndef SWC_BUTTON_H
#define SWC_BUTTON_H

#include "Std_Types.h"

class Swc_Button {
private:
    uint16 _debouncedState;
    uint16 _lastRawState;
    uint32 _lastDebounceTime;
    static const uint32 DEBOUNCE_DELAY_MS = 20;

    uint16 ReadRawPorts();

public:
    Swc_Button();
    void Init();
    void Update();
    inline uint16 GetState() const { return _debouncedState; }
};

#endif