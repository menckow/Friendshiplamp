#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPUpdate.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <PubSubClient.h>
#include <Preferences.h>
#include <DNSServer.h>
#include <time.h>
#include <ArduinoJson.h>

const char* FW_VERSION = "V2.1.3";

// == Globale Einstellungen =================================================
#define POTENTIOMETER_PIN 34
#define BUTTON_PIN 4
#define NEOPIXEL_PIN 13
#define TOUCH_PIN 32
#define NUM_PIXELS 40
#define TOUCH_THRESHOLD 40
#define BRIGHTNESS_ANIM_DELAY 10
#define DEBOUNCE_DELAY 50

// == Konfiguration =========================================================
struct Config {
  char ssid[32];
  char password[64];
  char mqttServer[64];
  uint16_t mqttPort;
  bool mqttTls;
  char mqttCaCert[2048];
  char mqttTopic[64];
  char mqttUser[32];
  char mqttPassword[64];
  uint32_t identityColor;
  bool quietModeEnabled;
  uint8_t quietHourStart;
  uint8_t quietHourEnd;
  char adminPassword[64];
  char mqttClientId[64];
  uint16_t numPixels;
  char effect[32];
  uint32_t duration;
  uint16_t touchThreshold;
};

Config config;
Preferences preferences;
bool shouldReboot = false;
unsigned long rebootTime = 0;

// == Globale Objekte =======================================================
Adafruit_NeoPixel pixels(NUM_PIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
AsyncWebServer server(80);
WiFiClient espClient;
WiFiClientSecure espClientSecure;
PubSubClient client(espClient);
DNSServer dnsServer;

// == Globale Zustandsvariablen =============================================
bool isLampOn = false;
uint32_t currentColor = pixels.Color(255, 255, 255);
uint8_t currentBrightness = 100;
int lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
volatile bool buttonPressedFlag = false;
volatile unsigned long lastInterruptTime = 0;
unsigned long lastMqttReconnectAttempt = 0;
unsigned long lastWifiReconnectAttempt = 0;
const unsigned long RECONNECT_INTERVAL = 5000;
int brightnessDirection = -1;
unsigned long lastBrightnessAnimTime = 0;

enum TouchState { IDLE, TOUCH_DETECTED, LONG_TOUCH_ACTIVE };
TouchState touchState = IDLE;
unsigned long touchStartTime = 0;

#define RECEIVED_COLOR_DURATION 10000 
bool inReceivedColorMode = false;
uint32_t receivedColor = 0;
unsigned long receivedColorEndTime = 0;
bool preReceivedState_isLampOn = false;
uint32_t preReceivedState_Color = 0;
char receivedEffect[32] = "fade";
unsigned long lastEffectTime = 0;
int effectStep = 0;

// == Funktionsdeklarationen ===============================================
void initializeHardware();
void loadConfiguration();
void saveConfiguration();
bool setupWifi();
void setupWebServer();
void setupMqtt();
void reconnectMqtt();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void setAllPixels(uint32_t color, int brightness = -1);
void handleButtonPress();
void handlePotentiometer();
void handleTouch();
void handleReceivedColorMode();
uint32_t hexToColor(const char* hexStr);
void startNormalMode();
void startApMode();
bool isQuietTime();
void performOtaUpdate(const char* url, const char* version);
String trimString(String str);

// == Web-Templates (PROGMEM) ==============================================
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Freundschaftslampe Konfiguration</title>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <style>
    body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif; background-color: #f0f2f5; color: #333; margin: 0; padding: 20px; display: flex; justify-content: center; }
    .container { width: 100%%; max-width: 700px; background-color: #fff; padding: 20px; border-radius: 8px; box-shadow: 0 4px 12px rgba(0,0,0,0.1); }
    h1 { text-align: center; color: #1c1e21; font-size: 2em; margin-bottom: 1em; }
    form { display: flex; flex-direction: column; }
    fieldset { border: 1px solid #ddd; border-radius: 8px; padding: 20px; margin-top: 20px; background-color: #fafafa; }
    legend { font-size: 1.2em; font-weight: 600; padding: 0 10px; color: #007bff; margin-left: 10px; }
    label { font-weight: 600; margin-top: 15px; margin-bottom: 5px; }
    input[type='text'], input[type='password'], input[type='number'], textarea, select { padding: 12px; margin-top: 5px; background-color: #fff; color: #333; border: 1px solid #ccc; border-radius: 6px; font-size: 1em; width: 100%%; box-sizing: border-box; }
    input:focus, textarea:focus, select:focus { border-color: #007bff; box-shadow: 0 0 0 3px rgba(0, 123, 255, 0.25); outline: none; }
    textarea { font-family: monospace; resize: vertical; min-height: 120px; }
    .checkbox-container { display: flex; align-items: center; margin-top: 15px; }
    input[type='checkbox'] { margin-right: 10px; width: 18px; height: 18px; }
    input[type='color'] { width: 100%%; height: 45px; padding: 5px; border-radius: 6px; border: 1px solid #ccc; }
    input[type='submit'] { background-color: #007bff; color: white; cursor: pointer; margin-top: 30px; padding: 15px; font-size: 1.1em; font-weight: 600; border: none; border-radius: 6px; }
    input[type='submit']:hover { background-color: #0056b3; }
    .help-text { font-size: 0.9em; color: #666; margin-top: 5px; }
  </style>
</head>
<body>
  <div class="container">
    <h1>Freundschaftslampe</h1>
    <form action='/save' method='POST'>
      <fieldset><legend>Administrator</legend>
        <label for='admin_pass'>Webinterface Passwort:</label>
        <input type='password' id='admin_pass' name='admin_pass' value='%ADMIN_PASS%'>
      </fieldset>
      <fieldset><legend>WLAN-Einstellungen</legend>
        <label for='ssid_select'>Verfügbare WLANs:</label>
        <select id='ssid_select' onchange='if(this.value) document.getElementById("ssid").value = this.value;'><option value=''>Suche WLANs...</option></select>
        <label for='ssid'>WLAN SSID:</label>
        <input type='text' id='ssid' name='ssid' value='%SSID%'>
        <label for='password'>WLAN Passwort:</label>
        <input type='password' id='password' name='password' value='%WIFI_PASS%'>
      </fieldset>
      <fieldset><legend>MQTT-Einstellungen</legend>
        <label for='mqtt'>MQTT Broker:</label>
        <input type='text' id='mqtt' name='mqtt' value='%MQTT_SERVER%'>
        <label for='mqtt_port'>MQTT Port:</label>
        <input type='number' id='mqtt_port' name='mqtt_port' value='%MQTT_PORT%'>
        <div class="checkbox-container"><input type='checkbox' id='mqtt_tls' name='mqtt_tls' %MQTT_TLS_CHECKED%><label for='mqtt_tls'>MQTT mit TLS/SSL</label></div>
        <label for='mqtt_ca'>CA Zertifikat (optional):</label>
        <textarea id='mqtt_ca' name='mqtt_ca'>%MQTT_CA%</textarea>
        <label for='mqtt_client_id'>MQTT Client ID:</label>
        <input type='text' id='mqtt_client_id' name='mqtt_client_id' value='%CLIENT_ID%'>
        <label for='mqtt_topic'>MQTT Topic:</label>
        <input type='text' id='mqtt_topic' name='mqtt_topic' value='%TOPIC%'>
        <label for='mqtt_user'>Benutzername:</label>
        <input type='text' id='mqtt_user' name='mqtt_user' value='%MQTT_USER%'>
        <label for='mqtt_pass'>Passwort:</label>
        <input type='password' id='mqtt_pass' name='mqtt_pass' value='%MQTT_PASS%'>
      </fieldset>
      <fieldset><legend>Lampen-Einstellungen</legend>
        <label for='num_pixels'>Anzahl LEDs:</label>
        <input type='number' id='num_pixels' name='num_pixels' value='%NUM_PIXELS%'>
        <label for='touch_threshold'>Touch-Empfindlichkeit (niedriger = empfindlicher):</label>
        <input type='number' id='touch_threshold' name='touch_threshold' value='%TOUCH_THRESHOLD%' min='1' max='100'>
        <label for='color'>Deine Identitätsfarbe:</label>
        <input type='color' id='color' name='color' value='%COLOR%'>
        <label for='effect'>Lichteffekt:</label>
        <select id='effect' name='effect'>
          <option value='fade' %FX_FADE%>Fade</option>
          <option value='color_wipe' %FX_WIPE%>Color Wipe</option>
          <option value='theater_chase' %FX_CHASE%>Theater Chase</option>
          <option value='rainbow_cycle' %FX_RAINBOW%>Rainbow</option>
          <option value='breathe' %FX_BREATHE%>Breathe</option>
          <option value='fire' %FX_FIRE%>Feuer</option>
          <option value='comet' %FX_COMET%>Komet</option>
        </select>
        <label for='duration'>Anzeigedauer (Sekunden):</label>
        <input type='number' id='duration' name='duration' value='%DURATION%'>
      </fieldset>
      <fieldset><legend>Ruhemodus</legend>
        <div class="checkbox-container"><input type='checkbox' id='quiet_enabled' name='quiet_enabled' %QUIET_CHECKED%><label for='quiet_enabled'>Aktivieren</label></div>
        <label for='quiet_start'>Startzeit (0-23):</label>
        <input type='number' id='quiet_start' name='quiet_start' value='%QUIET_START%'>
        <label for='quiet_end'>Endzeit (0-23):</label>
        <input type='number' id='quiet_end' name='quiet_end' value='%QUIET_END%'>
      </fieldset>
      <input type='submit' value='Speichern & Neustarten'>
    </form>
  </div>
  <script>
    function scanWlan() {
      fetch('/scan').then(r => r.json()).then(nets => {
        if(!nets) return;
        let s = document.getElementById('ssid_select');
        s.innerHTML = '<option value="">-- WLAN auswählen --</option>';
        nets.forEach(n => { let o = document.createElement('option'); o.value = n.ssid; o.textContent = n.ssid + " (" + n.rssi + " dBm)"; s.appendChild(o); });
      });
    }
    scanWlan();
  </script>
</body>
</html>
)rawliteral";

const char SUCCESS_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head><title>Gespeichert!</title><meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0">
<style>body { font-family: sans-serif; background: #f0f2f5; text-align: center; padding: 50px; } .card { background: white; padding: 30px; border-radius: 8px; max-width: 500px; margin: 0 auto; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }</style>
</head>
<body><div class="card"><h1>Gespeichert!</h1><p>Die Lampe startet neu...</p><a href="/">Zurück</a></div>
<script>setTimeout(() => { fetch('/reboot'); }, 1000);</script>
</body>
</html>
)rawliteral";

// == Hauptprogramm-Logik ==================================================
void IRAM_ATTR isr_button() {
  unsigned long interruptTime = millis();
  if (interruptTime - lastInterruptTime > DEBOUNCE_DELAY) {
      buttonPressedFlag = true;
  }
  lastInterruptTime = interruptTime;
}

void setup() {
  Serial.begin(115200);
  initializeHardware();
  loadConfiguration();
  if (strlen(config.ssid) > 0 && strcmp(config.ssid, "DEIN_WLAN_SSID") != 0) {
    if (setupWifi()) startNormalMode(); else startApMode();
  } else startApMode();
}

void loop() {
  handleReceivedColorMode();

  WiFiMode_t mode = WiFi.getMode();

  // Hardware-Interaktionen und MQTT (STA oder AP_STA)
  if (mode == WIFI_STA || mode == WIFI_AP_STA) {
    if (WiFi.status() == WL_CONNECTED) {
       if (!client.connected()) {
          if (millis() - lastMqttReconnectAttempt > RECONNECT_INTERVAL) {
             lastMqttReconnectAttempt = millis();
             reconnectMqtt(); 
          }
       } else client.loop();
    } else if (mode == WIFI_STA) {
       if (millis() - lastWifiReconnectAttempt > RECONNECT_INTERVAL) {
          lastWifiReconnectAttempt = millis();
          WiFi.reconnect();
       }
    }
    handleButtonPress();
    handlePotentiometer();
    handleTouch();
  }

  // DNS Server für Captive Portal (AP oder AP_STA)
  if (mode == WIFI_AP || mode == WIFI_AP_STA) {
    dnsServer.processNextRequest();
  }

  if (shouldReboot && millis() > rebootTime) {
    Serial.println("Neustart wird jetzt ausgeführt...");
    ESP.restart();
  }
  delay(10);
}

void initializeHardware() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), isr_button, FALLING);
  pixels.updateLength(config.numPixels > 0 ? config.numPixels : NUM_PIXELS);
  pixels.begin();
  setAllPixels(0);
}

void startNormalMode() {
  setupWebServer();
  setupMqtt();
  if (isLampOn) setAllPixels(currentColor);
}

void startApMode() {
  setAllPixels(pixels.Color(128, 0, 128));
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("Freundschaftslampe-Setup", "12345678");
  dnsServer.start(53, "*", WiFi.softAPIP());
  setupWebServer();
}

bool setupWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(config.ssid, config.password);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) delay(500);
  if (WiFi.status() == WL_CONNECTED) {
    configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", "pool.ntp.org");
    return true;
  }
  return false;
}

bool checkAuth(AsyncWebServerRequest *request) {
  if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) return true;
  if (strlen(config.adminPassword) > 0 && strcmp(config.adminPassword, "none") != 0) {
    if (!request->authenticate("admin", config.adminPassword)) {
      request->requestAuthentication();
      return false;
    }
  }
  return true;
}

uint32_t hexToColor(const char* hexStr) {
  if (!hexStr) return 0;
  if (hexStr[0] == '#') hexStr++;
  return (uint32_t) strtoul(hexStr, NULL, 16);
}

String templateProcessor(const String& var) {
  if (var == "ADMIN_PASS") return String(config.adminPassword);
  if (var == "SSID") return String(config.ssid);
  if (var == "WIFI_PASS") return String(config.password);
  if (var == "MQTT_SERVER") return String(config.mqttServer);
  if (var == "MQTT_PORT") return String(config.mqttPort);
  if (var == "MQTT_TLS_CHECKED") return config.mqttTls ? "checked" : "";
  if (var == "MQTT_CA") return String(config.mqttCaCert);
  if (var == "CLIENT_ID") return String(config.mqttClientId);
  if (var == "TOPIC") return String(config.mqttTopic);
  if (var == "MQTT_USER") return String(config.mqttUser);
  if (var == "MQTT_PASS") return String(config.mqttPassword);
  if (var == "NUM_PIXELS") return String(config.numPixels);
  if (var == "TOUCH_THRESHOLD") return String(config.touchThreshold);
  if (var == "COLOR") { char h[8]; sprintf(h, "#%06X", config.identityColor); return String(h); }
  if (var == "DURATION") return String(config.duration / 1000);
  if (var == "QUIET_CHECKED") return config.quietModeEnabled ? "checked" : "";
  if (var == "QUIET_START") return String(config.quietHourStart);
  if (var == "QUIET_END") return String(config.quietHourEnd);
  if (var == "FX_FADE") return strcmp(config.effect, "fade") == 0 ? "selected" : "";
  if (var == "FX_WIPE") return strcmp(config.effect, "color_wipe") == 0 ? "selected" : "";
  if (var == "FX_CHASE") return strcmp(config.effect, "theater_chase") == 0 ? "selected" : "";
  if (var == "FX_RAINBOW") return strcmp(config.effect, "rainbow_cycle") == 0 ? "selected" : "";
  if (var == "FX_BREATHE") return strcmp(config.effect, "breathe") == 0 ? "selected" : "";
  if (var == "FX_FIRE") return strcmp(config.effect, "fire") == 0 ? "selected" : "";
  if (var == "FX_COMET") return strcmp(config.effect, "comet") == 0 ? "selected" : "";
  return String();
}

void setupWebServer() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("Webserver: Index-Seite angefordert.");
    if (!checkAuth(request)) return;
    request->send_P(200, "text/html", INDEX_HTML, templateProcessor);
  });
  server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!checkAuth(request)) return;
    if (request->hasParam("admin_pass", true)) strlcpy(config.adminPassword, request->getParam("admin_pass", true)->value().c_str(), sizeof(config.adminPassword));
    if (request->hasParam("mqtt_client_id", true)) strlcpy(config.mqttClientId, request->getParam("mqtt_client_id", true)->value().c_str(), sizeof(config.mqttClientId));
    if (request->hasParam("ssid", true)) strlcpy(config.ssid, request->getParam("ssid", true)->value().c_str(), sizeof(config.ssid));
    if (request->hasParam("password", true)) strlcpy(config.password, request->getParam("password", true)->value().c_str(), sizeof(config.password));
    if (request->hasParam("mqtt", true)) strlcpy(config.mqttServer, request->getParam("mqtt", true)->value().c_str(), sizeof(config.mqttServer));
    if (request->hasParam("mqtt_port", true)) config.mqttPort = request->getParam("mqtt_port", true)->value().toInt();
    config.mqttTls = request->hasParam("mqtt_tls", true);
    if (request->hasParam("mqtt_ca", true)) strlcpy(config.mqttCaCert, request->getParam("mqtt_ca", true)->value().c_str(), sizeof(config.mqttCaCert));
    if (request->hasParam("mqtt_topic", true)) strlcpy(config.mqttTopic, request->getParam("mqtt_topic", true)->value().c_str(), sizeof(config.mqttTopic));
    if (request->hasParam("mqtt_user", true)) strlcpy(config.mqttUser, request->getParam("mqtt_user", true)->value().c_str(), sizeof(config.mqttUser));
    if (request->hasParam("mqtt_pass", true)) strlcpy(config.mqttPassword, request->getParam("mqtt_pass", true)->value().c_str(), sizeof(config.mqttPassword));
    if (request->hasParam("color", true)) config.identityColor = hexToColor(request->getParam("color", true)->value().c_str());
    if (request->hasParam("effect", true)) strlcpy(config.effect, request->getParam("effect", true)->value().c_str(), sizeof(config.effect));
    if (request->hasParam("duration", true)) config.duration = request->getParam("duration", true)->value().toInt() * 1000;
    config.quietModeEnabled = request->hasParam("quiet_enabled", true);
    if (request->hasParam("quiet_start", true)) config.quietHourStart = request->getParam("quiet_start", true)->value().toInt();
    if (request->hasParam("quiet_end", true)) config.quietHourEnd = request->getParam("quiet_end", true)->value().toInt();
    if (request->hasParam("num_pixels", true)) config.numPixels = request->getParam("num_pixels", true)->value().toInt();
    if (request->hasParam("touch_threshold", true)) config.touchThreshold = request->getParam("touch_threshold", true)->value().toInt();
    saveConfiguration();
    request->send_P(200, "text/html", SUCCESS_HTML);
  });
  server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request){
    if (!checkAuth(request)) return;
    request->send(200, "text/plain", "Rebooting...");
    shouldReboot = true; rebootTime = millis() + 1000;
  });
  server.on("/scan", HTTP_GET, [](AsyncWebServerRequest *request){
    if (!checkAuth(request)) return;
    int16_t n = WiFi.scanComplete();
    if(n == -2){ WiFi.scanNetworks(true); request->send(202); }
    else if(n == -1) request->send(202);
    else {
      String json = "[";
      for (int i = 0; i < n; i++) {
        if (i > 0) json += ",";
        json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
      }
      json += "]";
      WiFi.scanDelete(); request->send(200, "application/json", json);
    }
  });
  server.onNotFound([](AsyncWebServerRequest *request){
    if (request->host() != WiFi.softAPIP().toString()) request->redirect("http://" + WiFi.softAPIP().toString());
    else request->send(404);
  });
  server.begin();
}

void setupMqtt() {
  if (config.mqttTls) {
    if (strlen(config.mqttCaCert) > 0) espClientSecure.setCACert(config.mqttCaCert);
    else espClientSecure.setInsecure();
    client.setClient(espClientSecure);
  } else client.setClient(espClient);
  client.setServer(config.mqttServer, config.mqttPort);
  client.setCallback(mqttCallback);
}

void reconnectMqtt() {
  String clientId = String(config.mqttClientId);
  String statusTopic = "freundschaftslampe/status/" + clientId;
  if ((strlen(config.mqttUser) > 0 ? client.connect(clientId.c_str(), config.mqttUser, config.mqttPassword, statusTopic.c_str(), 1, true, "offline") : client.connect(clientId.c_str(), statusTopic.c_str(), 1, true, "offline"))) {
    client.subscribe(config.mqttTopic);
    client.subscribe("freundschaftslampe/update/trigger");
    client.publish(statusTopic.c_str(), (String(FW_VERSION) + ":online").c_str(), true);
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';
  if (strcmp(topic, config.mqttTopic) == 0) {
    if (isQuietTime()) return;
    JsonDocument doc;
    if (!deserializeJson(doc, payload, length)) {
      uint32_t color = hexToColor(doc["color"] | "");
      if (color) {
        strlcpy(receivedEffect, doc["effect"] | "fade", sizeof(receivedEffect));
        preReceivedState_isLampOn = isLampOn; preReceivedState_Color = currentColor;
        inReceivedColorMode = true; receivedColor = color;
        receivedColorEndTime = millis() + (uint32_t)(doc["duration"] | RECEIVED_COLOR_DURATION);
        effectStep = 0; lastEffectTime = 0;
      }
    }
  } else if (strcmp(topic, "freundschaftslampe/update/trigger") == 0) {
    JsonDocument doc;
    if (!deserializeJson(doc, payload, length)) {
      const char* url = doc["url"] | "";
      const char* version = doc["version"] | "";
      if (strlen(url) > 0 && strcmp(version, FW_VERSION) != 0) performOtaUpdate(url, version);
    }
  }
}

void setAllPixels(uint32_t color, int brightness) {
  uint8_t effectiveBrightness = (brightness == -1) ? currentBrightness : (uint8_t)brightness;
  uint8_t r = ((color >> 16) & 0xFF) * (effectiveBrightness / 255.0);
  uint8_t g = ((color >> 8) & 0xFF) * (effectiveBrightness / 255.0);
  uint8_t b = (color & 0xFF) * (effectiveBrightness / 255.0);
  for (int i = 0; i < pixels.numPixels(); i++) pixels.setPixelColor(i, r, g, b);
  pixels.show();
}

void handleButtonPress() {
  if (buttonPressedFlag) {
    buttonPressedFlag = false;
    JsonDocument doc;
    doc["client_id"] = config.mqttClientId;
    char hexColor[10]; sprintf(hexColor, "#%06X", config.identityColor);
    doc["color"] = hexColor; doc["effect"] = config.effect; doc["duration"] = config.duration;
    String p; serializeJson(doc, p);
    client.publish(config.mqttTopic, p.c_str());
  }
}

void handlePotentiometer() {
  if (inReceivedColorMode || !isLampOn) return;
  int raw = analogRead(POTENTIOMETER_PIN);
  uint16_t hue = map(raw, 0, 4095, 0, 65535);
  uint32_t c = pixels.gamma32(pixels.ColorHSV(hue));
  if (c != currentColor) { currentColor = c; setAllPixels(c); }
}

void renderEffect() {
  unsigned long now = millis();
  
  // Grundfarben extrahieren (0-255)
  uint8_t baseR = (receivedColor >> 16) & 0xFF;
  uint8_t baseG = (receivedColor >> 8) & 0xFF;
  uint8_t baseB = receivedColor & 0xFF;
  
  // SICHERHEIT: Falls die Lampe aus ist (Helligkeit 0) oder sehr dunkel, 
  // nutzen wir für empfangene Signale eine Mindesthelligkeit von ~60% (150/255)
  uint8_t effectiveBrightness = currentBrightness;
  if (effectiveBrightness < 150) effectiveBrightness = 150;
  
  float bFact = effectiveBrightness / 255.0;

  if (strcmp(receivedEffect, "color_wipe") == 0) {
    if (now - lastEffectTime > 50) {
      lastEffectTime = now;
      if (effectStep < config.numPixels) {
        uint8_t r = baseR * bFact;
        uint8_t g = baseG * bFact;
        uint8_t b = baseB * bFact;
        pixels.setPixelColor(effectStep, pixels.gamma32(pixels.Color(r, g, b)));
        pixels.show();
        effectStep++;
      }
    }
  } else if (strcmp(receivedEffect, "theater_chase") == 0) {
    if (now - lastEffectTime > 100) {
      lastEffectTime = now;
      pixels.clear();
      uint8_t r = baseR * bFact;
      uint8_t g = baseG * bFact;
      uint8_t b = baseB * bFact;
      uint32_t color = pixels.gamma32(pixels.Color(r, g, b));
      for (int i = 0; i < config.numPixels; i += 3) {
        if (i + effectStep < config.numPixels) {
          pixels.setPixelColor(i + effectStep, color);
        }
      }
      pixels.show();
      effectStep = (effectStep + 1) % 3;
    }
  } else if (strcmp(receivedEffect, "rainbow_cycle") == 0) {
    if (now - lastEffectTime > 20) {
      lastEffectTime = now;
      for(int i=0; i<config.numPixels; i++) {
        int pixelHue = effectStep + (i * 65536L / config.numPixels);
        uint32_t c = pixels.gamma32(pixels.ColorHSV(pixelHue));
        uint8_t r = ((c >> 16) & 0xFF) * bFact;
        uint8_t g = ((c >> 8) & 0xFF) * bFact;
        uint8_t b = (c & 0xFF) * bFact;
        pixels.setPixelColor(i, pixels.Color(r, g, b));
      }
      pixels.show();
      effectStep += 256;
      if(effectStep >= 65536) effectStep = 0;
    }
  } else if (strcmp(receivedEffect, "breathe") == 0) {
    if (now - lastEffectTime > 15) {
      lastEffectTime = now;
      float pulse = (sin(effectStep * 0.05) * 127 + 128) / 255.0;
      uint8_t r = baseR * bFact * pulse;
      uint8_t g = baseG * bFact * pulse;
      uint8_t b = baseB * bFact * pulse;
      uint32_t color = pixels.gamma32(pixels.Color(r, g, b));
      for(int i=0; i<config.numPixels; i++) {
        pixels.setPixelColor(i, color);
      }
      pixels.show();
      effectStep++;
    }
  } else if (strcmp(receivedEffect, "fire") == 0) {
    if (now - lastEffectTime > 50) {
      lastEffectTime = now;
      for(int i=0; i<config.numPixels; i++) {
        float flicker = random(50, 255) / 255.0;
        uint8_t r = baseR * bFact * flicker;
        uint8_t g = baseG * bFact * flicker;
        uint8_t b = baseB * bFact * flicker;
        pixels.setPixelColor(i, pixels.gamma32(pixels.Color(r, g, b)));
      }
      pixels.show();
      effectStep++;
    }
  } else if (strcmp(receivedEffect, "comet") == 0) {
    if (now - lastEffectTime > 30) {
      lastEffectTime = now;
      for(int i=0; i<config.numPixels; i++) {
        uint32_t c = pixels.getPixelColor(i);
        uint8_t r = ((c >> 16) & 0xFF) * 0.8;
        uint8_t g = ((c >> 8) & 0xFF) * 0.8;
        uint8_t b = (c & 0xFF) * 0.8;
        pixels.setPixelColor(i, pixels.Color(r, g, b));
      }
      uint8_t r = baseR * bFact;
      uint8_t g = baseG * bFact;
      uint8_t b = baseB * bFact;
      uint32_t head = pixels.gamma32(pixels.Color(r, g, b));
      pixels.setPixelColor(effectStep % config.numPixels, head);
      pixels.show();
      effectStep++;
    }
  } else {
    if (effectStep == 0) {
      uint8_t r = baseR * bFact;
      uint8_t g = baseG * bFact;
      uint8_t b = baseB * bFact;
      uint32_t color = pixels.gamma32(pixels.Color(r, g, b));
      for(int i=0; i<config.numPixels; i++) {
        pixels.setPixelColor(i, color);
      }
      pixels.show();
      effectStep = 1;
    }
  }
}

void handleReceivedColorMode() {
  if (!inReceivedColorMode) return;
  if (millis() >= receivedColorEndTime) {
    inReceivedColorMode = false;
    isLampOn = preReceivedState_isLampOn; currentColor = preReceivedState_Color;
    if (isLampOn) setAllPixels(currentColor); else setAllPixels(0);
    Serial.println("Anzeige beendet.");
  } else {
    renderEffect();
  }
}

void handleTouch() {
  if (inReceivedColorMode) return;
  int val = touchRead(TOUCH_PIN);
  unsigned long now = millis();
  if (val < config.touchThreshold) {
    if (touchState == IDLE) { touchState = TOUCH_DETECTED; touchStartTime = now; }
    if (touchState == TOUCH_DETECTED && (now - touchStartTime) >= 1000) {
      touchState = LONG_TOUCH_ACTIVE;
      if (!isLampOn) { isLampOn = true; if (currentBrightness == 0) currentBrightness = 10; setAllPixels(currentColor); }
    }
    if (touchState == LONG_TOUCH_ACTIVE && (now - lastBrightnessAnimTime > BRIGHTNESS_ANIM_DELAY)) {
      lastBrightnessAnimTime = now;
      int next = currentBrightness + brightnessDirection;
      if (next <= 10) { next = 10; brightnessDirection = 1; }
      else if (next >= 255) { next = 255; brightnessDirection = -1; }
      currentBrightness = next; setAllPixels(currentColor);
    }
  } else {
    if (touchState == TOUCH_DETECTED) {
      isLampOn = !isLampOn;
      if (isLampOn) { if (currentBrightness == 0) currentBrightness = 255; setAllPixels(currentColor); }
      else setAllPixels(0);
    }
    touchState = IDLE;
  }
}

bool isQuietTime() {
  if (!config.quietModeEnabled) return false;
  struct tm t; if (!getLocalTime(&t)) return false;
  if (config.quietHourStart < config.quietHourEnd) return (t.tm_hour >= config.quietHourStart && t.tm_hour < config.quietHourEnd);
  return (t.tm_hour >= config.quietHourStart || t.tm_hour < config.quietHourEnd);
}

void loadConfiguration() {
  preferences.begin("f-lampe", true);
  preferences.getString("ssid", config.ssid, 32);
  preferences.getString("password", config.password, 64);
  preferences.getString("mqttServer", config.mqttServer, 64);
  config.mqttPort = preferences.getUShort("mqttPort", 1883);
  config.mqttTls = preferences.getBool("mqttTls", false);
  preferences.getString("mqttCaCert", config.mqttCaCert, 2048);
  preferences.getString("mqttTopic", config.mqttTopic, 64);
  preferences.getString("mqttUser", config.mqttUser, 32);
  preferences.getString("mqttPassword", config.mqttPassword, 64);
  config.identityColor = preferences.getUInt("identityColor", 0x0000FF);
  config.quietModeEnabled = preferences.getBool("quietEnabled", false);
  config.quietHourStart = preferences.getUChar("quietStart", 22);
  config.quietHourEnd = preferences.getUChar("quietEnd", 6);
  preferences.getString("adminPassword", config.adminPassword, 64);
  preferences.getString("mqttClientId", config.mqttClientId, 64);
  config.numPixels = preferences.getUShort("numPixels", NUM_PIXELS);
  preferences.getString("effect", config.effect, 32);
  config.duration = preferences.getUInt("duration", 10000);
  config.touchThreshold = preferences.getUShort("touch_threshold", TOUCH_THRESHOLD);
  preferences.end();
}

void saveConfiguration() {
  preferences.begin("f-lampe", false);
  preferences.putString("ssid", config.ssid);
  preferences.putString("password", config.password);
  preferences.putString("mqttServer", config.mqttServer);
  preferences.putUShort("mqttPort", config.mqttPort);
  preferences.putBool("mqttTls", config.mqttTls);
  preferences.putString("mqttCaCert", config.mqttCaCert);
  preferences.putString("mqttTopic", config.mqttTopic);
  preferences.putString("mqttUser", config.mqttUser);
  preferences.putString("mqttPassword", config.mqttPassword);
  preferences.putUInt("identityColor", config.identityColor);
  preferences.putBool("quietEnabled", config.quietModeEnabled);
  preferences.putUChar("quietStart", config.quietHourStart);
  preferences.putUChar("quietEnd", config.quietHourEnd);
  preferences.putString("adminPassword", config.adminPassword);
  preferences.putString("mqttClientId", config.mqttClientId);
  preferences.putUShort("numPixels", config.numPixels);
  preferences.putString("effect", config.effect);
  preferences.putUInt("duration", config.duration);
  preferences.putUShort("touch_threshold", config.touchThreshold);
  preferences.end();
}

void performOtaUpdate(const char* url, const char* version) {
  Serial.println("OTA Update Prozess gestartet...");
  Serial.printf("Update-URL: %s\n", url);
  
  // Status via MQTT melden
  String statusTopic = "freundschaftslampe/status/" + String(config.mqttClientId);
  String startMsg = "Updating to " + String(version);
  client.publish(statusTopic.c_str(), (String(FW_VERSION) + ":" + startMsg).c_str(), true);
  client.publish("freundschaftslampe/update/status", startMsg.c_str());
  client.loop(); 

  WiFiClientSecure otaClient;
  otaClient.setInsecure();
  Serial.println("OTA: Nutze Insecure-Mode.");

  // Ring blau leuchten lassen während des Updates (mit voller Helligkeit)
  setAllPixels(pixels.Color(0, 0, 255), 100);

  httpUpdate.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);

  // Fortschritts-Handling
  httpUpdate.onProgress([](int cur, int total) {
    static int lastPercent = -1;
    int percent = (cur * 100) / total;
    if (percent % 10 == 0 && percent != lastPercent) {
      lastPercent = percent;
      Serial.printf("Download: %d%%\n", percent);
    }
  });

  t_httpUpdate_return ret = httpUpdate.update(otaClient, url);

  switch (ret) {
    case HTTP_UPDATE_FAILED: {
      String errorMsg = "Update failed: " + httpUpdate.getLastErrorString();
      Serial.println(errorMsg);
      client.publish("freundschaftslampe/update/status", errorMsg.c_str());
      setAllPixels(pixels.Color(255, 0, 0), 100); // Rot bei Fehler (mit voller Helligkeit)
      delay(2000);
      setAllPixels(0, 0);
      break;
    }
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("Keine Updates verfügbar.");
      client.publish("freundschaftslampe/update/status", "No updates available");
      break;
    case HTTP_UPDATE_OK:
      Serial.println("Update erfolgreich! ESP32 startet neu...");
      client.publish("freundschaftslampe/update/status", "Success! Rebooting...");
      client.loop();
      delay(1000);
      break;
  }
}
