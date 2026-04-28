#pragma once
#include <Arduino.h>

// Temperature thresholds (°C). Fan is fully off below MIN, at 100% above MAX.
static constexpr int8_t TEMP_MIN_C           = 40;
static constexpr int8_t TEMP_MAX_C           = 70;

static constexpr uint8_t TEMP_RANGE = TEMP_MAX_C - TEMP_MIN_C;

// Fan duty cycle floor (%) when temp is between MIN and MAX.
static constexpr uint8_t FAN_MIN_DUTY         = 20;
static constexpr uint8_t FAN_MAX_DUTY         = 100;
static constexpr uint8_t FAN_RANGE            = FAN_MAX_DUTY - FAN_MIN_DUTY;

// Tachometer: pulses per revolution (standard PC fans emit 2 pulses/rev)
static constexpr uint8_t TACH_PULSES_PER_REV  = 2;

// Fault detection: if RPM == 0 while duty > 0 for this long → fault
static constexpr uint16_t FAN_FAULT_TIMEOUT_MS = 3000UL;

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
