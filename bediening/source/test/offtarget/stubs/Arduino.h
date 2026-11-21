#pragma once
// Minimal stand-in for the Arduino core so firmware sources can be syntax-checked / run on a host.
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
extern uint32_t stub_millis;
inline uint32_t millis() { return stub_millis; }
inline long random(long max) { return max > 0 ? rand() % max : 0; }
#define pdMS_TO_TICKS(x) (x)
inline void vTaskDelay(int) {}
#define PROGMEM
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2
#define HIGH 1
#define LOW 0
inline void pinMode(int, int) {}
inline void digitalWrite(int, int) {}
inline int digitalRead(int) { return 0; }
struct SerialStub {
    void begin(int) {}
    void setTxTimeoutMs(int) {}
    int printf(const char*, ...) { return 0; }
};
extern SerialStub Serial;

#define D0 0
#define D1 1
#define D2 2
#define D3 3
#define D4 4
#define D5 5
#define D6 6
#define D7 7
#define D8 8
#define D9 9
#define D10 10
#define D11 11
#define D12 12
#define D13 13
#define A0 14
#define A1 15
#define A2 16
#define A3 17
#define A4 18
#define A5 19
#define A6 20
#define A7 21
