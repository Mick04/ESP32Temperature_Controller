// ==================================================
// File: src/WiFiManagerCustom.h
// ==================================================

#pragma once
#include <Arduino.h>
#include "config.h"

struct SystemStatus;
void initWiFi(SystemStatus &status);
void handleWiFi(SystemStatus &status);