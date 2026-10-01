/**
 * @file Swc_Button.cpp
 * @brief Button debounce and bitmask input implementation.
 */

#include "Swc_Button.h"
#include "Remote_Pin_Cfg.h"

Swc_Button::Swc_Button() : _debouncedState(0), _lastRawState(0), _lastDebounceTime(0) {}

void Swc_Button::Init() {
    // Cấu hình Pin 9 làm Input có trở kéo lên nội (INPUT_PULLUP)
    // Khi thả nút = HIGH (1), Khi bấm nút = LOW (0)
    pinMode(REMOTE_PIN_BTN_1, INPUT_PULLUP);
}

uint16 Swc_Button::ReadRawPorts() {
    uint16 btnMask = 0;
    
    // Đọc trạng thái Pin 9 (Đảo trạng thái logic: Bấm = 1, Thả = 0)
    if (digitalRead(REMOTE_PIN_BTN_1) == LOW) {
        btnMask |= (1 << 0); // Bit 0 đại diện cho Button 1
    }
    
    return btnMask;
}

void Swc_Button::Update() {
    uint16 rawRead = ReadRawPorts();
    
    if (rawRead != _lastRawState) {
        _lastDebounceTime = millis();
    }
    
    if ((millis() - _lastDebounceTime) > DEBOUNCE_DELAY_MS) {
        _debouncedState = rawRead;
    }
    
    _lastRawState = rawRead;
}