# NFC Attendance System - Implementation Details

## Project Overview
A student organization attendance tracking system using NFC cards for workshop/event check-ins. Students can view their participation history via a web dashboard, while admins manage events and registrations.

---

## Tech Stack

### Frontend & Backend
- **Framework**: Next.js 15 with App Router
- **Styling**: Tailwind CSS
- **Hosting**: Vercel (Free Tier)
- **State Management**: React Query (TanStack Query) for caching

### Database & Auth
- **Database**: Supabase (Free Tier)
- **Authentication**: Supabase Auth with student_id + password
- **Storage**: Supabase PostgreSQL

### Hardware (ESP32 Device)
- **Microcontroller**: ESP32 DevKit (38-pin)
- **NFC Reader**: PN532 NFC Module (I2C interface)
- **Display**: 0.96" OLED Display (128x64, I2C)
- **Mode Switch**: Push button for Registration/Check-in mode
- **Indicator**: RGB LED or separate LEDs for status
- **Power**: 3.7V Li-ion rechargeable battery + TP4056 charging module
- **WiFi Setup**: WiFiManager library (captive portal for easy configuration)
- **Offline Mode**: SPIFFS storage for queuing check-ins when offline

### NFC Cards
- **Type**: NTAG215/216 cards (ISO14443A)
- **Printing**: Custom printed with student name + student ID
- **UID**: 7-byte factory UID (read-only, permanent)
- **Usage**: Card UID linked to student_id in database

---

## System Architecture

### Core Workflow

```
1. STUDENT ACCOUNT SETUP (Before Orientation)
   Admin bulk imports student list â†’ Creates accounts (student_id + auto-generated password)
   â†’ Passwords sent via email or printed sheet

2. CARD PROCUREMENT & ACTIVATION
   Order printed cards from vendor â†’ Cards arrive at orientation
   â†’ Students tap cards at activation kiosk â†’ Admin links card UID to student_id

3. EVENT ACTIVATION
   Admin manually activates event via web dashboard

4. CHECK-IN PHASE
   Student taps NFC card on ESP32 device â†’ Device reads UID â†’ Sends to API
   â†’ System validates & records attendance â†’ OLED shows confirmation

5. DASHBOARD VIEW
   Student logs in with student_id + password â†’ Views attendance history, upcoming events
```

### Data Flow

```
CARD ACTIVATION (Orientation Day):

[Pre-printed NFC Card: "John Doe, ID: 23-01-002", UID: "ABC123"]
         â†“
[ESP32 Device in Registration Mode]
    â”œâ”€ Student taps card
    â”œâ”€ ESP32 reads UID: "ABC123"
    â”œâ”€ Sends to web interface via WebSocket/API
    â””â”€ Web page shows: "Card detected: ABC123"
         â†“
[Admin types student ID: "23-01-002" on web page]
         â†“
[POST /api/cards/register { studentId: "23-01-002", cardUid: "ABC123" }]
         â†“
[Supabase: UPDATE students SET card_uid = "ABC123" WHERE student_id = "23-01-002"]
         â†“
[ESP32 OLED displays: "âœ“ Activated! John Doe"]

--- Later, at event ---

EVENT CHECK-IN:

[ESP32 Device in Check-in Mode]
    â”œâ”€ Student taps card
    â”œâ”€ ESP32 reads UID: "ABC123"
    â””â”€ POST /api/check-in { cardUid: "ABC123" }
         â†“
[Next.js API Route]
    â”œâ”€ Find active event
    â”œâ”€ Lookup student by card UID
    â”œâ”€ Verify registration
    â”œâ”€ Check for duplicates
    â””â”€ Insert attendance record
         â†“
[API responds: { success: true, name: "John Doe", studentId: "23-01-002" }]
         â†“
[ESP32 OLED displays: "âœ“ John Doe (23-01-002)"]
         â†“
[Student dashboard updates with new attendance]
```

---

## Database Schema (Supabase)

### Tables

#### **students**
```sql
- id (uuid, primary key)
- student_id (text, unique, not null) - Format: YY-DD-NNN (e.g., "23-01-002")
- name (text, not null)
- email (text, unique, nullable)
- password_hash (text, not null) - Managed by Supabase Auth
- card_uid (text, unique, nullable) - NFC card UID (assigned during activation)
- bio (text, default: '') - Student-editable
- created_at (timestamp)
- updated_at (timestamp)
```

#### **events**
```sql
- id (uuid, primary key)
- name (text)
- description (text)
- start_time (timestamp)
- end_time (timestamp)
- status (enum: 'upcoming' | 'active' | 'completed')
- created_by (uuid, foreign key â†’ students.id)
- created_at (timestamp)
- deleted_at (timestamp, nullable) - soft delete
```

#### **registrations**
```sql
- id (uuid, primary key)
- student_id (uuid, foreign key â†’ students.id)
- event_id (uuid, foreign key â†’ events.id)
- registered_at (timestamp)
- unique(student_id, event_id) - prevent duplicates
```

#### **attendance**
```sql
- id (uuid, primary key)
- student_id (uuid, foreign key â†’ students.id)
- event_id (uuid, foreign key â†’ events.id)
- checked_in_at (timestamp)
- unique(student_id, event_id) - prevent duplicate check-ins
```

### Indexes (for performance)
```sql
- students: index on card_uid
- events: index on status, start_time
- registrations: composite index on (student_id, event_id)
- attendance: composite index on (student_id, event_id)
```

---

## User Roles & Permissions

### Student Role
**Can:**
- Login with student_id + password
- View own dashboard (attendance count, history, upcoming events)
- Edit own name and bio
- Change password
- Register for events (via Google Form, imported by admin)

**Cannot:**
- Edit attendance records
- Create/manage events
- View other students' data
- Access admin panel

### Admin Role
**Can:**
- Create/edit/delete events
- Manually activate/deactivate events
- Import registrations from Google Sheets
- View all attendance records
- Assign NFC card UIDs to students
- View analytics (total attendees per event, etc.)

**Cannot:**
- Edit student attendance after it's recorded (data integrity)

---

## Key Features

### Student Dashboard
- **Login**: student_id + password (Supabase Auth)
- **Header**: Name, student ID, editable bio
- **Stats Card**: Total events attended
- **Attendance History**: Last 10 events with dates (paginated if needed)
- **Upcoming Events**: Events student is registered for
- **Edit Profile**: Change name, bio, and password

### Admin Dashboard
- **Event Management**:
  - Create new event (name, description, start/end time)
  - List all events (filter by status: upcoming/active/completed)
  - Manually activate event (sets status='active', others to 'completed')
  - Delete event (soft delete with cascading to registrations/attendance)
  
- **Registration Import**:
  - Connect to Google Sheets API
  - Fetch form responses (columns: Student ID, Name, Email, Event)
  - Bulk insert into registrations table
  - Show success/error summary
  
  - **Card Management** (via web dashboard):
  - View all students and their card assignment status
  - View statistics (cards assigned, unassigned)
  - Search students by ID or name
  - Manual card reassignment for lost cards
  - Bulk import students with auto-generated passwords
  
- **Card Activation Interface** (web page for orientation):
  - Real-time card detection from ESP32 device
  - Student ID input field with validation
  - Auto-display student name after ID entry
  - Progress counter (X/200 activated)
  - Recent activations list

- **Analytics**:
  - Event attendance count
  - Top attendees leaderboard
  - Monthly participation trends

### ESP32 Unified Device (Registration + Check-in)

**Single device with two operating modes:**

#### **Mode 1: Registration/Activation Mode**
- **Purpose**: Link NFC cards to student accounts (orientation day)
- **Operation**: 
  - ESP32 reads card UID
  - Sends UID to web interface (WebSocket or polling)
  - Admin types student ID on web page
  - System links card to student
  - OLED displays confirmation
  
- **Web Interface** (yoursite.com/activate):
  ```
  â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”
  â”‚  Card Activation                   â”‚
  â”‚                                    â”‚
  â”‚  Waiting for card...               â”‚
  â”‚  (ESP32 device status: Connected)  â”‚
  â”‚                                    â”‚
  â”‚  [Card detected automatically]     â”‚
  â”‚                                    â”‚
  â”‚  âœ“ Card UID: ABC123                â”‚
  â”‚                                    â”‚
  â”‚  Enter Student ID:                 â”‚
  â”‚  [23-01-002_______]   [Activate]  â”‚
  â”‚                                    â”‚
  â”‚  Progress: 47/200 activated        â”‚
  â”‚                                    â”‚
  â”‚  Recent:                           â”‚
  â”‚  âœ“ 23-01-001 - Jane Smith          â”‚
  â”‚  âœ“ 22-03-045 - Bob Wilson          â”‚
  â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜
  ```

#### **Mode 2: Check-in Mode**
- **Purpose**: Attendance tracking at events
- **Operation**:
  - Fully autonomous (no web page needed)
  - Student taps card
  - ESP32 reads UID â†’ Sends to API
  - API validates and records attendance
  - OLED displays result
  - Audio beep for feedback
  
- **OLED Display States**:
  ```
  Ready state:
  â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”
  â”‚ Workshop on AI   â”‚
  â”‚                  â”‚
  â”‚   TAP CARD       â”‚
  â”‚      HERE        â”‚
  â”‚                  â”‚
  â”‚   23/70 checked  â”‚
  â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜
  
  Success:
  â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”
  â”‚   âœ“ SUCCESS!     â”‚
  â”‚                  â”‚
  â”‚   John Doe       â”‚
  â”‚   23-01-002      â”‚
  â”‚                  â”‚
  â”‚   24/70 checked  â”‚
  â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜
  
  Error:
  â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”
  â”‚   âœ— ERROR        â”‚
  â”‚                  â”‚
  â”‚ Not registered   â”‚
  â”‚ for this event   â”‚
  â”‚                  â”‚
  â”‚ See admin desk   â”‚
  â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜
  
  Offline mode:
  â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”
  â”‚   âš  OFFLINE      â”‚
  â”‚                  â”‚
  â”‚   John Doe       â”‚
  â”‚   Saved locally  â”‚
  â”‚                  â”‚
  â”‚   5 queued       â”‚
  â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜
  ```

#### **Mode Switching**
- **Hardware**: Push button on device
- **Operation**:
  1. Press and hold button for 3 seconds
  2. OLED shows mode selection menu
  3. Short press to cycle options
  4. Long press to confirm selection
  5. Mode saved to EEPROM (persists after restart)
  
- **Visual Indicators**:
  - Blue LED blinking = Registration mode
  - Green LED solid = Check-in mode (ready)
  - Green LED blinking = Check-in mode (processing)
  - Red LED blinking = WiFi error / No connection
  - Yellow LED = Offline mode (queuing data)

#### **WiFi Configuration**
- **Library**: WiFiManager (captive portal)
- **First-time setup**:
  1. Power on ESP32
  2. ESP32 creates WiFi hotspot: "OrgName-Setup"
  3. Admin connects phone to hotspot
  4. Browser opens automatically: http://192.168.4.1
  5. Admin selects WiFi network and enters password
  6. ESP32 saves credentials and connects
  7. Future boots auto-connect to saved WiFi
  
- **Change WiFi** (different venue):
  1. Press reset button 3 times quickly
  2. ESP32 enters config mode
  3. Repeat setup process

#### **Offline Mode**
- **Storage**: ESP32 SPIFFS (up to 200 check-ins)
- **Operation**:
  1. Student taps card
  2. If WiFi available: Send to API immediately
  3. If WiFi unavailable: 
     - Save check-in locally (UID + timestamp)
     - Display "Saved (Offline)" message
     - Add to sync queue
  4. When WiFi returns:
     - Auto-upload all queued check-ins
     - Display "Syncing... X pending"
     - Clear queue after successful sync
  5. Queue persists through power cycles

#### **Hardware Specifications**
- **ESP32**: DevKit 38-pin version
- **NFC Reader**: PN532 module (I2C interface)
- **Display**: 0.96" OLED (128x64 pixels, I2C)
- **Button**: Tactile push button (mode switch)
- **LEDs**: RGB LED or 3 separate LEDs (status indicators)
- **Power**: 
  - Testing: Direct USB power (5V)
  - Production: 3.7V Li-ion rechargeable battery + TP4056 charging module
  - Battery life: 4-6 hours continuous use
- **Enclosure**: Custom 3D-printed or plastic project box

#### **Wiring (I2C Bus)**
```
ESP32 Pin    â†’    Component
GPIO21 (SDA) â†’    PN532 SDA, OLED SDA
GPIO22 (SCL) â†’    PN532 SCL, OLED SCL
GPIO15       â†’    Mode switch button
GPIO2        â†’    RGB LED (or separate GPIO for R/G/B)
3.3V         â†’    PN532 VCC, OLED VCC
GND          â†’    All GND pins
VIN or 3.3V  â†’    Battery input (via TP4056)
```

---

## Critical Optimizations

### 1. API Design
**âœ… DO:**
- Use single consolidated endpoints (e.g., `/api/dashboard/[id]` returns all dashboard data)
- Implement bulk operations for imports (single `INSERT` with array)
- Return only necessary fields in queries (`select('id, name')` not `select('*')`)

**âŒ DON'T:**
- Make multiple API calls for one page load
- Use loops for database operations (use batch inserts)
- Fetch entire tables without limits

### 2. Caching Strategy
**React Query Configuration:**
```javascript
staleTime: 5 minutes - data considered fresh
cacheTime: 30 minutes - cache persists in memory
refetchOnWindowFocus: false - don't refetch on tab switch
```

**What to cache:**
- Student dashboard data (5 min stale time)
- Event lists (1 min stale time for admins)
- Attendance history (10 min stale time)

**What NOT to cache:**
- Active event status (real-time accuracy needed)
- Check-in responses (must be fresh)

### 3. Database Query Optimization
**Examples:**
```javascript
// âœ… GOOD: Minimal data with limit
const events = await supabase
  .from('attendance')
  .select('event:events(name, date)')
  .eq('student_id', id)
  .order('checked_in_at', { ascending: false })
  .limit(10);

// âŒ BAD: Fetches everything
const events = await supabase
  .from('attendance')
  .select('*, events(*), students(*)');
```

### 4. Bulk Operations
**Registration Import:**
```javascript
// âœ… GOOD: Single bulk insert
await supabase.from('registrations').insert(allRows);

// âŒ BAD: Loop with individual inserts
for (const row of allRows) {
  await supabase.from('registrations').insert(row);
}
```

### 5. Image Optimization
- Use Next.js `<Image>` component for any images
- Serve optimized WebP format automatically
- Lazy load images below the fold

---

## Google Form Integration

### Registration Form Structure
**Google Form Fields:**
1. Student Name (text)
2. Student Email (email) - must match Supabase auth email
3. Student ID (text) - format: YY-DD-NNN (e.g., 23-01-002)
4. Event Selection (dropdown) - populated with upcoming event names
5. Timestamp (automatic)

### Import Process
1. Admin navigates to "Import Registrations" page on web dashboard
2. Selects event from dropdown
3. Clicks "Fetch from Google Sheets"
4. System:
   - Calls Google Sheets API (using service account)
   - Filters rows for selected event
   - Matches student IDs to database records
   - Bulk inserts into registrations table
   - Shows summary: "Imported 70 students, 3 errors (student ID not found)"

### Google Sheets API Setup
- Service account with read-only access to specific sheet
- Sheet ID stored in environment variable
- Rate limit: 100 requests per 100 seconds (plenty for use case)

### Initial Student Database Setup
**One-time bulk import of all students:**
1. Export student list from Google Sheet (columns: student_id, name, email)
2. Admin uploads CSV to web dashboard
3. System bulk inserts into students table
4. All 200+ students now in database
5. Cards can be assigned to these existing records

---

## NFC Hardware Details

### Card Specifications
- **Type**: NTAG215/216 (ISO14443A)
- **Frequency**: 13.56 MHz
- **Range**: 1-4 inches (requires intentional tap)
- **UID**: 7 bytes (unique per card, factory-set, read-only)
- **Cost**: à§³30-60 per card (~$0.30-0.60) for printed cards
- **Lifespan**: 100,000+ read cycles
- **Printing**: Custom printed with student name + student ID

### Card Design (Printed by Vendor)
```
â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”
â”‚          [ORG LOGO]                 â”‚
â”‚                                     â”‚
â”‚         JOHN DOE                    â”‚
â”‚       ID: 23-01-002                 â”‚
â”‚                                     â”‚
â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜
    (NFC UID embedded: ABC123)
```

### ESP32 Unified Device Components

**Core Components:**
- **ESP32 DevKit**: ESP32-WROOM-32 (38-pin version)
  - Built-in WiFi + Bluetooth
  - Price: à§³500-700 (~$5-7)
  
- **PN532 NFC Module**: I2C/SPI interface
  - Supports NTAG215/216, MIFARE Classic
  - Read range: 2-5 cm
  - Price: à§³600-800 (~$6-8)
  
- **0.96" OLED Display**: 128Ã—64 pixels, I2C
  - SSD1306 driver chip
  - Clear visibility in daylight
  - Price: à§³300-500 (~$3-5)
  
- **Mode Switch Button**: Tactile push button
  - For switching Registration/Check-in modes
  - Price: à§³10-20 (~$0.10-0.20)
  
- **Status LEDs**: RGB LED or 3 separate LEDs
  - Blue = Registration mode
  - Green = Check-in mode / Success
  - Red = Error / No WiFi
  - Yellow = Offline mode
  - Price: à§³20-50 (~$0.20-0.50)
  
- **Resistors**: 220Î© for LEDs
  - Price: à§³10-20 for pack

- **Breadboard** (testing) or **PCB** (production)
  - Breadboard: à§³150-300
  - Custom PCB: à§³500-1,000
  
- **Jumper Wires**: Male-to-male, male-to-female
  - Price: à§³100-200 for set

**Power Supply:**

**For Testing:**
- Micro USB cable (ESP32 powered via USB)
- Connected to laptop or power bank
- Price: à§³100-200

**For Production:**
- 3.7V Li-ion battery (18650 or similar)
  - Capacity: 2000-3000mAh
  - Runtime: 4-6 hours continuous
  - Price: à§³300-500 per battery
  
- TP4056 Charging Module
  - USB rechargeable circuit
  - Overcharge/discharge protection
  - Price: à§³50-100
  
- Battery Holder
  - For 1Ã— 18650 battery
  - Price: à§³50-100
  
- Optional: DC-DC Buck/Boost Converter
  - Stable 3.3V output from battery
  - Price: à§³80-150

**Enclosure:**
- Plastic project box (85Ã—58Ã—33mm or similar)
  - Price: à§³150-400
- OR 3D-printed custom enclosure
  - Design file provided
  - Print cost: à§³300-800

**Total Cost per Device:**
- **Testing setup**: à§³1,690-2,550 (~$17-25)
- **Production setup**: à§³2,390-3,750 (~$24-37)

**Where to Buy in Bangladesh:**
- **Elephant Road, Dhaka**: IDB Bhaban electronics shops
- **Online**: 
  - techshopbd.com
  - roboticsbd.com
  - daraz.com.bd
- **AliExpress**: Cheapest but 2-4 week shipping

### Device Wiring Diagram

```
I2C Bus Configuration (All components share SDA/SCL):

ESP32 GPIO21 (SDA) â”€â”¬â”€ PN532 SDA
                    â”œâ”€ OLED SDA
                    
ESP32 GPIO22 (SCL) â”€â”¬â”€ PN532 SCL
                    â”œâ”€ OLED SCL

ESP32 3.3V â”€â”€â”€â”€â”€â”€â”€â”€â”€â”¬â”€ PN532 VCC
                    â”œâ”€ OLED VCC
                    
ESP32 GND â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”¬â”€ PN532 GND
                    â”œâ”€ OLED GND
                    â”œâ”€ Button GND
                    â””â”€ LED GND (via resistor)

ESP32 GPIO15 â”€â”€â”€â”€â”€â”€â”€â”€ Mode Switch Button
ESP32 GPIO2 â”€â”€â”€â”¬â”€â”€â”€â”€â”€ LED+ (via 220Î© resistor)
               â””â”€â”€â”€â”€â”€ Or RGB LED control

Power Options:
- Testing: ESP32 VIN â† USB 5V
- Production: ESP32 VIN â† TP4056 â†’ Battery 3.7V
```

### Arduino Libraries Needed

```cpp
// NFC Reading
#include <Wire.h>
#include <PN532_I2C.h>
#include <PN532.h>

// OLED Display
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// WiFi & HTTP
#include <WiFi.h>
#include <HTTPClient.h>

// WiFi Configuration Portal
#include <WiFiManager.h>

// Offline Storage
#include <SPIFFS.h>

// Persistent Settings
#include <Preferences.h>

// JSON Parsing
#include <ArduinoJson.h>
```

---

## Event Activation Logic

### Manual Activation (Chosen Approach)
**Why manual?**
- Event times often change last minute
- Prevents accidental early/late check-ins
- Gives admin full control

**Process:**
1. Admin opens "Active Events" page
2. Sees list of upcoming events
3. Clicks "Start Event" button next to target event
4. System:
   - Sets `status='active'` for selected event
   - Sets `status='completed'` for any other active events
   - Only ONE event can be active at a time
5. Check-in readers now accept taps for this event

**Stopping Events:**
- Automatic: After end_time passes, cron job sets status='completed'
- Manual: Admin clicks "End Event" button

---

## Error Handling

### API Error Scenarios

#### Check-in Endpoint Errors
```javascript
// No active event
â†’ Return: { error: 'No active event at this time' }
â†’ Reader displays: "No event active"

// Card UID not found
â†’ Return: { error: 'Card not registered' }
â†’ Reader displays: "Unknown card"

// Student not registered for event
â†’ Return: { error: 'Not registered for this event' }
â†’ Reader displays: "Not registered"

// Duplicate check-in attempt
â†’ Return: { error: 'Already checked in' }
â†’ Reader displays: "Already attended"

// Database error
â†’ Return: { error: 'System error, contact admin' }
â†’ Reader displays: "Error - try again"
```

#### Import Errors
```javascript
// Email not found in students table
â†’ Skip row, add to error list

// Event ID invalid
â†’ Abort import, show error

// Google Sheets API failure
â†’ Show retry button with error message
```

### Frontend Error Handling
- **Toast notifications** for non-critical errors (using shadcn/ui)
- **Error boundaries** for React component crashes
- **Retry mechanisms** for failed API calls (React Query automatic)
- **Loading states** for all async operations

---

## Security Considerations

### Authentication
- **Students**: student_id + password (Supabase Auth)
- **Admins**: Same as students + admin flag in database
- **ESP32 Device**: API key or device token for authentication

### Authorization Checks
```javascript
// Every API route checks:
1. Is request authenticated? (Supabase session or device token)
2. Does user have permission? (student can only access own data)
3. Is resource owned by user? (student_id matches session user)
4. Is device authorized? (valid device token for check-in endpoint)
```

### Data Validation
- **Card UID format**: Must match regex `^[0-9A-F:]+$`
- **Event times**: start_time < end_time
- **Email format**: Valid email regex
- **Input sanitization**: All user inputs escaped

### Rate Limiting
- **Check-in API**: Max 10 requests/minute per reader (prevent spam)
- **Dashboard**: Max 60 requests/hour per user
- **Admin imports**: Max 5 imports/hour

---

## Deployment Strategy

### Environment Variables
```bash
# Supabase
NEXT_PUBLIC_SUPABASE_URL=
NEXT_PUBLIC_SUPABASE_ANON_KEY=
SUPABASE_SERVICE_ROLE_KEY=

# Google Sheets API
GOOGLE_SHEETS_CLIENT_EMAIL=
GOOGLE_SHEETS_PRIVATE_KEY=
GOOGLE_SHEET_ID=

# ESP32 Device Authentication
DEVICE_API_KEY=your-secure-device-key

# Next.js API URL (for ESP32)
NEXT_PUBLIC_API_URL=https://yoursite.com

# App Config
NEXT_PUBLIC_ORG_NAME="Your Org Name"
```

### Next.js Website Deployment (Vercel)
1. Connect GitHub repo to Vercel
2. Set environment variables in Vercel dashboard
3. Deploy main branch â†’ Production
4. Add custom domain when ready
5. API routes available at: yoursite.com/api/*

### Supabase Setup
1. Create new project (free tier)
2. Run SQL migrations for schema
3. Configure password-based auth (email + password)
4. Set up Row Level Security (RLS) policies
5. Add redirect URLs (Vercel domain)

### ESP32 Device Setup

**Initial Programming (One-time):**

```bash
1. Install Arduino IDE or PlatformIO
2. Install ESP32 board support
3. Install required libraries:
   - PN532
   - Adafruit_SSD1306
   - Adafruit_GFX
   - WiFiManager
   - HTTPClient
   - ArduinoJson
   - Preferences

4. Configure constants in code:
   - API_URL = "https://yoursite.com/api"
   - DEVICE_API_KEY = "your-secure-device-key"
   - DEVICE_ID = "device-001"

5. Upload code to ESP32 via USB
6. Open Serial Monitor to verify boot

7. First boot - WiFi setup:
   - ESP32 creates "OrgName-Setup" hotspot
   - Connect phone to hotspot
   - Browser opens config page
   - Select your WiFi and enter password
   - ESP32 saves and connects

8. Test NFC reading:
   - Tap test card
   - Check Serial Monitor for UID
   - Verify OLED display works

9. Test API connection:
   - Trigger check-in with test card
   - Verify API receives request
   - Check response on OLED
```

**Production Deployment:**

```bash
1. Assemble device:
   - Solder connections or use PCB
   - Mount in enclosure
   - Connect battery and charging module
   - Add mode switch button
   - Add status LEDs

2. Label device:
   - "Card Activation Device" or "Check-in Device"
   - Device ID sticker

3. Power on and verify:
   - Mode indicator LED works
   - OLED displays startup screen
   - Connects to WiFi automatically

4. Provide to admin team:
   - Quick start guide
   - Troubleshooting steps
   - Contact info for technical support
```

**Device Configuration:**
- WiFi credentials stored in ESP32 flash
- Mode preference saved to EEPROM
- Offline queue stored in SPIFFS
- No sensitive data stored on device

---

## Testing Strategy

### Unit Tests (Vitest)
- API route handlers (/api/cards/register, /api/check-in)
- Utility functions (student ID validation, UID format checking)
- Database query builders

### Integration Tests
- Full card registration flow (desktop app â†’ API â†’ database)
- Full check-in flow (desktop app â†’ API â†’ database)
- Event activation/deactivation
- Offline queue sync

### Desktop App Testing
- NFC reader connection/disconnection
- Card scanning accuracy
- Offline mode behavior
- Error handling scenarios

### Manual Testing Checklist
- [ ] Student can register and log in with student_id + password
- [ ] Student can change password on first login
- [ ] Admin can bulk import student list from CSV
- [ ] Admin can create event
- [ ] Admin can import event registrations from Google Form
- [ ] Admin can activate event manually
- [ ] ESP32 device: Successfully reads card UIDs
- [ ] ESP32 device: Can switch between Registration and Check-in modes
- [ ] ESP32 device: WiFiManager setup works (can configure WiFi)
- [ ] Card activation: ESP32 sends UID to web interface
- [ ] Card activation: Admin can link card to student_id
- [ ] Card activation: OLED shows success confirmation
- [ ] Card activation: Prevents duplicate card assignments
- [ ] Event check-in: ESP32 successfully checks in student
- [ ] Event check-in: Displays student name + ID on OLED
- [ ] Duplicate check-in is prevented (same student, same event)
- [ ] Student dashboard shows correct attendance count and history
- [ ] Student can edit name, bio, and password
- [ ] Admin can view attendance for event
- [ ] Soft delete works for events
- [ ] Offline mode: ESP32 queues check-ins when WiFi unavailable
- [ ] Offline mode: Auto-syncs when WiFi restored
- [ ] Lost card can be reassigned via admin panel
- [ ] ESP32 battery lasts 4-6 hours continuous use

---

## Future Enhancements (Out of Scope for v1)

### Phase 2 Features
- **Mobile App**: React Native app for students (NFC phone tap instead of card)
- **QR Code Backup**: Fallback check-in method if NFC fails
- **Email Notifications**: Send confirmation after event attendance
- **Analytics Dashboard**: Graphs, trends, engagement metrics
- **Certificate Generation**: Auto-generate participation certificates
- **Offline Mode**: ESP32 stores check-ins locally if WiFi fails, syncs later

### Phase 3 Features
- **Multi-organization**: Support multiple student orgs on same platform
- **Payment Integration**: Paid events with Stripe
- **Feedback System**: Students rate events after attending
- **Skill Tracking**: Tag events with skills (e.g., "Public Speaking", "Web Dev")

---

## Support & Maintenance

### Monitoring
- **Vercel Analytics**: Track bandwidth, function execution time
- **Supabase Dashboard**: Monitor database size, API requests
- **Error Logging**: Sentry integration for production errors

### Backup Strategy
- **Database**: Supabase automatic daily backups (free tier)
- **Code**: GitHub repository (version controlled)
- **Environment Variables**: Documented in secure password manager

### Common Issues & Fixes

#### "Card not reading"
â†’ Check ESP32 power supply, ensure PN532 wired correctly

#### "Import failed - email not found"
â†’ Student needs to sign up first before registration import

#### "Already checked in"
â†’ Expected behavior, duplicate prevention working

#### "Function timeout"
â†’ Check for loops in code, switch to bulk operations

---

## Project Constraints & Limits

### Scale Limits (with optimizations)
- **Maximum students**: 850 (organizational limit)
- **Concurrent check-ins**: 10-15 per second (limited by USB reader speed, not software)
- **Events per month**: 5-10
- **Attendees per event**: 70 average, 150 max
- **Years of operation**: 10+ on free tier
- **Desktop apps**: Can run on Windows 7+, macOS 10.10+, Ubuntu 14.04+

### Technical Limits
- **Vercel function timeout**: 10 seconds (all operations under 3s)
- **Supabase database**: 500 MB (will use <2% even at max scale)
- **Bandwidth**: 100 GB/month (will use <0.1%)
- **Google Sheets API**: 100 requests/100s (need 5/month)
- **USB NFC reader**: ~1-2 cards/second reading speed

### Hardware Limits
- **Card reading distance**: 1-4 inches (student must intentionally tap)
- **ESP32 offline storage**: 100-200 check-ins (SPIFFS capacity)
- **Battery life**: 4-6 hours continuous use (3.7V Li-ion)
- **NFC reader speed**: ~1-2 cards/second
- **WiFi range**: Depends on venue (use mobile hotspot if needed)

### Business Rules
- **One card per student** - lifetime use
- **One active event** at a time (prevents confusion)
- **No retroactive attendance** - must check in during event
- **Registration required** - can't attend without prior registration
- **Admin approval** - for card assignments and imports
- **Student ID printed on card** - must match database format (YY-DD-NNN)

---

## Success Metrics

### Key Performance Indicators (KPIs)
- **Check-in speed**: <2 seconds per student
- **System uptime**: 99%+ availability
- **Data accuracy**: 0 duplicate check-ins
- **User satisfaction**: <5% support tickets per event
- **Hardware reliability**: <1 reader failure per 100 events

### Analytics to Track
- Average attendance per event
- Student engagement rate (attended events / registered events)
- Monthly active users
- Peak check-in times (to optimize reader placement)
- Most popular events

---

## Contact & Documentation

### Developer Documentation
- API routes documented with JSDoc comments
- README.md with setup instructions
- CONTRIBUTING.md for future contributors
- Hardware assembly guide with photos

### User Documentation
- Student guide: How to register and check in
- Admin guide: Managing events and imports
- Troubleshooting FAQ

### Support Channels
- GitHub Issues for bug reports
- Slack/Discord channel for admin support
- Email support for urgent issues

---

## Revision History

**v1.0 - Initial Implementation (Current)**
- Core attendance tracking system
- Manual event activation
- Google Form integration
- Student dashboard with basic stats
- Admin dashboard with event management
- ESP32 + PN532 NFC reader integration

---

## Notes for Future Reference

### Why These Decisions Were Made

**Why NTAG215/216 over MIFARE Classic?**
- Better compatibility, same price, future-proof for phone NFC

**Why manual event activation vs automatic?**
- Real-world events often start late or get rescheduled

**Why Google Forms vs built-in registration?**
- Students already familiar with Forms, easier adoption

**Why bulk import vs real-time sync?**
- Fewer API calls, admin can review before importing

**Why ESP32 over Raspberry Pi?**
- Cheaper, lower power, WiFi built-in, sufficient for task

**Why no realtime subscriptions?**
- Unnecessary bandwidth usage, polling/refresh is enough

**Why soft delete vs hard delete?**
- Data recovery possible, analytics preserved

**Why React Query over Redux?**
- Built-in caching, less boilerplate, perfect for server state

---

## Implementation Checklist

### Phase 1: Setup & Procurement (Week 1)
- [ ] Order ESP32 + PN532 + OLED components from Bangladesh vendors
- [ ] Order 250 pre-printed NFC cards from vendor (with student names + IDs)
- [ ] Set up Next.js project (or enhance existing deployed site)
- [ ] Set up Tailwind CSS (if not already configured)
- [ ] Create Supabase project and run migrations
- [ ] Configure password-based auth (student_id + password)
- [ ] Install and configure React Query
- [ ] Export student list CSV from Google Sheets (student_id, name, email)

### Phase 2: Student Account Setup (Week 1-2)
- [ ] Build student bulk import feature (CSV upload)
- [ ] Implement auto-password generation (password = student_id initially)
- [ ] Create email templates for account credentials
- [ ] Import all 200+ students to Supabase
- [ ] Send welcome emails with login credentials
- [ ] Build password change flow (force change on first login)

### Phase 3: Core Web Features (Week 2)
- [ ] Build student login page (student_id + password)
- [ ] Create student dashboard UI
- [ ] Implement profile editing (name, bio, password)
- [ ] Build admin dashboard skeleton
- [ ] Create event CRUD operations
- [ ] Implement event activation logic (manual start/stop)
- [ ] Build Google Sheets integration for event registrations

### Phase 4: API Development (Week 3)
- [ ] Build /api/cards/register endpoint (for card activation)
- [ ] Build /api/cards/notify endpoint (receive UID from ESP32)
- [ ] Build /api/cards/status/[uid] endpoint (activation status)
- [ ] Build /api/check-in endpoint (for event attendance)
- [ ] Implement attendance recording logic
- [ ] Add duplicate check-in prevention
- [ ] Build /api/events/active endpoint
- [ ] Add device authentication (API key)
- [ ] Create error handling middleware
- [ ] Write API tests

### Phase 5: Card Activation Web Interface (Week 3)
- [ ] Build /activate page (real-time card detection)
- [ ] Implement WebSocket or polling for ESP32 communication
- [ ] Create student ID input with validation
- [ ] Add progress counter (X/200 activated)
- [ ] Build recent activations list
- [ ] Test end-to-end activation flow

### Phase 6: ESP32 Firmware Development (Week 4)
- [ ] Set up Arduino IDE or PlatformIO
- [ ] Install all required libraries
- [ ] Implement NFC card reading (PN532)
- [ ] Implement OLED display driver
- [ ] Build WiFiManager captive portal
- [ ] Create mode switching logic (button + EEPROM)
- [ ] Implement Registration Mode (send UID to web interface)
- [ ] Implement Check-in Mode (validate & record attendance)
- [ ] Add offline queue (SPIFFS storage)
- [ ] Add LED status indicators
- [ ] Test with physical hardware

### Phase 7: Hardware Assembly (Week 5)
- [ ] Receive ESP32, PN532, OLED components
- [ ] Wire components on breadboard (I2C bus)
- [ ] Test NFC reading with sample cards
- [ ] Test OLED display output
- [ ] Test mode switching button
- [ ] Test WiFi configuration portal
- [ ] Test API communication
- [ ] Test offline mode and sync
- [ ] Assemble in enclosure (optional for testing)
- [ ] Set up battery + charging circuit (for production)

### Phase 8: Card Procurement & Activation (Week 6)
- [ ] Receive 250 printed NFC cards from vendor
- [ ] Verify card printing quality
- [ ] Sort cards alphabetically
- [ ] Set ESP32 to Registration Mode
- [ ] Open card activation web interface
- [ ] Activate all 200 cards (scan + type student ID)
- [ ] Verify all cards linked correctly in database
- [ ] Prepare cards for distribution at orientation

### Phase 9: Testing & Documentation (Week 7)
- [ ] Perform end-to-end testing of full workflow
- [ ] Test card activation process
- [ ] Test event check-in with multiple students
- [ ] Test offline mode thoroughly (disconnect WiFi)
- [ ] Test mode switching
- [ ] Test battery life (4-6 hour target)
- [ ] Write user documentation for admins
- [ ] Create quick-start guide for ESP32 device (1-page)
- [ ] Record video tutorial for card activation
- [ ] Train 2-3 admin staff on system
- [ ] Set up monitoring (Vercel Analytics, Supabase Dashboard)
- [ ] Create troubleshooting FAQ

### Phase 10: Orientation & Launch (Week 8)
- [ ] Set up card activation station at orientation
- [ ] ESP32 device in Registration Mode
- [ ] Laptop with activation interface open
- [ ] Distribute NFC cards to students (or activate pre-distributed cards)
- [ ] Monitor activation process (troubleshoot issues)
- [ ] Verify all students activated successfully
- [ ] Switch ESP32 to Check-in Mode
- [ ] Deploy website updates to Vercel production
- [ ] Run first live event with ESP32 check-in
- [ ] Monitor for issues during first event
- [ ] Gather feedback from students and admins
- [ ] Fix any bugs discovered
- [ ] Celebrate! ðŸŽ‰

### Ongoing Maintenance
- [ ] Weekly: Check Vercel and Supabase usage metrics
- [ ] Weekly: Check ESP32 device battery level
- [ ] Monthly: Review attendance data for anomalies
- [ ] Quarterly: Update ESP32 firmware if needed
- [ ] As needed: Reassign lost cards via admin panel
- [ ] As needed: Reconfigure ESP32 WiFi for new venues

---

**Last Updated**: December 2025  
**Document Version**: 1.0  
**Status**: Ready for Implementation