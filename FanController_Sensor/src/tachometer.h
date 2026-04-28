#pragma once
#include <stdint.h>

namespace tachometer {
// Configure INT0 (PB2) for falling-edge tachometer input and seed the timer.
void init();

// Sample the pulse counter, compute RPM, update internal state, and return it.
// Resets the internal pulse counter on each call.
uint16_t calcRPM();

// Return the most recently computed RPM without sampling.
uint16_t getRPM();
} // namespace tachometer