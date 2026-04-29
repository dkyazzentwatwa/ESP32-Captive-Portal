# Repository Guidelines

Contributor guide for **Cypher-Portal** — Educational Research Captive Portal System

---

## Project Structure

```
ESP32-Captive-Portal/
├── ESP32-Captive-Portal.ino   # Main sketch (all-in-one)
├── README.md                   # Project overview
├── LICENSE                     # MIT License
├── AGENTS.md                   # This file
└── (legacy assets preserved: src/, favicon.png)
```

> All portal logic lives in the single `.ino` file. This is an Arduino sketch, not a modular codebase.

---

## Build & Flash Commands

```bash
# Compile sketch
arduino-cli compile --fqbn esp32:esp32:esp32 .

# Upload to ESP32 (adjust port as needed)
arduino-cli upload --fqbn esp32:esp32:esp32 -p /dev/ttyUSB0 .

# Compile + upload combined
arduino-cli compile --fqbn esp32:esp32:esp32 . && \
  arduino-cli upload --fqbn esp32:esp32:esp32 -p /dev/ttyUSB0 .
```

**Board:** ESP32 Dev Module (`esp32:esp32:esp32`)  
**Baud:** 115200

---

## Coding Style

- **Indentation:** 2 spaces (Arduino convention)
- **Braces:** K&R style on same line
- **Constants:** `UPPER_SNAKE_CASE`
- **Enums:** `PascalCase` prefixed (e.g., `TEMPLATE_HOTEL`)
- **Global vars:** `camelCase` prefixed (e.g., `portalRunning`)
- **HTML templates:** Store in PROGMEM as raw string literals
- **Max line length:** ~120 chars (wrap long strings)

**Adding a new template:**
1. Add enum entry to `PortalTemplate`
2. Add to `TEMPLATE_NAMES[]` and `TEMPLATE_SSIDS[]`
3. Create `HTML_<TEMPLATE>[] PROGMEM` constant
4. Add case to `getTemplateHTML()` switch

---

## Testing

No automated test suite (Arduino sketch). Manual testing:

1. **Compile check** — `arduino-cli compile` must succeed
2. **Flash to board** — Verify boot screen on OLED
3. **Connect test** — Phone/laptop connects to SSID, browser redirects
4. **Form submit** — LED flashes 3x, data appears in captures
5. **Button nav** — L/R scroll templates, C selects

---

## Commit Guidelines

- Prefix: `feat:`, `fix:`, `docs:`, `refactor:`
- Example: `feat: add conference template with badge ID field`
- Reference issues: `Closes #12` in PR body
- Screenshots for UI changes

---

## Adding Templates

Each template needs:
1. Enum entry in `PortalTemplate`
2. Name in `TEMPLATE_NAMES[]`
3. SSID in `TEMPLATE_SSIDS[]`
4. HTML in PROGMEM
5. Case in `getTemplateHTML()` switch

```cpp
// Example: TEMPLATE_RESTAURANT
enum PortalTemplate {
  TEMPLATE_HOTEL = 0,
  TEMPLATE_COFFEE,
  // ... add here
  TEMPLATE_RESTAURANT,  // ← new entry
  TEMPLATE_COUNT         // ← always last
};
```

---

## Security Notes (Educational Research)

- Code is for authorized testing only
- No credential storage encryption (intentional for research)
- Captured data clears on reset or `AT+PORTAL=CLEAR`
- Always include disclaimer comments on sensitive functions

---

## Pin Reference

| Pin | Function |
|-----|----------|
| 2 | Built-in LED (capture flash) |
| 4 | OLED SCL |
| 5 | OLED SDA |
| 34 | Button LEFT |
| 36 | Button CENTER |
| 39 | Button RIGHT |
