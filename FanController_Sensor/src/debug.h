#pragma once
#include <stdint.h>
#include <Arduino.h>
#if DEBUG
#include "DigiCDC.h"
#endif // DEBUG

namespace debug {

inline void init() {
#if DEBUG
    SerialUSB.begin();
#endif // DEBUG
}

// Emit one telemetry line per control tick over DigiCDC serial.
// Format: "<now_ms> <tempC> <dutyPct> <rpm> <fault>\n"
//   now_ms  — millis() cast to uint16_t (wraps every ~65 s, sufficient for logging)
//   tempC   — signed temperature in °C
//   dutyPct — fan duty cycle 0–100
//   rpm     — tachometer RPM
//   fault   — 0=ok, 1=fan fault active
inline void recordState(uint32_t now, int16_t tempC, uint8_t dutyPct, uint16_t rpm) {
#if DEBUG
    SerialUSB.print(static_cast<uint16_t>(now));
    SerialUSB.print(" ");
    SerialUSB.print(tempC);
    SerialUSB.print(" ");
    SerialUSB.print(dutyPct);
    SerialUSB.print(" ");
    SerialUSB.print(rpm);
    // SerialUSB.print(" ");
    // SerialUSB.print(static_cast<uint8_t>(fault ? 1 : 0));
    SerialUSB.print("\n");
#else
    (void)now;
    (void)tempC;
    (void)dutyPct;
    (void)rpm;
    // (void)fault;
#endif // DEBUG
}

template <typename... Args>
inline void info(const Args&... args) {
#if DEBUG
    (SerialUSB.print(args), ...);
#else
    ((void)args, ...);
#endif // DEBUG
}

template <typename... Args>
inline void error(const Args&... args) {
#if DEBUG
    SerialUSB.print("E: ");
    (SerialUSB.print(args), ...);
    while (true)
        ; // halt — avoids pulling in abort() from assert
#else
    ((void)args, ...);
#endif // DEBUG
}

// Must be called regularly from loop() to keep the DigiCDC USB stack alive.
inline void update() {
#if DEBUG
    SerialUSB.refresh();
#endif // DEBUG
}
} // namespace debug