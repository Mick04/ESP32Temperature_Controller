// ==================================================
// File: src/TimeManager.h
// ==================================================

#ifndef TIMEMANAGER_H
#define TIMEMANAGER_H

#include <WiFiUdp.h>
#include <NTPClient.h>
#include <TimeLib.h>

// Time configuration constants
#define UTC_OFFSET_STANDARD 0 // UTC+0 (GMT - Greenwich Mean Time)
#define UTC_OFFSET_DST 0      // UTC+0 (GMT - no DST adjustment for GMT)
#define NTP_SERVER "pool.ntp.org"
#define NTP_UPDATE_INTERVAL 60000 // Update every 60 seconds

// Global time variables
extern int currentDay;
extern int currentMonth;
extern int Hours;
extern int Minutes;
extern int nextDay;
extern bool asBeenSaved;

// Function declarations
void initTimeManager();
void getTime();
bool isDST(int day, int month, int hour);
void storeDateToFirebase();
void handleTimeManager();
String getFormattedTime();
String getFormattedDate();

#endif // TIMEMANAGER_H
