#include "attendance_mode.h"
#include <WiFi.h>

// ==========================================
// STATE VARIABLES
// ==========================================

static AttendanceState currentState = ATT_NO_EVENT;
static String activeEventId = "";
static String activeEventName = "";
static String lastCheckedInStudent = ""; // Store last check-in for display
static int lastCheckInStatus = 0; // Store HTTP status code

// ==========================================
// INITIALIZATION
// ==========================================

void initAttendanceMode() {
  currentState = ATT_NO_EVENT;
  activeEventId = "";
  activeEventName = "";
  Serial.println("\n========================================");
  Serial.println("ATTENDANCE MODE INITIALIZED");
  Serial.println("========================================");
}

// ==========================================
// MAIN ATTENDANCE MODE LOGIC
// ==========================================

void runAttendanceMode(Adafruit_SSD1306& display, String cardUid) {
  // Only process card if we're in ready state and have a valid UID
  if (currentState == ATT_READY && cardUid.length() > 0) {
    currentState = ATT_CHECKING_IN;
    displayCheckingIn(display, cardUid);
    
    bool success = checkInCard(cardUid);
    
    if (success) {
      // Display welcome message with student name from checkInCard
      displayWelcome(display, lastCheckedInStudent);
      delay(CHECKIN_SUCCESS_DISPLAY);
    } else {
      // Check error type
      if (lastCheckInStatus == 409) {
        displayAttendanceError(display, "Already checked in");
      } else {
        displayAttendanceError(display, "Check-in failed");
      }
      delay(2000);
    }
    
    // Return to ready state
    currentState = ATT_READY;
    displayAttendanceReady(display, activeEventName);
  }
}

// ==========================================
// BUTTON HANDLING
// ==========================================

void handleFetchButton(Adafruit_SSD1306& display, bool isLongPress) {
  if (isLongPress) {
    // Long press is for mode switch (handled in main.cpp)
    Serial.println("Long press - mode switch");
    return;
  }
  
  // Short press - fetch event from API (only when no event loaded)
  if (currentState == ATT_NO_EVENT) {
    Serial.println("Fetch button pressed - getting active event");
    currentState = ATT_FETCHING_EVENT;
    displayFetchingEvent(display);
    
    if (fetchActiveEvent()) {
      currentState = ATT_READY;
      displayAttendanceReady(display, activeEventName);
      Serial.println("Event loaded successfully");
    } else {
      currentState = ATT_NO_EVENT;
      displayNoEvent(display);
      Serial.println("No active event found");
    }
  } else {
    Serial.println("Fetch button ignored - event already loaded");
  }
}

void handleClearButton(Adafruit_SSD1306& display) {
  // Clear event only if we're in ready state
  if (currentState == ATT_READY) {
    Serial.println("Clear button pressed - removing event");
    clearActiveEvent();
    currentState = ATT_NO_EVENT;
    displayNoEvent(display);
  } else {
    Serial.println("Clear button ignored - no event to clear");
  }
}

// ==========================================
// API FUNCTIONS
// ==========================================

bool fetchActiveEvent() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("✗ WiFi not connected!");
    return false;
  }
  
  HTTPClient http;
  String url = String(API_URL) + String(ENDPOINT_EVENTS_ACTIVE);
  
  Serial.println("GET " + url);
  
  http.begin(url);
  http.addHeader("x-device-api-key", DEVICE_API_KEY);
  http.setTimeout(API_TIMEOUT);
  
  int httpCode = http.GET();
  
  Serial.println("Response code: " + String(httpCode));
  
  if (httpCode == 200) {
    String response = http.getString();
    Serial.println("Response: " + response);
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);
    
    if (error) {
      Serial.println("✗ JSON parse error: " + String(error.c_str()));
      http.end();
      return false;
    }
    
    // Parse event data
    activeEventId = doc["event"]["id"].as<String>();
    activeEventName = doc["event"]["name"].as<String>();
    
    Serial.println("✅ Event loaded:");
    Serial.println("  ID: " + activeEventId);
    Serial.println("  Name: " + activeEventName);
    
    http.end();
    return true;
  }
  
  Serial.println("✗ No active event found");
  http.end();
  return false;
}

bool checkInCard(String cardUid) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("✗ WiFi not connected!");
    return false;
  }
  
  if (activeEventId.length() == 0) {
    Serial.println("✗ No active event!");
    return false;
  }
  
  HTTPClient http;
  String url = String(API_URL) + String(ENDPOINT_CHECK_IN);
  
  Serial.println("POST " + url);
  
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("x-device-api-key", DEVICE_API_KEY);
  http.setTimeout(API_TIMEOUT);
  
  // Create JSON payload
  JsonDocument doc;
  doc["uid"] = cardUid;
  doc["eventId"] = activeEventId;
  
  String payload;
  serializeJson(doc, payload);
  
  Serial.println("Sending: " + payload);
  
  int httpCode = http.POST(payload);
  
  Serial.println("Response code: " + String(httpCode));
  
  // Store status for error handling
  lastCheckInStatus = httpCode;
  
  if (httpCode == 409) {
    // Already checked in
    Serial.println("⚠️ Already checked in!");
    http.end();
    return false; // Return false to show error message
  }
  
  if (httpCode == 200) {
    String response = http.getString();
    Serial.println("Response: " + response);
    
    JsonDocument responseDoc;
    DeserializationError error = deserializeJson(responseDoc, response);
    
    if (error) {
      Serial.println("✗ JSON parse error: " + String(error.c_str()));
      http.end();
      return false;
    }
    String studentName = responseDoc["studentName"].as<String>();
    
    Serial.println("✅ CHECK-IN SUCCESS!");
    Serial.println("  Student: " + studentName);
    
    // Store student name for display
    lastCheckedInStudent = studentName;
    
    http.end();
    return true;
  }
  
  http.end();
  return false;
}

// ==========================================
// STATE MANAGEMENT
// ==========================================

void clearActiveEvent() {
  activeEventId = "";
  activeEventName = "";
  Serial.println("Event cleared from memory");
}

String getActiveEventId() {
  return activeEventId;
}

String getActiveEventName() {
  return activeEventName;
}

// ==========================================
// DISPLAY FUNCTIONS
// ==========================================

void displayNoEvent(Adafruit_SSD1306& display) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 10);
  display.println("No event found");
  display.println("");
  display.println("Press button to");
  display.println("fetch event");
  display.display();
}

void displayFetchingEvent(Adafruit_SSD1306& display) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 20);
  display.println("Fetching event");
  display.println("from server...");
  display.display();
}

void displayAttendanceReady(Adafruit_SSD1306& display, String eventName) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Ready:");
  display.println("");
  display.setTextSize(2);
  
  // Truncate long event names
  String displayName = eventName;
  if (eventName.length() > 10) {
    displayName = eventName.substring(0, 10) + "...";
  }
  
  display.println(displayName);
  display.setTextSize(1);
  display.println("");
  display.println("Tap card to check in");
  display.display();
}

void displayCheckingIn(Adafruit_SSD1306& display, String uid) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 10);
  display.println("Card detected!");
  display.println("");
  display.println("UID:");
  display.println(uid.substring(0, 17));
  display.println("");
  display.println("Checking in...");
  display.display();
}

void displayWelcome(Adafruit_SSD1306& display, String studentName) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 10);
  display.println("Welcome!");
  display.println("");
  display.setTextSize(2);
  
  // Truncate long names
  String displayName = studentName;
  if (studentName.length() > 10) {
    displayName = studentName.substring(0, 10);
  }
  
  display.println(displayName);
  display.display();
}

void displayAttendanceError(Adafruit_SSD1306& display, String error) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 20);
  display.println("Error:");
  display.println(error);
  display.display();
}
