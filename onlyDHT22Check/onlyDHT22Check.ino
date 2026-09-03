/*
===========================================================
 GOOUUU ESP32-S3-CAM V1.3
 ROOM TEMPERATURE MONITOR

 DHT22 + PHONE CAMERA DASHBOARD

 ----------------------------------------------------------
 Hardware:
   ESP32-S3
   DHT22

 DHT22:
   DATA -> GPIO 1
   VCC  -> 3.3V
   GND  -> GND

 ----------------------------------------------------------
 Features:

   [1] DHT22 temperature
   [2] DHT22 humidity
   [3] Live dashboard
   [4] Temperature + Humidity graph
   [5] Phone camera via WebRTC / PeerJS
   [6] QR code for phone camera
   [7] Wi-Fi saved in NVS
   [8] Default Wi-Fi fallback
   [9] Management AP
  [10] Wi-Fi configuration page
  [11] BOOT button 3 sec -> setup mode
  [12] Multiple ESP32 support

 ----------------------------------------------------------
 Management AP:

   RoomMonitor-01
   Password: 12345678

   AP IP:
   192.168.4.1

 ----------------------------------------------------------
 Normal dashboard:

   http://<LAB_IP>

 ----------------------------------------------------------
*/


#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <DHT.h>


// ========================================================
// DEVICE ID
// ========================================================
// ESP32 #1 -> "01"
// ESP32 #2 -> "02"
// ESP32 #3 -> "03"
// ========================================================

#define DEVICE_ID "01"


// ========================================================
// DHT22
// ========================================================

#define DHT_PIN 1
#define DHTTYPE DHT22

DHT dht(
  DHT_PIN,
  DHTTYPE
);


// ========================================================
// BOOT BUTTON
// ========================================================

#define BOOT_BUTTON 0

const unsigned long BOOT_HOLD_TIME =
  3000;


// ========================================================
// DEFAULT WIFI
// ========================================================
// CHANGE THESE
// ========================================================

const char* DEFAULT_WIFI_SSID =
  "YOUR_DEFAULT_WIFI";

const char* DEFAULT_WIFI_PASSWORD =
  "YOUR_DEFAULT_PASSWORD";


// ========================================================
// PHONE CAMERA PAGE
// ========================================================
// This is your already-working GitHub camera page.
// ========================================================

const char* CAMERA_PAGE_URL =
  "https://shri7ul.github.io/Esp32-Phone-Camera-web-dashboard-test/";


// ========================================================
// MANAGEMENT AP
// ========================================================

String AP_SSID =
  String("RoomMonitor-") +
  DEVICE_ID;

const char* AP_PASSWORD =
  "12345678";


// ========================================================
// AP NETWORK
// ========================================================

IPAddress AP_IP(
  192,
  168,
  4,
  1
);

IPAddress AP_GATEWAY(
  192,
  168,
  4,
  1
);

IPAddress AP_SUBNET(
  255,
  255,
  255,
  0
);


// ========================================================
// DNS SERVER
// ========================================================

DNSServer dnsServer;

const byte DNS_PORT = 53;


// ========================================================
// WEB SERVER
// ========================================================

WebServer server(
  80
);


// ========================================================
// NVS
// ========================================================

Preferences preferences;


// ========================================================
// STORED WIFI
// ========================================================

String savedSSID = "";
String savedPassword = "";


// ========================================================
// WIFI STATE
// ========================================================

bool wifiConnected = false;

bool setupMode = false;


// ========================================================
// DHT STATE
// ========================================================

float temperature = NAN;

float humidity = NAN;

bool dhtOK = false;

unsigned long lastDHTRead = 0;

const unsigned long DHT_INTERVAL =
  2500;


// ========================================================
// BOOT BUTTON STATE
// ========================================================

bool bootPressed = false;

unsigned long bootStart = 0;


// ========================================================
// FORCE SETUP FLAG
// ========================================================

bool forceSetup = false;


// ========================================================
// FORWARD DECLARATIONS
// ========================================================

void loadWiFi();

void saveWiFi(
  String ssid,
  String password
);

void clearWiFi();

bool connectWiFi(
  String ssid,
  String password
);

void startAP();

void startSetupMode();

void startNormalMode();

void readDHT();

void checkBootButton();

void handleRoot();

void handleData();

void handleWiFiPage();

void handleSaveWiFi();

void handleResetWiFi();

void handleSetupPage();

void handleNotFound();

String makeSetupPage();

String makeWiFiPage();

String makeDashboard();


// ========================================================
// LOAD WIFI
// ========================================================

void loadWiFi() {

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
    "========== STORED WIFI =========="
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


  Serial.println(
    "================================="
  );

}


// ========================================================
// SAVE WIFI
// ========================================================

void saveWiFi(
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


// ========================================================
// CLEAR WIFI
// ========================================================

void clearWiFi() {

  preferences.begin(
    "wifi",
    false
  );


  preferences.clear();


  preferences.end();


  savedSSID = "";

  savedPassword = "";


  Serial.println(
    "Saved Wi-Fi credentials cleared."
  );

}


// ========================================================
// START MANAGEMENT AP
// ========================================================

void startAP() {

  Serial.println();

  Serial.println(
    "========================================"
  );

  Serial.println(
    "STARTING MANAGEMENT ACCESS POINT"
  );

  Serial.println(
    "========================================"
  );


  WiFi.mode(
    WIFI_AP_STA
  );


  delay(300);


  WiFi.softAPConfig(
    AP_IP,
    AP_GATEWAY,
    AP_SUBNET
  );


  bool result =
    WiFi.softAP(
      AP_SSID.c_str(),
      AP_PASSWORD
    );


  if (
    result
  ) {

    Serial.println(
      "Management AP: READY"
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

  }

  else {

    Serial.println(
      "ERROR: AP START FAILED"
    );

  }


  Serial.println(
    "========================================"
  );

}


// ========================================================
// CONNECT WIFI
// ========================================================

bool connectWiFi(
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
    "Connecting to: "
  );

  Serial.println(
    ssid
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
    WiFi.status() ==
    WL_CONNECTED
  ) {


    wifiConnected =
      true;


    Serial.println(
      "WIFI CONNECTED"
    );


    Serial.print(
      "SSID: "
    );

    Serial.println(
      WiFi.SSID()
    );


    Serial.print(
      "LAB IP: "
    );

    Serial.println(
      WiFi.localIP()
    );


    Serial.print(
      "GATEWAY: "
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
    "WIFI CONNECTION FAILED"
  );


  WiFi.disconnect(
    false
  );


  delay(300);


  return false;

}


// ========================================================
// SETUP PAGE HTML
// ========================================================

String makeSetupPage() {

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

  font-family: Arial, sans-serif;

}

.container {

  max-width: 520px;

  margin: auto;

}

.card {

  background: #1f2937;

  border-radius: 22px;

  padding: 28px;

  margin-top: 25px;

}

h1 {

  text-align: center;

  color: #22c55e;

  margin-bottom: 8px;

}

.subtitle {

  text-align: center;

  color: #cbd5e1;

  line-height: 1.5;

}

.device {

  background: #111827;

  padding: 15px;

  border-radius: 12px;

  margin: 20px 0;

  text-align: center;

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

  margin-top: 22px;

  border: none;

  border-radius: 10px;

  background: #22c55e;

  color: white;

  font-size: 17px;

  font-weight: bold;

}

.info {

  background: #111827;

  border-radius: 12px;

  padding: 15px;

  margin-top: 20px;

  line-height: 1.7;

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

<b>
RoomMonitor-%DEVICE%
</b>

<br><br>

Setup IP:

<br>

<b>
192.168.4.1
</b>

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


<button type="submit">

Save Wi-Fi & Restart

</button>


</form>


<div class="info">

<b>
Management Hotspot
</b>

<br><br>

SSID:

<b>
RoomMonitor-%DEVICE%
</b>

<br>

Password:

<b>
12345678
</b>

<br>

IP:

<b>
192.168.4.1
</b>

</div>


<div class="info warning">

If saved Wi-Fi and default Wi-Fi
both fail, setup mode will remain
available through this hotspot.

<br><br>

Hold BOOT for 3 seconds anytime
to enter Wi-Fi setup.

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


  return html;

}


// ========================================================
// SETUP PAGE HANDLER
// ========================================================

void handleSetupPage() {

  server.send(
    200,
    "text/html; charset=UTF-8",
    makeSetupPage()
  );

}


// ========================================================
// WIFI SETTINGS PAGE
// ========================================================

String makeWiFiPage() {


  String currentSSID =
    WiFi.SSID();


  String currentIP =
    WiFi.localIP().toString();


  if (
    !wifiConnected
  ) {

    currentSSID =
      "Not connected";


    currentIP =
      "0.0.0.0";

  }


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

* {
  box-sizing: border-box;
}

body {

  margin: 0;

  padding: 20px;

  background: #111827;

  color: white;

  font-family: Arial;

}

.card {

  max-width: 520px;

  margin: 30px auto;

  padding: 25px;

  background: #1f2937;

  border-radius: 20px;

}

h1 {

  text-align: center;

  color: #60a5fa;

}

.info {

  background: #111827;

  padding: 15px;

  border-radius: 12px;

  line-height: 1.8;

  margin-bottom: 20px;

}

label {

  display: block;

  margin-top: 15px;

  margin-bottom: 6px;

  font-weight: bold;

}

input {

  width: 100%;

  padding: 13px;

  border-radius: 9px;

  border: none;

  background: #374151;

  color: white;

  font-size: 16px;

}

button {

  width: 100%;

  padding: 14px;

  margin-top: 20px;

  background: #22c55e;

  color: white;

  border: none;

  border-radius: 10px;

  font-size: 16px;

  font-weight: bold;

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

.back {

  display: block;

  text-align: center;

  margin-top: 15px;

  color: #60a5fa;

}

</style>

</head>


<body>


<div class="card">


<h1>
Wi-Fi Settings
</h1>


<div class="info">

<b>
Device:
</b>

RoomMonitor-%DEVICE%

<br>

<b>
Current Wi-Fi:
</b>

%SSID%

<br>

<b>
Lab IP:
</b>

%IP%

<br>

<b>
Dashboard Port:
</b>

80

<br><br>

<b>
Management AP:
</b>

RoomMonitor-%DEVICE%

<br>

<b>
AP IP:
</b>

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


<a
class="back"
href="/">

← Back to Dashboard

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
    currentSSID
  );


  html.replace(
    "%IP%",
    currentIP
  );


  return html;

}


// ========================================================
// DASHBOARD
// ========================================================

String makeDashboard() {


  String html = R"rawliteral(

<!DOCTYPE html>

<html>

<head>

<meta charset="UTF-8">

<meta name="viewport"
content="width=device-width,
initial-scale=1.0">

<title>
Room Temperature Monitoring
</title>


<script
src="https://cdn.jsdelivr.net/npm/chart.js">
</script>


<script
src="https://cdnjs.cloudflare.com/ajax/libs/qrcodejs/1.0.0/qrcode.min.js">
</script>


<script
src="https://unpkg.com/peerjs@1.5.5/dist/peerjs.min.js">
</script>


<style>


* {

  box-sizing:
    border-box;

}


body {

  margin:
    0;

  background:
    #f4f8fc;

  color:
    #111827;

  font-family:
    Arial,
    Helvetica,
    sans-serif;

}


.container {

  width:
    calc(100% - 40px);

  max-width:
    1300px;

  margin:
    20px auto;

  padding:
    28px;

  border:
    2px solid #222;

  border-radius:
    28px;

  background:
    #f7fbff;

}


.title {

  text-align:
    center;

  font-size:
    23px;

  font-weight:
    800;

  color:
    #ef3333;

  margin-bottom:
    8px;

}


.device-info {

  text-align:
    center;

  color:
    #555;

  font-size:
    13px;

  margin-bottom:
    30px;

}


.dashboard {

  display:
    grid;

  grid-template-columns:
    160px
    160px
    minmax(350px, 1fr)
    280px;

  gap:
    25px;

  align-items:
    center;

}


/* =====================================================
   VALUE CARDS
===================================================== */


.value-card {

  height:
    280px;

  border:
    2px solid #222;

  border-radius:
    26px;

  display:
    flex;

  flex-direction:
    column;

  justify-content:
    center;

  align-items:
    center;

  padding:
    20px;

}


.card-title {

  font-size:
    18px;

  font-weight:
    800;

  text-align:
    center;

  line-height:
    1.3;

  margin-bottom:
    25px;

}


.card-value {

  font-size:
    28px;

  font-weight:
    800;

}


/* =====================================================
   GRAPH
===================================================== */


.graph-card {

  height:
    300px;

  border:
    2px solid #ef3333;

  border-radius:
    26px;

  padding:
    18px;

}


.graph-title {

  font-size:
    18px;

  font-weight:
    800;

  margin-bottom:
    5px;

}


.chart-container {

  position:
    relative;

  width:
    100%;

  height:
    245px;

}


/* =====================================================
   CAMERA
===================================================== */


.camera-section {

  text-align:
    center;

}


.camera-title {

  font-size:
    18px;

  font-weight:
    800;

  margin-bottom:
    10px;

}


.camera-card {

  min-height:
    235px;

  border:
    2px solid #222;

  border-radius:
    26px;

  padding:
    15px;

  display:
    flex;

  flex-direction:
    column;

  justify-content:
    center;

  align-items:
    center;

}


.camera-text {

  font-size:
    13px;

  font-weight:
    700;

  line-height:
    1.45;

  margin-bottom:
    8px;

}


#qrcode {

  width:
    120px;

  height:
    120px;

  margin:
    5px auto;

}


#qrcode img {

  width:
    120px !important;

  height:
    120px !important;

}


.camera-url {

  font-size:
    7px;

  word-break:
    break-all;

  max-width:
    240px;

}


#phoneCamera {

  display:
    none;

  width:
    100%;

  aspect-ratio:
    16 / 9;

  object-fit:
    contain;

  background:
    black;

  border-radius:
    15px;

}


.camera-button {

  display:
    inline-block;

  margin-top:
    7px;

  padding:
    8px 13px;

  border-radius:
    8px;

  background:
    #222;

  color:
    white;

  text-decoration:
    none;

  font-size:
    12px;

  font-weight:
    bold;

}


.disconnect-button {

  display:
    none;

  margin-top:
    7px;

  padding:
    8px 13px;

  border:
    none;

  border-radius:
    8px;

  background:
    #ef3333;

  color:
    white;

  font-size:
    12px;

  font-weight:
    bold;

}


.camera-status {

  margin-top:
    7px;

  font-size:
    11px;

  font-weight:
    800;

}


.camera-waiting {

  color:
    #d97706;

}


.camera-connected {

  color:
    #16a34a;

}


.camera-error {

  color:
    #dc2626;

}


/* =====================================================
   STATUS
===================================================== */


.status {

  margin-top:
    25px;

  text-align:
    center;

  font-size:
    15px;

  font-weight:
    800;

}


.status-good {

  color:
    #16a34a;

}


.status-warning {

  color:
    #d97706;

}


.status-error {

  color:
    #dc2626;

}


/* =====================================================
   WIFI INFO
===================================================== */


.wifi-box {

  margin-top:
    20px;

  padding:
    15px;

  border-radius:
    14px;

  background:
    #111827;

  color:
    white;

  text-align:
    center;

  line-height:
    1.7;

  font-size:
    13px;

}


.wifi-settings {

  display:
    inline-block;

  margin-top:
    8px;

  padding:
    8px 12px;

  background:
    #2563eb;

  color:
    white;

  text-decoration:
    none;

  border-radius:
    8px;

}


/* =====================================================
   RESPONSIVE
===================================================== */


@media(max-width:1100px) {

  .dashboard {

    grid-template-columns:
      1fr 1fr;

  }

}


@media(max-width:650px) {

  .container {

    width:
      calc(100% - 15px);

    padding:
      12px;

  }


  .dashboard {

    grid-template-columns:
      1fr;

  }


  .value-card {

    height:
      180px;

  }


  .graph-card {

    height:
      330px;

  }

}


</style>

</head>


<body>


<div class="container">


<div class="title">

Room Temperature Monitoring

</div>


<div class="device-info">

Device:
<b>
RoomMonitor-%DEVICE%
</b>

</div>


<div class="dashboard">


<!-- ===================================================
     HUMIDITY
=================================================== -->


<div class="value-card">


<div class="card-title">

Room<br>
Humidity

</div>


<div class="card-value">

<span id="humidity">
--
</span>

%

</div>


</div>


<!-- ===================================================
     TEMPERATURE
=================================================== -->


<div class="value-card">


<div class="card-title">

Room<br>
Temperature

</div>


<div class="card-value">

<span id="temperature">
--
</span>

°C

</div>


</div>


<!-- ===================================================
     GRAPH
=================================================== -->


<div class="graph-card">


<div class="graph-title">

Temperature &amp; Humidity

</div>


<div class="chart-container">

<canvas
id="sensorChart">
</canvas>

</div>


</div>


<!-- ===================================================
     PHONE CAMERA
=================================================== -->


<div class="camera-section">


<div class="camera-title">

Mobile Camera View

</div>


<div class="camera-card">


<div id="qrArea">


<div class="camera-text">

Scan QR with phone<br>

to open camera

</div>


<div id="qrcode">

Generating...

</div>


<div
id="cameraURL"
class="camera-url">

Connecting...

</div>


<a
id="cameraButton"
class="camera-button"
href="#"
target="_blank">

Open Camera

</a>


</div>


<video
id="phoneCamera"
autoplay
muted
playsinline>
</video>


<div
id="cameraStatus"
class="camera-status camera-waiting">

Connecting camera system...

</div>


<button
id="disconnectButton"
class="disconnect-button"
onclick="disconnectCamera()">

Disconnect Camera

</button>


</div>


</div>


</div>


<!-- ===================================================
     STATUS
=================================================== -->


<div
id="status"
class="status">

Connecting to ESP32...

</div>


<!-- ===================================================
     WIFI INFO
=================================================== -->


<div class="wifi-box">


Wi-Fi:
<b id="wifiName">
--
</b>


<br>


ESP32 Lab IP:
<b id="esp32IP">
--
</b>


<br>


Dashboard Port:
<b>
80
</b>


<br>


Management AP:
<b>
RoomMonitor-%DEVICE%
</b>


<br>


AP IP:
<b>
192.168.4.1
</b>


<br>


<a
href="/wifi"
class="wifi-settings">

⚙ Change Wi-Fi

</a>


</div>


</div>


<script>


// =====================================================
// CHART
// =====================================================


const ctx =
  document
    .getElementById(
      "sensorChart"
    )
    .getContext(
      "2d"
    );


const sensorChart =
  new Chart(
    ctx,
    {

      type:
        "line",


      data:
      {

        labels:
          [],


        datasets:
        [

          {

            label:
              "Temperature (°C)",

            data:
              [],

            borderColor:
              "#ef3333",

            backgroundColor:
              "rgba(239,51,51,0.08)",

            borderWidth:
              2,

            pointRadius:
              2,

            tension:
              0.3,

            yAxisID:
              "temperatureAxis"

          },


          {

            label:
              "Humidity (%)",

            data:
              [],

            borderColor:
              "#1976d2",

            backgroundColor:
              "rgba(25,118,210,0.08)",

            borderWidth:
              2,

            pointRadius:
              2,

            tension:
              0.3,

            yAxisID:
              "humidityAxis"

          }

        ]

      },


      options:
      {

        responsive:
          true,

        maintainAspectRatio:
          false,

        animation:
          false,


        interaction:
        {

          mode:
            "index",

          intersect:
            false

        },


        scales:
        {

          x:
          {

            ticks:
            {

              maxTicksLimit:
                7

            }

          },


          temperatureAxis:
          {

            type:
              "linear",

            position:
              "left",

            title:
            {

              display:
                true,

              text:
                "Temperature °C"

            }

          },


          humidityAxis:
          {

            type:
              "linear",

            position:
              "right",

            min:
              0,

            max:
              100,

            title:
            {

              display:
                true,

              text:
                "Humidity %"

            },


            grid:
            {

              drawOnChartArea:
                false

            }

          }

        }

      }

    }
  );


// =====================================================
// DHT DATA
// =====================================================

async function updateData() {


  try {


    const response =
      await fetch(
        "/data",
        {
          cache:
            "no-store"
        }
      );


    const data =
      await response.json();


    // =================================================
    // VALUES
    // =================================================


    if (
      data.temperature !== null &&
      data.humidity !== null
    ) {


      document
        .getElementById(
          "temperature"
        )
        .innerText =
        Number(
          data.temperature
        ).toFixed(
          1
        );


      document
        .getElementById(
          "humidity"
        )
        .innerText =
        Number(
          data.humidity
        ).toFixed(
          1
        );


      // ===============================================
      // GRAPH
      // ===============================================


      const now =
        new Date()
          .toLocaleTimeString();


      sensorChart
        .data
        .labels
        .push(
          now
        );


      sensorChart
        .data
        .datasets[0]
        .data
        .push(
          Number(
            data.temperature
          )
        );


      sensorChart
        .data
        .datasets[1]
        .data
        .push(
          Number(
            data.humidity
          )
        );


      // Keep last 30 points

      if (
        sensorChart.data.labels.length >
        30
      ) {

        sensorChart.data.labels.shift();

        sensorChart.data.datasets[0]
          .data
          .shift();

        sensorChart.data.datasets[1]
          .data
          .shift();

      }


      sensorChart.update();

    }


    // =================================================
    // STATUS
    // =================================================


    const status =
      document
        .getElementById(
          "status"
        );


    status.className =
      "status";


    if (
      data.status ===
      "DHT22 ERROR"
    ) {


      status.innerText =
        "DHT22 ERROR";


      status.classList.add(
        "status-error"
      );

    }

    else if (
      data.status ===
      "CRITICAL"
    ) {


      status.innerText =
        "STATUS: CRITICAL";


      status.classList.add(
        "status-error"
      );

    }

    else if (
      data.status ===
      "WARNING"
    ) {


      status.innerText =
        "STATUS: WARNING";


      status.classList.add(
        "status-warning"
      );

    }

    else {


      status.innerText =
        "STATUS: DHT22 ONLINE";


      status.classList.add(
        "status-good"
      );

    }


    // =================================================
    // WIFI
    // =================================================


    document
      .getElementById(
        "wifiName"
      )
      .innerText =
      data.ssid;


    document
      .getElementById(
        "esp32IP"
      )
      .innerText =
      data.ip;


  }

  catch(error) {


    console.error(
      "ESP32 data error:",
      error
    );


    const status =
      document
        .getElementById(
          "status"
        );


    status.innerText =
      "ESP32 connection lost";


    status.className =
      "status status-error";

  }

}


// =====================================================
// CAMERA VARIABLES
// =====================================================

// IMPORTANT: this declaration is inside the browser JavaScript scope.
// The ESP32 C++ CAMERA_PAGE_URL above is NOT visible to this JS code.
const CAMERA_PAGE_URL = "https://shri7ul.github.io/Esp32-Phone-Camera-web-dashboard-test/";

let peer = null;
let currentCall = null;

const qrArea = document.getElementById("qrArea");
const qr = document.getElementById("qrcode");
const cameraURL = document.getElementById("cameraURL");
const cameraButton = document.getElementById("cameraButton");
const phoneCamera = document.getElementById("phoneCamera");
const cameraStatus = document.getElementById("cameraStatus");
const disconnectButton = document.getElementById("disconnectButton");

function setCameraStatus(text, className) {
  cameraStatus.innerText = text;
  cameraStatus.className = "camera-status " + className;
}

function createQR(url) {
  qr.innerHTML = "";

  if (typeof QRCode === "undefined") {
    qr.innerText = "QR library unavailable";
    setCameraStatus("QR library unavailable", "camera-error");
    return;
  }

  new QRCode(qr, {
    text: url,
    width: 120,
    height: 120,
    correctLevel: QRCode.CorrectLevel.M
  });
}

// =====================================================
// START PEERJS CAMERA SYSTEM
// =====================================================

function startCameraSystem() {

  console.log("Starting PeerJS camera system...");

  setCameraStatus(
    "Connecting to camera service...",
    "camera-waiting"
  );

  if (typeof Peer === "undefined") {
    console.error("PeerJS library was not loaded.");
    setCameraStatus(
      "PeerJS library unavailable",
      "camera-error"
    );
    return;
  }

  try {
    // Use PeerJS Cloud's default configuration.
    peer = new Peer();
  } catch (error) {
    console.error("PeerJS initialization failed:", error);
    setCameraStatus(
      "PeerJS initialization failed",
      "camera-error"
    );
    return;
  }

  peer.on("open", function(id) {

    console.log("PeerJS connected.");
    console.log("Dashboard Peer ID:", id);

    const phoneURL =
      CAMERA_PAGE_URL +
      "?peer=" +
      encodeURIComponent(id);

    console.log("Phone camera URL:", phoneURL);

    cameraURL.innerText = phoneURL;
    cameraButton.href = phoneURL;

    createQR(phoneURL);

    setCameraStatus(
      "Scan QR code with phone",
      "camera-waiting"
    );
  });

  // ===================================================
  // INCOMING PHONE CAMERA CALL
  // ===================================================

  peer.on("call", function(call) {

    console.log("Incoming phone camera call.");

    currentCall = call;

    // Dashboard does not send a local camera stream.
    call.answer();

    setCameraStatus(
      "Phone connected — receiving video...",
      "camera-waiting"
    );

    call.on("stream", function(stream) {

      console.log("Phone video stream received.");
      console.log(
        "Video tracks:",
        stream.getVideoTracks().length
      );

      if (stream.getVideoTracks().length === 0) {
        setCameraStatus(
          "No video track received",
          "camera-error"
        );
        return;
      }

      phoneCamera.srcObject = stream;
      phoneCamera.muted = true;
      phoneCamera.autoplay = true;
      phoneCamera.playsInline = true;

      qrArea.style.display = "none";
      phoneCamera.style.display = "block";
      disconnectButton.style.display = "inline-block";

      const playVideo = function() {
        phoneCamera.play()
          .then(function() {
            console.log("Phone camera is LIVE.");
            setCameraStatus(
              "● PHONE CAMERA LIVE",
              "camera-connected"
            );
          })
          .catch(function(error) {
            console.error("Video play error:", error);
            setCameraStatus(
              "Video received but playback failed",
              "camera-error"
            );
          });
      };

      if (phoneCamera.readyState >= 2) {
        playVideo();
      } else {
        phoneCamera.onloadedmetadata = playVideo;
      }
    });

    call.on("close", function() {
      console.log("Phone camera call closed.");
      disconnectCamera();
    });

    call.on("error", function(error) {
      console.error("Media connection error:", error);
      setCameraStatus(
        "WebRTC connection error",
        "camera-error"
      );
    });
  });

  peer.on("error", function(error) {

    console.error("PeerJS error:", error);

    let message = "PeerJS error";

    if (error && error.type) {
      message = "PeerJS error: " + error.type;
    }

    setCameraStatus(message, "camera-error");
  });

  peer.on("disconnected", function() {
    console.warn("PeerJS disconnected.");
    setCameraStatus(
      "Camera service disconnected",
      "camera-error"
    );
  });

  peer.on("close", function() {
    console.warn("PeerJS connection closed.");
  });
}

// =====================================================
// DISCONNECT CAMERA
// =====================================================

function disconnectCamera() {

  console.log("Disconnecting phone camera...");

  if (currentCall) {
    try {
      currentCall.close();
    } catch (e) {
      console.warn(e);
    }
    currentCall = null;
  }

  if (phoneCamera.srcObject) {
    phoneCamera.srcObject
      .getTracks()
      .forEach(function(track) {
        track.stop();
      });
  }

  phoneCamera.pause();
  phoneCamera.srcObject = null;
  phoneCamera.style.display = "none";

  qrArea.style.display = "block";
  disconnectButton.style.display = "none";

  setCameraStatus(
    "Scan QR code with phone",
    "camera-waiting"
  );
}

// START DASHBOARD
// =====================================================


updateData();


setInterval(
  updateData,
  2500
);


setTimeout(
  startCameraSystem,
  700
);


</script>


</body>

</html>

)rawliteral";


  html.replace(
    "%DEVICE%",
    DEVICE_ID
  );


  return html;

}


// ========================================================
// ROOT
// ========================================================

void handleRoot() {


  if (
    setupMode
  ) {

    handleSetupPage();

    return;

  }


  server.send(
    200,
    "text/html; charset=UTF-8",
    makeDashboard()
  );

}


// ========================================================
// DATA API
// ========================================================

void handleData() {


  String json =
    "{";


  // ====================================================
  // TEMPERATURE
  // ====================================================

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


  // ====================================================
  // HUMIDITY
  // ====================================================

  json +=
    ",\"humidity\":";


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


  // ====================================================
  // STATUS
  // ====================================================

  json +=
    ",\"status\":\"";


  if (
    !dhtOK
  ) {

    json +=
      "DHT22 ERROR";

  }

  else if (
    temperature >= 35.0 ||
    humidity >= 90.0
  ) {

    json +=
      "CRITICAL";

  }

  else if (
    temperature >= 30.0 ||
    humidity >= 80.0
  ) {

    json +=
      "WARNING";

  }

  else {

    json +=
      "DHT22 ONLINE";

  }


  json +=
    "\"";


  // ====================================================
  // WIFI SSID
  // ====================================================

  json +=
    ",\"ssid\":\"";


  if (
    wifiConnected
  ) {

    json +=
      WiFi.SSID();

  }

  else {

    json +=
      "Not connected";

  }


  json +=
    "\"";


  // ====================================================
  // LAB IP
  // ====================================================

  json +=
    ",\"ip\":\"";


  json +=
    WiFi.localIP().toString();


  json +=
    "\"";


  // ====================================================
  // AP IP
  // ====================================================

  json +=
    ",\"ap_ip\":\"";


  json +=
    WiFi.softAPIP().toString();


  json +=
    "\"";


  // ====================================================
  // DEVICE
  // ====================================================

  json +=
    ",\"device\":\"RoomMonitor-";


  json +=
    DEVICE_ID;


  json +=
    "\"";


  // ====================================================
  // PORT
  // ====================================================

  json +=
    ",\"port\":80";


  json +=
    "}";


  server.send(
    200,
    "application/json",
    json
  );

}


// ========================================================
// WIFI SETTINGS PAGE
// ========================================================

void handleWiFiPage() {

  server.send(
    200,
    "text/html; charset=UTF-8",
    makeWiFiPage()
  );

}


// ========================================================
// SAVE WIFI
// ========================================================

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


  saveWiFi(
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

  background:
    #111827;

  color:
    white;

  font-family:
    Arial;

  text-align:
    center;

  padding:
    50px 20px;

}

.card {

  max-width:
    500px;

  margin:
    auto;

  padding:
    30px;

  background:
    #1f2937;

  border-radius:
    20px;

}

h1 {

  color:
    #22c55e;

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


  delay(
    1500
  );


  ESP.restart();

}


// ========================================================
// RESET WIFI
// ========================================================

void handleResetWiFi() {


  clearWiFi();


  server.send(
    200,
    "text/html; charset=UTF-8",
    "<h2>Wi-Fi cleared.<br>Restarting...</h2>"
  );


  delay(
    1500
  );


  ESP.restart();

}


// ========================================================
// 404
// ========================================================

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


// ========================================================
// READ DHT22
// ========================================================

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


// ========================================================
// START SETUP MODE
// ========================================================

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


  IPAddress apIP =
    WiFi.softAPIP();


  // ====================================================
  // DNS
  // ====================================================

  dnsServer.start(
    DNS_PORT,
    "*",
    apIP
  );


  // ====================================================
  // ROUTES
  // ====================================================

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


  // Android

  server.on(
    "/generate_204",
    HTTP_GET,
    handleSetupPage
  );


  // iPhone

  server.on(
    "/hotspot-detect.html",
    HTTP_GET,
    handleSetupPage
  );


  // Windows

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
    "AP SSID: "
  );

  Serial.println(
    AP_SSID
  );


  Serial.print(
    "AP IP: "
  );

  Serial.println(
    apIP
  );


  Serial.println(
    "Open: http://192.168.4.1"
  );

}


// ========================================================
// START NORMAL MODE
// ========================================================

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


  // ====================================================
  // AP + STA
  // ====================================================

  WiFi.mode(
    WIFI_AP_STA
  );


  // ====================================================
  // ENSURE AP
  // ====================================================

  if (
    WiFi.softAPIP() ==
    IPAddress(
      0,
      0,
      0,
      0
    )
  ) {

    startAP();

  }


  // ====================================================
  // SAVED WIFI
  // ====================================================

  bool connected =
    false;


  if (
    savedSSID.length() > 0
  ) {


    Serial.println(
      "Trying saved Wi-Fi..."
    );


    connected =
      connectWiFi(
        savedSSID,
        savedPassword
      );

  }


  // ====================================================
  // DEFAULT WIFI
  // ====================================================

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
      connectWiFi(
        DEFAULT_WIFI_SSID,
        DEFAULT_WIFI_PASSWORD
      );

  }


  // ====================================================
  // BOTH FAILED
  // ====================================================

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
      "SETUP MODE ACTIVE"
    );

    Serial.println(
      "========================================"
    );


    startSetupMode();


    return;

  }


  // ====================================================
  // NORMAL ROUTES
  // ====================================================

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
    handleSaveWiFi
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
    "Wi-Fi: "
  );

  Serial.println(
    WiFi.SSID()
  );


  Serial.print(
    "LAB IP: "
  );

  Serial.println(
    WiFi.localIP()
  );


  Serial.println(
    "DASHBOARD PORT: 80"
  );


  Serial.print(
    "DASHBOARD: http://"
  );

  Serial.println(
    WiFi.localIP()
  );


  Serial.print(
    "MANAGEMENT AP: "
  );

  Serial.println(
    AP_SSID
  );


  Serial.println(
    "MANAGEMENT AP IP: 192.168.4.1"
  );


  Serial.println(
    "MANAGEMENT AP: ON"
  );


  Serial.println(
    "========================================"
  );

}


// ========================================================
// BOOT BUTTON
// ========================================================

void checkBootButton() {


  bool pressed =
    digitalRead(
      BOOT_BUTTON
    ) == LOW;


  if (
    pressed &&
    !bootPressed
  ) {


    bootPressed =
      true;


    bootStart =
      millis();


    Serial.println();

    Serial.println(
      "BOOT button pressed..."
    );

  }


  if (
    pressed &&
    bootPressed
  ) {


    if (
      millis() -
      bootStart >=
      BOOT_HOLD_TIME
    ) {


      Serial.println();

      Serial.println(
        "BOOT held for 3 seconds."
      );


      Serial.println(
        "Entering Wi-Fi setup..."
      );


      // Save force setup flag

      preferences.begin(
        "system",
        false
      );


      preferences.putBool(
        "forceSetup",
        true
      );


      preferences.end();


      delay(
        300
      );


      ESP.restart();

    }

  }


  if (
    !pressed
  ) {

    bootPressed =
      false;

  }

}


// ========================================================
// SETUP
// ========================================================

void setup() {


  Serial.begin(
    115200
  );


  delay(
    1000
  );


  Serial.println();

  Serial.println(
    "========================================"
  );

  Serial.println(
    "GOOUUU ESP32-S3-CAM V1.3"
  );

  Serial.println(
    "ROOM TEMPERATURE MONITOR"
  );

  Serial.println(
    "DHT22 + PHONE CAMERA DASHBOARD"
  );

  Serial.println(
    "========================================"
  );


  // ====================================================
  // BOOT
  // ====================================================

  pinMode(
    BOOT_BUTTON,
    INPUT_PULLUP
  );


  // ====================================================
  // DHT
  // ====================================================

  dht.begin();


  Serial.println(
    "DHT22 initialized."
  );


  // ====================================================
  // LOAD WIFI
  // ====================================================

  loadWiFi();


  // ====================================================
  // CHECK FORCE SETUP
  // ====================================================

  preferences.begin(
    "system",
    true
  );


  forceSetup =
    preferences.getBool(
      "forceSetup",
      false
    );


  preferences.end();


  // ====================================================
  // CLEAR FORCE FLAG
  // ====================================================

  if (
    forceSetup
  ) {


    preferences.begin(
      "system",
      false
    );


    preferences.putBool(
      "forceSetup",
      false
    );


    preferences.end();

  }


  // ====================================================
  // START AP
  // ====================================================

  startAP();


  // ====================================================
  // FORCE SETUP
  // ====================================================

  if (
    forceSetup
  ) {


    Serial.println();

    Serial.println(
      "Manual setup requested."
    );


    startSetupMode();


    return;

  }


  // ====================================================
  // FIRST BOOT
  // ====================================================

  if (
    savedSSID.length() == 0
  ) {


    Serial.println();

    Serial.println(
      "No saved Wi-Fi."
    );


    Serial.println(
      "Starting first-run setup."
    );


    startSetupMode();


    return;

  }


  // ====================================================
  // NORMAL
  // ====================================================

  startNormalMode();

}


// ========================================================
// LOOP
// ========================================================

void loop() {


  // ====================================================
  // SETUP MODE
  // ====================================================

  if (
    setupMode
  ) {


    dnsServer.processNextRequest();


    server.handleClient();


    // DHT still works

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


  // ====================================================
  // NORMAL MODE
  // ====================================================

  server.handleClient();


  // ====================================================
  // DHT
  // ====================================================

  if (
    millis() -
    lastDHTRead >=
    DHT_INTERVAL
  ) {


    lastDHTRead =
      millis();


    readDHT();

  }


  // ====================================================
  // BOOT BUTTON
  // ====================================================

  checkBootButton();

}