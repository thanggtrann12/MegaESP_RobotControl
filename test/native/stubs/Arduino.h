#pragma once
// Minimal host-side Arduino stub for native logic tests.
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <deque>
#include <string>
#include <algorithm>

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))
#define constrain(amt, low, high) ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))
#define HIGH 1
#define LOW 0
#define INPUT 0
#define A0 54
#define A1 55
#define A2 56
#define A3 57
#define A4 58
#define A5 59

unsigned long millis();
void delay(unsigned long);
void pinMode(uint8_t, uint8_t);
int digitalRead(uint8_t);
int analogRead(uint8_t);
void test_set_millis(unsigned long);
void test_advance_millis(unsigned long);

class Print
{
public:
    virtual ~Print() = default;
    virtual size_t write(uint8_t b) = 0;
    size_t write(const uint8_t *buf, size_t n) { for (size_t i = 0; i < n; ++i) write(buf[i]); return n; }
    size_t print(const char *s) { size_t n = 0; while (*s) { write((uint8_t)*s++); ++n; } return n; }
    size_t print(char c) { return write((uint8_t)c); }
    size_t print(int v) { char b[16]; snprintf(b, sizeof b, "%d", v); return print(b); }
    size_t print(unsigned int v) { char b[16]; snprintf(b, sizeof b, "%u", v); return print(b); }
    size_t print(long v) { char b[24]; snprintf(b, sizeof b, "%ld", v); return print(b); }
    size_t print(unsigned long v) { char b[24]; snprintf(b, sizeof b, "%lu", v); return print(b); }
    size_t println(const char *s) { size_t n = print(s); write('\n'); return n + 1; }
    void flush() {}
};

class Stream : public Print
{
public:
    virtual int available() = 0;
    virtual int read() = 0;
};

class HardwareSerial : public Stream
{
public:
    void begin(unsigned long) {}
    int available() override { return (int)rx.size(); }
    int read() override { if (rx.empty()) return -1; int v = rx.front(); rx.pop_front(); return v; }
    size_t write(uint8_t b) override { tx.push_back(b); return 1; }
    using Print::write;
    std::deque<uint8_t> rx;
    std::vector<uint8_t> tx;
};

extern HardwareSerial Serial, Serial2, Serial3;
