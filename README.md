# Cypher-Portal

**Educational Research Captive Portal System for ESP32**

> For authorized security testing and educational research only. Use only on networks you own or have explicit permission to test.

*By [Littlehakr](https://github.com/dkyazzentwatwa)*

---

## Overview

Cypher-Portal is an enhanced captive portal system for ESP32 with:
- **10 Business-Themed Templates** — Hotel, Coffee Shop, Corporate, Airport, Library, Conference, Retail, Device Setup, University, Medical
- **SSD1306 OLED Display** — Menu navigation and status display
- **3-Button Control** — Left/Center/Right for template selection
- **DNS + Web Server** — Captive portal detection bypass
- **Data Capture** — Circular buffer storing last 50 submissions
- **Serial Control** — Full AT command interface

---

## Quick Start

### Hardware
- ESP32 Dev Module
- SSD1306 128x64 OLED (I2C: SDA=5, SCL=4)
- 3 Tactile buttons (GPIO 34, 36, 39)

### Flash
```bash
arduino-cli compile --fqbn esp32:esp32:esp32 .
arduino-cli upload --fqbn esp32:esp32:esp32 -p /dev/ttyUSB0 .
```

### Connect
1. Power on ESP32
2. Connect to the broadcast SSID (e.g., "GrandHotel_FreeWiFi")
3. Browser auto-redirects to captive portal
4. Fill form → data captured on ESP32

---

## Button Controls

| Button | Action |
|--------|--------|
| **Left/Right** | Navigate menu / scroll templates |
| **Center** | Select / Start portal |
| **Hold Any** | Reset ESP32 |

---

## Template Selection

Navigate to `Start Portal` → Use L/R to scroll through 10 templates → C to launch

| # | Template | SSID | Research Use |
|---|----------|------|--------------|
| 1 | Hotel/Guest | GrandHotel_FreeWiFi | Hospitality compliance |
| 2 | Coffee Shop | BeanAndBrew_WiFi | Email opt-in patterns |
| 3 | Corporate | ACME_Corp_Secure | Authority exploitation |
| 4 | Airport | SkyLink_Airport | Travel urgency bias |
| 5 | Library | CityLibrary_Free | Institutional trust |
| 6 | Conference | TechSummit2024 | Event/social pressure |
| 7 | Retail | TechZone_WiFi | Lead capture research |
| 8 | Device Setup | SmartHome_Setup | IoT provisioning attack |
| 9 | University | MetroU_Campus | Educational compliance |
| 10 | Medical | Wellness_Medical | Healthcare trust patterns |

---

## Serial Commands

Connect at 115200 baud:

```
AT+PORTAL=START         # Start portal with current template
AT+PORTAL=STOP          # Stop portal
AT+PORTAL=STATUS        # Show current status
AT+PORTAL=LIST          # List all templates
AT+PORTAL=TEMPLATE=<name>   # Set active template
AT+PORTAL=CAPTURES      # View captured data
AT+PORTAL=CLEAR         # Clear capture buffer
AT+PORTAL=HELP          # Show help
```

---

## Web Interface

| URL | Description |
|-----|-------------|
| `/` | Captive portal page (auto-served) |
| `/captures` | View all captured submissions |
| `/clear` | Clear capture buffer |
| `/success` | Shown after form submission |

---

## Captured Data

- LED blinks 3x on each capture
- Circular buffer: 50 entries max
- View via `/captures` page or `AT+PORTAL=CAPTURES`
- Format: `Template: field1=value1 field2=value2 ...`

---

## Educational Research Notes

This tool demonstrates captive portal behavior and user response patterns:

- **Authority** — Corporate/medical templates leverage trust
- **Urgency** — Airport/event portals exploit time pressure  
- **Familiarity** — Hotel/library use recognizable contexts
- **Incentive** — Coffee/retail offer rewards for data
- **Provisioning** — Device setup mimics real IoT onboarding

### Ethical Use
- Only test networks you own or have written permission for
- Do not capture credentials for unauthorized access
- Document research findings responsibly
- Consider impact on users who submit data

---

## Libraries Required

```
Adafruit GFX Library
Adafruit SSD1306
```

Install via Arduino IDE: `Sketch → Include Library → Manage Libraries`

---

## License

MIT License — Educational and research purposes only.
