#include "eeprom_log.h"

#include <avr/eeprom.h>

#include "config.h"

#if defined(MODE_RETRIEVE) || DEBUG_ENABLED
#include "DigiCDC.h"
#endif

namespace eeprom_log {
static uint16_t g_eeprom_addr = 0;
static constexpr uint16_t EEPROM_RECORD_CAPACITY = EEPROM_SIZE / sizeof(uint16_t);

static inline uint16_t pack(uint8_t t1, uint8_t t2, uint8_t t3) {
    uint8_t o1 = (t1 > 20) ? static_cast<uint8_t>(t1 - 20) : 0;
    uint8_t o2 = (t2 > 20) ? static_cast<uint8_t>(t2 - 20) : 0;
    uint8_t o3 = (t3 > 20) ? static_cast<uint8_t>(t3 - 20) : 0;

    if (o1 > 60) {
        o1 = 60;
    }
    if (o2 > 20) {
        o2 = 20;
    }
    if (o3 > 15) {
        o3 = 15;
    }

    return (static_cast<uint16_t>(o1) << 10) | (static_cast<uint16_t>(o2) << 5) | o3;
}

static inline bool packedIsValid(uint16_t word) {
    return ((word >> 10) & 0x3F) <= 60 && ((word >> 5) & 0x1F) <= 20 && (word & 0x1F) <= 15;
}

static inline uint16_t readWord(uint16_t byteOffset) {
    return eeprom_read_word(reinterpret_cast<const uint16_t *>(static_cast<uintptr_t>(byteOffset)));
}

static inline void writeWord(uint16_t byteOffset, uint16_t value) {
    eeprom_write_word(reinterpret_cast<uint16_t *>(static_cast<uintptr_t>(byteOffset)), value);
}

static inline void updateWord(uint16_t byteOffset, uint16_t value) {
    eeprom_update_word(reinterpret_cast<uint16_t *>(static_cast<uintptr_t>(byteOffset)), value);
}

static inline uint8_t readByte(uint16_t byteOffset) {
    return eeprom_read_byte(reinterpret_cast<const uint8_t *>(static_cast<uintptr_t>(byteOffset)));
}

#if defined(MODE_RETRIEVE) || DEBUG_ENABLED
static inline char hexDigit(uint8_t nibble) {
    return (nibble < 10) ? static_cast<char>('0' + nibble) : static_cast<char>('A' + (nibble - 10));
}

static inline void printHexByte(uint8_t value) {
    SerialUSB.write(hexDigit(static_cast<uint8_t>(value >> 4)));
    SerialUSB.write(hexDigit(static_cast<uint8_t>(value & 0x0F)));
}
#endif

uint16_t init_record() {
    const uint16_t eepromBytes = EEPROM_SIZE;

    for (uint16_t byteOffset = 0; byteOffset < eepromBytes; byteOffset += sizeof(uint16_t)) {
        const uint16_t word = readWord(byteOffset);
        if (word == 0xFFFF || !packedIsValid(word)) {
            g_eeprom_addr = byteOffset;
            return static_cast<uint16_t>(byteOffset / sizeof(uint16_t));
        }
    }

    g_eeprom_addr = eepromBytes;
    return EEPROM_RECORD_CAPACITY;
}

bool record(uint8_t t1, uint8_t t2, uint8_t t3) {
    const uint16_t eepromBytes = EEPROM_SIZE;
    if (g_eeprom_addr >= eepromBytes) {
        return false;
    }

    writeWord(g_eeprom_addr, pack(t1, t2, t3));
    g_eeprom_addr = static_cast<uint16_t>(g_eeprom_addr + sizeof(uint16_t));
    return true;
}

void dumpRaw() {
#if defined(MODE_RETRIEVE) || DEBUG_ENABLED
    const uint16_t eepromBytes = EEPROM_SIZE;

    SerialUSB.print("EEPROM:512\r\n");
    for (uint16_t byteOffset = 0; byteOffset < eepromBytes; ++byteOffset) {
        printHexByte(readByte(byteOffset));
        SerialUSB.print(" ");
        if ((byteOffset & 0x0F) == 0x0F) {
            SerialUSB.print("\r\n");
            SerialUSB.refresh();
        }
    }
    SerialUSB.print("END\r\n");
    SerialUSB.refresh();
#endif
}

void eraseAll() {
    const uint16_t eepromBytes = EEPROM_SIZE;
    for (uint16_t byteOffset = 0; byteOffset < eepromBytes; byteOffset += sizeof(uint16_t)) {
        updateWord(byteOffset, 0xFFFF);
    }
}
} // namespace eeprom_log
