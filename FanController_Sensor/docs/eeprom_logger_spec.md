# EEPROM Temperature Logger — Specification

## 1. Overview

The firmware has three independently flashed builds sharing the same source tree:

| Build | Pins used | Purpose |
|-------|-----------|--------|
| **RECORD** | PB5/PB3/PB4 as ADC | Reads three thermometers, logs to EEPROM at variable intervals. No USB, no fan/tach. |
| **RETRIEVE** | PB3/PB4 as DigiCDC USB | Dumps raw EEPROM hex over USB. Erases EEPROM **only on explicit user command**. |
| **RUN** | PB5/PB3/PB4 as ADC; PB1 PWM; PB2 tach | Fan controller with all three thermometers. PB3/PB4 used for USB only when `-DDEBUG_ENABLED` — in that case TEMP2/TEMP3 are unavailable. |

Record decoding (unpack) is done entirely **on the PC** from the raw hex dump — no unpack logic is needed in firmware.

---

## 2. Hardware Context

| MCU | ATtiny85 @ 16.5 MHz (Digispark) |
|-----|--------------------------------|
| EEPROM | 512 bytes |
| SRAM | 512 bytes |
| Flash available | ~6 KB (2 KB used by bootloader) |

### Pin assignments

| Pin | Port | RECORD | RETRIEVE | RUN | RUN + DEBUG |
|-----|------|--------|----------|-----|-------------|
| phys 1 | PB5 | TEMP1 — ADC0 | — | TEMP1 — ADC0 | TEMP1 — ADC0 |
| phys 2 | PB3 | TEMP2 — ADC3 | DigiCDC USB+ | TEMP2 — ADC3 | DigiCDC USB+ |
| phys 3 | PB4 | TEMP3 — ADC2 | DigiCDC USB− | TEMP3 — ADC2 | DigiCDC USB− |
| phys 6 | PB1 | — | — | Fan PWM out | Fan PWM out |
| phys 7 | PB2 | — | — | Tachometer (INT0) | Tachometer (INT0) |

All temperature sensors (MCP9700A) currently use the **1.1 V internal reference**.
- Vout = 500 mV + 10 mV/°C → clips at (1100−500)/10 = **60 °C** — acceptable for TEMP2 (≤40 °C) and TEMP3 (≤35 °C); TEMP1 may saturate above ~60 °C
- Resolution at 8-bit ADC: 1100/256 ≈ 4.3 mV/step ≈ **0.43 °C/step** (finer than 2.56 V ref)
- TEMP1 clipping above ~60 °C is a known limitation of the current calibration firmware; if the logged cycle shows sustained clipping, revisit TEMP1 reference selection before finalizing RUN-mode control behavior

---

## 3. PlatformIO Build Environments

```ini
[env:digispark-record]
platform       = atmelavr
board          = digispark-tiny
framework      = arduino
targets        = buildprog, compiledb
extra_scripts  = extra_scripts.py
build_flags    = -std=gnu++1z -DMODE_RECORD

[env:digispark-retrieve]
platform       = atmelavr
board          = digispark-tiny
framework      = arduino
targets        = buildprog, compiledb
extra_scripts  = extra_scripts.py
lib_deps       = DigisparkCDC
build_flags    = -std=gnu++1z -DMODE_RETRIEVE

[env:digispark-run]
platform       = atmelavr
board          = digispark-tiny
framework      = arduino
targets        = buildprog, compiledb
extra_scripts  = extra_scripts.py
build_flags    = -std=gnu++1z -DMODE_RUN
; add -DDEBUG_ENABLED to enable DigiCDC serial output (disables TEMP2/TEMP3, uses PB3/PB4 for USB)
; add -DDEBUG_ENABLED to digispark-run build_flags, or create a separate env:

[env:digispark-run-debug]
platform       = atmelavr
board          = digispark-tiny
framework      = arduino
targets        = buildprog, compiledb
extra_scripts  = extra_scripts.py
lib_deps       = DigisparkCDC
build_flags    = -std=gnu++1z -DMODE_RUN -DDEBUG_ENABLED
```

---

## 4. EEPROM Layout

### 4.1 Address map

```
Address  0x000 – 0x1FF  (512 bytes)

Record[0]   @ 0x000 – 0x001
Record[1]   @ 0x002 – 0x003
...
Record[255] @ 0x1FE – 0x1FF
```

256 records × 2 bytes = 512 bytes. No header or metadata bytes.

### 4.2 Record format (16-bit word, little-endian)

```
Bit  [15:10]  T1 offset above 20 °C   (6 bits, range 0–60  → 20–80 °C)
Bit  [ 9: 5]  T2 offset above 20 °C   (5 bits, range 0–20  → 20–40 °C)
Bit  [ 4: 0]  T3 offset above 20 °C   (5 bits, range 0–15  → 20–35 °C)
```

**Sentinel value:** `0xFFFF` (erased EEPROM) marks an empty slot.  
Maximum reachable packed value: `(60<<10)|(20<<5)|15 = 0xF28F` — `0xFFFF` is unreachable.

Any word where `((w >> 10) & 0x3F) > 60`, `((w >> 5) & 0x1F) > 20`, or `(w & 0x1F) > 15` is **invalid/corrupt** and is treated as end-of-log by both the firmware resume scan and the PC decoder.

### 4.3 pack (firmware-only)

Only `pack()` lives in firmware. `unpack()` lives in the PC-side Python decoder.

```c
// Firmware (eeprom_log.cpp)
static inline uint16_t pack(uint8_t t1, uint8_t t2, uint8_t t3) {
    uint8_t o1 = (t1 > 20) ? (t1 - 20) : 0;  if (o1 > 60) o1 = 60;
    uint8_t o2 = (t2 > 20) ? (t2 - 20) : 0;  if (o2 > 20) o2 = 20;
    uint8_t o3 = (t3 > 20) ? (t3 - 20) : 0;  if (o3 > 15) o3 = 15;
    return ((uint16_t)o1 << 10) | ((uint16_t)o2 << 5) | o3;
}
```

```python
# PC decoder (tools/decode_eeprom.py)
def unpack(w):
    t1 = ((w >> 10) & 0x3F) + 20
    t2 = ((w >>  5) & 0x1F) + 20
    t3 = ( w        & 0x1F) + 20
    return t1, t2, t3
```

---

## 5. Variable-Rate Recording Schedule

| Phase | Record index | Interval | Cumulative time at phase end |
|-------|-------------|----------|------------------------------|
| 1 | 0 – 29  (30 records) | 1 min | 30 min |
| 2 | 30 – 49 (20 records) | 2 min | 70 min |
| 3 | 50 – 255 (206 records) | 4 min | 894 min (~14.9 h) |

Interval selection is driven by constants in `config.h`:

```c
// config.h
static constexpr uint16_t EEPROM_RECORDS       = 256;

static constexpr uint16_t PHASE1_RECORDS       = 30;
static constexpr uint32_t RECORD_INTERVAL1_MS  =  60000UL;  // 1 min

static constexpr uint16_t PHASE2_RECORDS       = 50;
static constexpr uint32_t RECORD_INTERVAL2_MS  = 120000UL;  // 2 min

static constexpr uint32_t RECORD_INTERVAL      = 240000UL;  // 4 min

static inline uint32_t recordIntervalMs(uint16_t count) {
    if (count < PHASE1_RECORDS) return RECORD_INTERVAL1_MS;
    if (count < PHASE2_RECORDS) return RECORD_INTERVAL2_MS;
    return                                RECORD_INTERVAL;
}
```

Timing is non-blocking: compared against `millis()` in the control loop tick.
The first record is taken **after** one full interval, so sample index 0 corresponds to elapsed minute 1, not minute 0.
After an unexpected reset, the logger resumes the correct slot and phase, but PC-side `elapsed_min` remains the **nominal schedule** and does not include power-off downtime because no absolute timestamp is stored in EEPROM.

---

## 6. RECORD Mode — Boot Sequence

1. `eeprom_log::init_record()`:
    - Scan from record 0 to find the first `0xFFFF` **or invalid packed word** — resume position after unexpected reset
    - Set `g_eeprom_addr` to that address for the next write
    - Return the next record index so the caller resumes the correct timing phase
    - **No erase** — EEPROM is only erased by RETRIEVE mode
2. Configure PB3 (ADC3) and PB4 (ADC2) as ADC inputs (no DigiCDC init)
3. Enter logging loop

### 6.1 record() — called from loop()

```c
bool eeprom_log::record(uint8_t t1, uint8_t t2, uint8_t t3) {
    if (g_eeprom_addr >= EEPROM_RECORDS * 2) { return false; }
    eeprom_write_word(g_eeprom_addr, pack(t1, t2, t3));
    g_eeprom_addr += 2;
    return true;
}
```

`mode_setup()` seeds `g_rec_count` from `init_record()`. `mode_loop()` increments it only when `record()` returns `true`.

### 6.2 Power-loss safety

Records are written every 1–4 minutes. A power loss mid-write can leave the last slot as `0xFFFF`, an invalid packed word, or another valid-looking packed word. Firmware resume and the PC decoder treat invalid packed words as end-of-log and overwrite them on the next write; a torn write that happens to land on another valid packed word is still indistinguishable from a real sample. In practice, unavoidable corruption is bounded to the final sample/slot.

---

## 7. RETRIEVE Mode — Boot Sequence

1. `SerialUSB.begin()` (DigiCDC)
2. Print prompt: `"Send 'D' to dump EEPROM, 'E' to erase EEPROM\r\n"`
3. Enter command loop:
   ```c
   while (1) {
       SerialUSB.refresh();
       if (SerialUSB.available()) {
           const int c = SerialUSB.read();
           if (c == 'D') {
               eeprom_log::dumpRaw();
           } else if (c == 'E') {
               eeprom_log::eraseAll();
               SerialUSB.print("ERASED\r\n");
           }
       }
   }
   ```

**EEPROM is erased only on explicit `'E'` command**, never automatically, never in RECORD or RUN modes.

`SerialUSB.available()` + `SerialUSB.read()` are standard DigiCDC methods — USB reads work on this hardware.

---

## 8. Raw Dump Format

Firmware emits the full 512 EEPROM bytes as hex over DigiCDC:

```
EEPROM:512
AB CD EF 01 23 45 67 89 0A 0B 0C 0D 0E 0F 10 11
... (16 bytes per line, 512 bytes total)
END
```

- Each byte is emitted as **two uppercase hex digits** followed by a single space; line breaks may occur every 16 bytes
- `SerialUSB.print()` only — no `printf`, no heap; the firmware must zero-pad bytes explicitly before printing `HEX`
- `SerialUSB.refresh()` called every line to service the V-USB interrupt
- Decoding (unpack, `idxToMin`, CSV generation) is done entirely by `tools/decode_eeprom.py` on the PC

---

## 9. Public API (`eeprom_log.h`)

```c
namespace eeprom_log {
    // RECORD mode
    uint16_t init_record();                           // scan for resume position, return next record index, no erase
    bool record(uint8_t t1, uint8_t t2, uint8_t t3); // pack + write next slot; returns false when full

    // RETRIEVE mode
    void dumpRaw();   // emit all 512 EEPROM bytes as space-separated two-digit hex over DigiCDC
    void eraseAll();  // write 0xFFFF to all slots via eeprom_update_word
}
```

`record()` returns `false` when all 256 slots are used. Callers must only advance `g_rec_count` when the write succeeds.

---

## 10. Source File Structure

Each mode lives in its own `.hpp` file (not `.cpp`) and defines `mode_setup()` + `mode_loop()`. `main.cpp` `#include`s exactly one of them — forming a single translation unit so the compiler can inline freely. PlatformIO auto-compiles all `.cpp` files in `src/`, so `.hpp` avoids double-compilation with no `src_filter` needed. `#ifdef DEBUG_ENABLED` affects `debug.h` internals **and** RUN-mode sensor availability because PB3/PB4 are shared with DigiCDC.

```
src/
  main.cpp          — #includes exactly one mode .hpp; setup()/loop() call mode_setup()/mode_loop()
  mode_record.hpp   — included when MODE_RECORD
  mode_retrieve.hpp — included when MODE_RETRIEVE
  mode_run.hpp      — included when MODE_RUN
  eeprom_log.cpp/h
  temperature.cpp/h
  fan.cpp/h
  tachometer.cpp/h
  debug.h           — all debug functions are no-ops unless DEBUG_ENABLED defined
  config.h
```

### main.cpp
```c
#if   defined(MODE_RECORD)
  #include "mode_record.hpp"
#elif defined(MODE_RETRIEVE)
  #include "mode_retrieve.hpp"
#elif defined(MODE_RUN)
  #include "mode_run.hpp"
#else
  #error "No MODE defined"
#endif

void setup() { mode_setup(); }
void loop()  { mode_loop();  }
```

### mode_record.hpp
```c
static uint16_t g_rec_count   = 0;
static uint32_t g_last_rec_ms = 0;

void mode_setup() {
    g_rec_count = eeprom_log::init_record();  // scan resume position, no erase
    g_last_rec_ms = millis();
    temperature::init();        // ADC0, ADC2, ADC3 with 1.1V ref
}

void mode_loop() {
    const uint32_t now = millis();
    if (now - g_last_rec_ms >= recordIntervalMs(g_rec_count)) {
        if (eeprom_log::record(
            temperature::readTemp1C(),
            temperature::readTemp2C(),
            temperature::readTemp3C())) {
            g_last_rec_ms = now;
            g_rec_count++;
        }
    }
}
```

### mode_retrieve.hpp
```c
void mode_setup() {
    SerialUSB.begin();
    SerialUSB.print("Send 'D' to dump EEPROM, 'E' to erase EEPROM\r\n");
}

void mode_loop() {
    SerialUSB.refresh();
    if (SerialUSB.available()) {
        const int c = SerialUSB.read();
        if (c == 'D') {
            eeprom_log::dumpRaw();
        } else if (c == 'E') {
            eeprom_log::eraseAll();
            SerialUSB.print("ERASED\r\n");
        }
    }
}
```

### mode_run.hpp
```c
void mode_setup() {
    debug::init();           // no-op unless DEBUG_ENABLED defined; init DigiCDC if DEBUG_ENABLED
#ifdef DEBUG_ENABLED
    temperature::init();     // ADC0 only — PB3/PB4 used by DigiCDC
#else
    temperature::init();     // ADC0, ADC2, ADC3
#endif
    fan::PWMInit();
    tachometer::init();
}

void mode_loop() {
    debug::update();
    const uint32_t now = millis();
    if (now - g_lastControlMs >= LOOP_DELAY_MS) {
        g_lastControlMs = now;
        controlLoop(now);
    }
    debug::update();
}
```

---

## 11. Files Changed

| File | Change |
|------|--------|
| `platformio.ini` | Add `digispark-record`, `digispark-retrieve`, `digispark-run`, `digispark-run-debug` environments while preserving shared build settings such as `extra_scripts`, `lib_extra_dirs`, `targets`, and `-std=gnu++1z` |
| `config.h` | Add `PIN_TEMP2` (PB3/ADC3), `PIN_TEMP3` (PB4/ADC2), `EEPROM_RECORDS 256` as `uint16_t`; ref stays 1.1V; add `PHASE1_RECORDS 30`, `PHASE2_RECORDS 50`, `RECORD_INTERVAL1_MS`, `RECORD_INTERVAL2_MS`, `RECORD_INTERVAL`, `static inline recordIntervalMs(uint16_t)`, and remove any hardcoded `DEBUG_ENABLED` macro so debug is controlled only by `build_flags` |
| `temperature.h` | Add `ADC2`, `ADC3`; add `readTemp2C()`, `readTemp3C()`; ref = `INTERNAL_1_1V` |
| `eeprom_log.h` | New API: `init_record` returns the resume index, `record`, `dumpRaw`, `eraseAll` — no unpack, no idxToMin |
| `eeprom_log.cpp` | Full rewrite: `static inline pack()`, `static inline packedIsValid()`, `init_record` (scan/resume), `record`, `dumpRaw`, `eraseAll` |
| `main.cpp` | Single-TU dispatcher: `#include`s one `.hpp` based on `MODE_*`; `setup()`/`loop()` call `mode_setup()`/`mode_loop()` |
| `mode_record.hpp` | New: RECORD mode — phase-aware resume, `g_rec_count` restored from EEPROM scan, increment only on successful write |
| `mode_retrieve.hpp` | New: RETRIEVE mode — explicit `'D'` dump command and `'E'` erase command |
| `mode_run.hpp` | New: RUN mode — fan control; `#ifdef DEBUG_ENABLED` selects 1 vs 3 thermometers because PB3/PB4 are shared with DigiCDC |
| `tools/decode_eeprom.py` | New PC-side script: reads zero-padded space-separated hex dump, rejects invalid packed words, corrects elapsed-minute mapping, outputs CSV |

---

## 12. EEPROM Endurance

- Record pass (RECORD): each EEPROM byte is written at most once
- Erase pass (RETRIEVE): each EEPROM byte is written at most once (`eeprom_update_word` skips already-`0xFF` cells, so a partial session erases fewer cells)
- Worst case per full record+erase cycle: **2 writes per EEPROM byte**
- ATtiny85 endurance: 100,000 write/erase cycles per byte
- Minimum full record+erase cycles before per-cell wear-out: **~50,000**

## 13. PC Decoder (`tools/decode_eeprom.py`)

```python
import sys

def idx_to_min(i):
    if i <  30: return 1  + i
    if i <  50: return 32 + (i - 30) * 2
    return              74 + (i - 50) * 4

def unpack(w):
    return ((w >> 10) & 0x3F) + 20, ((w >> 5) & 0x1F) + 20, (w & 0x1F) + 20

def packed_is_valid(w):
    return ((w >> 10) & 0x3F) <= 60 and ((w >> 5) & 0x1F) <= 20 and (w & 0x1F) <= 15

# Parse hex dump from stdin or file
# Expected format: lines of space-separated two-digit hex bytes between "EEPROM:512" and "END"
bytes_ = []
for line in sys.stdin:
    line = line.strip()
    if not line or line in ('EEPROM:512', 'END'): continue
    bytes_.extend(int(h, 16) for h in line.split())

print('idx,elapsed_min,t1_c,t2_c,t3_c')
for i in range(0, len(bytes_) - 1, 2):
    w = bytes_[i] | (bytes_[i+1] << 8)  # little-endian
    idx = i // 2
    if w == 0xFFFF or not packed_is_valid(w): break
    t1, t2, t3 = unpack(w)
    print(f'{idx},{idx_to_min(idx)},{t1},{t2},{t3}')
```
