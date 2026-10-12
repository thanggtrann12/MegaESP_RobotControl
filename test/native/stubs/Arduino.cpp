#include "Arduino.h"
#include "Wire.h"
#include "EEPROM.h"
static unsigned long g_ms = 0;
unsigned long millis() { return g_ms; }
void delay(unsigned long ms) { g_ms += ms; }
void pinMode(uint8_t, uint8_t) {}
int digitalRead(uint8_t) { return 0; }
int analogRead(uint8_t) { return 0; }
void test_set_millis(unsigned long v) { g_ms = v; }
void test_advance_millis(unsigned long v) { g_ms += v; }
HardwareSerial Serial, Serial2, Serial3;
TwoWire Wire;
EEPROMClass EEPROM;
