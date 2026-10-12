#pragma once
#include "Arduino.h"
class TwoWire
{
public:
    void begin() { began = true; }
    void setClock(uint32_t) {}
    void beginTransmission(uint8_t a) { addr = a; cur.clear(); }
    size_t write(uint8_t b) { cur.push_back(b); return 1; }
    uint8_t endTransmission() { log.push_back(cur); return 0; }
    bool began = false;
    uint8_t addr = 0;
    std::vector<uint8_t> cur;
    std::vector<std::vector<uint8_t>> log;
};
extern TwoWire Wire;
