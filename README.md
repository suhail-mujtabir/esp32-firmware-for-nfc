# ESP32 NFC Attendance System

**Firmware for ESP32-based NFC attendance tracking device**

## Project Status

**Current Phase:** Phase 1 - Card Detection  
**Progress:** See `firmware_progress.md`

## Hardware Requirements

- ESP32 DevKit (38-pin)
- RFID-RC522 NFC Reader Module
- 0.96" OLED Display (128x64, I2C, SSD1306)
- Push Button
- NFC Cards (MIFARE Classic or compatible)
- Jumper wires and breadboard

**See `hardware.md` for complete wiring diagram**

## Software Requirements

- [PlatformIO](https://platformio.org/) or [Arduino IDE](https://www.arduino.cc/en/software)
- USB drivers for ESP32

## Quick Start

### 1. Install PlatformIO

```bash
# Using VS Code
# Install PlatformIO IDE extension from VS Code marketplace
```

### 2. Clone/Open Project

```bash
cd e:\aatcc-esp32-firmware
```

### 3. Build and Upload

```bash
# Using PlatformIO CLI
pio run --target upload

# Or use PlatformIO IDE buttons in VS Code
```

### 4. Open Serial Monitor

```bash
pio device monitor -b 115200

# Or use VS Code Serial Monitor
```

## Project Structure

```
aatcc-esp32-firmware/
├── platformio.ini          # PlatformIO configuration
├── include/
│   └── config.h           # Pin definitions and constants
├── src/
│   └── main.cpp           # Main firmware code (Phase 1)
├── hardware.md            # Hardware wiring guide
├── firmware_progress.md   # Implementation progress tracker
├── ESP32_FIRMWARE_PLAN.md # Complete implementation plan
└── README.md             # This file
```

## Phase 1: Card Detection

**Goal:** Read NFC card UIDs and display on Serial Monitor + OLED

### Features

- ✓ RFID-RC522 initialization and communication
- ✓ OLED display initialization
- ✓ Card UID reading (formatted as "A1:B2:C3:D4")
- ✓ Serial Monitor output with card details
- ✓ OLED visual feedback
- ✓ Card cooldown (prevents duplicate reads)

### Testing

1. Connect hardware as per `hardware.md`
2. Upload firmware to ESP32
3. Open Serial Monitor (115200 baud)
4. Tap NFC card on RC522 reader
5. Verify UID appears on Serial Monitor and OLED

### Troubleshooting

**RC522 not detected:**
- Check 3.3V power (NOT 5V!)
- Verify SPI connections (MOSI, MISO, SCK, SS, RST)
- Try shorter jumper wires
- See `hardware.md` troubleshooting section

**OLED not working:**
- Check I2C connections (SDA=21, SCL=22)
- Verify I2C address (0x3C is common)
- Run I2C scanner to detect address

**No card reads:**
- Verify card is MIFARE Classic or compatible (13.56 MHz)
- Try different distance (1-3cm from reader)
- Check card is not damaged

## Next Phases

- **Phase 2:** Card activation system with WiFi and API integration
- **Phase 3:** Event attendance recording system
- **Phase 4:** Production deployment and optimizations

See `firmware_progress.md` for detailed phase breakdown.

## Pin Configuration

| Component | Pin | GPIO |
|-----------|-----|------|
| RC522 SDA/SS | D5 | GPIO5 |
| RC522 RST | D4 | GPIO4 |
| RC522 MOSI | D23 | GPIO23 |
| RC522 MISO | D19 | GPIO19 |
| RC522 SCK | D18 | GPIO18 |
| OLED SDA | D21 | GPIO21 |
| OLED SCL | D22 | GPIO22 |
| Button | D15 | GPIO15 |

**See `hardware.md` for complete wiring diagram**

## Libraries Used

- `MFRC522` - RFID reader communication
- `Adafruit SSD1306` - OLED display driver
- `Adafruit GFX` - Graphics library
- `ArduinoJson` - JSON parsing (Phase 2+)
- `WiFiManager` - WiFi configuration (Phase 2+)

## License

This project is part of the AATCC NFC Attendance System.

## Support

For issues or questions:
1. Check `hardware.md` troubleshooting section
2. Check `firmware_progress.md` for known issues
3. Review serial monitor output for error messages

---

**Last Updated:** December 2, 2025  
**Firmware Version:** 1.0.0-phase1  
**Status:** Phase 1 - Ready for Testing
