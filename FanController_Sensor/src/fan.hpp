#pragma once
#include <avr/io.h>

#include <Arduino.h>

#include "config.h"
#include "debug.h"

namespace fan {

// Timer1 Fast PWM TOP (OCR1C).
// Timer counts 0..TOP (TOP+1 ticks) then resets, so:
//   f_PWM = F_CPU / (prescaler * (TOP + 1))
//         = 16500000 / (4 * 165) = 25000 Hz  (Intel 4-pin fan spec)
// Prescaler CS1[3:0] = 0b0110 → PCK/4 (TCCR1 bits CS11+CS10 in PWMInit).
// Below ~21 kHz the switching frequency becomes audible whine.
static constexpr uint8_t PWM_TOP = 164; // 16500000 / (4 * 25000) - 1

inline uint8_t s_dutyPct = 0;

// Initialize Timer1 for 25 kHz Fast PWM on PB1 (OC1A).
// F_CPU = 16500000 Hz, prescaler = PCK/4, TOP = 164 → actual 25000 Hz.
// Inlined: one callsite (setup), no module state.
inline void PWMInit() {
    debug::info("Initializing Fan PWM...\n");
    pinMode(PIN_FAN_PWM, OUTPUT);
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
inline void setDuty(uint8_t pct) {
    if (pct == 0) {
        debug::info("Setting fan duty to 0%\n");
        TCCR1 &= ~((1 << COM1A1) | (1 << COM1A0));
        digitalWrite(PIN_FAN_PWM, LOW);
    } else if (pct >= 100) {
        debug::info("Setting fan duty to 100%\n");
        TCCR1 &= ~((1 << COM1A1) | (1 << COM1A0));
        digitalWrite(PIN_FAN_PWM, HIGH);
    } else {
        // debug::info("Setting fan duty to %d%\n", pct);
        TCCR1 = (TCCR1 & ~(1 << COM1A0)) | (1 << COM1A1);
        OCR1A = (uint8_t)(((uint16_t)pct * ((uint16_t)PWM_TOP + 1)) / 100);
    }
    s_dutyPct = pct;
}

// Return the last duty cycle set via setDuty().
inline uint8_t getDuty() { return s_dutyPct; }
} // namespace fan
