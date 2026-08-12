#include "status_led.h"

#include <Arduino.h>

namespace
{
    constexpr unsigned long kDisconnectedBlinkPeriodMs = 1000; // 1Hz
    constexpr unsigned long kErrorBlinkPeriodMs = 100;         // 5Hz
    constexpr unsigned long kErrorMessageRepeatMs = 2000;
}

void StatusLedInit()
{
    pinMode(kStatusLedPin, OUTPUT);
    digitalWrite(kStatusLedPin, LOW);
}

void StatusLedUpdate(bool fmsConnected)
{
    if (fmsConnected)
    {
        digitalWrite(kStatusLedPin, HIGH);
        return;
    }

    bool on = (millis() / (kDisconnectedBlinkPeriodMs / 2)) % 2 == 0;
    digitalWrite(kStatusLedPin, on ? HIGH : LOW);
}

[[noreturn]] void HaltWithError(const char *message)
{
    // Re-print periodically, not just once, so the error is still visible to
    // anyone who connects a serial monitor after the fault occurred.
    unsigned long lastPrintMs = millis() - kErrorMessageRepeatMs;
    while (true)
    {
        if (millis() - lastPrintMs >= kErrorMessageRepeatMs)
        {
            Serial.println(message);
            lastPrintMs = millis();
        }
        digitalWrite(kStatusLedPin, HIGH);
        delay(kErrorBlinkPeriodMs);
        digitalWrite(kStatusLedPin, LOW);
        delay(kErrorBlinkPeriodMs);
    }
}
