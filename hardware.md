# ESP32 NFC Attendance System - Hardware Guide

**Last Updated:** December 3, 2025  
**Hardware Version:** 1.0 (Breadboard Prototype)  
**Firmware Phase:** Phase 3 - Attendance Mode with button controls

---

## Components List

| Component | Quantity | Notes |
|-----------|----------|-------|
| ESP32 DevKit (38-pin) | 1 | Main microcontroller |
| RFID-RC522 Module | 1 | 13.56MHz NFC reader (SPI interface) |
| 0.96" OLED Display | 1 | 128x64 pixels, I2C (SSD1306) |
| Push Button | 1 | Mode switch / Manual refresh |
| 10kΩ Resistor | 1 | Pull-down for button |
| NFC Cards | Multiple | MIFARE Classic or compatible |
| Breadboard | 1 | For prototyping |
| Jumper Wires | Set | Male-Male, Male-Female, Female-Female |
| Micro USB Cable | 1 | Power + programming |

**Optional Components:**
- RGB LED + 220Ω resistors (status indicators)
- Buzzer (audio feedback)
- 3.7V Li-ion battery + TP4056 charger (portable operation)

---

## Pin Connections

### RFID-RC522 to ESP32 (SPI Interface)

**IMPORTANT:** RC522 operates at 3.3V. Connect to ESP32's 3.3V pin, NOT 5V!

| RC522 Pin | ESP32 Pin          | Pin Number          | Description |
|-----------|--------------------|---------------------|-------------|
| SDA (SS) | GPIO5         | D5  | SPI Chip Select     |
| SCK      | GPIO18        | D18 | SPI Clock           |
| MOSI     | GPIO23        | D23 | SPI Master Out      |
| MISO     | GPIO19        | D19 | SPI Master In       |
| IRQ      | Not connected | -   | Not used            |
| GND      | GND           | GND | Ground              |
| RST      | GPIO22        | D22 | Reset               |
| 3.3V     | 3.3V          | 3.3V | Power (3.3V ONLY!) |

**Notes:**
- ESP32 default SPI pins: MOSI=23, MISO=19, SCK=18
- SDA/SS pin is configurable (using GPIO5 in this project)
- RST pin is configurable (using GPIO22 in this project)
- **Never connect 5V to RC522!** It will damage the module.

---

### OLED Display to ESP32 (I2C Interface)

| OLED Pin | ESP32 Pin | Pin Number | Description |
|----------|-----------|------------|-------------|
| VCC | 3.3V | 3.3V | Power |
| GND | GND | GND | Ground |
| SCL | GPIO21 | D21 | I2C Clock |
| SDA | GPIO22 | D22 | I2C Data |

**Wait, there's a conflict!** GPIO22 is used for both RC522 RST and OLED SCL.

**Solution:** Move RC522 RST to a different pin. Let's use **GPIO4**:

| RC522 Pin | ESP32 Pin | Pin Number | Description |
|-----------|-----------|------------|-------------|
| RST | GPIO4 | D4 | Reset (CHANGED) |

**Updated OLED Connection:**

| OLED Pin | ESP32 Pin | Pin Number | Description |
|----------|-----------|------------|-------------|
| VCC | 3.3V | 3.3V | Power |
| GND | GND | GND | Ground |
| SCL | GPIO22 | D22 | I2C Clock (default) |
| SDA | GPIO21 | D21 | I2C Data (default) |

**Notes:**
- ESP32 default I2C pins: SDA=21, SCL=22
- OLED I2C address is typically 0x3C
- Some OLED modules have different pin orders (GND, VCC, SCL, SDA)

---

### Push Button to ESP32

**Phase 3 Update:** Button now supports multiple press types for mode switching and event management.

**Button Functions:**
- **Single tap**: Clear event from memory (in Attendance Mode)
- **Double tap**: Fetch active event from API (when no event loaded)
- **5-second hold**: Switch between Registration Mode ↔ Attendance Mode

| Button Terminal | ESP32 Pin | Description |
|-----------------|-----------|-------------|
| Terminal 1 | GPIO25 | Digital input (with internal pull-up) |
| Terminal 2 | GND | Ground |

**Notes:**
- We'll use ESP32's internal pull-up resistor (INPUT_PULLUP in code)
- Button press = LOW signal (active-low)
- GPIO25 is safe for input and boot
- Button timing handled in firmware:
  * Single tap: < 500ms
  * Double tap: Two taps within 500ms window
  * Long press: ≥ 5000ms (5 seconds)

---

## Complete Wiring Diagram

```
ESP32 DevKit (38-pin)
┌─────────────────────────────────┐
│                                 │
│  3.3V  ──┬─→ RC522 VCC          │
│          └─→ OLED VCC           │
│                                 │
│  GND   ──┬─→ RC522 GND          │
│          ├─→ OLED GND           │
│          └─→ Button Terminal 2  │
│                                 │
│  GPIO5  ───→ RC522 SDA (SS)     │
│  GPIO18 ───→ RC522 SCK          │
│  GPIO23 ───→ RC522 MOSI         │
│  GPIO19 ───→ RC522 MISO         │
│  GPIO4  ───→ RC522 RST          │
│                                 │
│  GPIO21 ───→ OLED SDA           │
│  GPIO22 ───→ OLED SCL           │
│                                 │
│  GPIO25 ───→ Button Terminal 1  │
│       (internal pull-up enabled)│
│                                 │
└─────────────────────────────────┘

RC522 IRQ pin: Not connected
```

---

## Pin Summary Table

| GPIO Pin | Function | Connected To |
|----------|----------|--------------|
| GPIO4 | RC522 Reset | RC522 RST |
| GPIO5 | RC522 Chip Select | RC522 SDA/SS |
| GPIO18 | SPI Clock | RC522 SCK |
| GPIO19 | SPI MISO | RC522 MISO |
| GPIO21 | I2C SDA | OLED SDA |
| GPIO22 | I2C SCL | OLED SCL |
| GPIO23 | SPI MOSI | RC522 MOSI |
| GPIO25 | Button Input | Push Button (mode switch) |
| 3.3V | Power | RC522 VCC, OLED VCC |
| GND | Ground | All GND pins |

**Reserved Pins (Do Not Use):**
- GPIO0: Boot mode selection (connected to button on some boards)
- GPIO2: Boot mode, onboard LED
- GPIO6-11: Connected to flash memory
- GPIO12: Boot mode (MTDI)

---

## I2C and SPI Bus Notes

### I2C Bus (OLED)
- **Address:** 0x3C (default for most SSD1306 OLED displays)
- **Speed:** 100kHz (standard) or 400kHz (fast mode)
- **Pins:** SDA=21, SCL=22 (ESP32 default Wire pins)

### SPI Bus (RC522)
- **Mode:** SPI Mode 0 (CPOL=0, CPHA=0)
- **Speed:** Up to 10MHz (RC522 supports)
- **Chip Select:** GPIO5 (active LOW)
- **Pins:** MOSI=23, MISO=19, SCK=18 (ESP32 default VSPI pins)

---

## Power Supply

### Development/Testing:
- **Power Source:** USB cable (5V)
- **ESP32 Regulator:** Onboard 3.3V regulator supplies RC522 and OLED
- **Current Draw:** ~150mA total (ESP32: 80mA, RC522: 50mA, OLED: 20mA)

### Production/Portable (Optional):
- **Battery:** 3.7V Li-ion 18650 (2000-3000mAh)
- **Charger:** TP4056 charging module with micro USB
- **Runtime:** 4-6 hours continuous operation
- **Protection:** TP4056 includes overcharge/over-discharge protection

**Note:** For initial testing, USB power is sufficient. Battery setup is optional.

---

## Assembly Instructions

### Step 1: Prepare Components
1. Check all components are available
2. Identify pin labels on RC522 and OLED modules
3. Prepare breadboard and jumper wires

### Step 2: Connect RC522 Module
1. **CRITICAL:** Verify RC522 operates at 3.3V (check module label)
2. Connect RC522 to breadboard
3. Wire connections as per table above:
   - 3.3V → RC522 VCC
   - GND → RC522 GND
   - GPIO5 → RC522 SDA
   - GPIO18 → RC522 SCK
   - GPIO23 → RC522 MOSI
   - GPIO19 → RC522 MISO
   - GPIO4 → RC522 RST
4. **Double-check:** No 5V connections to RC522!

### Step 3: Connect OLED Display
1. Connect OLED to breadboard
2. Wire connections:
   - 3.3V → OLED VCC
   - GND → OLED GND
   - GPIO21 → OLED SDA
   - GPIO22 → OLED SCL
3. Note: Some OLED modules have pins in different order (check yours!)

### Step 4: Connect Button
1. Place button on breadboard (across center gap)
2. Connect one terminal to GPIO25
3. Connect other terminal to GND
4. (Internal pull-up will be enabled in code - INPUT_PULLUP mode)
5. Test: Press button, it should connect GPIO25 to GND (LOW)

### Step 5: Connect ESP32
1. Place ESP32 on breadboard
2. Connect all power rails (3.3V and GND)
3. Connect USB cable to computer
4. Verify power LED on ESP32 lights up

### Step 6: Test Connections
1. Use multimeter to verify:
   - 3.3V rail is at 3.3V
   - GND connections are continuous
   - No shorts between pins
2. Visually inspect all connections match diagram

---

## Troubleshooting Hardware

### RC522 Not Detected
- **Check:** 3.3V power supply (measure with multimeter)
- **Check:** All SPI wires connected correctly
- **Check:** RST and SS pins are correct GPIOs
- **Try:** Shorter jumper wires (SPI sensitive to long wires)
- **Try:** Re-seat RC522 module on breadboard
- **Try:** Different RC522 module (some Chinese clones are defective)

### OLED Not Displaying
- **Check:** I2C address (run I2C scanner sketch)
- **Check:** SDA and SCL not swapped
- **Check:** Power connections (3.3V and GND)
- **Try:** Different OLED module
- **Check:** OLED pin order (VCC/GND vs GND/VCC)

### Button Not Responding
- **Check:** Button is not damaged (test with multimeter continuity)
- **Check:** GPIO25 is not used by another component
- **Check:** Button wiring: One side to GPIO25, other side to GND
- **Test:** Short GPIO25 to GND with wire (should trigger button press in Serial Monitor)
- **Try:** External pull-down resistor (10kΩ) if internal pull-up not working
- **Check:** Button is pressed correctly (across pins, not diagonal)
- **Check:** Serial Monitor shows "Button pressed" messages

### ESP32 Won't Boot
- **Check:** GPIO0 not pulled LOW during boot
- **Check:** GPIO12 not pulled HIGH during boot
- **Unplug:** All peripherals and test ESP32 alone
- **Try:** Different USB cable or port
- **Check:** Onboard LED blinks during programming

### Random Behavior / Crashes
- **Check:** Power supply is stable (use powered USB hub)
- **Add:** 100µF capacitor between 3.3V and GND (near ESP32)
- **Check:** No loose connections on breadboard
- **Try:** Shorter wires to reduce noise
- **Check:** ESP32 is not overheating

---

## Safety Notes

⚠️ **CRITICAL:** 
- **RC522 operates at 3.3V ONLY.** Connecting 5V will permanently damage it!
- Never connect/disconnect modules while powered on
- Check all connections before applying power
- Use a current-limited power supply during testing

✅ **Good Practices:**
- Color-code wires (Red=3.3V, Black=GND, others=signals)
- Label modules on breadboard
- Take photos of working setup for reference
- Keep components away from static-prone surfaces

---

## Next Steps

1. ✅ Hardware connections complete
2. ⏳ Install Arduino libraries
3. ⏳ Test RC522 with example sketch
4. ⏳ Test OLED with example sketch
5. ⏳ Test button input
6. ⏳ Integrate all modules in firmware

---

## References

- [MFRC522 Library GitHub](https://github.com/miguelbalboa/rfid)
- [Adafruit SSD1306 Library](https://github.com/adafruit/Adafruit_SSD1306)
- [ESP32 Pinout Reference](https://randomnerdtutorials.com/esp32-pinout-reference-gpios/)
- [RC522 Datasheet](https://www.nxp.com/docs/en/data-sheet/MFRC522.pdf)
- [Random Nerd Tutorials - RC522 with Arduino](https://randomnerdtutorials.com/security-access-using-mfrc522-rfid-reader-with-arduino/)

---

**Document Version:** 1.0  
**Status:** Ready for Implementation  
**Verified:** Pinout conflicts resolved (RC522 RST moved to GPIO4)
