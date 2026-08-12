#pragma once

#include <cstdint>

// Status LED on GPIO45 (physical pin 7).
//
// Solid on:          connected to the FMS.
// Blinking at 1Hz:    not connected to the FMS (link down, reconnecting, etc.).
// Blinking at 5Hz:    fatal error (e.g. SD card / config problem).

constexpr uint8_t kStatusLedPin = 45;

void StatusLedInit();

void StatusLedUpdate(bool fmsConnected);

[[noreturn]] void HaltWithError(const char *message);
