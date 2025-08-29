#include <Arduino.h>
#include "config.h"
#include <WiFi.h>

#include "WiFiManagerCustom.h"
#include "StatusLEDs.h"
#include "FirebaseService.h"
#include "TemperatureSensors.h"
#include "TimeManager.h"
#include "MQTTManager.h"
#include "GetShedual.h"
#include "HeaterControl.h"

// put function declarations here:
int myFunction(int, int);
void updateHeaterControl();
bool AmFlag;
bool firstRun = true;
// Global system status
SystemStatus systemStatus;

void setup()
{
  Serial.begin(115200);
  delay(1000); // Wait for Serial to initialize

  Serial.println("\n=== ESP32 Temperature Controller Starting ===");
  Serial.print("Free heap: ");
  Serial.print(ESP.getFreeHeap());
  Serial.println(" bytes");
  Serial.print("Free PSRAM: ");
  Serial.print(ESP.getFreePsram());
  Serial.println(" bytes");

  // Initialize system status
  systemStatus.heater = HEATER_OFF; // Start with heater off

  // Initialize relay pin for heater control
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // Start with heater off

  Serial.println("✅ Basic hardware initialized");

  // Initialize Status LEDs first
  initStatusLEDs();
  Serial.println("✅ Status LEDs initialized");

  // Initialize temperature sensors
  initTemperatureSensors();
  Serial.println("✅ Temperature sensors initialized");

  // // Initialize schedule manager
  // initScheduleManager();
  // Serial.println("✅ Schedule manager initialized");

  // Initialize WiFi
  initWiFi(systemStatus);
  Serial.println("✅ WiFi initialization started");

  Serial.print("Free heap after setup: ");
  Serial.print(ESP.getFreeHeap());
  Serial.println(" bytes");

  // Initialize MQTT, Time Manager (after WiFi)
  // Will be called in loop when WiFi is connected

  // Note: Firebase, MQTT and TimeManager will be initialized automatically when WiFi connects
  // via handleFirebase(), handleMQTT() and handleTimeManager() in the main loop
}

void loop()
{
  long rssi = WiFi.RSSI();
  Serial.print("********===========********Signal strength (RSSI): ");
  Serial.print(rssi);
  Serial.print(" dBm");

  // Handle WiFi connection status
  handleWiFi(systemStatus);

  // Handle Firebase connection status (will initialize when WiFi is ready)
  static bool firebaseInitialized = false;
  if (systemStatus.wifi == CONNECTED && !firebaseInitialized)
  {
    // Initialize Firebase immediately after WiFi connection
    Serial.println("🔥 WiFi connected! Initializing Firebase...");
    initFirebase(systemStatus);
    firebaseInitialized = true;
  }

  if (firebaseInitialized)
  {
    handleFirebase(systemStatus);
  }

  // Handle time management (will initialize when WiFi is ready)
  static bool timeManagerInitialized = false;
  if (systemStatus.wifi == CONNECTED && !timeManagerInitialized)
  {
    initTimeManager();
    timeManagerInitialized = true;
  }
  if (timeManagerInitialized)
  {
    handleTimeManager();
  }

  // Handle MQTT connection (will initialize when WiFi is ready)
  static bool mqttInitialized = false;
  if (systemStatus.wifi == CONNECTED && !mqttInitialized)
  {
    initMQTT();
    mqttInitialized = true;
  }
  if (mqttInitialized)
  {
    handleMQTT(); // This calls mqttClient.loop() internally
    systemStatus.mqtt = getMQTTStatus();
  }

  // Update heater control
  updateHeaterControl();
  // Update LED status indicators
  updateLEDs(systemStatus);

  // If Firebase is connected, check for changes and push data when needed
  static unsigned long lastFirebaseCheck = 0;

  if (systemStatus.firebase == FB_CONNECTED && millis() - lastFirebaseCheck > 5000) // Check every 5 seconds
  {
    // Read all temperature sensors first
    readAllSensors();

    // Only push sensor data to Firebase if there are changes
    if (checkTemperatureChanges())
    {
      Serial.println("\n=== Firebase Push (Sensor Change Detected) ===");

      // Push sensor data to Firebase (no fetching needed)
      pushSensorValuesToFirebase();

      Serial.println("=== End Firebase Push ===\n");
    }

    // Always check and push target temperature changes (independent of sensor changes)
    checkAndPushTargetTemperature();

    // Check for external target temperature changes (from React app)
    checkFirebaseTargetTemperatureChanges();

    // NOTE: Schedule data is only fetched once at startup via Firebase initialization
    // Future schedule updates will come via MQTT
    // No need to fetch sensor data back from Firebase

    // Update heater control based on temperature and local schedule settings

    lastFirebaseCheck = millis();
  }

  // If MQTT is connected, check for temperature changes and publish when needed
  static unsigned long lastMQTTCheck = 0;
  if (systemStatus.mqtt == MQTT_STATE_CONNECTED && millis() - lastMQTTCheck > 5000) // Check every 5 seconds
  {
    // Read all temperature sensors first
    readAllSensors();

    // Check if any temperature values have changed
    if (checkTemperatureChanges())
    {
      Serial.println("\n=== MQTT Publish (Temperature Change Detected) ===");

      // Publish sensor data (includes time and system data)
      publishSensorData();

      Serial.println("=== End MQTT Publish ===\n");
    }

    lastMQTTCheck = millis();
  }

  // Small delay to prevent watchdog resets and excessive CPU usage
  delay(50);

  // Periodic memory monitoring (every 30 seconds)
  static unsigned long lastMemoryCheck = 0;
  if (millis() - lastMemoryCheck > 30000)
  {
    Serial.print("Free heap: ");
    Serial.print(ESP.getFreeHeap());
    Serial.println(" bytes");
    lastMemoryCheck = millis();
  }
}

// put function definitions here:
int myFunction(int x, int y)
{
  return x + y;
}