#ifndef CONFIG_H
#define CONFIG_H

// ==========================================
// HARDWARE PIN CONFIGURATION
// ==========================================

// RFID-RC522 (SPI Interface)
// Using VSPI (default SPI for ESP32)
// These pins are known to work reliably on most ESP32 boards
#define RC522_SS_PIN    5   // SDA/SS - Chip Select (GPIO5/D5)
#define RC522_RST_PIN   27  // Reset pin (GPIO27/D27)

// SPI Pins (ESP32 Default VSPI - Hardware defined, cannot change)
// MOSI: GPIO23 (hardware defined)
// MISO: GPIO19 (hardware defined)
// SCK:  GPIO18 (hardware defined)

// OLED Display (I2C Interface)
#define OLED_SDA_PIN   21   // I2C Data (ESP32 default)
#define OLED_SCL_PIN   4    // I2C Clock (CHANGED - was GPIO22, moved to avoid conflict)
#define OLED_I2C_ADDR  0x3C // I2C address (common for SSD1306)
#define OLED_WIDTH     128
#define OLED_HEIGHT    64
#define OLED_RESET     -1   // No reset pin

// Button
#define BUTTON_PIN     25   // Mode switch / Fetch event button
#define BUTTON_CLEAR_PIN 26 // Clear event button (new)

// Buzzer
#define BUZZER_PIN     32   // Buzzer for card detection feedback

// ==========================================
// FIRMWARE CONFIGURATION
// ==========================================

#define FIRMWARE_VERSION "1.0.0-phase3"
#define DEVICE_ID "device-001"

// API Configuration
#define API_URL "https://aatcc.vercel.app"  // Your PC's local IP where backend is running
#define DEVICE_API_KEY "0eb480a26f15e979371df45b1912160b5f380bab0fb087cee8f5557c707cd08a"
#define DEVICE_ID "device-001"

// API Endpoints
#define ENDPOINT_CARDS_DETECTED "/api/cards/detected"
#define ENDPOINT_CARDS_STATUS "/api/cards/status/"
#define ENDPOINT_CHECK_IN "/api/check-in"
#define ENDPOINT_EVENTS_ACTIVE "/api/events/active"

// Timing Constants
#define CARD_COOLDOWN 2000           // 2 seconds between same card reads (ms)
#define BUTTON_DEBOUNCE 50           // Button debounce time (ms)
#define API_TIMEOUT 10000            // 10 seconds HTTP timeout
#define CARD_SENT_WAIT_TIME 5000     // Wait 5 seconds after sending card before accepting next
#define BUTTON_LONG_PRESS 5000       // 5 seconds hold to switch modes
#define CHECKIN_SUCCESS_DISPLAY 1000 // Display welcome message for 1 second
#define BUZZER_DURATION 200          // Buzzer beep duration (ms)

// Serial Debug
#define SERIAL_BAUD 115200

#endif // CONFIG_H
