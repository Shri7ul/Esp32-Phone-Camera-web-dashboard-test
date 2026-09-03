#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <DHT.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


// =====================================================
// WIFI
// =====================================================

const char* WIFI_SSID = "Wifi Name";
const char* WIFI_PASSWORD = "Wifi Pass";


// =====================================================
// GITHUB PHONE CAMERA PAGE
// =====================================================

const char* CAMERA_PAGE_URL =
  "https://shri7ul.github.io/Esp32-Phone-Camera-web-dashboard-test/";


// =====================================================
// WEB SERVER
// =====================================================

WebServer server(80);


// =====================================================
// PIN CONFIGURATION
// =====================================================

#define DHT_PIN     1
#define DHTTYPE     DHT22

#define LED_GREEN   14
#define LED_YELLOW  45
#define LED_RED     47

#define BUZZER_PIN  21

#define SDA_PIN     41
#define SCL_PIN     42


// =====================================================
// OLED
// =====================================================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  -1
);

bool oledOK = false;


// =====================================================
// DHT
// =====================================================

DHT dht(
  DHT_PIN,
  DHTTYPE
);


// =====================================================
// BUZZER
// =====================================================

#define BUZZER_FREQ 1800
#define BUZZER_RES  8


// =====================================================
// SENSOR DATA
// =====================================================

float temperature = 0.0;
float humidity = 0.0;

bool dhtOK = false;

unsigned long lastDHTRead = 0;

const unsigned long DHT_INTERVAL = 2500;


// =====================================================
// ALERT THRESHOLDS
// =====================================================

const float TEMP_WARNING = 30.0;
const float HUM_WARNING  = 80.0;

const float TEMP_CRITICAL = 35.0;
const float HUM_CRITICAL  = 90.0;


// =====================================================
// ALERT STATE
// =====================================================

enum AlertState {

  NORMAL,
  WARNING,
  CRITICAL

};

AlertState currentState = NORMAL;


unsigned long lastBlink = 0;

bool blinkState = false;

const unsigned long BLINK_INTERVAL = 500;


// =====================================================
// FORWARD DECLARATIONS
// =====================================================

void handleRoot();
void handleData();
void updateAlert();
void updateOLED();


// =====================================================
// DASHBOARD HTML
// =====================================================

String makeDashboard() {

  String html = R"rawliteral(

<!DOCTYPE html>

<html lang="en">

<head>

<meta charset="UTF-8">

<meta name="viewport"
      content="width=device-width, initial-scale=1.0">

<title>Room Temperature Monitoring</title>

<script src="https://cdn.jsdelivr.net/npm/chart.js"></script>

<script src="https://cdnjs.cloudflare.com/ajax/libs/qrcodejs/1.0.0/qrcode.min.js"></script>

<script src="https://unpkg.com/peerjs@1.5.5/dist/peerjs.min.js"></script>


<style>

* {
  box-sizing: border-box;
}

body {

  margin: 0;

  background: #f4f8fc;

  color: #202124;

  font-family:
    Arial,
    Helvetica,
    sans-serif;

}

.container {

  width: calc(100% - 40px);

  max-width: 1250px;

  margin: 20px auto;

  padding: 28px;

  border: 2px solid #222;

  border-radius: 28px;

  background: #f7fbff;

}

.title {

  text-align: center;

  font-size: 23px;

  font-weight: 800;

  color: #ef3333;

  margin-bottom: 48px;

}

.dashboard {

  display: grid;

  grid-template-columns:
    175px
    175px
    minmax(380px, 1fr)
    270px;

  gap: 28px;

  align-items: center;

}

.value-card {

  height: 290px;

  border: 2px solid #222;

  border-radius: 28px;

  display: flex;

  flex-direction: column;

  justify-content: center;

  align-items: center;

  padding: 20px;

}

.card-title {

  font-size: 19px;

  font-weight: 800;

  text-align: center;

  line-height: 1.3;

  margin-bottom: 25px;

}

.card-value {

  font-size: 28px;

  font-weight: 800;

}

.graph-card {

  height: 305px;

  border: 2px solid #ef3333;

  border-radius: 28px;

  padding: 18px;

}

.graph-title {

  font-size: 18px;

  font-weight: 800;

  margin-bottom: 8px;

}

.chart-container {

  position: relative;

  width: 100%;

  height: 245px;

}

#sensorChart {

  width: 100% !important;

  height: 100% !important;

}


/* ==============================
   CAMERA
============================== */

.camera-section {

  text-align: center;

}

.camera-title {

  font-size: 19px;

  font-weight: 800;

  margin-bottom: 12px;

}

.camera-card {

  min-height: 235px;

  border: 2px solid #222;

  border-radius: 28px;

  padding: 18px;

  display: flex;

  flex-direction: column;

  justify-content: center;

  align-items: center;

}

.camera-text {

  font-size: 14px;

  font-weight: 700;

  line-height: 1.45;

  margin-bottom: 10px;

}

#qrcode {

  width: 120px;

  height: 120px;

  min-height: 120px;

  display: flex;

  align-items: center;

  justify-content: center;

  margin: 5px auto;

}

#qrcode img {

  width: 120px !important;

  height: 120px !important;

}

#phoneCamera {

  display: none;

  width: 100%;

  aspect-ratio: 16 / 9;

  object-fit: contain;

  background: #000;

  border-radius: 18px;

}

.camera-url {

  font-size: 8px;

  word-break: break-all;

  margin-top: 5px;

  max-width: 230px;

}

.camera-button {

  display: inline-block;

  margin-top: 8px;

  padding: 9px 14px;

  border-radius: 9px;

  background: #222;

  color: white;

  text-decoration: none;

  font-size: 13px;

  font-weight: 700;

}

.disconnect-button {

  display: none;

  margin-top: 8px;

  padding: 9px 14px;

  border: none;

  border-radius: 9px;

  background: #ef3333;

  color: white;

  font-size: 13px;

  font-weight: 700;

}

.camera-status {

  margin-top: 8px;

  font-size: 12px;

  font-weight: 800;

}

.camera-waiting {

  color: #d97706;

}

.camera-connected {

  color: #16a34a;

}

.camera-error {

  color: #dc2626;

}

.status {

  text-align: center;

  font-size: 16px;

  font-weight: 800;

  margin-top: 26px;

}

@media(max-width: 1050px) {

  .dashboard {

    grid-template-columns:
      1fr 1fr;

  }

}

@media(max-width: 650px) {

  .container {

    width: calc(100% - 20px);

    padding: 15px;

  }

  .dashboard {

    grid-template-columns: 1fr;

  }

  .value-card {

    height: 180px;

  }

  .graph-card {

    height: 330px;

  }

}

</style>

</head>


<body>


<div class="container">


<div class="title">

Room Temperature Monitoring

</div>


<div class="dashboard">


<!-- HUMIDITY -->

<div class="value-card">

<div class="card-title">

Room<br>
Humidity

</div>

<div class="card-value">

<span id="humidity">--</span> %

</div>

</div>


<!-- TEMPERATURE -->

<div class="value-card">

<div class="card-title">

Room<br>
Temperature

</div>

<div class="card-value">

<span id="temperature">--</span> °C

</div>

</div>


<!-- GRAPH -->

<div class="graph-card">

<div class="graph-title">

Temperature &amp; Humidity

</div>

<div class="chart-container">

<canvas id="sensorChart"></canvas>

</div>

</div>


<!-- CAMERA -->

<div class="camera-section">

<div class="camera-title">

Mobile Camera View

</div>


<div class="camera-card">


<div id="qrArea">

<div class="camera-text">

Same Wi-Fi-তে থাকলে<br>

QR Code scan করুন<br>

Camera View open হবে

</div>


<div id="qrcode">

Generating QR...

</div>


<div
  class="camera-url"
  id="cameraURL">

Connecting...

</div>


<a
  class="camera-button"
  id="cameraButton"
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


<div
  class="status"
  id="status">

Connecting to ESP32...

</div>


</div>


<script>


// =====================================================
// CHART
// =====================================================

const ctx =
  document
    .getElementById("sensorChart")
    .getContext("2d");


const chart =
  new Chart(
    ctx,
    {

      type: "line",

      data: {

        labels: [],

        datasets: [

          {

            label:
              "Temperature (°C)",

            data: [],

            borderColor:
              "#ef3333",

            backgroundColor:
              "rgba(239,51,51,0.08)",

            borderWidth: 2,

            pointRadius: 2,

            tension: 0.3,

            yAxisID:
              "temperatureAxis"

          },

          {

            label:
              "Humidity (%)",

            data: [],

            borderColor:
              "#1976d2",

            backgroundColor:
              "rgba(25,118,210,0.08)",

            borderWidth: 2,

            pointRadius: 2,

            tension: 0.3,

            yAxisID:
              "humidityAxis"

          }

        ]

      },

      options: {

        responsive: true,

        maintainAspectRatio: false,

        animation: false,

        interaction: {

          mode: "index",

          intersect: false

        },

        scales: {

          x: {

            ticks: {

              maxTicksLimit: 7

            }

          },

          temperatureAxis: {

            type: "linear",

            position: "left",

            title: {

              display: true,

              text:
                "Temperature °C"

            }

          },

          humidityAxis: {

            type: "linear",

            position: "right",

            min: 0,

            max: 100,

            title: {

              display: true,

              text:
                "Humidity %"

            },

            grid: {

              drawOnChartArea: false

            }

          }

        }

      }

    }
  );


// =====================================================
// SENSOR DATA
// =====================================================

async function updateData() {

  try {

    const response =
      await fetch(
        "/data",
        {
          cache: "no-store"
        }
      );


    const data =
      await response.json();


    document
      .getElementById("temperature")
      .innerText =
      Number(
        data.temperature
      ).toFixed(1);


    document
      .getElementById("humidity")
      .innerText =
      Number(
        data.humidity
      ).toFixed(1);


    document
      .getElementById("status")
      .innerText =
      data.status;


    const now =
      new Date()
        .toLocaleTimeString();


    chart.data.labels.push(now);


    chart.data.datasets[0]
      .data
      .push(
        Number(data.temperature)
      );


    chart.data.datasets[1]
      .data
      .push(
        Number(data.humidity)
      );


    if (
      chart.data.labels.length > 30
    ) {

      chart.data.labels.shift();

      chart.data.datasets[0]
        .data
        .shift();

      chart.data.datasets[1]
        .data
        .shift();

    }


    chart.update();

  }

  catch(error) {

    document
      .getElementById("status")
      .innerText =
      "ESP32 connection lost";

  }

}


// =====================================================
// PHONE CAMERA
// =====================================================

let peer = null;

let currentCall = null;


// তোমার GitHub Pages URL

const CAMERA_PAGE_URL =
  "https://shri7ul.github.io/Esp32-Phone-Camera-web-dashboard-test/";


// =====================================================
// ELEMENTS
// =====================================================

const qr =
  document.getElementById(
    "qrcode"
  );

const cameraURL =
  document.getElementById(
    "cameraURL"
  );

const cameraButton =
  document.getElementById(
    "cameraButton"
  );

const phoneCamera =
  document.getElementById(
    "phoneCamera"
  );

const cameraStatus =
  document.getElementById(
    "cameraStatus"
  );

const qrArea =
  document.getElementById(
    "qrArea"
  );

const disconnectButton =
  document.getElementById(
    "disconnectButton"
  );


// =====================================================
// CAMERA STATUS
// =====================================================

function setCameraStatus(
  text,
  cls
) {

  cameraStatus.innerText =
    text;

  cameraStatus.className =
    "camera-status " + cls;

  console.log(
    "[CAMERA]",
    text
  );

}


// =====================================================
// CREATE QR
// =====================================================

function createQR(
  url
) {


  console.log(
    "Creating QR:",
    url
  );


  qr.innerHTML =
    "";


  if (
    typeof QRCode === "undefined"
  ) {

    qr.innerHTML =
      "QR library failed";

    setCameraStatus(
      "QR library unavailable",
      "camera-error"
    );

    return;

  }


  new QRCode(

    qr,

    {

      text:
        url,

      width:
        120,

      height:
        120,

      correctLevel:
        QRCode.CorrectLevel.M

    }

  );

}


// =====================================================
// CREATE PEER
// =====================================================

function createDashboardPeer() {


  console.log(
    "Starting PeerJS..."
  );


  setCameraStatus(
    "Connecting to camera service...",
    "camera-waiting"
  );


  try {


    peer =
      new Peer({

        host:
          "0.peerjs.com",

        port:
          443,

        path:
          "/",

        secure:
          true,

        debug:
          2,

        config: {

          iceServers: [

            {

              urls:
                "stun:stun.l.google.com:19302"

            },

            {

              urls:
                "stun:stun1.l.google.com:19302"

            }

          ]

        }

      });


  }

  catch(error) {


    console.error(
      "Peer creation failed:",
      error
    );


    setCameraStatus(
      "PeerJS initialization failed",
      "camera-error"
    );


    return;

  }


  // =================================================
  // PEER OPEN
  // =================================================

  peer.on(
    "open",
    function(id) {


      console.log(
        "================================"
      );

      console.log(
        "DASHBOARD PEER ID:",
        id
      );

      console.log(
        "================================"
      );


      // ===============================================
      // PHONE CAMERA URL
      // ===============================================

      const phoneURL =
        CAMERA_PAGE_URL +
        "?peer=" +
        encodeURIComponent(id);


      console.log(
        "PHONE URL:",
        phoneURL
      );


      cameraURL.innerText =
        phoneURL;


      cameraButton.href =
        phoneURL;


      // ===============================================
      // GENERATE QR
      // ===============================================

      createQR(
        phoneURL
      );


      setCameraStatus(
        "Scan QR code with phone",
        "camera-waiting"
      );

    }
  );


  // =================================================
  // INCOMING CALL
  // =================================================

  peer.on(
    "call",
    function(call) {


      console.log(
        "PHONE CAMERA CALL RECEIVED"
      );


      currentCall =
        call;


      // Answer without local video

      call.answer();


      setCameraStatus(
        "Phone connected — receiving video...",
        "camera-waiting"
      );


      call.on(
        "stream",
        function(stream) {


          console.log(
            "REMOTE PHONE STREAM RECEIVED"
          );


          console.log(
            "Video tracks:",
            stream.getVideoTracks().length
          );


          if (
            stream.getVideoTracks().length === 0
          ) {

            setCameraStatus(
              "No video track received",
              "camera-error"
            );

            return;

          }


          phoneCamera.srcObject =
            stream;


          phoneCamera.style.display =
            "block";


          qrArea.style.display =
            "none";


          disconnectButton.style.display =
            "inline-block";


          phoneCamera
            .play()
            .then(
              function() {


                setCameraStatus(
                  "● PHONE CAMERA LIVE",
                  "camera-connected"
                );


              }
            )
            .catch(
              function(error) {

                console.error(
                  "Video play error:",
                  error
                );

              }
            );

        }
      );


      call.on(
        "close",
        function() {

          disconnectCamera();

        }
      );


      call.on(
        "error",
        function(error) {

          console.error(
            "Call error:",
            error
          );


          setCameraStatus(
            "WebRTC connection error",
            "camera-error"
          );

        }
      );

    }
  );


  // =================================================
  // PEER ERROR
  // =================================================

  peer.on(
    "error",
    function(error) {


      console.error(
        "================================"
      );

      console.error(
        "PEERJS ERROR:",
        error
      );

      console.error(
        "================================"
      );


      setCameraStatus(
        "PeerJS error: " +
        error.type,
        "camera-error"
      );

    }
  );


  // =================================================
  // DISCONNECT
  // =================================================

  peer.on(
    "disconnected",
    function() {


      console.log(
        "PeerJS disconnected"
      );


      setCameraStatus(
        "Camera service disconnected",
        "camera-error"
      );

    }
  );

}


// =====================================================
// DISCONNECT CAMERA
// =====================================================

function disconnectCamera() {


  if (currentCall) {

    try {

      currentCall.close();

    }

    catch(e) {}

    currentCall =
      null;

  }


  phoneCamera.pause();

  phoneCamera.srcObject =
    null;

  phoneCamera.style.display =
    "none";

  qrArea.style.display =
    "block";

  disconnectButton.style.display =
    "none";


  setCameraStatus(
    "Scan QR code with phone",
    "camera-waiting"
  );

}


// =====================================================
// START
// =====================================================

updateData();

setInterval(
  updateData,
  2500
);


// Start PeerJS

setTimeout(
  createDashboardPeer,
  500
);


</script>


</body>

</html>

)rawliteral";


  return html;

}


// =====================================================
// ROOT
// =====================================================

void handleRoot() {

  server.send(
    200,
    "text/html; charset=UTF-8",
    makeDashboard()
  );

}


// =====================================================
// DATA API
// =====================================================

void handleData() {

  String json = "{";


  json += "\"temperature\":";
  json += String(
    temperature,
    1
  );


  json += ",";


  json += "\"humidity\":";
  json += String(
    humidity,
    1
  );


  json += ",";


  json += "\"status\":\"";


  if (!dhtOK) {

    json += "DHT22 ERROR";

  }

  else if (
    currentState == CRITICAL
  ) {

    json += "CRITICAL";

  }

  else if (
    currentState == WARNING
  ) {

    json += "WARNING";

  }

  else {

    json += "DHT22 ONLINE";

  }


  json += "\"";


  json += "}";


  server.send(
    200,
    "application/json",
    json
  );

}


// =====================================================
// ALERT SYSTEM
// =====================================================

void updateAlert() {


  if (!dhtOK) {

    currentState =
      NORMAL;


    digitalWrite(
      LED_GREEN,
      LOW
    );

    digitalWrite(
      LED_YELLOW,
      LOW
    );

    digitalWrite(
      LED_RED,
      LOW
    );


    ledcWriteTone(
      BUZZER_PIN,
      0
    );


    return;

  }


  // ===================================================
  // DETERMINE STATE
  // ===================================================

  if (
    temperature >= TEMP_CRITICAL ||
    humidity >= HUM_CRITICAL
  ) {

    currentState =
      CRITICAL;

  }

  else if (
    temperature >= TEMP_WARNING ||
    humidity >= HUM_WARNING
  ) {

    currentState =
      WARNING;

  }

  else {

    currentState =
      NORMAL;

  }


  // ===================================================
  // BLINK
  // ===================================================

  if (
    millis() - lastBlink >=
    BLINK_INTERVAL
  ) {

    lastBlink =
      millis();

    blinkState =
      !blinkState;

  }


  // ===================================================
  // NORMAL
  // ===================================================

  if (
    currentState == NORMAL
  ) {

    digitalWrite(
      LED_GREEN,
      HIGH
    );

    digitalWrite(
      LED_YELLOW,
      LOW
    );

    digitalWrite(
      LED_RED,
      LOW
    );

    ledcWriteTone(
      BUZZER_PIN,
      0
    );

  }


  // ===================================================
  // WARNING
  // ===================================================

  else if (
    currentState == WARNING
  ) {

    digitalWrite(
      LED_GREEN,
      LOW
    );

    digitalWrite(
      LED_RED,
      LOW
    );

    digitalWrite(
      LED_YELLOW,
      blinkState
    );

    ledcWriteTone(
      BUZZER_PIN,
      0
    );

  }


  // ===================================================
  // CRITICAL
  // ===================================================

  else {

    digitalWrite(
      LED_GREEN,
      LOW
    );

    digitalWrite(
      LED_YELLOW,
      LOW
    );

    digitalWrite(
      LED_RED,
      blinkState
    );


    if (blinkState) {

      ledcWriteTone(
        BUZZER_PIN,
        BUZZER_FREQ
      );

    }

    else {

      ledcWriteTone(
        BUZZER_PIN,
        0
      );

    }

  }

}


// =====================================================
// OLED
// =====================================================

void updateOLED() {


  if (!oledOK) {

    return;

  }


  display.clearDisplay();

  display.setTextColor(
    SSD1306_WHITE
  );


  // ===================================================
  // TITLE
  // ===================================================

  display.setTextSize(1);

  display.setCursor(
    0,
    0
  );

  display.println(
    "ROOM AIR MONITOR"
  );


  display.drawLine(
    0,
    10,
    127,
    10,
    SSD1306_WHITE
  );


  // ===================================================
  // TEMPERATURE
  // ===================================================

  display.setCursor(
    0,
    18
  );

  display.print(
    "Temp: "
  );


  if (dhtOK) {

    display.print(
      temperature,
      1
    );

    display.println(
      " C"
    );

  }

  else {

    display.println(
      "ERROR"
    );

  }


  // ===================================================
  // HUMIDITY
  // ===================================================

  display.setCursor(
    0,
    31
  );

  display.print(
    "Hum : "
  );


  if (dhtOK) {

    display.print(
      humidity,
      1
    );

    display.println(
      " %"
    );

  }

  else {

    display.println(
      "ERROR"
    );

  }


  // ===================================================
  // STATUS
  // ===================================================

  display.setCursor(
    0,
    45
  );

  display.print(
    "Status: "
  );


  if (!dhtOK) {

    display.println(
      "ERROR"
    );

  }

  else if (
    currentState == CRITICAL
  ) {

    display.println(
      "CRITICAL"
    );

  }

  else if (
    currentState == WARNING
  ) {

    display.println(
      "WARNING"
    );

  }

  else {

    display.println(
      "GOOD"
    );

  }


  display.display();

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
    "GOOUUU ESP32-S3 ROOM MONITOR"
  );

  Serial.println(
    "PHONE CAMERA WEBRTC VERSION"
  );

  Serial.println(
    "========================================"
  );


  // ===================================================
  // LED
  // ===================================================

  pinMode(
    LED_GREEN,
    OUTPUT
  );

  pinMode(
    LED_YELLOW,
    OUTPUT
  );

  pinMode(
    LED_RED,
    OUTPUT
  );


  digitalWrite(
    LED_GREEN,
    LOW
  );

  digitalWrite(
    LED_YELLOW,
    LOW
  );

  digitalWrite(
    LED_RED,
    LOW
  );


  // ===================================================
  // BUZZER
  // ===================================================

  ledcAttach(
    BUZZER_PIN,
    BUZZER_FREQ,
    BUZZER_RES
  );


  ledcWriteTone(
    BUZZER_PIN,
    0
  );


  // ===================================================
  // DHT
  // ===================================================

  dht.begin();


  // ===================================================
  // OLED
  // ===================================================

  Wire.begin(
    SDA_PIN,
    SCL_PIN
  );


  if (
    display.begin(
      SSD1306_SWITCHCAPVCC,
      OLED_ADDR
    )
  ) {

    oledOK = true;


    Serial.println(
      "OLED: OK"
    );


    display.clearDisplay();

    display.setTextColor(
      SSD1306_WHITE
    );

    display.setTextSize(2);

    display.setCursor(
      0,
      0
    );

    display.println(
      "ROOM"
    );

    display.println(
      "MONITOR"
    );


    display.setTextSize(1);

    display.setCursor(
      0,
      45
    );

    display.println(
      "Starting..."
    );

    display.display();

  }

  else {

    Serial.println(
      "OLED: NOT FOUND"
    );

  }


  // ===================================================
  // WIFI
  // ===================================================

  WiFi.mode(
    WIFI_STA
  );


  WiFi.begin(
    WIFI_SSID,
    WIFI_PASSWORD
  );


  Serial.print(
    "Connecting WiFi"
  );


  while (
    WiFi.status() != WL_CONNECTED
  ) {

    delay(500);

    Serial.print(
      "."
    );

  }


  Serial.println();

  Serial.println(
    "WiFi connected"
  );


  Serial.print(
    "IP Address: "
  );

  Serial.println(
    WiFi.localIP()
  );


  // ===================================================
  // WEB SERVER
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


  server.begin();


  Serial.println(
    "Dashboard server started"
  );


  Serial.print(
    "Dashboard: http://"
  );

  Serial.println(
    WiFi.localIP()
  );


  Serial.println();

  Serial.println(
    "Phone camera:"
  );

  Serial.println(
    "Scan QR from dashboard"
  );

}


// =====================================================
// LOOP
// =====================================================

void loop() {


  server.handleClient();


  // ===================================================
  // DHT READ
  // ===================================================

  if (
    millis() - lastDHTRead >=
    DHT_INTERVAL
  ) {


    lastDHTRead =
      millis();


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


    updateAlert();

    updateOLED();

  }

}