# ESP32 NFC Attendance Firmware - Implementation Plan

## Document Overview
**Purpose:** Complete implementation guide for ESP32 NFC reader firmware  
**Target Hardware:** ESP32 DevKit (38-pin) + PN532 NFC Module + 0.96" OLED Display  
**Created:** December 2, 2025  
**Status:** In Progress  

---

## Implementation Progress Tracker

### Phase 1: Backend API Endpoints (Web Repository)

#### Critical Endpoints
- [x] **POST /api/check-in** - Core attendance recording (✅ DONE)
- [x] **GET /api/events/active** - Get current active event info (✅ DONE)
- [x] **POST /api/cards/register** - Link card UID to student (✅ DONE)
- [ ] **POST /api/cards/notify** - Real-time card detection for activation (OPTIONAL)
- [ ] **GET /api/cards/status/[uid]** - Check card activation status (OPTIONAL)
- [x] **lib/device-auth.ts** - Device authentication middleware (✅ DONE)

#### Testing & Deployment
- [ ] Test all endpoints with Postman/curl
- [ ] Deploy to Vercel staging
- [ ] Verify DEVICE_API_KEY in environment variables
- [ ] Test from external HTTP client

### Phase 2: Hardware Procurement
- [ ] Order ESP32 DevKit 38-pin (~৳600)
- [ ] Order PN532 NFC Module (~৳700)
- [ ] Order 0.96" OLED Display (~৳400)
- [ ] Order supporting components (buttons, LEDs, resistors, wires)
- [ ] Order 18650 battery + TP4056 charger (production)
- [ ] Order project enclosure

### Phase 3: ESP32 Firmware Development (Separate Repository)
- [ ] Create new repository: `aatcc-esp32-firmware`
- [ ] Setup PlatformIO or Arduino IDE
- [ ] Install required libraries
- [ ] Implement core modules
- [ ] Test with staging API
- [ ] Flash to device

### Phase 4: Integration & Testing
- [ ] End-to-end check-in flow
- [ ] Card activation flow
- [ ] Offline mode testing
- [ ] Battery life testing
- [ ] Production assembly

---

## 1. Backend API Endpoints (Current Task)

### Environment Variables Required

```bash
# Already configured in .env:
DEVICE_API_KEY=0eb480a26f15e979371df45b1912160b5f380bab0fb087cee8f5557c707cd08a
NEXT_PUBLIC_SUPABASE_URL=<your-url>
SUPABASE_SERVICE_ROLE_KEY=<your-key>
```

---

### 1.1 Device Authentication Middleware

**File:** `lib/device-auth.ts`

**Status:** ✅ DONE

**Purpose:** Verify ESP32 device requests using API key

**Implementation:**
```typescript
import { NextRequest } from 'next/server';

export function verifyDeviceAuth(request: NextRequest): boolean {
  const apiKey = request.headers.get('X-Device-API-Key');
  return apiKey === process.env.DEVICE_API_KEY;
}
```

**Usage:**
```typescript
if (!verifyDeviceAuth(request)) {
  return NextResponse.json({ error: 'Unauthorized device' }, { status: 401 });
}
```

---

### 1.2 POST /api/check-in

**File:** `app/api/check-in/route.ts`

**Status:** ✅ DONE

**Purpose:** ESP32 submits card UID for attendance recording

**Request:**
```typescript
Headers:
  X-Device-API-Key: <DEVICE_API_KEY>
  Content-Type: application/json

Body:
{
  cardUid: "A1:B2:C3:D4:E5:F6:07"
}
```

**Response (Success - 200):**
```typescript
{
  success: true,
  student: {
    name: "John Doe",
    studentId: "23-01-002"
  },
  event: {
    name: "Workshop on AI"
  },
  attendedCount: 24,
  registeredCount: 70
}
```

**Response (Errors):**
```typescript
401: { error: 'Unauthorized device' }        // Invalid API key
404: { error: 'No active event at this time' } // No active event
404: { error: 'Card not registered' }        // Card UID not in database
403: { error: 'Not registered for this event' } // Student not registered
409: { error: 'Already checked in' }         // Duplicate check-in
500: { error: 'Internal server error' }      // Database error
```

**Logic Flow:**
1. Verify device API key
2. Find active event (status='active', deleted_at IS NULL)
3. Lookup student by card_uid
4. Verify student is registered for event
5. Check for duplicate attendance
6. Insert attendance record
7. Get attendance counts
8. Return success response

---

### 1.3 GET /api/events/active

**File:** `app/api/events/active/route.ts`

**Status:** ✅ DONE

**Purpose:** ESP32 retrieves current active event info for display

**Request:**
```typescript
Headers:
  X-Device-API-Key: <DEVICE_API_KEY>
```

**Response (200):**
```typescript
{
  event: {
    id: "uuid",
    name: "Workshop on AI",
    description: "...",
    startTime: "2025-12-05T10:00:00Z",
    endTime: "2025-12-05T12:00:00Z",
    registeredCount: 70,
    attendedCount: 23
  }
}

// Or if no active event:
{
  event: null
}
```

**Logic Flow:**
1. Verify device API key
2. SELECT FROM events WHERE status='active' AND deleted_at IS NULL
3. Count registrations for event
4. Count attendance for event
5. Return event with counts

---

### 1.4 POST /api/cards/register

**File:** `app/api/cards/register/route.ts`

**Status:** ✅ DONE

**Purpose:** Admin links NFC card UID to student account (orientation day)

**Request:**
```typescript
Headers:
  Cookie: admin_session=<session>
  Content-Type: application/json

Body:
{
  studentId: "23-01-002",
  cardUid: "A1:B2:C3:D4:E5:F6:07"
}
```

**Response (Success - 200):**
```typescript
{
  success: true,
  student: {
    id: "uuid",
    studentId: "23-01-002",
    name: "John Doe",
    cardUid: "A1:B2:C3:D4:E5:F6:07"
  }
}
```

**Response (Errors):**
```typescript
401: { error: 'Unauthorized' }               // Not admin
400: { error: 'Student ID and card UID required' }
400: { error: 'Invalid card UID format' }    // Regex validation failed
409: { error: 'Card already assigned to ...' } // Duplicate card
404: { error: 'Student not found' }          // Invalid student_id
```

**Logic Flow:**
1. Require admin authentication
2. Validate studentId and cardUid present
3. Validate cardUid format (regex: `^[0-9A-F:]+$`)
4. Check if card already assigned to another student
5. UPDATE students SET card_uid WHERE student_id
6. Return updated student record

---

### 1.5 POST /api/cards/notify (OPTIONAL)

**File:** `app/api/cards/notify/route.ts`

**Status:** ⏳ TO DO (OPTIONAL - for real-time card detection UI)

**Purpose:** ESP32 sends detected card UID to web interface (for activation page)

**Request:**
```typescript
Headers:
  X-Device-API-Key: <DEVICE_API_KEY>
  Content-Type: application/json

Body:
{
  cardUid: "A1:B2:C3:D4:E5:F6:07",
  deviceId: "device-001"
}
```

**Response (200):**
```typescript
{
  success: true
}
```

**Implementation Note:**
- Store in temporary cache (Redis or in-memory Map)
- TTL: 30 seconds
- Web page polls GET /api/cards/latest?deviceId=device-001

---

### 1.6 GET /api/cards/status/[uid] (OPTIONAL)

**File:** `app/api/cards/status/[uid]/route.ts`

**Status:** ⏳ TO DO (OPTIONAL - for polling activation status)

**Purpose:** ESP32 polls to check if card was activated by admin

**Request:**
```typescript
Headers:
  X-Device-API-Key: <DEVICE_API_KEY>
```

**Response (200):**
```typescript
{
  activated: true,
  studentName: "John Doe",
  studentId: "23-01-002"
}

// Or if not activated yet:
{
  activated: false
}
```

**Logic Flow:**
1. Verify device API key
2. SELECT FROM students WHERE card_uid = [uid]
3. If found: return activated=true with student info
4. If not found: return activated=false

---

## 2. Hardware Wiring Diagram

### Pin Connections (I2C Bus)

```
ESP32 DevKit (38-pin)
┌─────────────────────────┐
│ 3.3V  →  PN532 VCC      │
│ 3.3V  →  OLED VCC       │
│ GND   →  PN532 GND      │
│ GND   →  OLED GND       │
│ GND   →  Button GND     │
│ GPIO21 (SDA) → PN532 SDA│
│ GPIO21 (SDA) → OLED SDA │
│ GPIO22 (SCL) → PN532 SCL│
│ GPIO22 (SCL) → OLED SCL │
│ GPIO15 → Mode Button    │
│ GPIO25 → LED Red        │
│ GPIO26 → LED Green      │
│ GPIO27 → LED Blue       │
└─────────────────────────┘

I2C Addresses:
- PN532: 0x24 (default)
- OLED:  0x3C (common)
```

---

## 3. ESP32 Firmware Structure (Separate Repository)

### Repository: `aatcc-esp32-firmware`

```
aatcc-esp32-firmware/
├── README.md
├── platformio.ini           # PlatformIO config
├── src/
│   ├── main.cpp            # Entry point & state machine
│   ├── config.h            # API URL, device key, GPIO pins
│   ├── wifi_manager.cpp    # WiFiManager integration
│   ├── nfc_reader.cpp      # PN532 NFC interface
│   ├── oled_display.cpp    # SSD1306 OLED driver
│   ├── api_client.cpp      # HTTP requests to web API
│   ├── mode_manager.cpp    # Registration/Check-in mode switching
│   ├── offline_queue.cpp   # SPIFFS storage for offline mode
│   └── led_controller.cpp  # RGB LED status indicators
├── include/
│   └── *.h                 # Header files
├── docs/
│   ├── wiring-diagram.md   # Hardware assembly guide
│   ├── setup-guide.md      # First-time setup instructions
│   └── troubleshooting.md  # Common issues & fixes
└── .gitignore
```

### Required Libraries

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200

lib_deps = 
    adafruit/Adafruit PN532@^1.3.1
    adafruit/Adafruit SSD1306@^2.5.7
    adafruit/Adafruit GFX Library@^1.11.3
    bblanchon/ArduinoJson@^6.21.3
    tzapu/WiFiManager@^2.0.16-rc.2
```

---

## 4. Firmware State Machine

### Boot Sequence
```
1. Initialize Serial (115200 baud)
2. Initialize SPIFFS (offline storage)
3. Load saved settings (mode, WiFi credentials)
4. Initialize I2C bus
5. Initialize PN532 NFC reader
6. Initialize OLED display
7. Initialize LED controller
8. Check WiFi credentials
   ├─ Saved? → Connect
   └─ None? → Start WiFiManager portal
9. Verify API connectivity (GET /api/events/active)
10. Load last used mode (Registration/Check-in)
11. Enter main loop
```

### Main Loop
```
loop() {
  1. Check mode button (long press = mode menu)
  2. Update LED status
  3. Check WiFi connection
  4. Poll NFC reader for card
  5. Process card based on current mode
  6. Update OLED display
  7. Sync offline queue if WiFi restored
  delay(100)
}
```

### Check-in Mode Flow
```
Ready State:
┌─────────────────────┐
│ Workshop on AI      │
│                     │
│    TAP CARD HERE    │
│                     │
│  23/70 checked in   │
└─────────────────────┘
        ↓ Card detected
Read UID → POST /api/check-in
        ↓
    ┌───┴───┐
SUCCESS    ERROR
    ↓       ↓
┌─────┐  ┌─────┐
│ ✓   │  │ ✗   │
│John │  │Not  │
│Doe  │  │Reg. │
└─────┘  └─────┘
    ↓       ↓
Delay 3s → Ready
```

---

## 5. Bill of Materials

| Component | Qty | Price (BDT) | Supplier |
|-----------|-----|-------------|----------|
| ESP32 DevKit 38-pin | 1 | 600 | Elephant Road/techshopbd.com |
| PN532 NFC Module (I2C) | 1 | 700 | Elephant Road/roboticsbd.com |
| 0.96" OLED 128x64 I2C | 1 | 400 | Elephant Road |
| Tactile Button | 1 | 10 | Elephant Road |
| RGB LED Common Cathode | 1 | 30 | Elephant Road |
| 220Ω Resistors | 3 | 15 | Elephant Road |
| 10kΩ Resistor | 1 | 5 | Elephant Road |
| Breadboard | 1 | 200 | Elephant Road |
| Jumper Wires | 1 set | 150 | Elephant Road |
| Micro USB Cable | 1 | 150 | Elephant Road |
| **Testing Total** | | **2,260** | **~$22 USD** |
| | | | |
| **Production Add-ons:** | | | |
| 18650 Li-ion Battery | 1 | 400 | Elephant Road |
| TP4056 Charger Module | 1 | 80 | Elephant Road |
| Battery Holder | 1 | 70 | Elephant Road |
| Project Enclosure | 1 | 300 | Elephant Road |
| **Production Total** | | **3,110** | **~$31 USD** |

---

## 6. Configuration File (config.h)

```cpp
#ifndef CONFIG_H
#define CONFIG_H

// API Configuration (UPDATE THESE!)
#define API_URL "https://your-site.vercel.app"
#define DEVICE_API_KEY "0eb480a26f15e979371df45b1912160b5f380bab0fb087cee8f5557c707cd08a"
#define DEVICE_ID "device-001"

// I2C Addresses
#define PN532_I2C_ADDRESS 0x24
#define OLED_I2C_ADDRESS 0x3C

// GPIO Pins
#define I2C_SDA 21
#define I2C_SCL 22
#define MODE_BUTTON_PIN 15
#define LED_R_PIN 25
#define LED_G_PIN 26
#define LED_B_PIN 27
#define BUZZER_PIN 4  // Optional

// Display Settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

// Timing Constants
#define CARD_COOLDOWN 2000           // 2 seconds between same card
#define API_TIMEOUT 10000            // 10 seconds
#define MODE_BUTTON_HOLD 3000        // 3 seconds for mode menu
#define OFFLINE_SYNC_INTERVAL 60000  // 1 minute

// LED Colors
#define LED_OFF 0
#define LED_RED 1
#define LED_GREEN 2
#define LED_BLUE 3
#define LED_YELLOW 4

// Offline Queue
#define MAX_OFFLINE_QUEUE 200

#define FIRMWARE_VERSION "1.0.0"

#endif
```

---

## 7. API Client Example (api_client.cpp)

```cpp
#include "api_client.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "config.h"

CheckInResponse APIClient::checkIn(String cardUid) {
  CheckInResponse response;
  response.success = false;
  
  HTTPClient http;
  http.begin(String(API_URL) + "/api/check-in");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-Device-API-Key", DEVICE_API_KEY);
  http.setTimeout(API_TIMEOUT);
  
  // Build JSON payload
  StaticJsonDocument<200> doc;
  doc["cardUid"] = cardUid;
  
  String payload;
  serializeJson(doc, payload);
  
  int httpCode = http.POST(payload);
  response.httpCode = httpCode;
  
  if (httpCode == 200) {
    String responseBody = http.getString();
    
    DynamicJsonDocument responseDoc(1024);
    DeserializationError error = deserializeJson(responseDoc, responseBody);
    
    if (!error) {
      response.success = true;
      response.studentName = responseDoc["student"]["name"].as<String>();
      response.studentId = responseDoc["student"]["studentId"].as<String>();
      response.eventName = responseDoc["event"]["name"].as<String>();
      response.attendedCount = responseDoc["attendedCount"] | 0;
      response.registeredCount = responseDoc["registeredCount"] | 0;
    }
    
  } else if (httpCode == 409) {
    response.errorMessage = "Already checked in";
  } else if (httpCode == 403) {
    response.errorMessage = "Not registered";
  } else if (httpCode == 404) {
    response.errorMessage = "No active event";
  } else if (httpCode == 401) {
    response.errorMessage = "Device unauthorized";
  } else {
    response.errorMessage = "Error - see admin";
  }
  
  http.end();
  return response;
}
```

---

## 8. Testing Checklist

### Backend API Testing (Postman/curl)

```bash
# Test 1: Check-in endpoint
curl -X POST https://your-site.vercel.app/api/check-in \
  -H "X-Device-API-Key: your-key" \
  -H "Content-Type: application/json" \
  -d '{"cardUid":"AA:BB:CC:DD:EE:FF:00"}'

# Expected: 404 (no active event) or 404 (card not registered)

# Test 2: Active event endpoint
curl https://your-site.vercel.app/api/events/active \
  -H "X-Device-API-Key: your-key"

# Expected: { "event": null } or event object

# Test 3: Card registration (need admin cookie)
curl -X POST https://your-site.vercel.app/api/cards/register \
  -H "Cookie: admin_session=..." \
  -H "Content-Type: application/json" \
  -d '{"studentId":"23-01-002","cardUid":"AA:BB:CC:DD:EE:FF:00"}'

# Expected: 200 with student object
```

### Hardware Testing

- [ ] I2C Scanner: Verify PN532 (0x24) and OLED (0x3C) detected
- [ ] PN532: Read test card UID, print to Serial
- [ ] OLED: Display test messages
- [ ] Button: Detect press, debounce working
- [ ] LED: Cycle through colors (Red, Green, Blue, Yellow)
- [ ] WiFiManager: Connect to captive portal, save credentials
- [ ] API: GET /api/events/active returns valid response
- [ ] Check-in: Full flow from card tap to API response

---

## 9. Troubleshooting Guide

### Problem: PN532 not detected
```
Symptoms: Serial shows "PN532 not found"
Solutions:
1. Check I2C wiring (SDA/SCL swapped?)
2. Verify 3.3V power (not 5V!)
3. Run I2C scanner sketch
4. Check PN532 mode switch (set to I2C)
5. Try different I2C address in code
```

### Problem: WiFi not connecting
```
Symptoms: Stuck on "Connecting WiFi..."
Solutions:
1. Check WiFi credentials
2. Check WiFi signal strength
3. Try mobile hotspot
4. Reset WiFi: Press reset button 3x quickly
5. Check Serial monitor for error codes
```

### Problem: API returns 401
```
Symptoms: OLED shows "Device unauthorized"
Solutions:
1. Verify DEVICE_API_KEY matches web .env
2. Check API_URL in config.h (HTTPS required)
3. Test with curl first
4. Check Vercel deployment logs
```

---

## 10. Timeline Estimate

| Week | Tasks | Deliverables |
|------|-------|--------------|
| **Week 1** | API endpoints + hardware order | 3 endpoints deployed |
| **Week 2** | Receive hardware, assemble | Wired on breadboard |
| **Week 3** | Firmware modules | Basic check-in working |
| **Week 4** | Offline mode, testing | Firmware v1.0.0 |
| **Week 5** | Card activation flow | Full system working |
| **Week 6** | Production assembly | Device ready |

**Total: 4-6 weeks**

---

## 11. Next Steps (Workflow)

### Current Phase: API Implementation ✅ COMPLETE
1. ✅ Create this plan document
2. ✅ Implement backend API endpoints in web repo
3. ✅ Mark each endpoint as "done" in this document
4. ⏳ Test endpoints with Postman (YOUR TASK)
5. ⏳ Deploy to Vercel (automatic on git push)

### Next Phase: Hardware Setup
6. Order components (~$25-30)
7. Wait for delivery (2-3 weeks if from Bangladesh, 3-4 weeks if AliExpress)

### Next Phase: Firmware Development
8. Create new folder for firmware
9. Open VS Code in that folder
10. Reference this plan
11. Implement firmware modules
12. Flash to ESP32
13. Integration testing

---

## 12. Success Criteria

- ✅ All API endpoints return correct responses
- ✅ ESP32 reads NFC cards consistently
- ✅ Check-in completes in <2 seconds
- ✅ OLED displays clear messages
- ✅ Offline queue syncs when WiFi restored
- ✅ Mode switching works reliably
- ✅ Battery lasts 4+ hours
- ✅ Zero duplicate check-ins

---

**Document Version:** 1.0  
**Last Updated:** December 2, 2025  
**Status:** ✅ Phase 1 COMPLETE (API Endpoints Implemented)  
**Next Milestone:** Test APIs → Order Hardware → Create Firmware Repo

---

## 13. Quick Testing Guide (Test Before Hardware Arrives)

### Prerequisites
```bash
# Make sure DEVICE_API_KEY is in your .env.local:
DEVICE_API_KEY=0eb480a26f15e979371df45b1912160b5f380bab0fb087cee8f5557c707cd08a
```

### Test 1: Check Active Event
```bash
curl https://your-site.vercel.app/api/events/active \
  -H "X-Device-API-Key: 0eb480a26f15e979371df45b1912160b5f380bab0fb087cee8f5557c707cd08a"

# Expected if no active event:
# {"event":null}

# Expected if event is active:
# {"event":{"id":"...","name":"Workshop on AI","registeredCount":70,"attendedCount":0}}
```

### Test 2: Register Card (Need Admin Login First)
```bash
# 1. Login as admin via browser first
# 2. Get cookie from browser DevTools
# 3. Then run:

curl -X POST https://your-site.vercel.app/api/cards/register \
  -H "Cookie: admin_session=YOUR_SESSION_COOKIE" \
  -H "Content-Type: application/json" \
  -d '{"studentId":"23-01-002","cardUid":"AA:BB:CC:DD:EE:FF:00"}'

# Expected success:
# {"success":true,"student":{"id":"...","studentId":"23-01-002","name":"John Doe","cardUid":"AA:BB:CC:DD:EE:FF:00"}}
```

### Test 3: Check-in (After Card Registered)
```bash
curl -X POST https://your-site.vercel.app/api/check-in \
  -H "X-Device-API-Key: 0eb480a26f15e979371df45b1912160b5f380bab0fb087cee8f5557c707cd08a" \
  -H "Content-Type: application/json" \
  -d '{"cardUid":"AA:BB:CC:DD:EE:FF:00"}'

# Expected if no active event:
# {"error":"No active event at this time"}

# Expected if card not registered:
# {"error":"Card not registered"}

# Expected if not registered for event:
# {"error":"Not registered for this event"}

# Expected success:
# {"success":true,"student":{"name":"John Doe","studentId":"23-01-002"},"event":{"name":"Workshop on AI"},"attendedCount":1,"registeredCount":70}
```

### Testing Workflow
1. **Create an event** (admin dashboard)
2. **Activate the event** (admin dashboard)
3. **Register test student for event** (import registrations or manual)
4. **Register card to student** (Test 2 above)
5. **Test check-in** (Test 3 above)
6. **Verify attendance** (student dashboard should show attendance)

---

## 14. Files Created in This Session

```
✅ lib/device-auth.ts                      - Device authentication middleware
✅ app/api/check-in/route.ts               - Check-in endpoint (core functionality)
✅ app/api/events/active/route.ts          - Active event endpoint
✅ app/api/cards/register/route.ts         - Card registration endpoint
✅ ESP32_FIRMWARE_PLAN.md                  - This implementation plan
```

**All critical API endpoints are now implemented and ready for testing!**

When you're ready to build the ESP32 firmware:
1. Order hardware components (~$25-30)
2. Create new folder: `aatcc-esp32-firmware`
3. Open that folder in VS Code
4. Copy this plan file to that folder
5. Ask me to implement the firmware (I'll reference this plan for context)
