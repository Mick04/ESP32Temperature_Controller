// ==================================================
// File: src/FirebaseService.h
// ==================================================

#pragma once
#include <Arduino.h>
#include "config.h"
#include <Firebase_ESP_Client.h>

// External Firebase data object
extern FirebaseData fbData;

struct SystemStatus;
void initFirebase(SystemStatus &status);
void handleFirebase(SystemStatus &status);
void pushSensorValuesToFirebase();
void checkAndPushTargetTemperature();
void checkFirebaseTargetTemperatureChanges();
void fetchControlValuesFromFirebase();
void setControlValue(const char *path, float value);
void setControlValue(const char *path, bool value);
void setControlValue(const char *path, const char *value);

// Schedule data management
bool isInitialScheduleFetched();
void markInitialScheduleAsFetched();
