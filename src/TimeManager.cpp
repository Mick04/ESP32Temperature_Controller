// ==================================================
// File: src/TimeManager.cpp
// ==================================================

#include "TimeManager.h"
#include "FirebaseService.h"
#include "config.h"
#include <WiFi.h>
#include <Firebase_ESP_Client.h>

// Create NTP client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_SERVER, UTC_OFFSET_STANDARD, NTP_UPDATE_INTERVAL);

// Global time variables
int currentDay = 1;
int currentMonth = 1;
int Hours = 0;
int Minutes = 0;
int nextDay = 2;
bool asBeenSaved = false;

void initTimeManager()
{
    Serial.println("Initializing Time Manager...");

    // Wait for WiFi connection
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("WiFi not connected, cannot initialize time manager");
        return;
    }

    // Initialize NTP client with GMT offset (0)
    timeClient.begin();
    timeClient.setTimeOffset(0); // Set to GMT (UTC+0)
    timeClient.update();

    // Get initial time
    time_t epochTime = timeClient.getEpochTime();
    currentDay = day(epochTime);
    currentMonth = month(epochTime);
    nextDay = currentDay + 1;

    Serial.println("Time Manager initialized successfully (GMT)");
    Serial.print("Current date: ");
    Serial.print(currentDay);
    Serial.print("/");
    Serial.print(currentMonth);
    Serial.print("/");
    Serial.println(year(epochTime));

    // Print current GMT time for verification
    Serial.print("Current GMT time: ");
    Serial.print(hour(epochTime));
    Serial.print(":");
    Serial.println(minute(epochTime));
}

void handleTimeManager()
{
    // Update time periodically
    static unsigned long lastTimeUpdate = 0;
    if (millis() - lastTimeUpdate > 30000) // Update every 30 seconds
    {
        getTime();
        lastTimeUpdate = millis();
    }
}

/***************************************
 *   function to get the time          *
 *           start                     *
 ***************************************/
void getTime()
{
    timeClient.update();
    time_t epochTime = timeClient.getEpochTime();

    currentDay = day(epochTime);
    currentMonth = month(epochTime);

    int currentYear = year(epochTime);
    Hours = hour(epochTime);
    Minutes = minute(epochTime);
    int currentSecond = second(epochTime);
    String currentTime = getFormattedTime();

    // For GMT, we always use offset 0 (no DST adjustment)
    timeClient.setTimeOffset(0);

    // Print the complete date and time for debugging
    Serial.printf("GMT Date and Time: %02d-%02d-%04d %02d:%02d:%02d\n", currentDay, currentMonth, currentYear, Hours, Minutes, currentSecond);

    if (currentDay == nextDay)
    {
        if (currentMonth == 2) // February
        {
            Serial.println("February");
            // Check for leap year
            if ((year() % 4 == 0 && year() % 100 != 0) || (year() % 400 == 0)) // Leap year
            {
                if (currentDay > 28)
                {
                    Serial.println("line 594");
                    // publishTimeToMQTT();
                    storeDateToFirebase();
                    asBeenSaved = true;
                    nextDay = 1; // Reset nextDay to 1 for the next month
                }
            }
            else // Non-leap year
            {
                if (currentDay > 27)
                {
                    Serial.println("line 607");
                    // publishTimeToMQTT();
                    storeDateToFirebase();
                    asBeenSaved = true;
                    nextDay = 1; // Reset nextDay to 1 for the next month
                    Serial.println("Non-leap year February");
                }
            }
        }
        else if (currentMonth == 4 || currentMonth == 6 || currentMonth == 9 || currentMonth == 11) // Months with 30 days
        {
            if (currentDay > 29)
            {
                Serial.println("April, June, September, November");
                Serial.println("line 623");
                // publishTimeToMQTT();
                storeDateToFirebase();
                asBeenSaved = true;
                nextDay = 1; // Reset nextDay to 1 for the next month
            }
        }
        else // Months with 31 days
        {
            if (currentDay > 30)
            {
                Serial.println("All other months have 31 days");
                Serial.println("line 637");
                // publishTimeToMQTT();
                storeDateToFirebase();
                asBeenSaved = true;
                nextDay = 1; // Reset nextDay to 1 for the next month
            }
        }

        // Reset to January if the month exceeds December
        if (currentMonth > 12)
        {
            Serial.println("Resetting month to January");
            currentMonth = 1;
        }
        if (asBeenSaved == false)
        {
            // Store the date in Firebase
            storeDateToFirebase();
            // Publish the time to MQTT
            Serial.println("line 658");
            // publishTimeToMQTT();
            asBeenSaved = true;
            nextDay++; // Increment nextDay for the next day
        }
    }
}

bool isDST(int day, int month, int hour)
{
    // For GMT time, we don't apply DST adjustments
    // GMT stays constant year-round
    return false;
}

/***************************************
 *   function to get the time          *
 *           end                       *
 ***************************************/

void storeDateToFirebase()
{
    Serial.println("Storing date to Firebase...");

    // Get current timestamp
    time_t epochTime = timeClient.getEpochTime();

    // Store date components
    String datePath = "/system/date/";

    // Create date object with current values
    char dateBuffer[20];
    char timeBuffer[10];

    sprintf(dateBuffer, "%d/%d/%d", currentDay, currentMonth, year(epochTime));
    sprintf(timeBuffer, "%d:%02d", Hours, Minutes);

    String dateStr = String(dateBuffer);
    String timeStr = String(timeBuffer);

    // Store in Firebase (using external Firebase functions)
    setControlValue("/system/date/current", dateStr.c_str());
    setControlValue("/system/time/current", timeStr.c_str());
    setControlValue("/system/timestamp", (float)epochTime);

    Serial.print("Date stored: ");
    Serial.println(dateStr);
    Serial.print("Time stored: ");
    Serial.println(timeStr);
}

String getFormattedTime()
{
    char timeBuffer[10];
    sprintf(timeBuffer, "%02d:%02d", Hours, Minutes);  // Always 2 digits for hours
    return String(timeBuffer);
}

String getFormattedDate()
{
    time_t epochTime = timeClient.getEpochTime();
    char dateBuffer[20];
    sprintf(dateBuffer, "%d/%d/%d", currentDay, currentMonth, year(epochTime));
    return String(dateBuffer);
}
