# ESP32 NFC Attendance System - Production Hardware Guide

**FINAL VERSION - FOR SOLDERING**  
**Date:** December 4, 2025  
**Firmware Version:** 1.0.0-phase3 (Production Ready)  
**Status:** ✅ TESTED AND VERIFIED

---

## ⚠️ CRITICAL - READ BEFORE SOLDERING

- **Double-check every connection before soldering**
- **Use a multimeter to verify continuity after soldering**
- **Test with breadboard first if unsure**
- **RC522 operates at 3.3V - connecting 5V will destroy it**

---

## 🔌 FINAL PRODUCTION CONNECTIONS

### Quick Connection Reference

```
ESP32 DevKit
    ├─ GPIO32  ──────► Buzzer (+)  [Active/Passive 3.3V]
    ├─ GPIO27  ──────► RC522 RST
    ├─ GPIO26  ──────► Button 2 (Clear Event)
    ├─ GPIO25  ──────► Button 1 (Fetch/Mode)
    ├─ GPIO23  ──────► RC522 MOSI  [Hardware SPI]
    ├─ GPIO21  ──────► OLED SDA    [Hardware I2C]
    ├─ GPIO19  ──────► RC522 MISO  [Hardware SPI]
    ├─ GPIO18  ──────► RC522 SCK   [Hardware SPI]
    ├─ GPIO5   ──────► RC522 SS
    ├─ GPIO4   ──────► OLED SCL    [Hardware I2C]
    ├─ VIN     ──────► RC522 3.3V  [⚠️ Important!]
    ├─ 3.3V    ──────► OLED VCC
    └─ GND     ──────► All GND (RC522, OLED, Buttons, Buzzer)
```

---

### ESP32 to RC522 (RFID Reader)

| RC522 Pin | ESP32 Pin | GPIO | Wire Color Recommendation |
|-----------|-----------|------|---------------------------|
| SDA (SS)  | D5        | GPIO5  | Yellow |
| SCK       | D18       | GPIO18 | Orange |
| MOSI      | D23       | GPIO23 | Blue |
| MISO      | D19       | GPIO19 | Green |
| IRQ       | -         | -      | **NOT CONNECTED** |
| GND       | GND       | GND    | Black |
| RST       | D27       | GPIO27 | White |
| 3.3V      | VIN       | VIN    | **Red (IMPORTANT: Use VIN for stable power)** |

**⚠️ CRITICAL:** Connect RC522 **3.3V** pin to ESP32 **VIN** pin (NOT 3.3V pin). RC522 needs more current than 3.3V regulator can provide.

---

### ESP32 to OLED Display (128x64 I2C)

| OLED Pin | ESP32 Pin | GPIO | Wire Color Recommendation |
|----------|-----------|------|---------------------------|
| VCC      | 3.3V      | 3.3V | Red |
| GND      | GND       | GND  | Black |
| SCL      | D4        | GPIO4  | Yellow |
| SDA      | D21       | GPIO21 | Green |

**Note:** OLED I2C Address = 0x3C (standard for SSD1306)

---

### Button 1 - Fetch Event / Mode Switch

| Button Terminal | ESP32 Pin | GPIO | Wire Color Recommendation |
|-----------------|-----------|------|---------------------------|
| Terminal 1      | D25       | GPIO25 | Blue |
| Terminal 2      | GND       | GND    | Black |

**Function:**
- **Short press:** Fetch active event from API (in Attendance Mode)
- **5-second hold:** Switch between Registration Mode ↔ Attendance Mode

---

### Button 2 - Clear Event

| Button Terminal | ESP32 Pin | GPIO | Wire Color Recommendation |
|-----------------|-----------|------|---------------------------|
| Terminal 1      | D26       | GPIO26 | Purple |
| Terminal 2      | GND       | GND    | Black |

**Function:**
- **Short press:** Clear current event from memory (in Attendance Mode)

---

### Buzzer - Card Detection Feedback

| Buzzer Terminal | ESP32 Pin | GPIO | Wire Color Recommendation |
|-----------------|-----------|------|---------------------------|
| Positive (+)    | D32       | GPIO32 | Red |
| Negative (-)    | GND       | GND    | Black |

**Function:**
- **Beeps for 200ms** when any NFC card is detected (both modes)

**Note:** Use passive buzzer (3.3V compatible). Active buzzers will work but may not stop cleanly.

---

## 📋 COMPLETE PIN SUMMARY

| ESP32 GPIO | Connected To | Function | Critical Notes |
|------------|--------------|----------|----------------|
| GPIO4      | OLED SCL     | I2C Clock | - |
| GPIO5      | RC522 SDA    | SPI Chip Select | - |
| GPIO18     | RC522 SCK    | SPI Clock | Hardware SPI |
| GPIO19     | RC522 MISO   | SPI Data In | Hardware SPI |
| GPIO21     | OLED SDA     | I2C Data | Hardware I2C |
| GPIO23     | RC522 MOSI   | SPI Data Out | Hardware SPI |
| GPIO25     | Button 1     | Fetch/Mode | Internal pull-up |
| GPIO26     | Button 2     | Clear Event | Internal pull-up |
| GPIO27     | RC522 RST    | Reset | - |
| GPIO32     | Buzzer (+)   | Card Feedback | 200ms beep |
| VIN        | RC522 3.3V   | Power | **Use VIN, not 3.3V!** |
| 3.3V       | OLED VCC     | Power | - |
| GND        | All GND      | Ground | Common ground |

---

## 🔋 POWER SUPPLY

**Development/Production:** 5V USB (Micro USB or USB-C depending on ESP32 board)
**Current Draw:**
- ESP32: ~80mA
- RC522: ~50mA (peak during card read)
- OLED: ~20mA
- Buzzer: ~30mA (peak during beep)
- **Total:** ~180mA (peak when card detected)

**Recommended:** Use quality USB power adapter (5V 1A minimum)
**Recommended:** Use quality USB power adapter (5V 1A minimum)

---

## 🛠️ SOLDERING TIPS

1. **Pre-tin all wires** before soldering
2. **Use heat shrink tubing** on all connections
3. **Keep wire lengths short** (< 15cm for SPI connections)
4. **Use solid core wire** for permanent connections (22-24 AWG)
5. **Label each wire** with masking tape before soldering
6. **Test continuity** with multimeter after each connection
7. **Solder in this order:**
   - Ground connections first
   - Power connections second
   - Signal wires last
8. **DO NOT** bridge adjacent pins

---

## ✅ POST-SOLDERING VERIFICATION

### Before Powering On:
- [ ] Visual inspection: No solder bridges
- [ ] Multimeter continuity test: GND to GND
- [ ] Multimeter continuity test: Each signal wire
- [ ] Verify NO shorts between VCC and GND
- [ ] Verify RC522 3.3V connected to ESP32 VIN (not 3.3V pin)

### After Powering On:
- [ ] ESP32 power LED lights up
- [ ] Measure VIN: Should be ~5V
- [ ] Measure 3.3V pin: Should be ~3.3V
- [ ] Tap NFC card - should be detected
- [ ] Buzzer beeps when card detected
- [ ] Press Button 1 - should show in Serial Monitor
- [ ] Tap NFC card - should be detected
- [ ] Press Button 1 - should show in Serial Monitor
- [ ] Press Button 2 - should show in Serial Monitor

---

## 🚨 TROUBLESHOOTING

| Problem | Check | Solution |
|---------|-------|----------|
| Button not responding | Ground | Verify button GND connection |
| Buzzer not beeping | Buzzer polarity | Check +/- terminals, try passive buzzer |
| Card reads unreliable | Wire length | Keep SPI wires < 15cm ||
| Button not responding | Ground | Verify button GND connection |
| Card reads unreliable | Wire length | Keep SPI wires < 15cm |
| Random resets | Power supply | Use better USB adapter (1A+) |
| Device won't boot | GPIO0/GPIO12 | Check no buttons connected to boot pins |

---

## 📦 ENCLOSURE RECOMMENDATIONS

**Minimum Size:** 100mm x 80mm x 40mm

**Mounting:**
- ESP32: Standoffs or hot glue (non-conductive)
- RC522: Double-sided tape (keep antenna clear)
- OLED: Front panel cutout (visible display)
- Buttons: Panel mount (easily accessible)

**Ventilation:** Add small holes for cooling

**Access:**
- USB port accessible for power
- Reset button accessible (if needed)

---

## 🔐 SECURITY NOTES

- **API Key** hardcoded in firmware (change for each device in production)
- **WiFi credentials** stored in ESP32 EEPROM (can be reset via WiFiManager)
- **Device ID** hardcoded in firmware (change for each device)

**For production deployment:**
- Generate unique API key per device
- Use unique Device ID per device
- Update `config.h` before flashing each unit

---

## 📊 PRODUCTION FIRMWARE STATUS

**Phases Implemented:**
- ✅ Phase 1: Card Detection (RC522 + OLED)
- ✅ Phase 2: Card Registration (WiFi + API)
- ✅ Phase 3: Event Attendance (Check-in system)

**Features:**
- ✅ WiFi Manager (captive portal for first setup)
- ✅ Two operating modes (Registration / Attendance)
- ✅ Mode switching (5-second button hold)
- ✅ Event fetching from API
- ✅ Student check-in with duplicate detection (409 error)
- ✅ Clear event functionality
- ✅ OLED feedback for all operations
- ✅ Serial debug output

**Tested Scenarios:**
- ✅ Card registration flow
- ✅ Card already activated detection
- ✅ Event fetch from API
- ✅ Student check-in
- ✅ Duplicate check-in prevention
- ✅ WiFi reconnection
- ✅ Button press detection (short/long)
- ✅ Mode switching

---

## 📝 FINAL CHECKLIST FOR PRODUCTION

- [ ] All connections soldered and verified
- [ ] Heat shrink applied to all joints
- [ ] Wire routing neat and secure
- [ ] No loose connections
- [ ] All components mounted securely
- [ ] Enclosure assembled
- [ ] Device ID configured in firmware
- [ ] API key configured in firmware
- [ ] WiFi credentials configured (or ready for first-time setup)
- [ ] Full system test performed
- [ ] Card registration tested
- [ ] Event check-in tested
- [ ] Duplicate check-in tested
- [ ] Button functions tested
- [ ] Mode switching tested
- [ ] Device labeled with ID

---

## 🎉 READY FOR PRODUCTION!

**Congratulations!** Your ESP32 NFC Attendance System is production-ready.

**GitHub Repository:** https://github.com/suhail-mujtabir/esp32-firmware-for-nfc

**Support:** Refer to firmware_progress.md for detailed implementation notes

**Happy deployment! 🚀**

---

**Document Version:** 1.0 FINAL  
**Last Updated:** December 4, 2025  
**Hardware Verified:** ✅  
**Firmware Verified:** ✅  
**Production Status:** READY ✅
