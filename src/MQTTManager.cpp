// ==================================================
// File: src/MQTTManager.cpp
// ==================================================

#include "MQTTManager.h"
#include "TemperatureSensors.h"
#include "TimeManager.h"
#include "GetShedual.h"
#include "HeaterControl.h"
#include <WiFi.h>
#include <ArduinoJson.h>

// MQTT Client setup
WiFiClientSecure wifiClientSecure;
PubSubClient mqttClient(wifiClientSecure);

// Global MQTT status
MQTTState mqttStatus = MQTT_STATE_DISCONNECTED;

// Client ID for MQTT connection
String clientId = "ESP32-TemperatureController-";

// Previous temperature values for change detection
static float prevTempRed = NAN;
static float prevTempBlue = NAN;
static float prevTempGreen = NAN;
static bool firstReading = true;

void initMQTT()
{
    Serial.println("Initializing MQTT Manager...");

    // Check WiFi connection
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("WiFi not connected, cannot initialize MQTT");
        mqttStatus = MQTT_STATE_DISCONNECTED;
        return;
    }

    // Generate unique client ID to avoid conflicts with React app
    clientId = "ESP32_TempController_";
    clientId += String(WiFi.macAddress());
    clientId.replace(":", "");
    clientId += "_" + String(millis()); // Add timestamp for absolute uniqueness

    Serial.print("🆔 MQTT Client ID: ");
    Serial.println(clientId);

    // Configure secure WiFi client for TLS connection
    wifiClientSecure.setInsecure(); // For testing - in production, use proper certificates

    // CRITICAL FIX: Set MQTT server and callback with enhanced registration
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT_TLS);

    // Set callback MULTIPLE times to ensure it sticks
    mqttClient.setCallback(onMQTTMessage);
    delay(50);
    mqttClient.setCallback(onMQTTMessage); // Double registration for safety

    Serial.println("✅ MQTT callback registered (double-confirmed)");

    // Set buffer size for larger messages and keepalive
    mqttClient.setBufferSize(512);
    mqttClient.setKeepAlive(60); // 60 second keepalive

    // Test callback registration immediately
    Serial.println("✅ MQTT callback registered");

    Serial.println("MQTT Manager initialized");
}

void handleMQTT()
{
    // Add debug to verify this function is being called
    static unsigned long lastDebugHandleMQTT = 0;
    if (millis() - lastDebugHandleMQTT > 60000) // Debug every 60 seconds
    {
        Serial.println("🔧 handleMQTT() function called");
        lastDebugHandleMQTT = millis();
    }

    // Don't proceed if WiFi is not connected
    if (WiFi.status() != WL_CONNECTED)
    {
        mqttStatus = MQTT_STATE_DISCONNECTED;
        return;
    }

    // Try to connect if not connected
    if (!mqttClient.connected())
    {
        mqttStatus = MQTT_STATE_CONNECTING;
        if (connectToMQTT())
        {
            mqttStatus = MQTT_STATE_CONNECTED;
        }
        else
        {
            mqttStatus = MQTT_STATE_ERROR;
            // Rate limit connection attempts
            static unsigned long lastConnectAttempt = 0;
            if (millis() - lastConnectAttempt > 30000) // Try every 30 seconds
            {
                Serial.println("MQTT connection failed, will retry in 30 seconds");
                lastConnectAttempt = millis();
            }
        }
    }
    else
    {
        mqttStatus = MQTT_STATE_CONNECTED;
        // Keep the connection alive and process incoming messages
        // Call loop() more frequently for better message processing
        mqttClient.loop();

        static unsigned long lastLoopDebug = 0;
        if (millis() - lastLoopDebug > 30000) // Debug every 30 seconds
        {
            Serial.println("🔄 MQTT client loop() running, waiting for messages...");
            lastLoopDebug = millis();
        }
    }
}

bool connectToMQTT()
{
    Serial.print("Connecting to MQTT broker: ");
    Serial.println(MQTT_SERVER);

    // Attempt to connect with credentials
    if (mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD))
    {
        Serial.println("MQTT connected successfully!");

        // CRITICAL: Re-register callback after connection
        mqttClient.setCallback(onMQTTMessage);
        Serial.println("🔄 Callback re-registered after connection");

        // Subscribe to control topics (for receiving commands) with QoS 1 for guaranteed delivery
        Serial.println("📡 Subscribing to MQTT topics with QoS 1...");
        if (mqttClient.subscribe("esp32/control/+", 1))
        {
            Serial.println("✅ Subscribed to esp32/control/+ (QoS 1)");
        }
        else
        {
            Serial.println("❌ Failed to subscribe to esp32/control/+");
        }

        if (mqttClient.subscribe("esp32/commands/+", 1))
        {
            Serial.println("✅ Subscribed to esp32/commands/+ (QoS 1)");
        }
        else
        {
            Serial.println("❌ Failed to subscribe to esp32/commands/+");
        }

        // Subscribe to schedule topics to match React app format with QoS 1
        if (mqttClient.subscribe(TOPIC_CONTROL_AM_TEMP, 1))
        {
            Serial.println("✅ Subscribed to " TOPIC_CONTROL_AM_TEMP " (QoS 1)");
        }
        else
        {
            Serial.println("❌ Failed to subscribe to " TOPIC_CONTROL_AM_TEMP);
        }

        if (mqttClient.subscribe(TOPIC_CONTROL_PM_TEMP, 1))
        {
            Serial.println("✅ Subscribed to " TOPIC_CONTROL_PM_TEMP " (QoS 1)");
        }
        else
        {
            Serial.println("❌ Failed to subscribe to " TOPIC_CONTROL_PM_TEMP);
        }

        if (mqttClient.subscribe(TOPIC_CONTROL_AM_TIME, 1))
        {
            Serial.println("✅ Subscribed to " TOPIC_CONTROL_AM_TIME " (QoS 1)");
        }
        else
        {
            Serial.println("❌ Failed to subscribe to " TOPIC_CONTROL_AM_TIME);
        }

        if (mqttClient.subscribe(TOPIC_CONTROL_PM_TIME, 1))
        {
            Serial.println("✅ Subscribed to " TOPIC_CONTROL_PM_TIME " (QoS 1)");
        }
        else
        {
            Serial.println("❌ Failed to subscribe to " TOPIC_CONTROL_PM_TIME);
        }

        // Subscribe to JSON schedule updates with QoS 1
        if (mqttClient.subscribe(TOPIC_CONTROL_SCHEDULE, 1))
        {
            Serial.println("✅ Subscribed to " TOPIC_CONTROL_SCHEDULE " (QoS 1)");
        }
        else
        {
            Serial.println("❌ Failed to subscribe to " TOPIC_CONTROL_SCHEDULE);
        }

        // Subscribe to additional React app topics with QoS 1
        if (mqttClient.subscribe(TOPIC_CONTROL_AM_ENABLED, 1))
        {
            Serial.println("✅ Subscribed to " TOPIC_CONTROL_AM_ENABLED " (QoS 1)");
        }
        if (mqttClient.subscribe(TOPIC_CONTROL_PM_ENABLED, 1))
        {
            Serial.println("✅ Subscribed to " TOPIC_CONTROL_PM_ENABLED " (QoS 1)");
        }
        if (mqttClient.subscribe(TOPIC_CONTROL_PM_SCHEDULED_TIME, 1))
        {
            Serial.println("✅ Subscribed to " TOPIC_CONTROL_PM_SCHEDULED_TIME " (QoS 1)");
        }

        // Publish connection status
        publishSingleValue(TOPIC_STATUS, "online");

        // Add debug info about connection
        Serial.print("✅ MQTT Client State: ");
        Serial.println(mqttClient.state());
        Serial.println("📡 All subscriptions complete");

        // CRITICAL TEST: Send a self-test message to trigger callback with QoS 1
        Serial.println("🧪 Sending self-test message to verify callback...");
        delay(1000); // Give subscriptions time to settle

        // Call loop() multiple times to ensure message processing
        for (int i = 0; i < 5; i++)
        {
            mqttClient.loop();
            delay(100);
        }

        mqttClient.publish("esp32/commands/status", "SELF_TEST_CALLBACK", true); // Retained message with QoS
        Serial.println("📤 Self-test message sent with QoS, callback should trigger...");

        // Process the message immediately
        for (int i = 0; i < 10; i++)
        {
            mqttClient.loop();
            delay(50);
        }

        return true;
    }
    else
    {
        Serial.print("MQTT connection failed, rc=");
        Serial.print(mqttClient.state());
        Serial.println(" retrying...");
        return false;
    }
}

void publishSensorData()
{
    if (mqttStatus != MQTT_STATE_CONNECTED)
    {
        Serial.println("MQTT not connected, cannot publish sensor data");
        return;
    }

    // Get temperature readings
    float tempRed = getTemperature(0);   // Red sensor
    float tempBlue = getTemperature(1);  // Blue sensor
    float tempGreen = getTemperature(2); // Green sensor

    // Check if any temperature has changed (or this is the first reading)
    // Using 0.1°C as the minimum change threshold to avoid noise
    bool hasChanged = firstReading ||
                      (abs(tempRed - prevTempRed) > 0.1 && !isnan(tempRed)) ||
                      (abs(tempBlue - prevTempBlue) > 0.1 && !isnan(tempBlue)) ||
                      (abs(tempGreen - prevTempGreen) > 0.1 && !isnan(tempGreen)) ||
                      (isnan(tempRed) != isnan(prevTempRed)) ||
                      (isnan(tempBlue) != isnan(prevTempBlue)) ||
                      (isnan(tempGreen) != isnan(prevTempGreen));

    if (!hasChanged)
    {
        // Only print this message every 30 seconds to avoid spam
        static unsigned long lastNoChangeMessage = 0;
        if (millis() - lastNoChangeMessage > 30000)
        {
            Serial.println("📊 No significant temperature changes detected");
            lastNoChangeMessage = millis();
        }
        return;
    }

    Serial.println("🌡️  Temperature change detected, publishing sensor data to MQTT...");

    // Update previous values
    prevTempRed = tempRed;
    prevTempBlue = tempBlue;
    prevTempGreen = tempGreen;
    firstReading = false;

    // Calculate average temperature
    float avgTemp = 0;
    int validSensors = 0;

    if (!isnan(tempRed))
    {
        publishSingleValue(TOPIC_TEMP_RED, (float)(round(tempRed * 10) / 10.0)); // Round to 1 decimal place
        avgTemp += tempRed;
        validSensors++;
    }
    else
    {
        publishSingleValue(TOPIC_TEMP_RED, "ERROR");
    }

    if (!isnan(tempBlue))
    {
        publishSingleValue(TOPIC_TEMP_BLUE, (float)(round(tempBlue * 10) / 10.0)); // Round to 1 decimal place
        avgTemp += tempBlue;
        validSensors++;
    }
    else
    {
        publishSingleValue(TOPIC_TEMP_BLUE, "ERROR");
    }

    if (!isnan(tempGreen))
    {
        publishSingleValue(TOPIC_TEMP_GREEN, (float)(round(tempGreen * 10) / 10.0)); // Round to 1 decimal place
        avgTemp += tempGreen;
        validSensors++;
    }
    else
    {
        publishSingleValue(TOPIC_TEMP_GREEN, "ERROR");
    }

    // Publish average temperature
    if (validSensors > 0)
    {
        avgTemp /= validSensors;
        publishSingleValue(TOPIC_TEMP_AVG, (float)(round(avgTemp * 10) / 10.0)); // Round to 1 decimal place
        Serial.print("📊 Average temperature: ");
        Serial.print(avgTemp);
        Serial.print("°C (from ");
        Serial.print(validSensors);
        Serial.println(" sensors)");
    }
    else
    {
        publishSingleValue(TOPIC_TEMP_AVG, "ERROR");
        Serial.println("⚠️  No valid temperature sensors available");
    }

    // Publish dummy current data (until current sensor is implemented)
    float dummyCurrent = random(0, 100) / 10.0; // Random current 0-10A
    publishSingleValue(TOPIC_CURRENT, dummyCurrent);

    // Also publish time and system data when temperature changes
    publishTimeData();
    publishSystemData();
}

void publishTimeData()
{
    if (mqttStatus != MQTT_STATE_CONNECTED)
    {
        Serial.println("MQTT not connected, cannot publish time data");
        return;
    }

    Serial.println("Publishing time data to MQTT...");

    // Get formatted time and date
    String timeStr = getFormattedTime();
    String dateStr = getFormattedDate();

    publishSingleValue(TOPIC_TIME, timeStr.c_str());
    publishSingleValue(TOPIC_DATE, dateStr.c_str());
}

void publishSystemData()
{
    if (mqttStatus != MQTT_STATE_CONNECTED)
    {
        Serial.println("MQTT not connected, cannot publish system data");
        return;
    }

    Serial.println("Publishing system data to MQTT...");

    // Publish WiFi signal strength
    int rssi = WiFi.RSSI();
    publishSingleValue(TOPIC_WIFI_RSSI, rssi);

    // Publish uptime in seconds
    unsigned long uptime = millis() / 1000;
    publishSingleValue(TOPIC_UPTIME, (int)uptime);

    // Publish system status
    publishSingleValue(TOPIC_STATUS, "online");
}

void publishSingleValue(const char *topic, float value)
{
    if (mqttStatus != MQTT_STATE_CONNECTED)
        return;

    String payload = String(value, 2); // 2 decimal places
    if (mqttClient.publish(topic, payload.c_str()))
    {
        Serial.print("Published to ");
        Serial.print(topic);
        Serial.print(": ");
        Serial.println(payload);
    }
    else
    {
        Serial.print("Failed to publish to ");
        Serial.println(topic);
    }
}

void publishSingleValue(const char *topic, int value)
{
    if (mqttStatus != MQTT_STATE_CONNECTED)
        return;

    String payload = String(value);
    if (mqttClient.publish(topic, payload.c_str()))
    {
        Serial.print("Published to ");
        Serial.print(topic);
        Serial.print(": ");
        Serial.println(payload);
    }
    else
    {
        Serial.print("Failed to publish to ");
        Serial.println(topic);
    }
}

void publishSingleValue(const char *topic, const char *value)
{
    if (mqttStatus != MQTT_STATE_CONNECTED)
        return;

    if (mqttClient.publish(topic, value))
    {
        Serial.print("Published to ");
        Serial.print(topic);
        Serial.print(": ");
        Serial.println(value);
    }
    else
    {
        Serial.print("Failed to publish to ");
        Serial.println(topic);
    }
}

void parseAndUpdateScheduleJson(const String &jsonMessage)
{
    Serial.println("===================================");
    Serial.println("");
    Serial.println("🔍 Parsing JSON schedule data...");

    // Use the new JsonDocument instead of StaticJsonDocument
    JsonDocument doc;

    // Parse JSON
    DeserializationError error = deserializeJson(doc, jsonMessage);

    if (error)
    {
        Serial.print("❌ JSON parsing failed: ");
        Serial.println(error.c_str());
        return;
    }

    Serial.println("✅ JSON parsed successfully");

    bool dataUpdated = false;

    // Extract AM schedule data
    if (doc["am"])
    {
        JsonObject am = doc["am"];

        // Update AM temperature
        if (am["temperature"])
        {
            float amTemp = am["temperature"];
            if (isValidTemperature(amTemp))
            {
                setAMTemperature(amTemp);
                Serial.print("✅ AM Temperature updated to: ");
                Serial.print(amTemp);
                Serial.println("°C");
                dataUpdated = true;

                // Update Firebase
                updateFirebaseScheduleData("/schedule/amTemperature", String(amTemp));
            }
            else
            {
                Serial.println("❌ Invalid AM temperature in JSON");
            }
        }

        // Update AM time
        if (am["scheduledTime"])
        {
            String amTime = am["scheduledTime"];
            if (isValidTime(amTime))
            {
                setAMTime(amTime);
                Serial.print("✅ AM Time updated to: ");
                Serial.println(amTime);
                dataUpdated = true;

                // Update Firebase
                updateFirebaseScheduleData("/schedule/amScheduledTime", amTime);
            }
            else
            {
                Serial.println("❌ Invalid AM time format in JSON");
            }
        }
    }

    // Extract PM schedule data
    if (doc["pm"])
    {
        JsonObject pm = doc["pm"];

        // Update PM temperature
        if (pm["temperature"])
        {
            float pmTemp = pm["temperature"];
            if (isValidTemperature(pmTemp))
            {
                setPMTemperature(pmTemp);
                Serial.print("✅ PM Temperature updated to: ");
                Serial.print(pmTemp);
                Serial.println("°C");
                dataUpdated = true;

                // Update Firebase
                updateFirebaseScheduleData("/schedule/pmTemperature", String(pmTemp));
            }
            else
            {
                Serial.println("❌ Invalid PM temperature in JSON");
            }
        }

        // Update PM time
        if (pm["scheduledTime"])
        {
            String pmTime = pm["scheduledTime"];
            if (isValidTime(pmTime))
            {
                setPMTime(pmTime);
                Serial.print("✅ PM Time updated to: ");
                Serial.println(pmTime);
                dataUpdated = true;

                // Update Firebase
                updateFirebaseScheduleData("/schedule/pmScheduledTime", pmTime);
            }
            else
            {
                Serial.println("❌ Invalid PM time format in JSON");
            }
        }
    }

    if (dataUpdated)
    {
        Serial.println("📅 Schedule updated from JSON:");
        printScheduleData();

        // CRITICAL: Force schedule cache refresh immediately after JSON update
        refreshScheduleCache();
        Serial.println("🔄 Schedule cache force-refreshed after JSON update");
    }
    else
    {
        Serial.println("⚠️  No valid schedule data found in JSON");
    }
    Serial.println("===================================");
    Serial.println("");
}

void onMQTTMessage(char *topic, byte *payload, unsigned int length)
{
    Serial.println("===================================");
    Serial.println("");
    // CRITICAL: Add immediate debug output FIRST before any processing
    Serial.println("🚨🚨🚨 CALLBACK ENTRY POINT HIT! 🚨🚨🚨");
    Serial.flush(); // Force immediate output

    // Add immediate debug output to verify callback is triggered
    Serial.println("🚨 MQTT CALLBACK TRIGGERED! 🚨");
    Serial.print("Callback called with topic: ");
    Serial.println(topic ? topic : "NULL");
    Serial.print("Payload length: ");
    Serial.println(length);
    Serial.flush(); // Force immediate output

    // Convert payload to string
    String message = "";
    for (int i = 0; i < length; i++)
    {
        message += (char)payload[i];
    }

    Serial.print("MQTT message received on topic: ");
    Serial.println(topic);
    Serial.print("*****************Message: ");
    Serial.println(message);

    // Handle incoming MQTT commands here
    String topicStr = String(topic);

    Serial.print("🔍 Processing topic: '");
    Serial.print(topicStr);
    Serial.println("'");

    if (topicStr.startsWith("esp32/schedule/"))
    {
        // Handle schedule updates via MQTT (individual fields)
        Serial.println("📡 Schedule update received via MQTT");
        handleScheduleUpdate(topic, message);
    }
    else if (topicStr.startsWith("esp32/control/"))
    {
        Serial.println("🎯 Matched esp32/control/ prefix");
        // Handle control commands
        if (topicStr.endsWith("schedule"))
        {
            // Handle JSON schedule data
            Serial.println("📡 JSON Schedule update received via MQTT");
            parseAndUpdateScheduleJson(message);
        }
        else if (topicStr.startsWith("esp32/control/schedule/"))
        {
            // Handle individual schedule field updates from React app
            Serial.println("📡 Individual schedule field update received via MQTT");

            Serial.print("🔍 Checking exact topic match for: ");
            Serial.println(topicStr);

            if (topicStr == TOPIC_CONTROL_AM_TEMP)
            {
                Serial.println("✅ Matched AM temperature topic");
                float temp = message.toFloat();
                Serial.print("🌡️  Parsed temperature: ");
                Serial.println(temp);
                if (isValidTemperature(temp))
                {
                    setAMTemperature(temp);
                    Serial.print("✅ AM Temperature updated via MQTT: ");
                    Serial.println(temp);

                    // CRITICAL: Force schedule cache refresh immediately
                    refreshScheduleCache();
                    Serial.println("🔄 Schedule cache force-refreshed after AM temp update");
                }
                else
                {
                    Serial.println("❌ Invalid temperature value");
                }
            }
            else if (topicStr == TOPIC_CONTROL_PM_TEMP)
            {
                Serial.println("✅ Matched PM temperature topic");
                float temp = message.toFloat();
                Serial.print("🌡️  Parsed temperature: ");
                Serial.println(temp);
                if (isValidTemperature(temp))
                {
                    setPMTemperature(temp);
                    Serial.print("✅ PM Temperature updated via MQTT: ");
                    Serial.println(temp);

                    // CRITICAL: Force schedule cache refresh immediately
                    refreshScheduleCache();
                    Serial.println("🔄 Schedule cache force-refreshed after PM temp update");
                }
                else
                {
                    Serial.println("❌ Invalid temperature value");
                }
            }
            else if (topicStr == TOPIC_CONTROL_AM_TIME || topicStr == "esp32/control/schedule/am/scheduledTime")
            {
                Serial.println("✅ Matched AM time topic");
                Serial.print("🕐 Parsed time: ");
                Serial.println(message);
                if (isValidTime(message))
                {
                    setAMTime(message);
                    Serial.print("✅ AM Time updated via MQTT: ");
                    Serial.println(message);

                    // CRITICAL: Force schedule cache refresh immediately
                    refreshScheduleCache();
                    Serial.println("🔄 Schedule cache force-refreshed after AM time update");
                }
                else
                {
                    Serial.println("❌ Invalid time format");
                }
            }
            else if (topicStr == TOPIC_CONTROL_PM_TIME || topicStr == "esp32/control/schedule/pm/scheduledTime")
            {
                Serial.println("✅ Matched PM time topic");
                Serial.print("🕐 Parsed time: ");
                Serial.println(message);
                if (isValidTime(message))
                {
                    setPMTime(message);
                    Serial.print("✅ PM Time updated via MQTT: ");
                    Serial.println(message);

                    // CRITICAL: Force schedule cache refresh immediately
                    refreshScheduleCache();
                    Serial.println("🔄 Schedule cache force-refreshed after PM time update");
                }
                else
                {
                    Serial.println("❌ Invalid time format");
                }
            }
        }
        else if (topicStr.endsWith("target_temperature"))
        {
            float targetTemp = message.toFloat();
            Serial.print("Setting target temperature to: ");
            Serial.println(targetTemp);
            // You can integrate this with Firebase or local control
        }
        else if (topicStr.endsWith("heater_enable"))
        {
            bool enable = (message == "true" || message == "1");
            Serial.print("Setting heater enable to: ");
            Serial.println(enable);
            // You can integrate this with relay control
        }
    }
    else if (topicStr.startsWith("esp32/commands/"))
    {
        // Handle system commands
        if (topicStr.endsWith("restart"))
        {
            Serial.println("Restart command received");
            ESP.restart();
        }
        else if (topicStr.endsWith("status"))
        {
            Serial.println("Status request received");
            publishSystemData();
            publishSensorData();
            publishTimeData();
        }
    }
    Serial.println("===================================");
    Serial.println("");
}

MQTTState getMQTTStatus()
{
    return mqttStatus;
}

bool checkTemperatureChanges()
{
    // Get current temperature readings
    float tempRed = getTemperature(0);   // Red sensor
    float tempBlue = getTemperature(1);  // Blue sensor
    float tempGreen = getTemperature(2); // Green sensor

    // Check if any temperature has changed (or this is the first reading)
    // Using 1.0°C as the minimum change threshold to avoid noise
    bool hasChanged = firstReading ||
                      (abs(tempRed - prevTempRed) > 1.0 && !isnan(tempRed)) ||
                      (abs(tempBlue - prevTempBlue) > 1.0 && !isnan(tempBlue)) ||
                      (abs(tempGreen - prevTempGreen) > 1.0 && !isnan(tempGreen)) ||
                      (isnan(tempRed) != isnan(prevTempRed)) ||
                      (isnan(tempBlue) != isnan(prevTempBlue)) ||
                      (isnan(tempGreen) != isnan(prevTempGreen));

    return hasChanged;
}
