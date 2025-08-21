// ==================================================
// File: src/TemperatureSensors.h
// ==================================================

#pragma once
#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "config.h"

// Function declarations
void initTemperatureSensors();
float getTemperature(int sensorIndex);
void readAllSensors();
int getConnectedSensorCount();

// External declarations for sensor objects
extern OneWire oneWire;
extern DallasTemperature sensors;
extern DeviceAddress red, blue, green;

 // Pin for OneWire bus
