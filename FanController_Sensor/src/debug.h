#pragma once
#include <stdint.h>
#include <Arduino.h>

#if DEBUG_PRINT
    #include "DigiCDC.h"
#else
    #define DEBUG_PRINT 0

// SerialUSB is a non-dependent name: it is looked up at template definition
// time, not instantiation time. Even a discarded if constexpr branch must
// have it declared. This stub is zero-cost — all methods are empty and the
// compiler eliminates every call via if constexpr.
namespace { struct _NoOpSerial {
    void begin() {}
    void refresh() {}
    template <typename T> void print(T) {}
} SerialUSB; } // namespace
#endif

namespace debug {

enum class Level : uint8_t {
    ERROR,
    WARNING,
    INFO,
    DEBUG,
};
bool constexpr operator>=(Level a, Level b) { return static_cast<uint8_t>(a) >= static_cast<uint8_t>(b); }

inline void init() {
    if constexpr (DEBUG_PRINT) { SerialUSB.begin(); }
}

// Emit one telemetry line per control tick over DigiCDC serial.
// Format: "<now_ms> <tempC> <dutyPct> <rpm>\n"
//   now_ms  — millis() cast to uint16_t (wraps every ~65 s, sufficient for logging)
//   tempC   — signed temperature in °C
//   dutyPct — fan duty cycle 0–100
//   rpm     — tachometer RPM
inline void recordState(uint32_t now, int16_t tempC, uint8_t dutyPct, uint16_t rpm) {
    if constexpr (DEBUG_PRINT) {
        SerialUSB.print(static_cast<uint16_t>(now));
        SerialUSB.print(" ");
        SerialUSB.print(tempC);
        SerialUSB.print(" ");
        SerialUSB.print(dutyPct);
        SerialUSB.print(" ");
        SerialUSB.print(rpm);
        SerialUSB.print("\n");
    } else {
        (void)now; (void)tempC; (void)dutyPct; (void)rpm;
    }
}

// Per-file logger: instantiate once with the desired threshold level.
// Example:
//   static constexpr debug::Logger<debug::Level::INFO> logger;
//   logger.info("fan init\n");   // printed
//   logger.debug("tick\n");      // compiled away
template <Level setLevel>
struct Logger {
    template <typename... Args>
    inline void error(const Args&... args) const {
        if constexpr (DEBUG_PRINT && setLevel >= Level::ERROR) {
            SerialUSB.print("E: ");
            (SerialUSB.print(args), ...);
        } else {
            ((void)args, ...);
        }
    }

    template <typename... Args>
    inline void warning(const Args&... args) const {
        if constexpr (DEBUG_PRINT && setLevel >= Level::WARNING) {
            SerialUSB.print("W: ");
            (SerialUSB.print(args), ...);
        } else {
            ((void)args, ...);
        }
    }

    template <typename... Args>
    inline void info(const Args&... args) const {
        if constexpr (DEBUG_PRINT && setLevel >= Level::INFO) {
            SerialUSB.print("I: ");
            (SerialUSB.print(args), ...);
        } else {
            ((void)args, ...);
        }
    }

    template <typename... Args>
    inline void debug(const Args&... args) const {
        if constexpr (DEBUG_PRINT && setLevel >= Level::DEBUG) {
            SerialUSB.print("D: ");
            (SerialUSB.print(args), ...);
        } else {
            ((void)args, ...);
        }
    }
};

// Must be called regularly from loop() to keep the DigiCDC USB stack alive.
inline void update() {
    if constexpr (DEBUG_PRINT) { SerialUSB.refresh(); }
}

} // namespace debug