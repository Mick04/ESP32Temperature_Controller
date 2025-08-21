// ==================================================
// File: src/GetShedual.h
// ==================================================

#ifndef GETSHEDUAL_H
#define GETSHEDUAL_H

#include <Arduino.h>

// Schedule structure to hold AM and PM values
struct ScheduleData
{
    float amTemp;  // Default AM temperature
    float pmTemp;  // Default PM temperature
    String amTime; // Default AM time
    String pmTime; // Default PM time
};

// Global schedule data
extern ScheduleData currentSchedule;

// Function declarations
void initScheduleManager();
void handleScheduleUpdate(const char *topic, const String &message);
void printScheduleData();
bool isValidTime(const String &timeStr);
bool isValidTemperature(float temp);

// Getter functions
float getAMTemperature();
float getPMTemperature();
String getAMTime();
String getPMTime();

// Setter functions (for manual updates or defaults)
void setAMTemperature(float temp);
void setPMTemperature(float temp);
void setAMTime(const String &time);
void setPMTime(const String &time);

// Firebase integration
void fetchScheduleDataFromFirebase();
void updateFirebaseScheduleData(const String &path, const String &value);
float getCurrentScheduledTemperature();
String formatTime(int hours, int minutes);

#endif // GETSHEDUAL_H
