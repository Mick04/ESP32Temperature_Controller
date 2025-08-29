// ==================================================
// File: src/GetShedual.cpp
// ==================================================

#include "GetShedual.h"
#include "FirebaseService.h"
#include "HeaterControl.h"
#include <Firebase_ESP_Client.h>

// Global schedule data instance - no default values
ScheduleData currentSchedule = {
    NAN,   // amTemp
    NAN,   // pmTemp
    "",    // amTime
    "",    // pmTime
    false, // amEnabled
    false  // pmEnabled
};

// Firebase data object (extern from FirebaseService.h)
extern FirebaseData fbData;

// void initScheduleManager()
// {
//     Serial.println("Schedule Manager initialized - waiting for Firebase data");
//     Serial.println("⚠️  No default values set - schedule data must be retrieved from Firebase");
//     printScheduleData();
// }
//=======================================================
// I only want to fetch the schedule data once on startup
//                        start     
//=======================================================
void fetchScheduleDataFromFirebase()
{
    Serial.println("=== Fetching Schedule Data from Firebase ===");

    bool allDataRetrieved = true;

    // Fetch AM scheduled time
    if (Firebase.RTDB.getString(&fbData, "/schedule/amScheduledTime"))
    {
        String amTime = fbData.stringData();
        if (isValidTime(amTime))
        {
            currentSchedule.amTime = amTime;
            Serial.print("✅ AM Time retrieved: ");
            Serial.println(amTime);
        }
        else
        {
            Serial.println("⚠️  Invalid AM time format - no fallback value");
        }
    }
    else
    {
        Serial.println("❌ Failed to retrieve AM time from Firebase");
        allDataRetrieved = false;
    }

    // Fetch PM scheduled time
    if (Firebase.RTDB.getString(&fbData, "/schedule/pmScheduledTime"))
    {
        String pmTime = fbData.stringData();
        if (isValidTime(pmTime))
        {
            currentSchedule.pmTime = pmTime;
            Serial.print("✅ PM Time retrieved: ");
            Serial.println(pmTime);
        }
        else
        {
            Serial.println("⚠️  Invalid PM time format - no fallback value");
        }
    }
    else
    {
        Serial.println("❌ Failed to retrieve PM time from Firebase");
        allDataRetrieved = false;
    }

    // Fetch AM temperature (try both string and float formats)
    if (Firebase.RTDB.getString(&fbData, "/schedule/amTemperature"))
    {
        String amTempStr = fbData.stringData();
        float amTemp = amTempStr.toFloat();
        if (isValidTemperature(amTemp))
        {
            currentSchedule.amTemp = amTemp;
            Serial.print("✅ AM Temperature retrieved: ");
            Serial.print(amTemp);
            Serial.println("°C");
        }
        else
        {
            Serial.println("⚠️  Invalid AM temperature - no fallback value");
        }
    }
    else if (Firebase.RTDB.getFloat(&fbData, "/schedule/amTemperature"))
    {
        float amTemp = fbData.floatData();
        if (isValidTemperature(amTemp))
        {
            currentSchedule.amTemp = amTemp;
            Serial.print("✅ AM Temperature retrieved (float): ");
            Serial.print(amTemp);
            Serial.println("°C");
        }
        else
        {
            Serial.println("⚠️  Invalid AM temperature - no fallback value");
        }
    }
    else
    {
        Serial.println("❌ Failed to retrieve AM temperature from Firebase");
        allDataRetrieved = false;
    }

    // Fetch PM temperature (try both string and float formats)
    if (Firebase.RTDB.getString(&fbData, "/schedule/pmTemperature"))
    {
        String pmTempStr = fbData.stringData();
        float pmTemp = pmTempStr.toFloat();
        if (isValidTemperature(pmTemp))
        {
            currentSchedule.pmTemp = pmTemp;
            Serial.print("✅ PM Temperature retrieved: ");
            Serial.print(pmTemp);
            Serial.println("°C");
        }
        else
        {
            Serial.println("⚠️  Invalid PM temperature - no fallback value");
        }
    }
    else if (Firebase.RTDB.getFloat(&fbData, "/schedule/pmTemperature"))
    {
        float pmTemp = fbData.floatData();
        if (isValidTemperature(pmTemp))
        {
            currentSchedule.pmTemp = pmTemp;
            Serial.print("✅ PM Temperature retrieved (float): ");
            Serial.print(pmTemp);
            Serial.println("°C");
        }
        else
        {
            Serial.println("⚠️  Invalid PM temperature - no fallback value");
        }
    }
    else
    {
        Serial.println("❌ Failed to retrieve PM temperature from Firebase");
        allDataRetrieved = false;
    }

    if (allDataRetrieved)
    {
        Serial.println("✅ All schedule data retrieved successfully from Firebase");
        // Refresh the heater control cache with the new Firebase data
        refreshScheduleCache();
    }
    else
    {
        Serial.println("⚠️  Some schedule data failed to retrieve - no default values available");
    }

    // Serial.println("=== Current Schedule Data ===");
    // printScheduleData();
    // Serial.println("==============================");
}
//=======================================================
// I only want to fetch the schedule data once on startup
//                        end     
//=======================================================

void handleScheduleUpdate(const char *topic, const String &message)
{
    Serial.print("📡 MQTT Schedule Update received on topic: ");
    Serial.println(topic);
    Serial.print("📡 Message: ");
    Serial.println(message);

    // Parse topic to determine which schedule parameter to update
    String topicStr = String(topic);
    topicStr.trim();
    topicStr.toLowerCase();
    Serial.print("🔍 Raw topic: '");
    Serial.print(topic);
    Serial.println("'");
    Serial.print("🔍 Normalized topic string: '");
    Serial.print(topicStr);
    Serial.println("'");
    Serial.print("🔍 Raw message: '");
    Serial.print(message);
    Serial.println("'");
    bool updateSuccessful = false;
    String firebasePath = "";

     if (topicStr.endsWith("control/schedule"))
     {
        if (topicStr == "am/scheduledTime")
        {
            String amTime = message;
            currentSchedule.amTime = amTime;
            Serial.print("[DEBUG] Extracted amTime from MQTT: ");
            Serial.println(amTime);
            if (isValidTime(amTime))
            {
                Serial.print("*********************");
                Serial.print("*********************");
                Serial.print("*********************");
                Serial.print(amTime);
                
                Serial.println("[DEBUG] setAMTime called from MQTT");
                setAMTime(amTime);
                updateFirebaseScheduleData("/schedule/amScheduledTime", amTime);
            }
            else
            {
                Serial.println("[DEBUG] amTime from MQTT is invalid");
            }
        }
        else if (topicStr == "pm/scheduledTime")
        {
            String pmTime = message;
            Serial.print("[DEBUG] Extracted pmTime from MQTT: ");
            Serial.println(pmTime);
            if (isValidTime(pmTime))
            {
                Serial.println("[DEBUG] setPMTime called from MQTT");
                setPMTime(pmTime);
                updateFirebaseScheduleData("/schedule/pmScheduledTime", pmTime);
            }
            else
            {
                Serial.println("[DEBUG] pmTime from MQTT is invalid");
            }
        }
        else if (topicStr == "am/temperature")
        {
            float amTemp = message.toFloat();
            Serial.print("[DEBUG] Extracted amTemp from MQTT: ");
            Serial.println(amTemp);
            if (isValidTemperature(amTemp))
            {
                Serial.println("[DEBUG] setAMTemperature called from MQTT");
                setAMTemperature(amTemp);
                updateFirebaseScheduleData("/schedule/amTemperature", String(amTemp));
            }
            else
            {
                Serial.println("[DEBUG] amTemp from MQTT is invalid");
            }
        }
        else if (topicStr == "pm/temperature")
        {
            float pmTemp = message.toFloat();
            Serial.print("[DEBUG] Extracted pmTemp from MQTT: ");
            Serial.println(pmTemp);
            if (isValidTemperature(pmTemp))
            {
                Serial.println("[DEBUG] setPMTemperature called from MQTT");
                setPMTemperature(pmTemp);
                updateFirebaseScheduleData("/schedule/pmTemperature", String(pmTemp));
            }
            else
            {
                Serial.println("[DEBUG] pmTemp from MQTT is invalid");
            }
        }
        else
        {
            Serial.println("❌ Unrecognized schedule control topic");
        }
     }
    // Individual topic handling (AM/PM temperature, time, enabled, scheduledTime)
    if (topicStr.endsWith("/am/temperature"))
    {
        float temp = message.toFloat();
        if (isValidTemperature(temp))
        {
            setAMTemperature(temp);
            updateSuccessful = true;
            firebasePath = "/schedule/amTemperature";
            Serial.println("✅ AM Temperature updated via MQTT");
        }
        else
        {
            Serial.println("❌ Invalid AM temperature received via MQTT");
        }
    }
    else if (topicStr.endsWith("/pm/temperature"))
    {
        float temp = message.toFloat();
        if (isValidTemperature(temp))
        {
            setPMTemperature(temp);
            updateSuccessful = true;
            firebasePath = "/schedule/pmTemperature";
            Serial.println("✅ PM Temperature updated via MQTT");
        }
        else
        {
            Serial.println("❌ Invalid PM temperature received via MQTT");
        }
    }
    else if (topicStr.endsWith("/am/time"))
    {
        if (isValidTime(message))
        {
            setAMTime(message);
            updateSuccessful = true;
            firebasePath = "/schedule/amScheduledTime";
            Serial.println("✅ AM Time updated via MQTT");
        }
        else
        {
            Serial.println("❌ Invalid AM time format received via MQTT");
        }
    }
    else if (topicStr.endsWith("/pm/time"))
    {
        if (isValidTime(message))
        {
            setPMTime(message);
            updateSuccessful = true;
            firebasePath = "/schedule/pmScheduledTime";
            Serial.println("✅ PM Time updated via MQTT");
        }
        else
        {
            Serial.println("❌ Invalid PM time format received via MQTT");
        }
    }
    else if (topicStr.endsWith("/am/enabled"))
    {
        bool enabled = (message == "true");
        currentSchedule.amEnabled = enabled;
        updateSuccessful = true;
        firebasePath = "/schedule/amEnabled";
        Serial.print("✅ AM Enabled updated via MQTT: ");
        Serial.println(enabled ? "true" : "false");
    }
    else if (topicStr.endsWith("/pm/enabled"))
    {
        bool enabled = (message == "true");
        currentSchedule.pmEnabled = enabled;
        updateSuccessful = true;
        firebasePath = "/schedule/pmEnabled";
        Serial.print("✅ PM Enabled updated via MQTT: ");
        Serial.println(enabled ? "true" : "false");
    }
    else if (topicStr.endsWith("/am/scheduledtime"))
    {
        if (isValidTime(message))
        {
            setAMTime(message);
            updateSuccessful = true;
            firebasePath = "/schedule/amScheduledTime";
            Serial.println("✅ AM ScheduledTime updated via MQTT");
        }
        else
        {
            Serial.println("❌ Invalid AM scheduledTime format received via MQTT");
        }
    }
    else if (topicStr.endsWith("/pm/scheduledtime"))
    {
        if (isValidTime(message))
        {
            setPMTime(message);
            updateSuccessful = true;
            firebasePath = "/schedule/pmScheduledTime";
            Serial.println("✅ PM ScheduledTime updated via MQTT");
        }
        else
        {
            Serial.println("❌ Invalid PM scheduledTime format received via MQTT");
        }
    }
    else
    {
        Serial.println("⚠️  Unknown schedule topic received");
    }

    // Update Firebase if MQTT update was successful
    if (updateSuccessful && firebasePath.length() > 0)
    {
        updateFirebaseScheduleData(firebasePath, message);
    }

    // Print updated schedule
    // printScheduleData();
}

// void printScheduleData()
// {
//     Serial.println("📅 Current Schedule:");

//     // Check AM data
//     if (currentSchedule.amTime.length() > 0 && !isnan(currentSchedule.amTemp))
//     {
//         Serial.print("   🌅 AM: ");
//         Serial.print(currentSchedule.amTime);
//         Serial.print(" → ");
//         Serial.print(currentSchedule.amTemp);
//         Serial.println("°C");
//     }
//     else
//     {
//         Serial.println("   🌅 AM: ❌ No data available");
//     }

//     // Check PM data
//     if (currentSchedule.pmTime.length() > 0 && !isnan(currentSchedule.pmTemp))
//     {
//         Serial.print("   🌆 PM: ");
//         Serial.print(currentSchedule.pmTime);
//         Serial.print(" → ");
//         Serial.print(currentSchedule.pmTemp);
//         Serial.println("°C");
//     }
//     else
//     {
//         Serial.println("   🌆 PM: ❌ No data available");
//     }
// }

bool isValidTime(const String &timeStr)
{
    // Check format HH:MM (5 characters)
    if (timeStr.length() != 5 || timeStr.charAt(2) != ':')
    {
        return false;
    }

    // Extract hours and minutes
    int hours = timeStr.substring(0, 2).toInt();
    int minutes = timeStr.substring(3, 5).toInt();

    // Validate ranges
    return (hours >= 0 && hours <= 23 && minutes >= 0 && minutes <= 59);
}

bool isValidTemperature(float temp)
{
    // Allow reasonable temperature range (0-50°C)
    return (temp >= 0.0 && temp <= 50.0 && !isnan(temp));
}

// Getter functions
float getAMTemperature()
{
    if (isnan(currentSchedule.amTemp))
    {
        Serial.println("⚠️  Warning: AM Temperature not set - returning NaN");
    }
    return currentSchedule.amTemp;
}

float getPMTemperature()
{
    if (isnan(currentSchedule.pmTemp))
    {
        Serial.println("⚠️  Warning: PM Temperature not set - returning NaN");
    }
    return currentSchedule.pmTemp;
}

String getAMTime()
{
    if (currentSchedule.amTime.length() == 0)
    {
        Serial.println("⚠️  Warning: AM Time not set - returning empty string");
    }
    return currentSchedule.amTime;
}

String getPMTime()
{
    if (currentSchedule.pmTime.length() == 0)
    {
        Serial.println("⚠️  Warning: PM Time not set - returning empty string");
    }
    return currentSchedule.pmTime;
}

// Setter functions
void setAMTemperature(float temp)
{
    Serial.print("[DEBUG] setAMTemperature called with value: ");
    Serial.println(temp);
    if (isValidTemperature(temp))
    {
        currentSchedule.amTemp = temp;
        Serial.print("🔄 AM Temperature set to: ");
        Serial.print(temp);
        Serial.println("°C");
    }
    else
    {
        Serial.println("❌ Invalid AM temperature provided");
    }
}

void setPMTemperature(float temp)
{
    Serial.print("[DEBUG] setPMTemperature called with value: ");
    Serial.println(temp);
    if (isValidTemperature(temp))
    {
        currentSchedule.pmTemp = temp;
        Serial.print("🔄 PM Temperature set to: ");
        Serial.print(temp);
        Serial.println("°C");
    }
    else
    {
        Serial.println("❌ Invalid PM temperature provided");
    }
}

void setAMTime(const String &time)
{
    Serial.print("[DEBUG] setAMTime called with value: ");
    Serial.println(time);
    if (isValidTime(time))
    {
        currentSchedule.amTime = time;
        Serial.print("🔄 AM Time set to: ");
        Serial.println(time);
    }
    else
    {
        Serial.println("❌ Invalid AM time format provided");
    }
}

void setPMTime(const String &time)
{
    Serial.print("[DEBUG] setPMTime called with value: ");
    Serial.println(time);
    if (isValidTime(time))
    {
        currentSchedule.pmTime = time;
        Serial.print("🔄 PM Time set to: ");
        Serial.println(time);
    }
    else
    {
        Serial.println("❌ Invalid PM time format provided");
    }
}

float getCurrentScheduledTemperature()
{
    // Get current time to determine if we should use AM or PM temperature
    extern int Hours;   // From TimeManager
    extern bool AmFlag; // From HeaterControl

    // Use the same logic as heater control to determine AM/PM
    // If AmFlag is true, use AM temperature, otherwise use PM temperature
    if (AmFlag)
    {
        if (!isnan(currentSchedule.amTemp))
        {
            return currentSchedule.amTemp;
        }
        else
        {
            Serial.println("⚠️  Warning: AM temperature not available");
            return NAN;
        }
    }
    else
    {
        if (!isnan(currentSchedule.pmTemp))
        {
            return currentSchedule.pmTemp;
        }
        else
        {
            Serial.println("⚠️  Warning: PM temperature not available");
            return NAN;
        }
    }
}

String formatTime(int hours, int minutes)
{
    String timeStr = "";
    if (hours < 10)
        timeStr += "0";
    timeStr += String(hours);
    timeStr += ":";
    if (minutes < 10)
        timeStr += "0";
    timeStr += String(minutes);
    return timeStr;
}

// Function to update Firebase schedule data
void updateFirebaseScheduleData(const String &path, const String &value)
{
    // This function updates Firebase with schedule changes received via MQTT
    // Use the existing Firebase data object from FirebaseService
    extern FirebaseData fbData;

    // Try to update Firebase - if it fails, it will handle the error gracefully
    if (Firebase.RTDB.setString(&fbData, path.c_str(), value))
    {
        Serial.print("✅ Firebase schedule updated: ");
        Serial.print(path);
        Serial.print(" = ");
        Serial.println(value);
    }
    else
    {
        Serial.print("❌ Failed to update Firebase schedule: ");
        Serial.print(path);
        Serial.print(" - Error: ");
        Serial.println(fbData.errorReason());
    }
}