//==============================
// Heater Control
//==============================
#include <Arduino.h>
#include "HeaterControl.h"
#include "TemperatureSensors.h"
#include "GetShedual.h"
#include "config.h"
#include "TimeManager.h"
#include "TemperatureSensors.h"
#include "StatusLEDs.h"

// External declarations
extern bool AmFlag;
extern SystemStatus systemStatus;

// Global flag to force schedule cache refresh
static bool forceScheduleRefresh = false;
// Heater control function
void updateHeaterControl()
{
    Serial.println("******************Updating Heater Control...**************");
    //getTime();
    String currentTime = getFormattedTime();
    if (currentTime < "12:00")
    {
        AmFlag = true;
    }
    else
    {
        AmFlag = false;
    }

    // Serial.println("******************Updating Heater Control...**************");

    // Cache schedule values to avoid unnecessary function calls
    // These values only change when updated via MQTT from React app
    static float cachedAmTemp = NAN;
    static float cachedPmTemp = NAN;
    static String cachedAmTime = "";
    static String cachedPmTime = "";
    static bool scheduleLoaded = false;

    // Only load schedule values once or when they need to be refreshed
    if (!scheduleLoaded || forceScheduleRefresh)
    {
        cachedAmTemp = getAMTemperature();
        cachedPmTemp = getPMTemperature();
        cachedAmTime = getAMTime();
        cachedPmTime = getPMTime();
        scheduleLoaded = true;
        forceScheduleRefresh = false; // Reset the refresh flag
        Serial.println("📋 Schedule values cached/refreshed");
    }

    // Use cached values
    float amTemp = cachedAmTemp;
    float pmTemp = cachedPmTemp;
    String amTime = cachedAmTime;
    String pmTime = cachedPmTime;

    readAllSensors();
    float tempRed = getTemperature(0); // Before the if statement
 
    float targetTemp = AmFlag ? amTemp : pmTemp;

    // Display current values BEFORE control logic
    Serial.print("************* Target Temperature **************: ");
    Serial.println(targetTemp);
    Serial.print("pmTime ");
    Serial.println(pmTime);
    Serial.print("amTime ");
    Serial.println(amTime);
    Serial.print("pmTemp ");
    Serial.println(pmTemp);
    Serial.print("amTemp ");
    Serial.println(amTemp);
    Serial.print("Current Red Sensor: ");
    Serial.print(tempRed);
    Serial.println("°C");
    Serial.println("*******************************");

    // Check if the current target temperature is valid
    if (targetTemp < tempRed)
    {
        digitalWrite(RELAY_PIN, LOW);
        systemStatus.heater = HEATER_OFF;
        publishSystemData();
        Serial.println("🔥 Heater OFF - Target > Current");
        return;
    }
    else if (targetTemp > tempRed)
    {
        digitalWrite(RELAY_PIN, HIGH);
        systemStatus.heater = HEATER_ON;
        publishSystemData();
        Serial.println("🔥 Heater ON - Target < Current");
    }
}

// // Function to refresh the cached schedule values
// // Call this whenever schedule data is updated via MQTT
void refreshScheduleCache()
{
    forceScheduleRefresh = true;
    Serial.println("🔄 Schedule cache refresh requested - will update on next heater control cycle");
}