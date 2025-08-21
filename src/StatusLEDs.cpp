// ==================================================
// File: src/StatusLEDs.cpp
// ==================================================

#include "StatusLEDs.h"

// Define the LED array
CRGB leds[NUM_LEDS];

void initStatusLEDs()
{
    FastLED.addLeds<LED_TYPE, WS2811_PIN, COLOR_ORDER>(leds, NUM_LEDS);
    FastLED.setBrightness(50); // Set brightness to 50/255
    // FastLED.clear();
    FastLED.show();
}

void updateLEDs(SystemStatus &status)
{
    // Clear all LEDs first
    // FastLED.clear();

    // Set WiFi LED based on status
    switch (status.wifi)
    {
    case CONNECTING:
        leds[LED_WIFI] = CRGB::Blue;
        break;
    case CONNECTED:
        leds[LED_WIFI] = CRGB::Green;
        break;
    case ERROR:
        leds[LED_WIFI] = CRGB::Red;
        break;
    }

    // Set Firebase LED based on status
    switch (status.firebase)
    {
    case FB_CONNECTING:
        leds[LED_FIREBASE] = CRGB::Blue;
        break;
    case FB_CONNECTED:
        leds[LED_FIREBASE] = CRGB::Green;
        break;
    case FB_ERROR:
        leds[LED_FIREBASE] = CRGB::Red;
        break;
    }

    // Set MQTT LED based on status
    switch (status.mqtt)
    {
    case MQTT_STATE_DISCONNECTED:
        leds[LED_MQTT] = CRGB::Black; // Off
        break;
    case MQTT_STATE_CONNECTING:
        leds[LED_MQTT] = CRGB::Blue;
        break;
    case MQTT_STATE_CONNECTED:
        leds[LED_MQTT] = CRGB::Green;
        break;
    case MQTT_STATE_ERROR:
        leds[LED_MQTT] = CRGB::Red;
        break;
    }

    // Set Heater LED based on status
    switch (status.heater)
    {
    case HEATER_OFF:
        leds[LED_HEATER] = CRGB::Green; // Off
        break;
    case HEATER_ON:
        leds[LED_HEATER] = CRGB::Red; // Heating
        break;
    case HEATER_AUTO:
        leds[LED_HEATER] = CRGB::Yellow; // Auto mode
        break;
    case HEATER_ERROR:
        leds[LED_HEATER] = CRGB::Red; // Error
        break;
    }

    FastLED.show();
}

// Helper to turn on just one LED at a time
void showSingleLed(int index, CRGB color)
{
    // FastLED.clear();
    leds[index] = color;
    FastLED.show();
}
