
//I wanted to build a simple Wi‑Fi–controlled external shutter trigger for a DSLR. 
//This controller uses an ESP8266 module as a web server and creates a local hotspot that allows you to connect from your mobile,tablet, or computer to control your DSLR remotely. 
//It includes functionality similar to that of a professional intervalometer,enabling full control over exposure time, delay between shots, and the number of shots. 
//Refer to your camera’s trigger connector pinout and connect it to GPIO #2 through a 1 kΩ resistor.

#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

#define SHUTTER_PIN 2  

unsigned long shutterDuration = 0; // in ms
unsigned long shutterDelay = 0;    // in ms
unsigned long maxShots = 0;        // maximum number of exposures (0 = infinite)
unsigned long shotCount = 0;       // track current shots

bool running = false;

// Access Point credentials
const char* ap_ssid = "Intervelometer";
const char* ap_password = "12345678";

AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <title>Camera Shutter Controller</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { background-color:black; font-family: Arial; text-align: center; margin-top: 50px; color:white; }
    .slider { width: 300px; }
    .killbtn { background-color:#E53935;color:white;border:none;padding:15px 30px;margin-top:15px;cursor:pointer;font-weight:bold;font-size:large;border-radius:6px;}
    .mainbtn { background-color:#0D47A1;color:white;border:none;padding:12px 24px;margin-top:10px;cursor:pointer;font-weight:bold;font-size:large;border-radius:6px;}
  </style>
</head>
<body>
  <h2>DSLR Camera Shutter Controller</h2>

  <p>Shutter Speed Duration: <span id="durationTxt">0 ms</span></p>
  <input type="range" min="10" max="300000" value="0" class="slider" id="durationSlider"
         oninput="showFormatted(this.value, 'durationTxt');setDuration(this.value)">

  <p>Delay Between Shots: <span id="delayTxt">1000 ms</span></p>
  <input type="range" min="1000" max="600000" value="1000" class="slider" id="delaySlider"
         oninput="showFormatted(this.value, 'delayTxt');setDelay(this.value)">

  <p>Shot Count: <span id="maxShotTxt">0 (∞)</span></p>
  <input type="range" min="0" max="100000" value="0" class="slider" id="maxShotSlider"
         oninput="showCount(this.value, 'maxShotTxt');setMaxShots(this.value)">

  <p>
    <button class="mainbtn" onclick="toggleTrigger()">Start/Stop Trigger</button>
  </p>
  <p>
    <button class="killbtn" onclick="killLoop()">KILL</button>
  </p>

  <p>Status: <span id="status">Stopped</span></p>
  <p>Shots Taken: <span id="shotCount">0</span></p>

  <script>
    function showFormatted(val, elem){
      let v = parseInt(val);
      document.getElementById(elem).innerText = (v < 1000) ? (v + " ms") : ((v / 1000).toFixed(0) + " s");
    }
    function showCount(val, elem){
      let v = parseInt(val);
      document.getElementById(elem).innerText = (v === 0) ? "0 (∞)" : v;
    }

    function setDuration(val){ fetch('/set?duration=' + val); }
    function setDelay(val){ fetch('/set?delay=' + val); }
    function setMaxShots(val){ fetch('/set?maxshots=' + val); }

    function toggleTrigger(){
      fetch('/toggle')
        .then(response => response.text())
        .then(data => document.getElementById('status').innerText = data);
    }
    function killLoop(){
      fetch('/kill')
        .then(response => response.text())
        .then(data => document.getElementById('status').innerText = data);
    }

    setInterval(() => {
      fetch('/status')
        .then(response => response.text())
        .then(data => document.getElementById('status').innerText = data);
      fetch('/shots')
        .then(response => response.text())
        .then(data => document.getElementById('shotCount').innerText = data);
    }, 1000);

    window.onload = function(){
      showFormatted(document.getElementById('durationSlider').value, 'durationTxt');
      showFormatted(document.getElementById('delaySlider').value, 'delayTxt');
      showCount(document.getElementById('maxShotSlider').value, 'maxShotTxt');
    };
  </script>
</body>
</html>
)rawliteral";

void handleShutter() {
  static unsigned long lastTrigger = 0;
  static bool shutterActive = false;
  static unsigned long shutterStart = 0;

  unsigned long now = millis();

  if (running) {
    if (maxShots > 0 && shotCount >= maxShots) {
      running = false;
      digitalWrite(SHUTTER_PIN, HIGH);
      return;
    }

    if (!shutterActive && (now - lastTrigger >= shutterDelay)) {
      digitalWrite(SHUTTER_PIN, LOW);
      shutterActive = true;
      shutterStart = now;
    } else if (shutterActive && (now - shutterStart >= shutterDuration)) {
      digitalWrite(SHUTTER_PIN, HIGH);
      shutterActive = false;
      lastTrigger = now;
      shotCount++;
    }
  } else {
    digitalWrite(SHUTTER_PIN, HIGH);
  }
}

void killLoop() {
  running = false;
  digitalWrite(SHUTTER_PIN, HIGH);
}

void setup() {
  pinMode(SHUTTER_PIN, OUTPUT);
  digitalWrite(SHUTTER_PIN, HIGH);

  Serial.begin(115200);
  WiFi.softAP(ap_ssid, ap_password);

  Serial.print("Access Point \"");
  Serial.print(ap_ssid);
  Serial.println("\" started.");
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html);
  });
  server.on("/set", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("duration"))
      shutterDuration = request->getParam("duration")->value().toInt();
    if (request->hasParam("delay"))
      shutterDelay = request->getParam("delay")->value().toInt();
    if (request->hasParam("maxshots"))
      maxShots = request->getParam("maxshots")->value().toInt();
    request->send(200, "text/plain", "OK");
  });
  server.on("/toggle", HTTP_GET, [](AsyncWebServerRequest *request) {
    running = !running;
    if (running) shotCount = 0;
    request->send(200, "text/plain", running ? "Running" : "Stopped");
  });
  server.on("/kill", HTTP_GET, [](AsyncWebServerRequest *request) {
    killLoop();
    request->send(200, "text/plain", "KILLED");
  });
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", running ? "Running" : "Stopped");
  });
  server.on("/shots", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", String(shotCount));
  });

  server.begin();
}

void loop() {
  handleShutter();
}
