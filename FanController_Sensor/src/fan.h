#pragma once
#include <avr/io.h>

#include <Arduino.h>

#include "debug.h"

namespace fan {
static constexpr debug::Logger<debug::Level::INFO> logger;
// PB1 / OC1A — hardware PWM output
static constexpr uint8_t PWM_PIN = 1;
// OCR1C TOP value: 16500000 / (4 * 25000) - 1 = 164
inline static constexpr uint8_t PWM_TOP = 164;

// Initialize Timer1 for 25 kHz Fast PWM on PB1 (OC1A).
// F_CPU = 16500000 Hz, prescaler = PCK/4, TOP = 164 → actual 25000 Hz.
// Inlined: one callsite (setup), no module state.
inline void PWMInit() {
    logger.info("Initializing Fan PWM...\n");
    pinMode(PWM_PIN, OUTPUT);
    OCR1C = PWM_TOP;
    OCR1A = 0;
    // CTC1=1 (OCR1C as TOP), PWM1A=1 (Fast PWM on OC1A),
    // COM1A1=1 COM1A0=0 (non-inverting), CS11+CS10 = PCK/4 prescaler
    TCCR1 = (1 << CTC1) | (1 << PWM1A) | (1 << COM1A1) | (1 << CS11) | (1 << CS10);
}

// Set fan duty cycle (0–100 %).
// 0   → OC1A disconnected, pin driven low
// 100 → OC1A disconnected, pin driven high
// else → hardware Fast PWM
// Not inlined: two callsites (normal path + fault path); uses s_dutyPct state.
void setDuty(uint8_t pct);

// Return the last duty cycle set via setDuty().
// Not inlined: two callsites; reads s_dutyPct owned by fan.cpp.
uint8_t getDuty();
} // namespace fan
