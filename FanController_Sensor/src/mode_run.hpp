#pragma once

#include <Arduino.h>

#include "config.h"
#include "debug.h"
#include "fan.hpp"
#include "tachometer.h"
#include "temperature.h"

static Time g_lastControlMs = 0;

/**
 * @brief Fan curve logic that maps temperature to duty cycle
 * 
 * @param tempC Temperature in Celsius
 * @return uint8_t Duty cycle 0-100
 */
inline uint8_t tempToDuty(uint8_t tempC) {
    if (tempC <= TEMP_MIN_C) {
        return 0;
    }
    if (tempC >= TEMP_MAX_C) {
        return FAN_MAX_DUTY;
    }

    return FAN_MIN_DUTY + (FAN_RANGE * (tempC - TEMP_MIN_C)) / TEMP_RANGE;
}

static inline void controlLoop(Time now) {
    const uint8_t tempC = temperature::readCelsius();
    const uint8_t duty = tempToDuty(tempC);
    fan::setDuty(duty);
    const uint16_t rpm = tachometer::calcRPM();
    debug::recordState(now, tempC, duty, rpm);
}

inline void mode_setup() {
    debug::init();
    temperature::init();
    fan::PWMInit();
    tachometer::init();

    debug::update();
    g_lastControlMs = millis();
}

inline void mode_loop() {
    debug::update();

    const Time now = millis();
    if (now - g_lastControlMs >= LOOP_DELAY_MS) {
        g_lastControlMs = now;
        controlLoop(now);
    }
}