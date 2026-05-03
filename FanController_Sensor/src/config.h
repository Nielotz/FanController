#pragma once

// Temperature thresholds (°C). Fan is fully off below MIN, at 100% above MAX.
#include <stdint.h>

static constexpr int8_t TEMP_MIN_C           = 40;
static constexpr int8_t TEMP_MAX_C           = 70;

static constexpr uint8_t TEMP_RANGE = TEMP_MAX_C - TEMP_MIN_C;

// Used only for estimates, if real RPM exceeds this value we may get invalid readouts
static constexpr uint16_t FAN_MAX_SPEED_RPM = 2000;

// Fan duty cycle floor (%) when temp is between MIN and MAX.
static constexpr uint8_t FAN_MIN_DUTY = 20;  // Min duty cycle to order
static constexpr uint8_t FAN_MAX_DUTY = 100; // Max duty cycle to order
static constexpr uint8_t FAN_RANGE = FAN_MAX_DUTY - FAN_MIN_DUTY;

// Tachometer: minimum RPM to consider valid
static constexpr uint16_t TACH_MIN_RPM  = 200;

// Tachometer: pulses per revolution (standard PC fans emit 2 pulses/rev)
static constexpr uint8_t TACH_PULSES_PER_REV  = 2;

// Tachometer: number of samples to average for RPM calculation
static constexpr uint16_t TACH_SAMPLES_TO_AVG = 5;

// Tachometer: delay(ms) between samples (if fan stopped)
static constexpr uint16_t TACH_ZERO_SAMPLE_MS = 2000;

// Fault detection: if RPM == 0 while duty > 0 for this long → fault
static constexpr uint16_t FAN_FAULT_TIMEOUT_MS = 3000UL;

static constexpr uint16_t MS_IN_MINUTE = 60000;
static constexpr uint32_t MAX_MS_BETWEEN_PULSES = MS_IN_MINUTE / (TACH_MIN_RPM * TACH_PULSES_PER_REV);

// Control loop period (ms)
static constexpr uint16_t LOOP_DELAY_MS        = 500;

// Pin definitions
static constexpr uint8_t PIN_TEMP1  = A0;   // PB5, ADC0 — MCP9700A VOUT
static constexpr uint8_t PIN_TEMP2  = PB3;  // PB3, ADC3 — MCP9700A VOUT
static constexpr uint8_t PIN_TEMP3  = PB4;  // PB4, ADC2 — MCP9700A VOUT
static constexpr uint8_t PIN_TACH   = PB2;  // INT0 — tachometer input
static constexpr uint8_t PIN_BUZZER = PB0;  // software PWM — passive buzzer
static constexpr uint8_t PIN_FAN_PWM = PB1; // OC1A — hardware PWM output

static constexpr uint16_t EEPROM_SIZE         = 512;

static constexpr uint16_t PHASE1_RECORDS      = 30;
static constexpr uint32_t RECORD_INTERVAL1_MS = 60000UL;

static constexpr uint16_t PHASE2_RECORDS      = 50;
static constexpr uint32_t RECORD_INTERVAL2_MS = 120000UL;

static constexpr uint32_t RECORD_INTERVAL     = 240000UL;

static inline uint32_t recordIntervalMs(uint16_t count) {
	if (count < PHASE1_RECORDS) {
		return RECORD_INTERVAL1_MS;
	}
	if (count < PHASE2_RECORDS) {
		return RECORD_INTERVAL2_MS;
	}
	return RECORD_INTERVAL;
}

using Time = decltype(millis());
