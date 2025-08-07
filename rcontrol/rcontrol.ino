#include <WiFi.h>
#include <ESPAsyncWebServer.h>

// --- WiFi Mode Configuration ---
#define AP_MODE // Comment this line out to enable Station mode

#ifdef AP_MODE
  // AP Mode Credentials
  const char* ssid = "ESP32-Robot-Control";
  const char* password = "password";
#else
  // Station Mode Credentials
  const char* ssid = "YOUR_WIFI_SSID";
  const char* password = "YOUR_WIFI_PASSWORD";
#endif

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP32 Robot Control</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body {
      font-family: Arial, sans-serif;
      text-align: center;
    }
    #joystick {
      width: 200px;
      height: 200px;
      border: 2px solid #000;
      border-radius: 50%;
      position: relative;
      margin: 50px auto;
    }
    #handle {
      width: 50px;
      height: 50px;
      background-color: red;
      border-radius: 50%;
      position: absolute;
      left: 75px;
      top: 75px;
    }
    .marker {
      position: absolute;
      font-size: 12px;
      color: #555;
    }
    .deg0 { top: 50%; right: -25px; transform: translateY(-50%); }
    .deg90 { top: -20px; left: 50%; transform: translateX(-50%); }
    .deg180 { top: 50%; left: -25px; transform: translateY(-50%); }
    .deg270 { bottom: -20px; left: 50%; transform: translateX(-50%); }
  </style>
</head>
<body>
  <h1>ESP32 Robot Control</h1>
  <div id="joystick">
    <div class="marker deg0">0</div>
    <div class="marker deg90">90</div>
    <div class="marker deg180">180</div>
    <div class="marker deg270">270</div>
    <div id="handle"></div>
  </div>

  <script>
    const joystick = document.getElementById('joystick');
    const handle = document.getElementById('handle');
    let isDragging = false;

    joystick.addEventListener('mousedown', (e) => {
      isDragging = true;
    });

    joystick.addEventListener('touchstart', (e) => {
      isDragging = true;
    });

    document.addEventListener('mouseup', () => {
      isDragging = false;
      resetHandle();
      sendJoystickData(0, 0);
    });

    document.addEventListener('touchend', () => {
      isDragging = false;
      resetHandle();
      sendJoystickData(0, 0);
    });

    document.addEventListener('mousemove', (e) => {
      if (isDragging) {
        updateHandle(e.clientX, e.clientY);
      }
    });

    joystick.addEventListener('touchmove', (e) => {
      e.preventDefault(); // Prevent pull-to-refresh
      if (isDragging) {
        updateHandle(e.touches[0].clientX, e.touches[0].clientY);
      }
    });

    function updateHandle(x, y) {
      const rect = joystick.getBoundingClientRect();
      let newX = x - rect.left - (handle.offsetWidth / 2);
      let newY = y - rect.top - (handle.offsetHeight / 2);

      const centerX = (joystick.offsetWidth / 2) - (handle.offsetWidth / 2);
      const centerY = (joystick.offsetHeight / 2) - (handle.offsetHeight / 2);

      // Constrain to joystick area
      const distance = Math.sqrt(Math.pow(newX - centerX, 2) + Math.pow(newY - centerY, 2));
      const maxDistance = joystick.offsetWidth / 2;

      if (distance > maxDistance) {
        const angle = Math.atan2(newY - centerY, newX - centerX);
        newX = centerX + maxDistance * Math.cos(angle);
        newY = centerY + maxDistance * Math.sin(angle);
      }

      handle.style.left = newX + 'px';
      handle.style.top = newY + 'px';

      // Convert to -100 to 100 range
      const joystickX = ((newX - centerX) / maxDistance) * 100;
      const joystickY = -(((newY - centerY) / maxDistance) * 100); // Invert Y

      sendJoystickData(joystickX.toFixed(0), joystickY.toFixed(0));
    }

    function resetHandle() {
      const centerX = (joystick.offsetWidth / 2) - (handle.offsetWidth / 2);
      const centerY = (joystick.offsetHeight / 2) - (handle.offsetHeight / 2);
      handle.style.left = centerX + 'px';
      handle.style.top = centerY + 'px';
    }

    function sendJoystickData(x, y) {
      fetch(`/joystick?x=${x}&y=${y}`);
    }

    resetHandle(); // Initialize handle position
  </script>
</body>
</html>
)rawliteral";

void setup() {
  // Start Serial for debugging
  Serial.begin(115200);

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  // Route to get joystick data
  server.on("/joystick", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String x_str = "0";
    String y_str = "0";
    if (request->hasParam("x")) {
      x_str = request->getParam("x")->value();
    }
    if (request->hasParam("y")) {
      y_str = request->getParam("y")->value();
    }

    int x = x_str.toInt();
    int y = y_str.toInt();

    // Calculate speed (magnitude)
    float speed = sqrt(pow(x, 2) + pow(y, 2));
    if (speed > 100) {
      speed = 100;
    }

    // Calculate direction (angle)
    float angle = atan2(y, x) * 180.0 / PI;
    if (angle < 0) {
      angle += 360.0;
    }

    Serial.printf("x: %d, y: %d, speed: %.0f, direction: %.0f\n", x, y, speed, angle);

    request->send(200, "text/plain", "OK");
  });

#ifdef AP_MODE
  // AP Mode
  Serial.print("Setting up AP...");
  WiFi.softAP(ssid, password);
  delay(100); // Add a small delay
  Serial.println("AP setup complete.");
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
#else
  // Station Mode
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
#endif

  // Start server
  server.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
}
