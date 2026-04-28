#pragma once

#include "DigiCDC.h"
#include "eeprom_log.h"

inline void mode_setup() {
    SerialUSB.begin();
    SerialUSB.print("Send 'D' to dump EEPROM, 'E' to erase EEPROM\r\n");
}

inline void mode_loop() {
    SerialUSB.refresh();
    if (SerialUSB.available()) {
        const int command = SerialUSB.read();
        if (command == 'D') {
            eeprom_log::dumpRaw();
        } else if (command == 'E') {
            eeprom_log::eraseAll();
            SerialUSB.print("ERASED\r\n");
        }
    }
}