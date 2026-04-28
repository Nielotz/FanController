#pragma once
#include <avr/io.h>
#include <avr/interrupt.h>
#include <Arduino.h>
#include <stdint.h>

#include "config.h"
#include "debug.h"

static constexpr debug::Logger<debug::Level::INFO> logger;

inline volatile uint32_t s_pulses = 0;
inline uint32_t          s_lastMs = 0;
inline uint16_t          s_rpm    = 0;

ISR(INT0_vect) {
    s_pulses = s_pulses + 1;
}

namespace tachometer {
// Configure INT0 (PB2) for falling-edge tachometer input and seed the timer.
inline void init() {
    logger.info("Initializing Tachometer...\n");
    s_lastMs = millis();
    pinMode(PIN_TACH, INPUT);
    // Trigger on falling edge (open-collector fan tach pulls low each pulse)
    MCUCR = (MCUCR & ~((1 << ISC01) | (1 << ISC00))) | (1 << ISC01);
    GIMSK |= (1 << INT0);
}

// Sample the pulse counter, compute RPM, update internal state, and return it.
// Resets the internal pulse counter on each call.
inline uint16_t calcRPM() {
    const uint32_t now = millis();
    const uint32_t elapsed = now - s_lastMs;
    if (elapsed == 0)
        return s_rpm;

    noInterrupts();
    const uint32_t pulses = s_pulses;
    s_pulses = 0;
    interrupts();

    s_lastMs = now;
    // rpm = (pulses / pulses_per_rev) * (60000 ms/min / elapsed_ms)
    s_rpm = (uint16_t)((pulses * 60000UL) / ((uint32_t)TACH_PULSES_PER_REV * elapsed));
    return s_rpm;
}

// Return the most recently computed RPM without sampling.
inline uint16_t getRPM() { return s_rpm; }
} // namespace tachometer