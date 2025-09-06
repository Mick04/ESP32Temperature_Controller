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
    readAllSensors();
    float tempRed = getTemperature(0); // Before the if statement
 
    float targetTemp = AmFlag ? currentSchedule.amTemp : currentSchedule.pmTemp;

    // // Display current values BEFORE control logic
    // Serial.print("************* Target Temperature **************: ");
    // Serial.println(targetTemp);
    // Serial.print("pmTime ");
    // Serial.println(currentSchedule.pmTime);
    // Serial.print("amTime ");
    // Serial.println(currentSchedule.amTime);
    // Serial.print("pmTemp ");
    // Serial.println(currentSchedule.pmTemp);
    // Serial.print("amTemp ");
    // Serial.println(currentSchedule.amTemp);
    // Serial.print("Current Red Sensor: ");
    // Serial.print(tempRed);
    // Serial.println("°C");
    // Serial.println("*******************************");

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
        bool currentDetected = voltageSensor();
        Serial.println(currentDetected ? "✅ Heater Current Detected" : "❌ No Heater Current Detected - Possible Fault!");
    }
}

// // Function to refresh the cached schedule values
// // Call this whenever schedule data is updated via MQTT
void refreshScheduleCache()
{
    forceScheduleRefresh = true;
    Serial.println("🔄 Schedule cache refresh requested - will update on next heater control cycle");
}