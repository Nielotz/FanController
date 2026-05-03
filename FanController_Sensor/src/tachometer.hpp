#pragma once
#include <avr/io.h>
#include <avr/interrupt.h>
#include <Arduino.h>

#include <stdint.h>

#include "config.h"
#include "debug.h"

// This place is not a place of honor, nothing valued is here.  
// Minimal index_sequence for fold-expression helper (no <utility> on AVR toolchain)
template<size_t... I> struct index_sequence {};
template<size_t N, size_t... I> struct make_index_sequence : make_index_sequence<N-1, N-1, I...> {};
template<size_t... I> struct make_index_sequence<0, I...> : index_sequence<I...> {};


namespace tachometer {
// 2 pulses per revolution, fan won't be faster than 3000 RPM so 6000 tachs per minute → 100 per second
inline volatile uint8_t s_isr_pulses = 0;
}

#if DEBUG_ENABLED
namespace tachometer {
inline bool s_isr_overflow = false;
}
ISR(INT0_vect) {
    using namespace tachometer;
    if (s_isr_pulses == 255) {
        s_isr_overflow = true;
    }
    s_isr_pulses = s_isr_pulses + 1;
}
#else
ISR(INT0_vect) {
    using namespace tachometer;
    s_isr_pulses = s_isr_pulses + 1;
}
#endif

namespace tachometer {

static constexpr debug::Logger<debug::Level::INFO> logger;

inline uint16_t s_lastMs = 0;

// RPM is calculated based on the collected samples
inline uint16_t s_rpm = 0;  // Currently calculated RPM
inline uint16_t rpmSamples[TACH_SAMPLES_TO_AVG] = {0};
inline uint8_t rpmSampleIndex = 0;
inline uint16_t msBetweenPulses = 0; // 16ms at 1875 rpm, 64ms at 468.75, 256 at 117.1875 <- 255 may overflow

/**
 * @brief Print the current tachometer state
 */
inline void recordState() {
#if DEBUG_ENABLED
    logger.info("Tach: lastMs=", s_lastMs, " rpm=", s_rpm, " msBetweenPulses=", msBetweenPulses, " overflow=", s_isr_overflow, "\n");
#else
    logger.info("Tach: lastMs=", s_lastMs, " rpm=", s_rpm, " msBetweenPulses=", msBetweenPulses, "\n");
#endif
}

// Configure INT0 (PB2) for falling-edge tachometer input and seed the timer.
inline void init() {
    logger.info("Initializing Tachometer...\n");

    s_lastMs = static_cast<uint16_t>(millis());

    pinMode(PIN_TACH, INPUT);
    // Trigger on falling edge (open-collector fan tach pulls low each pulse)
    MCUCR = (MCUCR & ~((1 << ISC01) | (1 << ISC00))) | (1 << ISC01);
    GIMSK |= (1 << INT0);
}

/**
 * @brief Read and reset the pulse counter atomically.
 * 
 * @return uint16_t The number of pulses counted since the last read.
 */
inline uint8_t _readPulses() {
    noInterrupts();

    const uint8_t pulses = s_isr_pulses;
    s_isr_pulses = 0;
    
    interrupts();
    
    return pulses;
}

/**
 * @brief Calculate RPM
 *
 * SHOULD NOT BE CALLED WITH elapsedMs == 0
 *
 * Calculate RPM with 2 tachs per revolution
 * 60/600/1200/1875 [rpm] = 1/10/20/31.25 rev/s = 2/20/40/62.5 pulses/s = max 1 puls per 16 ms
 * So there is no point measuring often than 16ms as measuring every 16ms would give 0 or 1 -> 0rpm or 1875 rpm.
 * Meas every 32ms -> 0,1,2 -> 0,987.5,1875 [rpm].
 * 64ms (2**6) -> 0,1,2,3,4 -> 4 speeds
 * 512ms (2**9) -> 0->32 speeds
 * So measuring every 1024ms (64 possible speeds) and averaging last 5? should be sufficient
 * 
 * @param pulses 
 * @param elapsedMs 
 * @return uint16_t 
 */
inline uint16_t _calcRPM(uint8_t pulses, uint16_t elapsedMs) {
    if (pulses == 0) {
        return 0;
    }

    msBetweenPulses = elapsedMs / pulses;
    if (msBetweenPulses > MAX_MS_BETWEEN_PULSES) {
        msBetweenPulses = MAX_MS_BETWEEN_PULSES;
    }

    //  4/40/80/120 * (60000 / 1000)  <- precision = (60000 % elapsed) / 2
    // rpm = (pulses/TACH_PULSES_PER_REV) * (MS_IN_MINUTE / elapsedMs)
    // rpm = pulses * ((MS_IN_MINUTE/TACH_PULSES_PER_REV) / elapsedMs)
    const uint16_t rpm = static_cast<uint16_t>(
        static_cast<uint32_t>(pulses) * (MS_IN_MINUTE/TACH_PULSES_PER_REV) / elapsedMs);
    if (rpm < TACH_MIN_RPM) {
        return 0;
    }
    return rpm;
}

/**
 * @brief Compute delay until the next sample is needed.
 * 
 * @return uint16_t Time in ms until the next sample should be taken.
 */
inline uint16_t _getTimeToNextSample() {
    if ((s_rpm < TACH_MIN_RPM) || (msBetweenPulses == 0)) {
        return TACH_ZERO_SAMPLE_MS;
    }
    
    // Return time to overflow pulse counter at current RPM * 2 (tach) * 3/4 (to be safe)
    constexpr uint16_t BUFFER_SIZE = 1u << (sizeof(s_isr_pulses) * 8);
    static_assert((BUFFER_SIZE / 4) * 3 * MAX_MS_BETWEEN_PULSES < UINT16_MAX, "Time to next sample may overflow uint16_t"); 
    return (BUFFER_SIZE / 4) * 3 * msBetweenPulses;
}

inline void _setSample(const uint16_t sample) {
    rpmSamples[rpmSampleIndex] = sample;
    rpmSampleIndex++;
    if (rpmSampleIndex >= TACH_SAMPLES_TO_AVG) {
        rpmSampleIndex = 0;
    }
}

template <size_t... I>
inline void _calcAverageRPM(index_sequence<I...>) {
    constexpr uint32_t maxSampleSum = TACH_SAMPLES_TO_AVG * FAN_MAX_SPEED_RPM;
    if constexpr (maxSampleSum < UINT16_MAX) {
        s_rpm = (rpmSamples[I] + ...) / sizeof...(I);
    } else {
        s_rpm = static_cast<uint16_t>((static_cast<uint32_t>(rpmSamples[I]) + ...) / sizeof...(I));
    }
}

/**
 * @brief Sample the pulse counter, compute RPM, update internal state, and return it.
 *
 * @return uint16_t Returns maximum time to next update in ms.
 */
inline uint16_t update() {
    const uint16_t nowMs = static_cast<uint16_t>(millis());

    const uint8_t pulses = _readPulses();
    const uint16_t elapsedMs = nowMs - s_lastMs;

    s_lastMs = nowMs;

    if (elapsedMs != 0) {
        const uint16_t rpm = _calcRPM(pulses, elapsedMs);
        _setSample(rpm);
        _calcAverageRPM(make_index_sequence<TACH_SAMPLES_TO_AVG>{});
    }

    recordState();
    return _getTimeToNextSample();
}

// Return the most recently computed RPM without sampling.
inline uint16_t getRPM() { return s_rpm; }
} // namespace tachometer
