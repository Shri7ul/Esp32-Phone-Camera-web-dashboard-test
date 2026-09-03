#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <DHT.h>


// =====================================================
// DEVICE ID
// =====================================================
// ESP32 #01 -> "01"
// ESP32 #02 -> "02"
// ESP32 #03 -> "03"
// =====================================================

#define DEVICE_ID "01"


// =====================================================
// DHT22
// =====================================================

#define DHT_PIN   1
#define DHTTYPE   DHT22

DHT dht(
  DHT_PIN,
  DHTTYPE
);


// =====================================================
// BOOT BUTTON
// ESP32-S3 BOOT = GPIO0
// =====================================================

#define BOOT_BUTTON 0

#define CONFIG_HOLD_TIME 3000


// =====================================================
// DEFAULT WIFI
// =====================================================
// Saved Wi-Fi fail করলে এগুলো try করবে.
// নিজের Lab/default Wi-Fi এখানে বসাও.
// =====================================================

const char* DEFAULT_WIFI_SSID =
  "YOUR_DEFAULT_WIFI";

const char* DEFAULT_WIFI_PASSWORD =
  "YOUR_DEFAULT_PASSWORD";


// =====================================================
// MANAGEMENT ACCESS POINT
// =====================================================

String AP_SSID =
  String("RoomMonitor-") +
  DEVICE_ID;

const char* AP_PASSWORD =
  "12345678";


// =====================================================
// WEB SERVER
// =====================================================

WebServer server(
  80
);


// =====================================================
// DNS SERVER
// শুধু setup/captive portal mode-এ ব্যবহার হবে
// =====================================================

DNSServer dnsServer;

const byte DNS_PORT = 53;


// =====================================================
// NVS / PREFERENCES
// =====================================================

Preferences preferences;


// =====================================================
// SAVED WIFI
// =====================================================

String savedSSID = "";
String savedPassword = "";


// =====================================================
// DEVICE STATE
// =====================================================

bool setupMode = false;

bool wifiConnected = false;


// =====================================================
// DHT DATA
// =====================================================

float temperature = NAN;
float humidity = NAN;

bool dhtOK = false;

unsigned long lastDHTRead = 0;

const unsigned long DHT_INTERVAL =
  2500;


// =====================================================
// BOOT BUTTON
// =====================================================

bool buttonPressed = false;

unsigned long buttonStart = 0;


// =====================================================
// FORWARD DECLARATIONS
// =====================================================

void startSetupMode();

void startNormalMode();

void handleRoot();

void handleWiFiPage();

void handleSaveWiFi();

void handleResetWiFi();

void handleData();

void handleNotFound();

void handleSetupPage();

void readDHT();

void loadWiFiCredentials();

void saveWiFiCredentials(
  String ssid,
  String password
);

void clearWiFiCredentials();

bool connectToWiFi(
  String ssid,
  String password
);

void startAccessPoint();

void checkBootButton();


// =====================================================
// LOAD SAVED WIFI
// =====================================================

void loadWiFiCredentials() {

  preferences.begin(
    "wifi",
    true
  );


  savedSSID =
    preferences.getString(
      "ssid",
      ""
    );


  savedPassword =
    preferences.getString(
      "password",
      ""
    );


  preferences.end();


  Serial.println();
  Serial.println(
    "Stored Wi-Fi"
  );

  Serial.println(
    "-------------------------"
  );


  if (
    savedSSID.length() > 0
  ) {

    Serial.print(
      "SSID: "
    );

    Serial.println(
      savedSSID
    );

  }

  else {

    Serial.println(
      "No saved Wi-Fi"
    );

  }

}


// =====================================================
// SAVE WIFI
// =====================================================

void saveWiFiCredentials(
  String ssid,
  String password
) {

  preferences.begin(
    "wifi",
    false
  );


  preferences.putString(
    "ssid",
    ssid
  );


  preferences.putString(
    "password",
    password
  );


  preferences.end();


  Serial.println();

  Serial.println(
    "Wi-Fi credentials saved."
  );

}


// =====================================================
// CLEAR WIFI
// =====================================================

void clearWiFiCredentials() {

  preferences.begin(
    "wifi",
    false
  );


  preferences.clear();


  preferences.end();


  Serial.println(
    "Saved Wi-Fi cleared."
  );

}


// =====================================================
// START ACCESS POINT
// =====================================================

void startAccessPoint() {

  Serial.println();

  Serial.println(
    "Starting Management AP..."
  );


  // AP + STA

  WiFi.mode(
    WIFI_AP_STA
  );


  delay(300);


  bool result =
    WiFi.softAP(
      AP_SSID.c_str(),
      AP_PASSWORD
    );


  if (result) {

    Serial.println();

    Serial.println(
      "========================================"
    );

    Serial.println(
      "MANAGEMENT AP READY"
    );

    Serial.println(
      "========================================"
    );


    Serial.print(
      "AP SSID: "
    );

    Serial.println(
      AP_SSID
    );


    Serial.print(
      "AP Password: "
    );

    Serial.println(
      AP_PASSWORD
    );


    Serial.print(
      "AP IP: "
    );

    Serial.println(
      WiFi.softAPIP()
    );


    Serial.println(
      "AP Dashboard Port: 80"
    );


    Serial.println(
      "========================================"
    );

  }

  else {

    Serial.println(
      "ERROR: Failed to start AP"
    );

  }

}


// =====================================================
// CONNECT WIFI
// =====================================================

bool connectToWiFi(
  String ssid,
  String password
) {


  if (
    ssid.length() == 0
  ) {

    return false;

  }


  Serial.println();

  Serial.println(
    "----------------------------------------"
  );


  Serial.print(
    "Trying Wi-Fi: "
  );

  Serial.println(
    ssid
  );


  WiFi.begin(
    ssid.c_str(),
    password.c_str()
  );


  unsigned long start =
    millis();


  while (
    WiFi.status() != WL_CONNECTED &&
    millis() - start < 15000
  ) {

    delay(500);

    Serial.print(
      "."
    );

  }


  Serial.println();


  if (
    WiFi.status() ==
    WL_CONNECTED
  ) {


    wifiConnected =
      true;


    Serial.println(
      "Wi-Fi CONNECTED"
    );


    Serial.print(
      "SSID: "
    );

    Serial.println(
      WiFi.SSID()
    );


    Serial.print(
      "Lab Wi-Fi IP: "
    );

    Serial.println(
      WiFi.localIP()
    );


    Serial.print(
      "Gateway: "
    );

    Serial.println(
      WiFi.gatewayIP()
    );


    Serial.print(
      "RSSI: "
    );

    Serial.print(
      WiFi.RSSI()
    );

    Serial.println(
      " dBm"
    );


    Serial.println(
      "----------------------------------------"
    );


    return true;

  }


  wifiConnected =
    false;


  Serial.println(
    "Wi-Fi FAILED"
  );


  WiFi.disconnect(
    false
  );


  delay(300);


  return false;

}


// =====================================================
// START SETUP MODE
// =====================================================

void startSetupMode() {

  setupMode =
    true;


  Serial.println();

  Serial.println(
    "========================================"
  );

  Serial.println(
    "WIFI SETUP MODE"
  );

  Serial.println(
    "========================================"
  );


  // Start AP

  if (
    WiFi.getMode() != WIFI_AP_STA
  ) {

    WiFi.mode(
      WIFI_AP_STA
    );

  }


  // Ensure AP is running

  if (
    WiFi.softAPIP() ==
    IPAddress(0, 0, 0, 0)
  ) {

    startAccessPoint();

  }


  IPAddress apIP =
    WiFi.softAPIP();


  // ===================================================
  // DNS
  // ===================================================

  dnsServer.start(
    DNS_PORT,
    "*",
    apIP
  );


  // ===================================================
  // ROUTES
  // ===================================================

  server.on(
    "/",
    HTTP_GET,
    handleSetupPage
  );


  server.on(
    "/save",
    HTTP_POST,
    handleSaveWiFi
  );


  server.on(
    "/reset",
    HTTP_GET,
    handleResetWiFi
  );


  server.on(
    "/generate_204",
    HTTP_GET,
    handleSetupPage
  );


  server.on(
    "/hotspot-detect.html",
    HTTP_GET,
    handleSetupPage
  );


  server.on(
    "/connecttest.txt",
    HTTP_GET,
    handleSetupPage
  );


  server.on(
    "/ncsi.txt",
    HTTP_GET,
    handleSetupPage
  );


  server.onNotFound(
    handleSetupPage
  );


  server.begin();


  Serial.println();

  Serial.println(
    "SETUP PORTAL READY"
  );


  Serial.print(
    "Open: http://"
  );

  Serial.println(
    apIP
  );

}


// =====================================================
// SETUP PAGE
// =====================================================

void handleSetupPage() {


  String html = R"rawliteral(

<!DOCTYPE html>

<html>

<head>

<meta charset="UTF-8">

<meta name="viewport"
      content="width=device-width,
               initial-scale=1.0">

<title>Room Monitor Setup</title>


<style>

* {
  box-sizing: border-box;
}

body {

  margin: 0;

  padding: 20px;

  background: #111827;

  color: white;

  font-family:
    Arial,
    sans-serif;

}

.container {

  max-width: 520px;

  margin: auto;

}

.card {

  margin-top: 25px;

  padding: 28px;

  background: #1f2937;

  border-radius: 22px;

}

h1 {

  text-align: center;

  color: #22c55e;

  margin-bottom: 8px;

}

.subtitle {

  text-align: center;

  color: #cbd5e1;

  margin-bottom: 25px;

  line-height: 1.5;

}

.device {

  text-align: center;

  background: #111827;

  padding: 14px;

  border-radius: 12px;

  margin-bottom: 20px;

}

label {

  display: block;

  margin-top: 18px;

  margin-bottom: 7px;

  font-weight: bold;

}

input {

  width: 100%;

  padding: 14px;

  border-radius: 10px;

  border: 1px solid #4b5563;

  background: #111827;

  color: white;

  font-size: 16px;

}

button {

  width: 100%;

  padding: 15px;

  margin-top: 24px;

  border: none;

  border-radius: 11px;

  background: #22c55e;

  color: white;

  font-size: 17px;

  font-weight: bold;

}

.info {

  margin-top: 20px;

  padding: 15px;

  background: #111827;

  border-radius: 12px;

  line-height: 1.6;

  font-size: 14px;

}

.warning {

  color: #facc15;

}

</style>

</head>


<body>


<div class="container">


<div class="card">


<h1>

🏠 Room Monitor

</h1>


<div class="subtitle">

Wi-Fi Configuration

</div>


<div class="device">

<b>Device:</b>

RoomMonitor-%DEVICE%

<br>

<b>Setup IP:</b>

192.168.4.1

</div>


<form
  action="/save"
  method="POST">


<label>

Wi-Fi Name / SSID

</label>


<input

  type="text"

  name="ssid"

  placeholder="Enter Wi-Fi name"

  required>


<label>

Wi-Fi Password

</label>


<input

  type="password"

  name="password"

  placeholder="Enter Wi-Fi password">


<button
  type="submit">

Save Wi-Fi & Restart

</button>


</form>


<div class="info">

<b>Management Hotspot</b>

<br><br>

SSID:

<strong>
RoomMonitor-%DEVICE%
</strong>

<br>

Password:

<strong>
12345678
</strong>

<br><br>

After saving, ESP32 will restart
and connect to your Wi-Fi.

</div>


<div class="info warning">

If the saved Wi-Fi and default Wi-Fi
both fail, this setup page will
automatically become available again.

<br><br>

You can also hold the ESP32 BOOT
button for 3 seconds to enter
Wi-Fi setup anytime.

</div>


</div>


</div>


</body>

</html>

)rawliteral";


  html.replace(
    "%DEVICE%",
    DEVICE_ID
  );


  server.send(
    200,
    "text/html; charset=UTF-8",
    html
  );

}


// =====================================================
// SAVE WIFI
// =====================================================

void handleSaveWiFi() {


  if (
    !server.hasArg(
      "ssid"
    )
  ) {

    server.send(
      400,
      "text/plain",
      "SSID missing"
    );

    return;

  }


  String newSSID =
    server.arg(
      "ssid"
    );


  String newPassword =
    server.arg(
      "password"
    );


  newSSID.trim();


  if (
    newSSID.length() == 0
  ) {

    server.send(
      400,
      "text/plain",
      "SSID cannot be empty"
    );

    return;

  }


  saveWiFiCredentials(
    newSSID,
    newPassword
  );


  String html = R"rawliteral(

<!DOCTYPE html>

<html>

<head>

<meta charset="UTF-8">

<meta name="viewport"
content="width=device-width,
initial-scale=1.0">

<title>Wi-Fi Saved</title>

<style>

body {

  background: #111827;

  color: white;

  font-family: Arial;

  text-align: center;

  padding: 50px 20px;

}

.card {

  max-width: 500px;

  margin: auto;

  padding: 30px;

  background: #1f2937;

  border-radius: 20px;

}

h1 {

  color: #22c55e;

}

</style>

</head>


<body>

<div class="card">

<h1>

✓ Wi-Fi Saved

</h1>

<p>

ESP32 is restarting...

</p>

<p>

Please reconnect to the configured Wi-Fi.

</p>

</div>

</body>

</html>

)rawliteral";


  server.send(
    200,
    "text/html; charset=UTF-8",
    html
  );


  delay(1500);


  ESP.restart();

}


// =====================================================
// RESET WIFI
// =====================================================

void handleResetWiFi() {


  clearWiFiCredentials();


  server.send(
    200,
    "text/html; charset=UTF-8",
    "<h2>Wi-Fi credentials cleared.<br>Restarting...</h2>"
  );


  delay(1500);


  ESP.restart();

}


// =====================================================
// NORMAL DASHBOARD
// =====================================================

void handleRoot() {


  String html = R"rawliteral(

<!DOCTYPE html>

<html>

<head>

<meta charset="UTF-8">

<meta name="viewport"
      content="width=device-width,
               initial-scale=1.0">

<meta http-equiv="refresh"
      content="5">


<title>Room Temperature Monitoring</title>


<style>

* {
  box-sizing: border-box;
}

body {

  margin: 0;

  background: #f4f8fc;

  color: #111827;

  font-family: Arial, sans-serif;

}

.container {

  max-width: 900px;

  margin: 25px auto;

  padding: 20px;

}

h1 {

  text-align: center;

  color: #ef3333;

}

.device {

  text-align: center;

  color: #4b5563;

  margin-bottom: 25px;

}

.cards {

  display: grid;

  grid-template-columns:
    1fr 1fr;

  gap: 20px;

}

.card {

  background: white;

  border: 2px solid #222;

  border-radius: 22px;

  padding: 30px;

  text-align: center;

}

.label {

  font-size: 18px;

  font-weight: bold;

}

.value {

  font-size: 40px;

  font-weight: bold;

  margin-top: 18px;

}

.status {

  margin-top: 20px;

  background: white;

  border-radius: 18px;

  padding: 22px;

  text-align: center;

  line-height: 1.8;

}

.online {

  color: #16a34a;

  font-weight: bold;

}

.offline {

  color: #dc2626;

  font-weight: bold;

}

.info {

  margin-top: 20px;

  padding: 20px;

  background: #111827;

  color: white;

  border-radius: 18px;

}

.config {

  display: block;

  margin-top: 20px;

  padding: 15px;

  background: #2563eb;

  color: white;

  text-align: center;

  text-decoration: none;

  border-radius: 11px;

  font-weight: bold;

}

@media(max-width:600px) {

  .cards {

    grid-template-columns: 1fr;

  }

}

</style>

</head>


<body>


<div class="container">


<h1>

Room Temperature Monitoring

</h1>


<div class="device">

Device:
<b>
RoomMonitor-%DEVICE%
</b>

</div>


<div class="cards">


<div class="card">

<div class="label">

Room Temperature

</div>

<div class="value">

%TEMP% °C

</div>

</div>


<div class="card">

<div class="label">

Room Humidity

</div>

<div class="value">

%HUM% %

</div>

</div>


</div>


<div class="status">

DHT22:

<span class="%DHTCLASS%">

%DHTSTATUS%

</span>

<br>

Wi-Fi:

<b>

%SSID%

</b>

<br>

Lab IP:

<b>

%IP%

</b>

<br>

Dashboard Port:

<b>

80

</b>

</div>


<div class="info">

<b>
Management AP
</b>

<br><br>

SSID:

RoomMonitor-%DEVICE%

<br>

AP IP:

192.168.4.1

<br><br>

<b>
Dashboard
</b>

<br>

http://%IP%

</div>


<a
  class="config"
  href="/wifi">

⚙ Change Wi-Fi Settings

</a>


</div>


</body>

</html>

)rawliteral";


  // ===================================================
  // VALUES
  // ===================================================

  String tempText;

  String humText;


  if (
    dhtOK
  ) {

    tempText =
      String(
        temperature,
        1
      );


    humText =
      String(
        humidity,
        1
      );

  }

  else {

    tempText =
      "--";


    humText =
      "--";

  }


  html.replace(
    "%TEMP%",
    tempText
  );


  html.replace(
    "%HUM%",
    humText
  );


  html.replace(
    "%DEVICE%",
    DEVICE_ID
  );


  html.replace(
    "%SSID%",
    WiFi.SSID()
  );


  html.replace(
    "%IP%",
    WiFi.localIP().toString()
  );


  html.replace(
    "%DHTSTATUS%",
    dhtOK
      ? "ONLINE"
      : "ERROR"
  );


  html.replace(
    "%DHTCLASS%",
    dhtOK
      ? "online"
      : "offline"
  );


  server.send(
    200,
    "text/html; charset=UTF-8",
    html
  );

}


// =====================================================
// WIFI SETTINGS PAGE
// =====================================================

void handleWiFiPage() {


  String html = R"rawliteral(

<!DOCTYPE html>

<html>

<head>

<meta charset="UTF-8">

<meta name="viewport"
content="width=device-width,
initial-scale=1.0">

<title>Wi-Fi Settings</title>


<style>

body {

  margin: 0;

  padding: 20px;

  background: #111827;

  color: white;

  font-family: Arial;

}

.card {

  max-width: 500px;

  margin: 30px auto;

  padding: 25px;

  background: #1f2937;

  border-radius: 20px;

}

input {

  width: 100%;

  padding: 13px;

  margin:
    8px 0 15px;

  border-radius: 9px;

  border: none;

  font-size: 16px;

}

button {

  width: 100%;

  padding: 14px;

  background: #22c55e;

  color: white;

  border: none;

  border-radius: 10px;

  font-size: 16px;

}

.reset {

  display: block;

  margin-top: 15px;

  padding: 14px;

  text-align: center;

  background: #ef4444;

  color: white;

  text-decoration: none;

  border-radius: 10px;

}

.info {

  background: #111827;

  padding: 15px;

  border-radius: 10px;

  margin-bottom: 20px;

  line-height: 1.6;

}

</style>

</head>


<body>


<div class="card">


<h2>

RoomMonitor-%DEVICE%

</h2>


<div class="info">

<b>Current Wi-Fi</b>

<br>

SSID:
%SSID%

<br>

IP:
%IP%

<br>

Port:
80

<br>

AP:
RoomMonitor-%DEVICE%

<br>

AP IP:
192.168.4.1

</div>


<form
  action="/savewifi"
  method="POST">


<label>

New Wi-Fi Name

</label>


<input

  type="text"

  name="ssid"

  value="%SSID%"

  required>


<label>

New Wi-Fi Password

</label>


<input

  type="password"

  name="password"

  placeholder="Enter password">


<button>

Save & Restart

</button>


</form>


<a
  class="reset"
  href="/reset">

Clear Saved Wi-Fi

</a>


</div>


</body>

</html>

)rawliteral";


  html.replace(
    "%DEVICE%",
    DEVICE_ID
  );


  html.replace(
    "%SSID%",
    WiFi.SSID()
  );


  html.replace(
    "%IP%",
    WiFi.localIP().toString()
  );


  server.send(
    200,
    "text/html; charset=UTF-8",
    html
  );

}


// =====================================================
// SAVE WIFI FROM NORMAL DASHBOARD
// =====================================================

void handleSaveWiFiNormal() {


  if (
    !server.hasArg("ssid")
  ) {

    server.send(
      400,
      "text/plain",
      "SSID missing"
    );

    return;

  }


  String newSSID =
    server.arg(
      "ssid"
    );


  String newPassword =
    server.arg(
      "password"
    );


  newSSID.trim();


  saveWiFiCredentials(
    newSSID,
    newPassword
  );


  server.send(
    200,
    "text/html; charset=UTF-8",
    "<h2>Wi-Fi saved. Restarting...</h2>"
  );


  delay(1500);


  ESP.restart();

}


// =====================================================
// DHT DATA API
// =====================================================

void handleData() {


  String json = "{";


  json +=
    "\"temperature\":";


  if (
    dhtOK
  ) {

    json +=
      String(
        temperature,
        1
      );

  }

  else {

    json +=
      "null";

  }


  json +=
    ",";


  json +=
    "\"humidity\":";


  if (
    dhtOK
  ) {

    json +=
      String(
        humidity,
        1
      );

  }

  else {

    json +=
      "null";

  }


  json +=
    ",";


  json +=
    "\"dht\":\"";


  json +=
    dhtOK
      ? "ONLINE"
      : "ERROR";


  json +=
    "\",";


  json +=
    "\"device\":\"RoomMonitor-";


  json +=
    DEVICE_ID;


  json +=
    "\",";


  json +=
    "\"ip\":\"";


  json +=
    WiFi.localIP().toString();


  json +=
    "\",";


  json +=
    "\"port\":80";


  json +=
    "}";


  server.send(
    200,
    "application/json",
    json
  );

}


// =====================================================
// NOT FOUND
// =====================================================

void handleNotFound() {


  if (
    setupMode
  ) {

    handleSetupPage();

  }

  else {

    server.send(
      404,
      "text/plain",
      "404 - Not Found"
    );

  }

}


// =====================================================
// READ DHT22
// =====================================================

void readDHT() {


  float t =
    dht.readTemperature();


  float h =
    dht.readHumidity();


  if (
    !isnan(t) &&
    !isnan(h)
  ) {


    temperature =
      t;


    humidity =
      h;


    dhtOK =
      true;


    Serial.print(
      "Temperature: "
    );

    Serial.print(
      temperature,
      1
    );


    Serial.print(
      " C | Humidity: "
    );

    Serial.print(
      humidity,
      1
    );


    Serial.println(
      " %"
    );

  }

  else {


    dhtOK =
      false;


    Serial.println(
      "DHT22 READ ERROR"
    );

  }

}


// =====================================================
// START NORMAL MODE
// =====================================================

void startNormalMode() {


  setupMode =
    false;


  Serial.println();

  Serial.println(
    "========================================"
  );

  Serial.println(
    "STARTING NORMAL MODE"
  );

  Serial.println(
    "========================================"
  );


  // ===================================================
  // KEEP AP + STA
  // ===================================================

  WiFi.mode(
    WIFI_AP_STA
  );


  // ===================================================
  // MAKE SURE AP IS RUNNING
  // ===================================================

  startAccessPoint();


  // ===================================================
  // CONNECT SAVED WIFI
  // ===================================================

  bool connected =
    false;


  if (
    savedSSID.length() > 0
  ) {


    connected =
      connectToWiFi(
        savedSSID,
        savedPassword
      );

  }


  // ===================================================
  // DEFAULT WIFI FALLBACK
  // ===================================================

  if (
    !connected
  ) {


    Serial.println();

    Serial.println(
      "Saved Wi-Fi failed."
    );


    Serial.println(
      "Trying DEFAULT Wi-Fi..."
    );


    connected =
      connectToWiFi(
        DEFAULT_WIFI_SSID,
        DEFAULT_WIFI_PASSWORD
      );

  }


  // ===================================================
  // BOTH FAILED
  // ===================================================

  if (
    !connected
  ) {


    Serial.println();

    Serial.println(
      "========================================"
    );


    Serial.println(
      "NO WIFI CONNECTION"
    );


    Serial.println(
      "Starting SETUP MODE"
    );


    Serial.println(
      "========================================"
    );


    startSetupMode();


    return;

  }


  // ===================================================
  // NORMAL ROUTES
  // ===================================================

  server.on(
    "/",
    HTTP_GET,
    handleRoot
  );


  server.on(
    "/data",
    HTTP_GET,
    handleData
  );


  server.on(
    "/wifi",
    HTTP_GET,
    handleWiFiPage
  );


  server.on(
    "/savewifi",
    HTTP_POST,
    handleSaveWiFiNormal
  );


  server.on(
    "/reset",
    HTTP_GET,
    handleResetWiFi
  );


  server.onNotFound(
    handleNotFound
  );


  server.begin();


  Serial.println();

  Serial.println(
    "========================================"
  );

  Serial.println(
    "ROOM MONITOR READY"
  );

  Serial.println(
    "========================================"
  );


  Serial.print(
    "Device: RoomMonitor-"
  );

  Serial.println(
    DEVICE_ID
  );


  Serial.print(
    "Lab Wi-Fi IP: "
  );

  Serial.println(
    WiFi.localIP()
  );


  Serial.println(
    "Lab Dashboard Port: 80"
  );


  Serial.print(
    "Lab Dashboard: http://"
  );

  Serial.println(
    WiFi.localIP()
  );


  Serial.print(
    "Management AP: "
  );

  Serial.println(
    AP_SSID
  );


  Serial.println(
    "Management AP IP: 192.168.4.1"
  );


  Serial.println(
    "Management AP Port: 80"
  );


  Serial.println(
    "========================================"
  );

}


// =====================================================
// BOOT BUTTON
// =====================================================

void checkBootButton() {


  bool pressed =
    digitalRead(
      BOOT_BUTTON
    ) == LOW;


  if (
    pressed &&
    !buttonPressed
  ) {


    buttonPressed =
      true;


    buttonStart =
      millis();


    Serial.println();

    Serial.println(
      "BOOT button pressed."
    );

  }


  if (
    pressed &&
    buttonPressed
  ) {


    if (
      millis() -
      buttonStart >=
      CONFIG_HOLD_TIME
    ) {


      Serial.println();

      Serial.println(
        "BOOT held for 3 seconds."
      );


      Serial.println(
        "Entering Wi-Fi setup..."
      );


      delay(500);


      startSetupMode();


      buttonPressed =
        false;

    }

  }


  if (
    !pressed
  ) {

    buttonPressed =
      false;

  }

}


// =====================================================
// SETUP
// =====================================================

void setup() {


  Serial.begin(
    115200
  );


  delay(1000);


  Serial.println();

  Serial.println(
    "========================================"
  );

  Serial.println(
    "GOOUUU ESP32-S3-CAM V1.3"
  );

  Serial.println(
    "ROOM MONITOR"
  );

  Serial.println(
    "DHT22 + WIFI MANAGER"
  );

  Serial.println(
    "========================================"
  );


  // ===================================================
  // BOOT
  // ===================================================

  pinMode(
    BOOT_BUTTON,
    INPUT_PULLUP
  );


  // ===================================================
  // DHT
  // ===================================================

  dht.begin();


  Serial.println(
    "DHT22 initialized."
  );


  // ===================================================
  // LOAD SAVED WIFI
  // ===================================================

  loadWiFiCredentials();


  // ===================================================
  // START AP FIRST
  // ===================================================

  startAccessPoint();


  // ===================================================
  // CHECK IF BOOT IS HELD
  // ===================================================

  delay(100);


  if (
    digitalRead(
      BOOT_BUTTON
    ) == LOW
  ) {


    unsigned long start =
      millis();


    Serial.println();

    Serial.println(
      "BOOT button is held."
    );


    while (
      digitalRead(
        BOOT_BUTTON
      ) == LOW
    ) {


      if (
        millis() -
        start >=
        CONFIG_HOLD_TIME
      ) {


        Serial.println(
          "Manual setup requested."
        );


        startSetupMode();


        return;

      }


      delay(50);

    }

  }


  // ===================================================
  // FIRST BOOT
  // ===================================================

  if (
    savedSSID.length() == 0
  ) {


    Serial.println();

    Serial.println(
      "No saved Wi-Fi."
    );


    Serial.println(
      "Starting first-run setup..."
    );


    startSetupMode();


    return;

  }


  // ===================================================
  // NORMAL MODE
  // ===================================================

  startNormalMode();

}


// =====================================================
// LOOP
// =====================================================

void loop() {


  // ===================================================
  // SETUP MODE
  // ===================================================

  if (
    setupMode
  ) {


    dnsServer.processNextRequest();


    server.handleClient();


    // DHT continues

    if (
      millis() -
      lastDHTRead >=
      DHT_INTERVAL
    ) {


      lastDHTRead =
        millis();


      readDHT();

    }


    return;

  }


  // ===================================================
  // NORMAL
  // ===================================================

  server.handleClient();


  // ===================================================
  // DHT
  // ===================================================

  if (
    millis() -
    lastDHTRead >=
    DHT_INTERVAL
  ) {


    lastDHTRead =
      millis();


    readDHT();

  }


  // ===================================================
  // BOOT BUTTON
  // ===================================================

  checkBootButton();

}