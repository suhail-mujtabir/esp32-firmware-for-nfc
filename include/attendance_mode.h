#ifndef ATTENDANCE_MODE_H
#define ATTENDANCE_MODE_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_SSD1306.h>
#include "config.h"

// ==========================================
// ATTENDANCE MODE STATE
// ==========================================

enum AttendanceState {
  ATT_NO_EVENT,           // No event in memory, waiting for double tap
  ATT_FETCHING_EVENT,     // Fetching event from API
  ATT_READY,              // Ready to scan cards
  ATT_CHECKING_IN         // Processing check-in
};

// ==========================================
// FUNCTION DECLARATIONS
// ==========================================

void initAttendanceMode();
void runAttendanceMode(Adafruit_SSD1306& display, String cardUid);
void handleFetchButton(Adafruit_SSD1306& display, bool isLongPress);
void handleClearButton(Adafruit_SSD1306& display);
bool fetchActiveEvent();
bool checkInCard(String cardUid);
void clearActiveEvent();
String getActiveEventId();
String getActiveEventName();

// Display functions for attendance mode
void displayNoEvent(Adafruit_SSD1306& display);
void displayFetchingEvent(Adafruit_SSD1306& display);
void displayAttendanceReady(Adafruit_SSD1306& display, String eventName);
void displayCheckingIn(Adafruit_SSD1306& display, String uid);
void displayWelcome(Adafruit_SSD1306& display, String studentName);
void displayAttendanceError(Adafruit_SSD1306& display, String error);

#endif // ATTENDANCE_MODE_H
