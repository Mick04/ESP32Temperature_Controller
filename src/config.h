// ==================================================
// File: src/config.h
// ==================================================

#pragma once

// === WiFi State Management Start ===
enum WiFiState
{
    CONNECTING,
    CONNECTED,
    ERROR
};

enum FirebaseState
{
    FB_CONNECTING,
    FB_CONNECTED,
    FB_ERROR
};

enum MQTTState
{
    MQTT_STATE_DISCONNECTED,
    MQTT_STATE_CONNECTING,
    MQTT_STATE_CONNECTED,
    MQTT_STATE_ERROR
};

enum HeaterState
{
    HEATER_OFF,
    HEATER_ON,
    HEATER_AUTO,
    HEATER_ERROR
};

struct SystemStatus
{
    WiFiState wifi;
    FirebaseState firebase;
    MQTTState mqtt;
    HeaterState heater;
    // Add other status fields as needed
};
//====WiFi State Management end ===

// === Wi-Fi Start ===
// #define WIFI_SSID "Gimp_EXT"
#define WIFI_SSID "Gimp"
#define WIFI_PASSWORD "FC7KUNPX"
// === Wi-Fi End ===

// === Firebase Start ===
#define FIREBASE_API_KEY "AIzaSyDkJinA0K6NqBLGR4KYnX8AdDNgXp2-FDI"
#define FIREBASE_DATABASE_URL "https://esp32-heater-controler-6d11f-default-rtdb.europe-west1.firebasedatabase.app/"
// === Firebase End ===

// === MQTT (HiveMQ) Start ===
#define MQTT_BROKER_HOST "broker.hivemq.com"
#define MQTT_BROKER_PORT 8884
#define MQTT_USERNAME "ESP32FireBaseTortoise"
#define MQTT_PASS "ESP32FireBaseHea1951Ter"

#define EMAIL_ALERT_ENABLED true // set to false to disable email alerts

//  === MQTT (HiveMQ) End    ===

/******************************
 *    Email Credentials       *
 *****************************/
#define SENDER_EMAIL "esp8266heaterapp@gmail.com"
#define SENDER_PASSWORD "uuyd ifav mpzd vuoj"
#define RECIPIENT_EMAIL "mac5y4@talktalk.net"
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 587

// === Pins ===
#define WS2811_PIN 25  // Pin for WS2811 LED strip
#define DS18B20_PIN 27 // Pin for DS18B20 temperature sensor (changed from 27)
#define RELAY_PIN 26   // Pin for relay control
#define SCT013_PIN 33  // Pin for SCT013 current sensor

// === LED Index Mapping ===
#define LED_WIFI 0
#define LED_FIREBASE 1
#define LED_MQTT 2
#define LED_HEATER 3

#include <FastLED.h>

// === Other ===
#define SENSOR_READ_INTERVAL 1000   // ms
#define FIREBASE_SYNC_INTERVAL 5000 // ms
