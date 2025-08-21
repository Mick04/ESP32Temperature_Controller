// ================================
// Heater Control Functions
// ================================

#ifndef HEATERCONTROL_H
#define HEATERCONTROL_H

#include <Arduino.h>
#include "TimeManager.h"

// Function declarations
void updateHeaterControl();
void refreshScheduleCache(); // Force refresh of cached schedule values
void getTime();
#endif // HEATERCONTROL_H
