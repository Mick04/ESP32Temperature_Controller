// ==================================================
// File: src/StatusLEDs.h
// ==================================================

#pragma once
#include <Arduino.h>
#include <FastLED.h>
#include "config.h"

struct SystemStatus;

// Function declarations
void initStatusLEDs();
void updateLEDs(SystemStatus &status);
void showSingleLed(int index, CRGB color);

/*************************
 *   FastLED Definitions  *
 *       start           *
 ************************/
#define NUM_LEDS 4
#define LED_TYPE WS2811
#define COLOR_ORDER RGB
/*************************
 *   FastLED Definitions  *
 *       end             *
 ************************/

// External declaration of the LED array
extern CRGB leds[NUM_LEDS];
