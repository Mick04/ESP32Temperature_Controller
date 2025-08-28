// ==================================================
// File: src/FirebaseService.cpp
// ==================================================

#include "FirebaseService.h"
#include "config.h"
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "TemperatureSensors.h"
#include "GetShedual.h"

// External variable declarations for debugging
extern ScheduleData currentSchedule;
extern bool AmFlag;
extern int Hours;

// Provide the RTC and API key for RTDB
FirebaseData fbData; // Made non-static so other files can access it
static FirebaseConfig fbConfig;
static FirebaseAuth fbAuth;
static bool fbInitialized = false;
static bool initialScheduleFetched = false; // Track initial schedule fetch

void setFirebaseOnlineStatus()
{
    Serial.println("🤠🤠🤠🤠🤠🤠🤠🤠🤠🤠🤠 Line 26 void setFirebaseOnlineStatus()...");
    Serial.println(" ");
    if (!fbInitialized)
        return;
    FirebaseJson json;
    json.set("status", "online");
    json.set("last_seen", millis()); // You can use a real timestamp if available
    Firebase.RTDB.setJSON(&fbData, "/system/device_status", &json);
}

void setFirebaseOfflineStatus()
{
    Serial.println("😈😈😈😈😈😈😈😈😈😈😈😈😈😈 Line 37 void setFirebaseOfflineStatus()...");
    Serial.println(" ");
    if (!fbInitialized)
        return;
    FirebaseJson json;
    json.set("status", "offline");
    json.set("last_seen", millis());
    Firebase.RTDB.setJSON(&fbData, "/system/device_status", &json);
}

void updateFirebaseLastSeen()
{
    Serial.println("👹👹👹👹👹👹👹👹👹👹👹👹👹👹👹 Line 48 void updateFirebaseLastSeen()...");
    Serial.println(" ");
    if (!fbInitialized)
        return;
    Firebase.RTDB.setInt(&fbData, "/system/device_status/last_seen", millis());
}

void initFirebase(SystemStatus &status)
{
    Serial.println("👺👺👺👺👺👺👺👺👺👺👺👺👺👺👺👺👺 Line 56 void initFirebase(SystemStatus &status)...");
    Serial.println(" ");
    // Initialize only when WiFi connected
    if (WiFi.status() != WL_CONNECTED)
    {
        status.firebase = FB_CONNECTING;
        Serial.println("WiFi not connected, cannot initialize Firebase");
        return;
    }

    Serial.println("Initializing Firebase...");

    // Debug print credentials (remove in production)
    Serial.println("Firebase credentials:");
    Serial.print("API Key: ");
    Serial.println(FIREBASE_API_KEY);
    Serial.print("Database URL: ");
    Serial.println(FIREBASE_DATABASE_URL);

    // Clear any previous configuration
    fbConfig = FirebaseConfig();
    fbAuth = FirebaseAuth();

    // Set the API key and database URL
    fbConfig.api_key = FIREBASE_API_KEY;
    fbConfig.database_url = FIREBASE_DATABASE_URL;

    // Set the host explicitly (extract from database URL)
    fbConfig.host = "esp32-heater-controler-6d11f-default-rtdb.europe-west1.firebasedatabase.app";

    // Set timeout and retry settings
    fbConfig.timeout.serverResponse = 15 * 1000;   // 15 seconds
    fbConfig.timeout.socketConnection = 15 * 1000; // 15 seconds

    // For anonymous authentication, we need to sign in after initialization
    Serial.println("Attempting anonymous authentication...");

    // Initialize Firebase first
    Firebase.begin(&fbConfig, &fbAuth);
    Firebase.reconnectWiFi(true);

    // Wait for initialization
    delay(3000);

    // Try anonymous authentication
    Serial.println("Signing in anonymously...");
    if (Firebase.signUp(&fbConfig, &fbAuth, "", ""))
    {
        Serial.println("Anonymous sign-up successful");
    }
    else
    {
        Serial.print("Anonymous sign-up failed: ");
        Serial.println("Check Firebase project settings for anonymous auth");
    }

    // Wait longer for authentication
    delay(2000);

    // Test the connection immediately
    Serial.println("Testing Firebase connection...");

    // Try to write a simple test value instead of just checking ready()
    if (Firebase.RTDB.setString(&fbData, "/test/connection", "esp32_test"))
    {
        fbInitialized = true;
        status.firebase = FB_CONNECTED;
        Serial.println("Firebase initialized and connected successfully");
        Serial.println("Test write successful");

        // Set device online status in Firebase (LWT-like)
        setFirebaseOnlineStatus();

        // Now try to read back the data we just wrote
        Serial.println("Testing data retrieval...");
        if (Firebase.RTDB.getString(&fbData, "/test/connection"))
        {
            String retrievedValue = fbData.stringData();
            Serial.print("Retrieved value: ");
            Serial.println(retrievedValue);

            // Test reading a timestamp
            if (Firebase.RTDB.setTimestamp(&fbData, "/test/last_connection"))
            {
                Serial.println("Timestamp written successfully");
                if (Firebase.RTDB.getInt(&fbData, "/test/last_connection"))
                {
                    int timestamp = fbData.intData();
                    Serial.print("Connection timestamp: ");
                    Serial.println(timestamp);
                }
            }

            // Immediately fetch schedule data from Firebase on startup
            Serial.println("🚀 Fetching initial schedule data from Firebase...");
            fetchScheduleDataFromFirebase();
            initialScheduleFetched = true;
            Serial.println("✅ Initial schedule fetch completed. Future updates will come via MQTT.");
        }
        else
        {
            Serial.println("Read test failed:");
            Serial.print("Error: ");
            Serial.println(fbData.errorReason());
        }
    }
    else
    {
        // Try a different approach - check if we can read instead
        Serial.println("Write failed, trying read test...");
        if (Firebase.ready())
        {
            fbInitialized = true;
            status.firebase = FB_CONNECTED;
            Serial.println("Firebase ready - assuming connection is good");
        }
        else
        {
            // Don't set fbInitialized = false here - we'll try again later
            status.firebase = FB_ERROR;
            Serial.println("Firebase initialization failed - will retry later");
            Serial.print("Error: ");
            Serial.println(fbData.errorReason());
            Serial.print("HTTP Code: ");
            Serial.println(fbData.httpCode());
        }
    }
}

/**
 * Main Firebase connection handler - manages Firebase initialization and connection monitoring
 * This function should be called regularly in the main loop to maintain Firebase connectivity
 *
 * @param status Reference to SystemStatus struct to update Firebase connection state
 *
 * Responsibilities:
 * 1. Attempts Firebase initialization when not yet initialized and WiFi is connected
 * 2. Monitors ongoing Firebase connection health
 * 3. Updates system status flags for Firebase connectivity
 * 4. Implements rate limiting to prevent excessive initialization attempts
 */
void handleFirebase(SystemStatus &status)
{
    Serial.println("🤡🤡🤡🤡🤡🤡🤡🤡🤡🤡🤡🤡🤡🤡🤡🤡🤡🤡 Line 198 handleFirebase...");
    Serial.println(" ");
    // === FIREBASE INITIALIZATION PHASE ===
    // If Firebase hasn't been initialized yet, try to initialize it
    if (!fbInitialized)
    {
        // Only attempt initialization if WiFi is connected
        if (WiFi.status() == WL_CONNECTED)
        {
            // Rate limit initialization attempts to avoid spam (every 30 seconds)
            static unsigned long lastInitAttempt = 0;
            if (millis() - lastInitAttempt > 30000)
            {
                Serial.println("****WiFi connected, initializing Firebase...");
                initFirebase(status); // Attempt Firebase initialization
                lastInitAttempt = millis();
            }
        }
        else
        {
            // WiFi not connected - set status and wait
            status.firebase = FB_CONNECTING;

            // Only print this message occasionally to avoid spam (every 5 seconds)
            static unsigned long lastWiFiMessage = 0;
            if (millis() - lastWiFiMessage > 5000)
            {
                Serial.println("WiFi not connected, waiting for connection...");
                lastWiFiMessage = millis();
            }
        }
        return; // Exit early if not initialized
    }

    // === CONNECTION MONITORING PHASE ===
    // Firebase is initialized, now monitor its connection health

    // Rate limit Firebase status checks to avoid excessive polling (every 10 seconds)
    static unsigned long lastStatusCheck = 0;
    if (millis() - lastStatusCheck < 10000)
    {
        return; // Skip this check cycle
    }
    lastStatusCheck = millis();

    // Check current Firebase connection status
    if (Firebase.ready())
    {
        // Firebase is ready and connected
        if (status.firebase != FB_CONNECTED)
        {
            // Status changed from disconnected to connected - log the recovery
            Serial.println("Firebase connected successfully");
        }
        status.firebase = FB_CONNECTED;
    }
    else
    {
        // Firebase connection is down
        status.firebase = FB_ERROR;
        Serial.println("Firebase connection lost");
    }
}

void pushSensorValuesToFirebase()
{
    Serial.println("💩💩💩💩💩💩💩💩💩💩💩💩💩💩💩💩 Line 263 pushSensorValuesToFirebase...");
    Serial.println(" ");
    if (!fbInitialized)
    {
        Serial.println("Firebase not initialized, cannot push data");
        return;
    }

    Serial.println("Pushing sensor data to Firebase...");

    // Get real temperature readings from sensors
    float temp0 = getTemperature(0); // Red sensor
    float temp1 = getTemperature(1); // Blue sensor
    float temp2 = getTemperature(2); // Green sensor

    // Calculate average temperature from connected sensors
    /*
    // float avgTemp = 0;
    // int validSensors = 0;

    // if (!isnan(temp0))
    // {
    //     avgTemp += temp0;
    //     validSensors++;
    // }
    // if (!isnan(temp1))
    // {
    //     avgTemp += temp1;
    //     validSensors++;
    // }
    // if (!isnan(temp2))
    // {
    //     avgTemp += temp2;
    //     validSensors++;
    // }

    // if (validSensors > 0)
    // {
    //     avgTemp /= validSensors;
    // }
    // else
    // {
    //     avgTemp = NAN; // No valid sensors
    // }
*/
    // Push individual sensor temperatures
    if (!isnan(temp0))
    {
        int roundedTemp0 = round(temp0);
        if (Firebase.RTDB.setInt(&fbData, "/sensors/temperature_red", roundedTemp0))
        {
            Serial.print("Red temperature pushed: ");
            Serial.print(roundedTemp0);
            Serial.println("°C");
        }
    }

    if (!isnan(temp1))
    {
        int roundedTemp1 = round(temp1);
        if (Firebase.RTDB.setInt(&fbData, "/sensors/temperature_blue", roundedTemp1))
        {
            Serial.print("Blue temperature pushed: ");
            Serial.print(roundedTemp1);
            Serial.println("°C");
        }
    }

    if (!isnan(temp2))
    {
        int roundedTemp2 = round(temp2);
        if (Firebase.RTDB.setInt(&fbData, "/sensors/temperature_green", roundedTemp2))
        {
            Serial.print("Green temperature pushed: ");
            Serial.print(roundedTemp2);
            Serial.println("°C");
        }
    }

    // Push current target temperature from schedule system (FORCE PUSH MODE)
    static float lastTargetTemp = NAN;

    // Check if schedule data has been loaded from Firebase
    Serial.println("🔍 Checking if schedule data is available...");
    bool scheduleDataLoaded = (!isnan(currentSchedule.amTemp) && !isnan(currentSchedule.pmTemp) &&
                               currentSchedule.amTime.length() > 0 && currentSchedule.pmTime.length() > 0);

    Serial.print("📊 Schedule data loaded: ");
    Serial.println(scheduleDataLoaded ? "YES" : "NO");

    // if (!scheduleDataLoaded)
    // {
    //     Serial.println("⚠️  Schedule data not yet loaded from Firebase - skipping target temperature push");
    //     Serial.println("🔄 Attempting to fetch schedule data...");
    //     fetchScheduleDataFromFirebase();
    //     return; // Skip temperature push until data is loaded
    // }

    float currentTarget = getCurrentScheduledTemperature();

    // Debug: Always show target temperature comparison
    Serial.print("🎯 Target temp check - Current: ");
    Serial.print(currentTarget);
    Serial.print("°C, Last: ");
    Serial.print(isnan(lastTargetTemp) ? "NaN" : String(lastTargetTemp));
    Serial.print("°C, Difference: ");
    if (!isnan(lastTargetTemp) && !isnan(currentTarget))
    {
        Serial.print(abs(currentTarget - lastTargetTemp));
    }
    else
    {
        Serial.print("N/A");
    }
    Serial.println("°C");

    // SMART PUSH: Push when temperature changes OR periodically for sync
    if (!isnan(currentTarget))
    {
        int roundedTarget = round(currentTarget);
        static unsigned long lastPushTime = 0;
        unsigned long currentTime = millis();
        bool shouldPush = false;

        // Push if temperature changed by more than 0.1°C
        if (isnan(lastTargetTemp) || abs(currentTarget - lastTargetTemp) > 0.1)
        {
            shouldPush = true;
            Serial.print("� Target temperature changed: ");
            Serial.print(roundedTarget);
            Serial.println("°C - pushing update");
        }
        // Or push periodically every 5 minutes to ensure Firebase stays in sync
        else if (currentTime - lastPushTime > 300000) // 5 minutes
        {
            shouldPush = true;
            Serial.print("🔄 Periodic sync: ");
            Serial.print(roundedTarget);
            Serial.println("°C - pushing for sync");
        }

        if (shouldPush)
        {
            Serial.print("🔗 Attempting Firebase push to /control/target_temperature with value: ");
            Serial.println(roundedTarget);

            if (Firebase.RTDB.setInt(&fbData, "/control/target_temperature", roundedTarget))
            {
                Serial.print("✅ Target temperature pushed successfully: ");
                Serial.print(roundedTarget);
                Serial.println("°C");
                lastTargetTemp = currentTarget; // Remember this value
                lastPushTime = currentTime;     // Remember when we pushed
            }
            else
            {
                Serial.println("❌ Failed to push target temperature to Firebase");
                Serial.print("Firebase error: ");
                Serial.println(fbData.errorReason());
                Serial.print("HTTP Code: ");
                Serial.println(fbData.httpCode());
                Serial.print("Firebase ready: ");
                Serial.println(Firebase.ready() ? "YES" : "NO");
            }
        }
        else
        {
            Serial.print("⏭️  Target temperature unchanged (");
            Serial.print(roundedTarget);
            Serial.println("°C) - skipping push");
        }
    }
    else
    {
        Serial.println("⚠️  No valid target temperature to push - currentTarget is NaN");
        Serial.println("🔍 Debugging schedule data availability...");

        // Debug schedule data
        Serial.print("🕐 Current Hours: ");
        Serial.println(Hours);
        Serial.print("🌅 AmFlag: ");
        Serial.println(AmFlag ? "true (AM period)" : "false (PM period)");
        Serial.print("🌅 AM Temp: ");
        Serial.println(isnan(currentSchedule.amTemp) ? "NaN" : String(currentSchedule.amTemp));
        Serial.print("🌆🌆🌆🌆🌆🌆🌆🌆🌆🌆 PM Temp: ");
        Serial.println(isnan(currentSchedule.pmTemp) ? "NaN" : String(currentSchedule.pmTemp));
        Serial.print("🌅 AM Time: ");
        Serial.println(currentSchedule.amTime.length() > 0 ? currentSchedule.amTime : "Empty");
        Serial.print("🌆 PM Time: ");
        Serial.println(currentSchedule.pmTime.length() > 0 ? currentSchedule.pmTime : "Empty");
    } // Push system status
    String statusPath = "/system/status";
    if (Firebase.RTDB.setString(&fbData, statusPath.c_str(), "online"))
    {
        Serial.println("System status pushed: online");
    }

    // Push timestamp
    String timestampPath = "/system/last_update";
    if (Firebase.RTDB.setTimestamp(&fbData, timestampPath.c_str()))
    {
        Serial.println("Timestamp updated");
    }
}

void checkAndPushTargetTemperature()
{
    Serial.println("👻👻👻👻👻👻👻👻👻👻👻👻👻👻👻👻 Line 469 checkAndPushTargetTemperature...");
    Serial.println(" ");
    if (!fbInitialized)
    {
        Serial.println("Firebase not initialized, cannot push target temperature");
        return;
    }

    // Static variables to track state
    static float lastTargetTemp = NAN;
    static unsigned long lastPushTime = 0;

    // Check if schedule data has been loaded from Firebase
    Serial.println("🔍 Checking if schedule data is available...");
    bool scheduleDataLoaded = (!isnan(currentSchedule.amTemp) && !isnan(currentSchedule.pmTemp) &&
                               currentSchedule.amTime.length() > 0 && currentSchedule.pmTime.length() > 0);

    Serial.print("📊 Schedule data loaded: ");
    Serial.println(scheduleDataLoaded ? "YES" : "NO");

    if (!scheduleDataLoaded)
    {
        Serial.println("⚠️  Schedule data not yet loaded from Firebase - skipping target temperature push");
        Serial.println("🔄 Attempting to fetch schedule data...");
        fetchScheduleDataFromFirebase();
        return; // Skip temperature push until data is loaded
    }

    float currentTarget = getCurrentScheduledTemperature();

    // Debug: Always show target temperature comparison
    Serial.print("🎯 Target temp check - Current: ");
    Serial.print(currentTarget);
    Serial.print("°C, Last: ");
    Serial.print(isnan(lastTargetTemp) ? "NaN" : String(lastTargetTemp));
    Serial.print("°C, Difference: ");
    if (!isnan(lastTargetTemp) && !isnan(currentTarget))
    {
        Serial.print(abs(currentTarget - lastTargetTemp));
    }
    else
    {
        Serial.print("N/A");
    }
    Serial.println("°C");

    // SMART PUSH: Push when temperature changes OR periodically for sync
    if (!isnan(currentTarget))
    {
        int roundedTarget = round(currentTarget);
        unsigned long currentTime = millis();
        bool shouldPush = false;

        // Push if temperature changed by more than 0.1°C
        if (isnan(lastTargetTemp) || abs(currentTarget - lastTargetTemp) > 0.1)
        {
            shouldPush = true;
            Serial.print("🔄 Target temperature changed: ");
            Serial.print(roundedTarget);
            Serial.println("°C - pushing update");
        }
        // Or push periodically every 5 minutes to ensure Firebase stays in sync
        else if (currentTime - lastPushTime > 300000) // 5 minutes
        {
            shouldPush = true;
            Serial.print("🔄 Periodic sync: ");
            Serial.print(roundedTarget);
            Serial.println("°C - pushing for sync");
        }

        if (shouldPush)
        {
            Serial.print("🔗 Attempting Firebase push to /control/target_temperature with value: ");
            Serial.println(roundedTarget);

            if (Firebase.RTDB.setInt(&fbData, "/control/target_temperature", roundedTarget))
            {
                Serial.print("✅ Target temperature pushed successfully: ");
                Serial.print(roundedTarget);
                Serial.println("°C");
                lastTargetTemp = currentTarget; // Remember this value
                lastPushTime = currentTime;     // Remember when we pushed
            }
            else
            {
                Serial.println("❌ Failed to push target temperature to Firebase");
                Serial.print("Firebase error: ");
                Serial.println(fbData.errorReason());
                Serial.print("HTTP Code: ");
                Serial.println(fbData.httpCode());
                Serial.print("Firebase ready: ");
                Serial.println(Firebase.ready() ? "YES" : "NO");
            }
        }
        else
        {
            Serial.print("⏭️  Target temperature unchanged (");
            Serial.print(roundedTarget);
            Serial.println("°C) - skipping push");
        }
    }
    else
    {
        Serial.println("⚠️  No valid target temperature to push - currentTarget is NaN");
        Serial.println("🔍 Debugging schedule data availability...");

        // Debug schedule data
        Serial.print("🕐 Current Hours: ");
        Serial.println(Hours);
        Serial.print("🌅 AmFlag: ");
        Serial.println(AmFlag ? "true (AM period)" : "false (PM period)");
        Serial.print("🌅 AM Temp: ");
        Serial.println(isnan(currentSchedule.amTemp) ? "NaN" : String(currentSchedule.amTemp));
        Serial.print("🌆 PM Temp: ");
        Serial.println(isnan(currentSchedule.pmTemp) ? "NaN" : String(currentSchedule.pmTemp));
        Serial.print("🌅 AM Time: ");
        Serial.println(currentSchedule.amTime.length() > 0 ? currentSchedule.amTime : "Empty");
        Serial.print("🌆 PM Time: ");
        Serial.println(currentSchedule.pmTime.length() > 0 ? currentSchedule.pmTime : "Empty");
    }
}

void checkFirebaseTargetTemperatureChanges()
{
    Serial.println("💀💀💀💀💀💀💀💀💀💀💀💀💀💀💀 Line 592 checkFirebaseTargetTemperatureChanges...");
    Serial.println(" ");
    if (!fbInitialized)
    {
        return; // Skip if Firebase not ready
    }

    // Rate limit checks to every 30 seconds to avoid excessive Firebase reads
    static unsigned long lastCheck = 0;
    unsigned long currentTime = millis();
    if (currentTime - lastCheck < 30000) // 30 seconds
    {
        return;
    }
    lastCheck = currentTime;

    // Read current target temperature from Firebase
    if (Firebase.RTDB.getInt(&fbData, "/control/target_temperature"))
    {
        int firebaseTarget = fbData.intData();
        float currentScheduledTarget = getCurrentScheduledTemperature();

        Serial.print("🔍 Firebase target check - Firebase: ");
        Serial.print(firebaseTarget);
        Serial.print("°C, Schedule: ");
        Serial.print(isnan(currentScheduledTarget) ? "NaN" : String(currentScheduledTarget));
        Serial.println("°C");

        // Check if Firebase target differs from our scheduled target by more than 1°C
        if (!isnan(currentScheduledTarget) && abs(firebaseTarget - currentScheduledTarget) > 1.0)
        {
            Serial.print("🚨 EXTERNAL CHANGE DETECTED! Firebase target (");
            Serial.print(firebaseTarget);
            Serial.print("°C) differs from schedule (");
            Serial.print(currentScheduledTarget);
            Serial.println("°C) - React app may have changed it");

            // Note: In a full implementation, you might want to:
            // 1. Temporarily override the schedule
            // 2. Send an MQTT notification
            // 3. Log the manual override
            // For now, we'll just log it and let the schedule continue
        }
        else
        {
            Serial.println("✅ Firebase target matches schedule - no external changes");
        }
    }
    else
    {
        Serial.println("⚠️  Could not read target temperature from Firebase");
    }
}

void fetchControlValuesFromFirebase()
{
    Serial.println("🤡🤡🤡🤡🤡🤡🤡🤡🤡🤡🤡🤡🤡🤡🤡🤡🤡 Line 647 fetchControlValuesFromFirebase...");
    Serial.println(" ");
    if (!fbInitialized)
    {
        Serial.println("Firebase not initialized, cannot fetch data");
        return;
    }

    Serial.println("Fetching sensor data from Firebase for verification...");

    // Read current sensor data back for verification only
    if (Firebase.RTDB.getInt(&fbData, "/sensors/temperature"))
    {
        int currentTemp = fbData.intData();
        Serial.print("Average temperature reading: ");
        Serial.print(currentTemp);
        Serial.println("°C");
    }

    // Read individual sensor temperatures
    if (Firebase.RTDB.getInt(&fbData, "/sensors/temperature_red"))
    {
        int redTemp = fbData.intData();
        Serial.print("Red sensor reading: ");
        Serial.print(redTemp);
        Serial.println("°C");
    }

    if (Firebase.RTDB.getInt(&fbData, "/sensors/temperature_blue"))
    {
        int blueTemp = fbData.intData();
        Serial.print("Blue sensor reading: ");
        Serial.print(blueTemp);
        Serial.println("°C");
    }

    if (Firebase.RTDB.getInt(&fbData, "/sensors/temperature_green"))
    {
        int greenTemp = fbData.intData();
        Serial.print("Green sensor reading: ");
        Serial.print(greenTemp);
        Serial.println("°C");
    }

    if (Firebase.RTDB.getFloat(&fbData, "/sensors/current"))
    {
        float currentReading = fbData.floatData();
        Serial.print("Current sensor reading: ");
        Serial.print(currentReading);
        Serial.println("A");
    }

    // Read system status
    if (Firebase.RTDB.getString(&fbData, "/system/status"))
    {
        String systemStatus = fbData.stringData();
        Serial.print("System status: ");
        Serial.println(systemStatus);
    }

    // Read last update timestamp
    if (Firebase.RTDB.getInt(&fbData, "/system/last_update"))
    {
        int lastUpdate = fbData.intData();
        Serial.print("Last update timestamp: ");
        Serial.println(lastUpdate);
    }
}

// Helper functions to set control values
void setControlValue(const char *path, float value)
{
    Serial.println("👽👽👽👽👽👽👽👽👽👽👽👽👽 Line 727 void setControlValue(const char *path, float value)...");
    Serial.println(" ");
    if (!fbInitialized)
    {
        Serial.println("Firebase not initialized, cannot set control value");
        return;
    }

    if (Firebase.RTDB.setFloat(&fbData, path, value))
    {
        Serial.print("Control value set: ");
        Serial.print(path);
        Serial.print(" = ");
        Serial.println(value);
    }
    else
    {
        Serial.print("Failed to set control value: ");
        Serial.println(path);
    }
}

void setControlValue(const char *path, bool value)
{
    Serial.println("👾👾👾👾👾👾👾👾👾👾👾👾👾👾👾👾👾 Line 751 void setControlValue...");
    Serial.println(" ");
    if (!fbInitialized)
    {
        Serial.println("Firebase not initialized, cannot set control value");
        return;
    }

    if (Firebase.RTDB.setBool(&fbData, path, value))
    {
        Serial.print("Control value set: ");
        Serial.print(path);
        Serial.print(" = ");
        Serial.println(value ? "true" : "false");
    }
    else
    {
        Serial.print("Failed to set control value: ");
        Serial.println(path);
    }
}

void setControlValue(const char *path, const char *value)
{
    Serial.println("🤖🤖🤖🤖🤖🤖🤖🤖🤖🤖🤖🤖🤖🤖🤖 Line 775 void setControlValue...");
    Serial.println(" ");
    if (!fbInitialized)
    {
        Serial.println("Firebase not initialized, cannot set control value");
        return;
    }

    if (Firebase.RTDB.setString(&fbData, path, value))
    {
        Serial.print("Control value set: ");
        Serial.print(path);
        Serial.print(" = ");
        Serial.println(value);
    }
    else
    {
        Serial.print("Failed to set control value: ");
        Serial.println(path);
    }
}

// Schedule data management functions
bool isInitialScheduleFetched()
{
    Serial.println("🎃🎃🎃🎃🎃🎃🎃🎃🎃🎃🎃🎃🎃 Line 800 bool isInitialScheduleFetched()...");
    return initialScheduleFetched;
}

void markInitialScheduleAsFetched()
{
    initialScheduleFetched = true;
}
