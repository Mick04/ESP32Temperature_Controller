//=====================================
// SCT-013-020 Current Monitor
//=====================================
// Measures current draw from heater to verify operation
// Uses EmonLib library for RMS current calculation
//=====================================
#include <Arduino.h>
#include "EmonLib.h"
#include <SPIFFS.h>
#include "config.h"

EnergyMonitor emon1;

// Check if heater is actually drawing current (for safety verification)
bool voltageSensor()
{
    Serial.println("");
    Serial.println("==============================");
    Serial.println("SCT-013-020 Current Sensor");
    Serial.println("==============================");
    Serial.println("");
    // Initialize sensor on first call only (one-time setup)
    static bool initialized = false;
    if (!initialized)
    {
        emon1.current(CURRENT_SENSOR_PIN, CALIBRATION_CONSTANT); // Setup SCT-013-020 with calibrated values
        initialized = true;
    }

    // Read RMS current from SCT-013-020 sensor (takes 5000 samples for accuracy)
    double Irms = emon1.calcIrms(SAMPLES_PER_READING);

    // Apply baseline offset correction to eliminate ADC drift when no current flows
    Irms -= BASELINE_OFFSET; // Subtract 1.75A offset
    if (Irms < 0)
        Irms = 0.0; // Ensure no negative readings

    // Filter out electrical noise below threshold
    if (Irms < NOISE_THRESHOLD) // Ignore readings below 0.1A
    {
        Irms = 0.0;
    }

    // Check if heater is drawing current (simplified for safety check)
    bool currentDetected = (Irms > HEATER_ON_THRESHOLD); // Current > 0.45A indicates heater is on

    // Debug output for safety verification can be removed in production
    if (ENABLE_DEBUG_OUTPUT)
    {
        Serial.print("Safety Check - Current: ");
        Serial.print(Irms, 3);
        Serial.print("A, Heater Detected: ");
        Serial.println(currentDetected ? "YES" : "NO");
    }

    return currentDetected; // Return true if heater is drawing current, false if not
}
