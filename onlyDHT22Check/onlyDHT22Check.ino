#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <DHT.h>


// =====================================================
// DHT22
// =====================================================

#define DHT_PIN     1
#define DHTTYPE     DHT22

DHT dht(
  DHT_PIN,
  DHTTYPE
);


// =====================================================
// ESP32 BOOT BUTTON
// GPIO0 = BOOT
// =====================================================

#define BOOT_BUTTON 0


// =====================================================
// DEFAULT WIFI
// =====================================================
//
// এখানে তোমার নিজের default Wi-Fi credentials দাও.
// এগুলো saved Wi-Fi fail করলে দ্বিতীয়বার try করবে.
//

const char* DEFAULT_WIFI_SSID =
  "YOUR_DEFAULT_WIFI";

const char* DEFAULT_WIFI_PASSWORD =
  "YOUR_DEFAULT_PASSWORD";


// =====================================================
// SETUP HOTSPOT
// =====================================================

const char* AP_SSID =
  "RoomMonitor-Setup";

const char* AP_PASSWORD =
  "12345678";


// =====================================================
// WEB SERVER
// =====================================================

WebServer server(80);


// =====================================================
// DNS SERVER
// Captive portal
// =====================================================

DNSServer dnsServer;

const byte DNS_PORT = 53;


// =====================================================
// PREFERENCES
// Persistent Wi-Fi credentials
// =====================================================

Preferences preferences;


// =====================================================
// WIFI CREDENTIALS
// =====================================================

String savedSSID = "";
String savedPassword = "";


// =====================================================
// MODE
// =====================================================

bool setupMode = false;


// =====================================================
// DHT DATA
// =====================================================

float temperature = 0.0;
float humidity = 0.0;

bool dhtOK = false;

unsigned long lastDHTRead = 0;

const unsigned long DHT_INTERVAL = 2500;


// =====================================================
// BOOT BUTTON CONFIG
// =====================================================

unsigned long bootPressStart = 0;

bool bootButtonActive = false;

const unsigned long CONFIG_HOLD_TIME = 3000;


// =====================================================
// READ SAVED WIFI
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
    "Stored Wi-Fi:"
  );


  if (savedSSID.length() > 0) {

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
    "Saved Wi-Fi credentials cleared."
  );

}


// =====================================================
// CONNECT TO WIFI
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

  Serial.print(
    "Trying Wi-Fi: "
  );

  Serial.println(
    ssid
  );


  WiFi.mode(
    WIFI_STA
  );


  WiFi.begin(
    ssid.c_str(),
    password.c_str()
  );


  unsigned long startTime =
    millis();


  while (
    WiFi.status() != WL_CONNECTED &&
    millis() - startTime < 15000
  ) {

    delay(500);

    Serial.print(
      "."
    );

  }


  Serial.println();


  if (
    WiFi.status() == WL_CONNECTED
  ) {


    Serial.println(
      "================================"
    );


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
      "IP Address: "
    );


    Serial.println(
      WiFi.localIP()
    );


    Serial.print(
      "Signal RSSI: "
    );


    Serial.print(
      WiFi.RSSI()
    );


    Serial.println(
      " dBm"
    );


    Serial.println(
      "================================"
    );


    return true;

  }


  Serial.println(
    "Wi-Fi connection FAILED."
  );


  WiFi.disconnect(
    true
  );


  delay(500);


  return false;

}


// =====================================================
// START SETUP HOTSPOT
// =====================================================

void startSetupMode() {


  setupMode = true;


  Serial.println();
  Serial.println(
    "========================================"
  );


  Serial.println(
    "STARTING WIFI SETUP HOTSPOT"
  );


  Serial.println(
    "========================================"
  );


  WiFi.disconnect(
    true
  );


  delay(500);


  WiFi.mode(
    WIFI_AP
  );


  WiFi.softAP(
    AP_SSID,
    AP_PASSWORD
  );


  delay(500);


  IPAddress apIP =
    WiFi.softAPIP();


  Serial.print(
    "Hotspot SSID: "
  );


  Serial.println(
    AP_SSID
  );


  Serial.print(
    "Hotspot Password: "
  );


  Serial.println(
    AP_PASSWORD
  );


  Serial.print(
    "Setup IP: "
  );


  Serial.println(
    apIP
  );


  // ===================================================
  // CAPTIVE PORTAL DNS
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


  // Android captive portal

  server.on(
    "/generate_204",
    HTTP_GET,
    handleSetupPage
  );


  // iPhone captive portal

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
      content="width=device-width, initial-scale=1.0">

<title>Room Monitor Wi-Fi Setup</title>


<style>

* {
  box-sizing: border-box;
}

body {

  margin: 0;

  padding: 20px;

  background: #111827;

  color: white;

  font-family: Arial, sans-serif;

}

.container {

  max-width: 500px;

  margin: auto;

}

.card {

  background: #1f2937;

  border-radius: 22px;

  padding: 25px;

  margin-top: 30px;

}

h1 {

  text-align: center;

  color: #22c55e;

}

.subtitle {

  text-align: center;

  color: #cbd5e1;

  line-height: 1.5;

  margin-bottom: 25px;

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

  margin-top: 25px;

  border: none;

  border-radius: 12px;

  background: #22c55e;

  color: white;

  font-size: 17px;

  font-weight: bold;

}

.info {

  margin-top: 20px;

  padding: 15px;

  background: #111827;

  border-radius: 10px;

  font-size: 14px;

  line-height: 1.6;

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

<br>

Connect your ESP32 to your Wi-Fi network.

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


<b>Setup Hotspot</b>

<br><br>

Network:
<strong>

RoomMonitor-Setup

</strong>

<br>

Password:
<strong>

12345678

</strong>

<br><br>

After saving, ESP32 will restart
and connect to the selected Wi-Fi.

</div>


<div class="info warning">

If the saved Wi-Fi fails, the ESP32
will automatically try the default Wi-Fi.

<br><br>

To open this setup again later,
hold the ESP32 BOOT button for
3 seconds during normal operation.

</div>


</div>


</div>


</body>

</html>

)rawliteral";


  server.send(
    200,
    "text/html; charset=UTF-8",
    html
  );

}


// =====================================================
// SAVE WIFI HANDLER
// =====================================================

void handleSaveWiFi() {


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
    server.arg("ssid");


  String newPassword =
    server.arg("password");


  newSSID.trim();


  Serial.println();
  Serial.println(
    "New Wi-Fi received:"
  );


  Serial.print(
    "SSID: "
  );


  Serial.println(
    newSSID
  );


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
content="width=device-width, initial-scale=1.0">

<title>Saved</title>

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

  background: #1f2937;

  padding: 30px;

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

ESP32 will restart and try to connect.

</p>

<p>

Please wait...

</p>

</div>


<script>

setTimeout(
  function() {

    location.href = "/";

  },
  5000
);

</script>

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
    "<h1>Wi-Fi credentials cleared. Restarting...</h1>"
  );


  delay(1500);


  ESP.restart();

}


// =====================================================
// NORMAL DASHBOARD
// =====================================================

void handleDashboard() {


  String html = R"rawliteral(

<!DOCTYPE html>

<html>

<head>

<meta charset="UTF-8">

<meta name="viewport"
      content="width=device-width, initial-scale=1.0">

<meta http-equiv="refresh"
      content="5">


<title>Room Temperature Monitor</title>


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

  max-width: 850px;

  margin: 30px auto;

  padding: 20px;

}

h1 {

  text-align: center;

  color: #ef3333;

}

.subtitle {

  text-align: center;

  color: #6b7280;

  margin-bottom: 30px;

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

  font-size: 38px;

  font-weight: bold;

  margin-top: 20px;

}

.status {

  margin-top: 25px;

  padding: 18px;

  background: white;

  border-radius: 15px;

  text-align: center;

  font-weight: bold;

}

.config {

  display: block;

  text-align: center;

  margin-top: 25px;

  padding: 14px;

  background: #111827;

  color: white;

  text-decoration: none;

  border-radius: 10px;

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


<div class="subtitle">

ESP32-S3 + DHT22

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

DHT22 Status:
%STATUS%

<br><br>

Wi-Fi:
%SSID%

<br>

IP:
%IP%

</div>


<a
  class="config"
  href="/wifi">

Change Wi-Fi Settings

</a>


</div>


</body>

</html>

)rawliteral";


  html.replace(
    "%TEMP%",
    String(
      temperature,
      1
    )
  );


  html.replace(
    "%HUM%",
    String(
      humidity,
      1
    )
  );


  html.replace(
    "%STATUS%",
    dhtOK
      ? "ONLINE"
      : "ERROR"
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
// WIFI CONFIG PAGE FROM NORMAL WIFI
// =====================================================

void handleWiFiConfig() {


  String html = R"rawliteral(

<!DOCTYPE html>

<html>

<head>

<meta charset="UTF-8">

<meta name="viewport"
content="width=device-width, initial-scale=1.0">

<title>Wi-Fi Settings</title>

<style>

body {

  font-family: Arial;

  background: #111827;

  color: white;

  padding: 20px;

}

.card {

  max-width: 500px;

  margin: auto;

  background: #1f2937;

  padding: 25px;

  border-radius: 20px;

}

input {

  width: 100%;

  padding: 13px;

  margin: 8px 0 15px;

  border-radius: 8px;

  border: none;

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

</style>

</head>

<body>

<div class="card">

<h2>

Wi-Fi Settings

</h2>

<form
action="/savewifi"
method="POST">

<label>

Wi-Fi Name

</label>

<input

name="ssid"

value="%SSID%"
required>


<label>

Wi-Fi Password

</label>

<input

name="password"
type="password"
placeholder="Enter new password">


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
    "%SSID%",
    WiFi.SSID()
  );


  server.send(
    200,
    "text/html; charset=UTF-8",
    html
  );

}


// =====================================================
// READ DHT
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
// BOOT BUTTON CHECK
// =====================================================

bool bootButtonHeld() {


  if (
    digitalRead(
      BOOT_BUTTON
    ) == LOW
  ) {


    unsigned long start =
      millis();


    Serial.println();

    Serial.println(
      "BOOT button detected."
    );


    while (
      digitalRead(
        BOOT_BUTTON
      ) == LOW
    ) {


      if (
        millis() - start >=
        CONFIG_HOLD_TIME
      ) {


        Serial.println(
          "BOOT button held for 3 seconds."
        );


        return true;

      }


      delay(50);

    }

  }


  return false;

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
    "DHT22 + WIFI MANAGER"
  );

  Serial.println(
    "========================================"
  );


  // ===================================================
  // BOOT BUTTON
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
  // CHECK BOOT BUTTON
  // ===================================================

  if (
    bootButtonHeld()
  ) {


    Serial.println(
      "Manual Wi-Fi configuration requested."
    );


    startSetupMode();


    return;

  }


  // ===================================================
  // LOAD SAVED WIFI
  // ===================================================

  loadWiFiCredentials();


  // ===================================================
  // FIRST BOOT
  // ===================================================

  if (
    savedSSID.length() == 0
  ) {


    Serial.println();

    Serial.println(
      "No saved Wi-Fi found."
    );


    Serial.println(
      "Starting first-run setup hotspot..."
    );


    startSetupMode();


    return;

  }


  // ===================================================
  // TRY SAVED WIFI
  // ===================================================

  if (
    connectToWiFi(
      savedSSID,
      savedPassword
    )
  ) {


    // Normal mode

  }

  else {


    // =================================================
    // TRY DEFAULT WIFI
    // =================================================

    Serial.println();

    Serial.println(
      "Saved Wi-Fi failed."
    );


    Serial.println(
      "Trying DEFAULT Wi-Fi..."
    );


    if (
      connectToWiFi(
        DEFAULT_WIFI_SSID,
        DEFAULT_WIFI_PASSWORD
      )
    ) {


      Serial.println(
        "Default Wi-Fi connected."
      );

    }

    else {


      // ===============================================
      // BOTH FAILED
      // ===============================================

      Serial.println();

      Serial.println(
        "Saved Wi-Fi FAILED."
      );


      Serial.println(
        "Default Wi-Fi FAILED."
      );


      Serial.println(
        "Starting setup hotspot..."
      );


      startSetupMode();


      return;

    }

  }


  // ===================================================
  // NORMAL SERVER
  // ===================================================

  server.on(
    "/",
    HTTP_GET,
    handleDashboard
  );


  server.on(
    "/wifi",
    HTTP_GET,
    handleWiFiConfig
  );


  server.on(
    "/savewifi",
    HTTP_POST,
    handleSaveWiFi
  );


  server.on(
    "/reset",
    HTTP_GET,
    handleResetWiFi
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
    "Dashboard: http://"
  );


  Serial.println(
    WiFi.localIP()
  );


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


    return;

  }


  // ===================================================
  // NORMAL MODE
  // ===================================================

  server.handleClient();


  // ===================================================
  // DHT
  // ===================================================

  if (
    millis() - lastDHTRead >=
    DHT_INTERVAL
  ) {


    lastDHTRead =
      millis();


    readDHT();

  }


  // ===================================================
  // BOOT BUTTON
  // ===================================================

  static unsigned long buttonStart =
    0;

  static bool buttonPressed =
    false;


  if (
    digitalRead(
      BOOT_BUTTON
    ) == LOW
  ) {


    if (!buttonPressed) {

      buttonPressed =
        true;

      buttonStart =
        millis();

    }


    if (
      millis() - buttonStart >=
      CONFIG_HOLD_TIME
    ) {


      Serial.println();

      Serial.println(
        "Manual Wi-Fi setup requested."
      );


      delay(500);


      startSetupMode();

    }

  }

  else {


    buttonPressed =
      false;

  }

}