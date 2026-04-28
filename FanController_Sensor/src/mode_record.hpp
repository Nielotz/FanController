#pragma once

#include <Arduino.h>

#include "config.h"
#include "eeprom_log.h"
#include "temperature.h"

static uint16_t g_rec_count = 0; // There is 512 bytes so uint16 is ok
static Time g_last_rec_ms = 0;

inline void mode_setup() {
    g_rec_count = eeprom_log::init_record();
    g_last_rec_ms = millis();
    temperature::init();
}

inline void mode_loop() {
    const Time now = millis();
    if (now - g_last_rec_ms >= recordIntervalMs(g_rec_count)) {
        if (eeprom_log::record(
                temperature::readTemp1C(),
                temperature::readTemp2C(),
                temperature::readTemp3C())) {
            g_last_rec_ms = now;
            ++g_rec_count;
        }
    }
}