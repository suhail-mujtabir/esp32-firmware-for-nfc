#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <MFRC522.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "config.h"
#include "attendance_mode.h"

// ==========================================
// MODE DEFINITIONS
// ==========================================

enum DeviceMode {
  MODE_REGISTRATION,    // Phase 2: Card registration mode
  MODE_ATTENDANCE       // Phase 3: Event attendance mode
};

// ==========================================
// GLOBAL OBJECTS
// ==========================================

MFRC522 rfid(RC522_SS_PIN, RC522_RST_PIN);
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);
WiFiManager wifiManager;

// ==========================================
// STATE VARIABLES
// ==========================================

DeviceMode currentMode = MODE_REGISTRATION;
String lastCardUid = "";
unsigned long lastCardTime = 0;

// Button state tracking
unsigned long buttonPressStart = 0;
unsigned long buttonClearPressStart = 0;
bool buttonPressed = false;
bool buttonClearPressed = false;

// ==========================================
// FUNCTION DECLARATIONS
// ==========================================

void initSerial();
void initButton();
void initBuzzer();
void buzzCard();
void initRFID();
void initOLED();
void initWiFi();
String readCardUID();
void displayOnOLED(String line1, String line2, String line3);
void handleCardDetected(String cardUid);
int sendCardToAPI(String cardUid);
void displaySending(String uid);
void displayWaiting();
void displayError(String line1, String line2);
void displayReady();
void checkFetchButton();
void checkClearButton();
void switchMode();

// ==========================================
// SETUP
// ==========================================

void setup() {
  Serial.println("\n\n");
  Serial.println("========================================");
  Serial.println("ESP32 NFC Attendance System");
  Serial.println("Firmware Version: " + String(FIRMWARE_VERSION));
  Serial.println("Phase 3: Registration + Attendance");
  Serial.println("========================================\n");

  initSerial();
  initButton();
  initBuzzer();
  initOLED();
  initRFID();
  initWiFi();
  
  Serial.println("\n✓ All systems initialized!");
  Serial.println("========================================");
  Serial.println("Mode: Registration (Hold button 5s to switch)");
  Serial.println("========================================\n");
  
  displayReady();
}

void loop() {
  // Check buttons first
  checkFetchButton();
  checkClearButton();
  
  // Check for new cards
  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }
  
  // Select one of the cards
  if (!rfid.PICC_ReadCardSerial()) {
    return;
  }
  
  // Card detected!
  String cardUid = readCardUID();
  
  // Cooldown check
  if (cardUid == lastCardUid && (millis() - lastCardTime < CARD_COOLDOWN)) {
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    return;
  }
  
  lastCardUid = cardUid;
  lastCardTime = millis();
  
  Serial.println("\n========================================");
  Serial.println("✓ CARD DETECTED!");
  Serial.println("========================================");
  Serial.println("Card UID: " + cardUid);
  Serial.println("Card Type: " + String(rfid.PICC_GetTypeName(rfid.PICC_GetType(rfid.uid.sak))));
  Serial.println("========================================\n");
  
  // Beep buzzer for feedback
  buzzCard();
  
  // Handle card based on current mode
  if (currentMode == MODE_REGISTRATION) {
    handleCardDetected(cardUid);
  } else {
    runAttendanceMode(display, cardUid);
  }
  
  // Halt PICC
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  
  delay(100);
}

// ==========================================
// INITIALIZATION FUNCTIONS
// ==========================================

void initSerial() {
  Serial.begin(SERIAL_BAUD);
  delay(100);
  Serial.println("✓ Serial initialized");
}

void initButton() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUTTON_CLEAR_PIN, INPUT_PULLUP);
  Serial.println("✓ Buttons initialized");
  Serial.println("  GPIO" + String(BUTTON_PIN) + " - Fetch/Mode switch");
  Serial.println("  GPIO" + String(BUTTON_CLEAR_PIN) + " - Clear event");
}

void initBuzzer() {
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  Serial.println("✓ Buzzer initialized");
  Serial.println("  GPIO" + String(BUZZER_PIN) + " - Card detection feedback");
}

void buzzCard() {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(BUZZER_DURATION);
  digitalWrite(BUZZER_PIN, LOW);
}

void initOLED() {
  Serial.println("\nInitializing OLED display...");
  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
  
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR)) {
    Serial.println("✗ OLED initialization failed!");
    Serial.println("  Check wiring: SDA=GPIO" + String(OLED_SDA_PIN) + ", SCL=GPIO" + String(OLED_SCL_PIN));
    Serial.println("  Make sure OLED VCC connected to 5V (not 3.3V)");
    while (1) delay(1000);
  }
  
  Serial.println("✓ OLED initialized (0x" + String(OLED_I2C_ADDR, HEX) + ")");
  
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("OLED OK!");
  display.display();
  delay(1000);
}

void initRFID() {
  Serial.println("\nInitializing RC522 NFC reader...");
  SPI.begin();
  rfid.PCD_Init();
  
  byte version = rfid.PCD_ReadRegister(rfid.VersionReg);
  
  if (version == 0x00 || version == 0xFF) {
    Serial.println("✗ RC522 communication failed!");
    Serial.println("  Version read: 0x" + String(version, HEX));
    Serial.println("  Check wiring and power (connect RC522 3.3V to ESP32 VIN)");
    displayOnOLED("RC522 FAIL", "Check wiring", "");
    while (1) delay(1000);
  }
  
  Serial.println("✓ RC522 initialized");
  Serial.println("  Firmware: 0x" + String(version, HEX));
  rfid.PCD_DumpVersionToSerial();
}

void initWiFi() {
  Serial.println("\nInitializing WiFi...");
  
  // Check if we have saved WiFi credentials
  WiFi.begin(); // Try to connect with saved credentials
  delay(100);
  
  if (WiFi.SSID() != "") {
    // We have saved credentials, try to connect
    displayOnOLED("WiFi", "Connecting to:", WiFi.SSID());
    Serial.println("Found saved WiFi: " + WiFi.SSID());
    
    // Wait up to 15 seconds for connection
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
      delay(500);
      Serial.print(".");
      attempts++;
    }
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
      // Successfully connected!
      Serial.println("✓ WiFi connected!");
      Serial.println("  SSID: " + WiFi.SSID());
      Serial.println("  IP: " + WiFi.localIP().toString());
      Serial.println("  Signal: " + String(WiFi.RSSI()) + " dBm");
      
      displayOnOLED("WiFi OK!", WiFi.localIP().toString(), "Ready!");
      delay(2000);
      return;
    }
    
    // Failed to connect with saved credentials
    Serial.println("✗ Saved WiFi failed, starting setup...");
  }
  
  // No saved credentials OR connection failed - start config portal
  displayOnOLED("WiFi Setup", "Connect to:", "ESP32-AATCC");
  delay(1000);
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("WiFi Setup Mode");
  display.println("");
  display.println("1. Connect phone to:");
  display.setTextSize(2);
  display.println("ESP32-AATCC");
  display.setTextSize(1);
  display.println("");
  display.println("2. Follow popup to");
  display.println("   select your WiFi");
  display.display();
  
  Serial.println("----------------------------------------");
  Serial.println("WiFi Setup Mode Active");
  Serial.println("1. Connect your phone/laptop to WiFi:");
  Serial.println("   Network: ESP32-AATCC");
  Serial.println("   (No password needed)");
  Serial.println("2. A popup will appear automatically");
  Serial.println("3. Select your WiFi and enter password");
  Serial.println("4. ESP32 will connect and save settings");
  Serial.println("----------------------------------------");
  
  // Set WiFiManager timeout
  wifiManager.setConfigPortalTimeout(180); // 3 minutes
  wifiManager.setConnectTimeout(30); // 30 seconds to connect
  
  if (!wifiManager.autoConnect("ESP32-AATCC")) {
    Serial.println("✗ WiFi setup timeout!");
    displayOnOLED("WiFi Failed", "Restarting...", "Try again");
    delay(3000);
    ESP.restart();
  }
  
  Serial.println("✓ WiFi connected!");
  Serial.println("  SSID: " + WiFi.SSID());
  Serial.println("  IP: " + WiFi.localIP().toString());
  Serial.println("  Signal: " + String(WiFi.RSSI()) + " dBm");
  
  displayOnOLED("WiFi OK!", WiFi.localIP().toString(), "Connected!");
  delay(2000);
}

// ==========================================
// RFID FUNCTIONS
// ==========================================

String readCardUID() {
  String uid = "";
  
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) {
      uid += "0";
    }
    uid += String(rfid.uid.uidByte[i], HEX);
    
    if (i < rfid.uid.size - 1) {
      uid += ":";
    }
  }
  
  uid.toUpperCase();
  return uid;
}

// ==========================================
// API CLIENT FUNCTIONS
// ==========================================

void handleCardDetected(String cardUid) {
  // Step 1: Send card to API
  displaySending(cardUid);
  int httpCode = sendCardToAPI(cardUid);
  
  if (httpCode == 409) {
    // Card already activated
    displayError("Card already", "activated");
    delay(3000);
    displayReady();
    return;
  }
  
  if (httpCode != 200) {
    displayError("API Error", "Check connection");
    delay(3000);
    displayReady();
    return;
  }
  
  // Step 2: Card sent successfully, wait 5 seconds
  displayWaiting();
  delay(5000);
  
  // Step 3: Return to ready (admin activates in browser)
  displayReady();
}

int sendCardToAPI(String cardUid) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("✗ WiFi not connected!");
    return -1;
  }
  
  HTTPClient http;
  String url = String(API_URL) + String(ENDPOINT_CARDS_DETECTED);
  
  Serial.println("POST " + url);
  
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("x-device-api-key", DEVICE_API_KEY);
  http.setTimeout(API_TIMEOUT);
  
  // Create JSON: { "uid": "AA:BB:CC:...", "deviceId": "device-001" }
  JsonDocument doc;
  doc["uid"] = cardUid;
  doc["deviceId"] = DEVICE_ID;
  
  String requestBody;
  serializeJson(doc, requestBody);
  
  Serial.println("Sending: " + requestBody);
  
  int httpCode = http.POST(requestBody);
  
  Serial.println("Response code: " + String(httpCode));
  
  http.end();
  
  return httpCode;
}

// ==========================================
// DISPLAY FUNCTIONS
// ==========================================

void displayReady() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println("Ready");
  
  display.setTextSize(1);
  display.setCursor(0, 24);
  display.println("Tap card to");
  display.setCursor(0, 40);
  display.println("register...");
  
  display.display();
}

void displaySending(String uid) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Card Detected!");
  display.println("");
  display.println("UID:");
  display.println(uid.substring(0, 17));
  display.println("");
  display.println("Sending...");
  display.display();
}

void displayWaiting() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 10);
  display.println("Card sent!");
  display.println("");
  display.println("Admin: activate");
  display.println("in browser");
  display.display();
}

void displayError(String line1, String line2) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 20);
  display.println(line1);
  display.println(line2);
  display.display();
}

void displayOnOLED(String line1, String line2, String line3) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println(line1);
  
  display.setTextSize(1);
  display.setCursor(0, 24);
  display.println(line2);
  
  display.setCursor(0, 40);
  display.println(line3);
  
  display.display();
}
// ==========================================
// BUTTON HANDLING
// ==========================================

void checkFetchButton() {
  bool buttonState = digitalRead(BUTTON_PIN) == LOW; // Active low (INPUT_PULLUP)
  
  if (buttonState && !buttonPressed) {
    // Button just pressed
    buttonPressed = true;
    buttonPressStart = millis();
    Serial.println("Fetch button pressed");
  }
  
  if (!buttonState && buttonPressed) {
    // Button just released
    buttonPressed = false;
    unsigned long pressDuration = millis() - buttonPressStart;
    
    Serial.println("Fetch button released - duration: " + String(pressDuration) + "ms");
    
    // Check for long press (mode switch)
    if (pressDuration >= BUTTON_LONG_PRESS) {
      Serial.println("Long press detected (>= 5s) - switching mode");
      switchMode();
      return;
    }
    
    // Short press - handle based on mode
    if (currentMode == MODE_ATTENDANCE) {
      Serial.println("Short press in attendance mode - fetching event");
      handleFetchButton(display, false);
    } else {
      Serial.println("Short press in registration mode - ignored");
    }
    
    delay(BUTTON_DEBOUNCE);
  }
}

void checkClearButton() {
  bool buttonState = digitalRead(BUTTON_CLEAR_PIN) == LOW; // Active low (INPUT_PULLUP)
  
  if (buttonState && !buttonClearPressed) {
    // Button just pressed
    buttonClearPressed = true;
    buttonClearPressStart = millis();
    Serial.println("Clear button pressed");
  }
  
  if (!buttonState && buttonClearPressed) {
    // Button just released
    buttonClearPressed = false;
    unsigned long pressDuration = millis() - buttonClearPressStart;
    
    Serial.println("Clear button released - duration: " + String(pressDuration) + "ms");
    
    // Only handle in attendance mode
    if (currentMode == MODE_ATTENDANCE) {
      Serial.println("Clear button - clearing event");
      handleClearButton(display);
    } else {
      Serial.println("Clear button in registration mode - ignored");
    }
    
    delay(BUTTON_DEBOUNCE);
  }
}

void switchMode() {
  if (currentMode == MODE_REGISTRATION) {
    currentMode = MODE_ATTENDANCE;
    initAttendanceMode();
    displayNoEvent(display);
    
    Serial.println("\n========================================");
    Serial.println("SWITCHED TO: ATTENDANCE MODE");
    Serial.println("========================================\n");
  } else {
    currentMode = MODE_REGISTRATION;
    displayReady();
    
    Serial.println("\n========================================");
    Serial.println("SWITCHED TO: REGISTRATION MODE");
    Serial.println("========================================\n");
  }
}
