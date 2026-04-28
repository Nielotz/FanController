#pragma once

#include <Arduino.h>

namespace eeprom_log {
uint16_t init_record();
bool record(uint8_t t1, uint8_t t2, uint8_t t3);
void dumpRaw();
void eraseAll();
} // namespace eeprom_log
