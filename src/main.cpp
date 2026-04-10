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
#include <esp_crt_bundle.h>

const char* FW_VERSION = "2.1.0";

const char* FW_VERSION = "2.1.0";

// == Globale Einstellungen =================================================
// -- Hardware-Pins --
#define POTENTIOMETER_PIN 34   // Pin für das Potentiometer (muss ein ADC-Pin sein) 1 ESP32 34
#define BUTTON_PIN 4          // Pin für den Taster
#define NEOPIXEL_PIN 13        // Pin für den NeoPixel-Ring (geändert auf einen sichereren Pin)
#define TOUCH_PIN 32          // Kapazitiver Touch-Pin (GPIO9) ESP32 32

// -- NeoPixel-Einstellungen --
#define NUM_PIXELS 40         // Anzahl der LEDs im Ring

// -- Touch-Einstellungen --
#define TOUCH_THRESHOLD 40    // Schwellenwert für die Berührungserkennung
#define BRIGHTNESS_ANIM_DELAY 10 // Zeit in ms zwischen den Helligkeitsschritten

// -- Taster-Einstellungen --
#define DEBOUNCE_DELAY 50 // Entprellzeit in ms

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
  char otaCaCert[3072];
};

Config config; // Globale Konfigurationsvariable
Preferences preferences; // Objekt für den Zugriff auf den NVS

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
int buttonState;
int lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;

volatile bool buttonPressedFlag = false;
volatile unsigned long lastInterruptTime = 0;
unsigned long lastMqttReconnectAttempt = 0;
unsigned long lastWifiReconnectAttempt = 0;
const unsigned long RECONNECT_INTERVAL = 5000;

// -- Helligkeits-Animation --
int brightnessDirection = -1; // -1 für dunkler, 1 für heller
unsigned long lastBrightnessAnimTime = 0;

// -- Kombinierte Touch-Steuerung --
enum TouchState { IDLE, TOUCH_DETECTED, LONG_TOUCH_ACTIVE };
TouchState touchState = IDLE;
unsigned long touchStartTime = 0;

// == Timer-Zustand =========================================================
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
void setAllPixels(uint32_t color);
void handleButtonPress();
void handlePotentiometer();
void handleTouch();
void handleBrightnessTouch();
void handleReceivedColorMode();
uint32_t hexToColor(const char* hexStr);
void startNormalMode();
void startApMode();
bool isQuietTime();
void performOtaUpdate(const char* url, const char* version);
String trimString(String str);

// == Hauptprogramm =========================================================
void setup() {
  Serial.begin(115200);
  Serial.println("\nFreundschaftslampe startet...");

  initializeHardware(); // Hardware IMMER initialisieren

  loadConfiguration(); // Gespeicherte Einstellungen laden

  bool wifiConnected = false;
  // Prüfen, ob eine SSID konfiguriert ist
  if (strlen(config.ssid) > 0 && strcmp(config.ssid, "DEIN_WLAN_SSID") != 0) {
    // Versuche, eine Verbindung zum gespeicherten WLAN herzustellen
    wifiConnected = setupWifi();
  }

  if (wifiConnected) {
    // WLAN-Verbindung erfolgreich, starte im normalen Modus
    startNormalMode();
  } else {
    // Keine SSID konfiguriert ODER Verbindung fehlgeschlagen, starte AP-Modus
    startApMode();
  }
}

void loop() {
  // Timer-Modus für empfangene Farben prüfen
  handleReceivedColorMode();

  // Nur im normalen Modus MQTT und Hardware-Interaktionen ausführen
  if (WiFi.getMode() == WIFI_STA) {
    if (WiFi.status() != WL_CONNECTED) {
       if (millis() - lastWifiReconnectAttempt > RECONNECT_INTERVAL) {
          lastWifiReconnectAttempt = millis();
          Serial.println("WLAN Verbindung verloren. Versuche Reconnect...");
          WiFi.reconnect();
       }
    } else {
       if (!client.connected()) {
          if (millis() - lastMqttReconnectAttempt > RECONNECT_INTERVAL) {
             lastMqttReconnectAttempt = millis();
             reconnectMqtt(); 
          }
       } else {
          client.loop();
       }
    }

    // Hardware abfragen
    handleButtonPress();
    handlePotentiometer();
    handleTouch();
  } else if (WiFi.getMode() == WIFI_AP) {
    // Im AP-Modus DNS-Anfragen für das Captive-Portal verarbeiten
    dnsServer.processNextRequest();
  }
  
  // Kurze Pause, um den ESP32 nicht zu überlasten
  delay(10); 
}

// == Funktionsdefinitionen =================================================

void IRAM_ATTR isr_button() {
  unsigned long interruptTime = millis();
  if (interruptTime - lastInterruptTime > DEBOUNCE_DELAY) {
      buttonPressedFlag = true;
  }
  lastInterruptTime = interruptTime;
}

void initializeHardware() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), isr_button, FALLING);
  pixels.updateLength(config.numPixels > 0 ? config.numPixels : NUM_PIXELS);
  pixels.begin();
  pixels.show(); // Pixel initial ausschalten
  Serial.println("Hardware initialisiert.");
}

void startNormalMode() {
  Serial.println("Starte im normalen Modus...");
  
  // Netzwerk-Dienste starten (WiFi ist bereits verbunden)
  setupWebServer();
  setupMqtt();
  
  // Lampe beim Start auf die Standardfarbe setzen, falls sie an sein soll
  if (isLampOn) {
    setAllPixels(currentColor);
  }

  Serial.println("Setup für normalen Modus abgeschlossen.");
}

void startApMode() {
  const char* ap_ssid = "Freundschaftslampe-Setup";
  
  Serial.println("Starte im Access-Point-Modus zur Konfiguration...");
  Serial.printf("Erstelle WLAN mit SSID: %s\n", ap_ssid);

  // Ring lila färben, um den AP-Modus anzuzeigen
  setAllPixels(pixels.Color(128, 0, 128));

  // AP-Modus starten mit Passwort (inklusive STA-Modus fürs Scannen von Netzwerken)
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ap_ssid, "12345678");
  
  // IP-Adresse des AP abrufen und ausgeben
  IPAddress apIP = WiFi.softAPIP();
  Serial.printf("AP IP-Adresse: %s\n", apIP.toString().c_str());

  // DNS-Server starten
  // Alle Anfragen werden auf die IP des ESPs umgeleitet
  dnsServer.start(53, "*", apIP);
  
  // Webserver starten (der die Konfig-Seite anzeigt)
  setupWebServer();

  Serial.println("AP-Modus-Setup abgeschlossen. Verbinde dich mit dem WLAN und öffne eine beliebige Webseite.");
}
bool setupWifi() {
  const unsigned long wifiTimeout = 30000; // 30 Sekunden Timeout
  unsigned long startTime = millis();

  Serial.print("Verbinde mit WLAN: ");
  Serial.println(config.ssid);
  WiFi.mode(WIFI_STA); // Explizit in den Station-Modus wechseln
  WiFi.begin(config.ssid, config.password);

  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - startTime >= wifiTimeout) {
      Serial.println("\nVerbindung zum WLAN fehlgeschlagen (Timeout).");
      WiFi.disconnect(); // Verbindung abbrechen
      return false;
    }
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWLAN verbunden!");
  Serial.print("IP-Adresse: ");
  Serial.println(WiFi.localIP());

  // Zeit via NTP synchronisieren
  Serial.println("Synchronisiere Zeit via NTP...");
  configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", "pool.ntp.org", "time.nist.gov");
  return true;
}

// Hilfsfunktion, um einen Hex-String (z.B. "#RRGGBB") in eine 32-Bit-Farbe umzuwandeln
uint32_t hexToColor(const char* hexStr) {
  if (hexStr == nullptr) return 0;
  if (hexStr[0] == '#') {
    hexStr++;
  }
  return (uint32_t) strtoul(hexStr, NULL, 16);
}

bool checkAuth(AsyncWebServerRequest *request) {
    if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) return true; // Captive Portal ist bereits durch WPA2-Passwort gesichert
    if (strlen(config.adminPassword) > 0 && strcmp(config.adminPassword, "none") != 0) {
        if (!request->authenticate("admin", config.adminPassword)) {
            request->requestAuthentication();
            return false;
        }
    }
    return true;
}

void setupWebServer() {
  // Konfigurationsseite unter der Haupt-URL "/"
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    if (!checkAuth(request)) return;
    String html;
    html.reserve(10000);
    html += R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Freundschaftslampe Konfiguration</title>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <style>
    body {
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
      background-color: #f0f2f5;
      color: #333;
      margin: 0;
      padding: 20px;
      display: flex;
      justify-content: center;
    }
    .container {
      width: 100%;
      max-width: 700px;
      background-color: #fff;
      padding: 20px;
      border-radius: 8px;
      box-shadow: 0 4px 12px rgba(0,0,0,0.1);
    }
    h1 {
      text-align: center;
      color: #1c1e21;
      font-size: 2em;
      margin-bottom: 1em;
    }
    form {
      display: flex;
      flex-direction: column;
    }
    fieldset {
      border: 1px solid #ddd;
      border-radius: 8px;
      padding: 20px;
      margin-top: 20px;
      background-color: #fafafa;
    }
    legend {
      font-size: 1.2em;
      font-weight: 600;
      padding: 0 10px;
      color: #007bff;
      margin-left: 10px;
    }
    label {
      font-weight: 600;
      margin-top: 15px;
      margin-bottom: 5px;
    }
    input[type='text'], input[type='password'], input[type='number'], textarea, select {
      padding: 12px;
      margin-top: 5px;
      background-color: #fff;
      color: #333;
      border: 1px solid #ccc;
      border-radius: 6px;
      font-size: 1em;
      width: 100%;
      box-sizing: border-box;
      transition: border-color 0.3s, box-shadow 0.3s;
    }
    input:focus, textarea:focus, select:focus {
        border-color: #007bff;
        box-shadow: 0 0 0 3px rgba(0, 123, 255, 0.25);
        outline: none;
    }
    textarea {
      font-family: monospace;
      resize: vertical;
      min-height: 120px;
    }
    .checkbox-container {
        display: flex;
        align-items: center;
        margin-top: 15px;
    }
    input[type='checkbox'] {
        margin-right: 10px;
        width: 18px;
        height: 18px;
    }
    .checkbox-container label {
        margin-top: 0; /* Align label with checkbox */
        font-weight: normal;
    }
    input[type='color'] {
        width: 100%;
        height: 45px;
        padding: 5px;
        border-radius: 6px;
        border: 1px solid #ccc;
        box-sizing: border-box;
    }
    input[type='submit'] {
      background-color: #007bff;
      color: white;
      cursor: pointer;
      margin-top: 30px;
      padding: 15px;
      font-size: 1.1em;
      font-weight: 600;
      border: none;
      border-radius: 6px;
      transition: background-color 0.3s;
    }
    input[type='submit']:hover {
        background-color: #0056b3;
    }
    .help-text {
        font-size: 0.9em;
        color: #666;
        margin-top: 5px;
    }
    /* Responsive adjustments */
    @media (max-width: 600px) {
      body {
        padding: 10px;
      }
      .container {
        padding: 15px;
      }
      fieldset {
        padding: 15px;
      }
      h1 {
        font-size: 1.8em;
      }
      legend {
        font-size: 1.1em;
      }
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>Freundschaftslampe</h1>
    <form action='/save' method='POST'>
      
      <fieldset>
        <legend>Administrator</legend>
        <label for='admin_pass'>Webinterface Passwort:</label>
        <input type='password' id='admin_pass' name='admin_pass' value=''>
      </fieldset>
      <fieldset>
        <legend>WLAN-Einstellungen</legend>
        <label for='ssid_select'>Verfügbare WLANs:</label>
        <select id='ssid_select' onchange='if(this.value) document.getElementById("ssid").value = this.value;'>
          <option value=''>Suche WLANs...</option>
        </select>
        <label for='ssid'>WLAN SSID (oder manuell eingeben):</label>
        <input type='text' id='ssid' name='ssid' placeholder="z.B. MeinWLAN" value=''>
        <label for='password'>WLAN Passwort:</label>
        <input type='password' id='password' name='password' value=''>
      </fieldset>

      <fieldset>
        <legend>MQTT-Einstellungen</legend>
        <label for='mqtt'>MQTT Broker:</label>
        <input type='text' id='mqtt' name='mqtt' placeholder="z.B. broker.hivemq.com" value=''>
        <label for='mqtt_port'>MQTT Port:</label>
        <input type='number' id='mqtt_port' name='mqtt_port' placeholder="z.B. 1883" value='1883'>
        <div class="checkbox-container">
          <input type='checkbox' id='mqtt_tls' name='mqtt_tls'>
          <label for='mqtt_tls'>MQTT mit TLS/SSL</label>
        </div>
        <label for='mqtt_ca'>CA Zertifikat (optional):</label>
        <textarea id='mqtt_ca' name='mqtt_ca' placeholder="-----BEGIN CERTIFICATE-----\n...\n-----END CERTIFICATE-----"></textarea>
        <label for='mqtt_client_id'>MQTT Client ID:</label>
        <input type='text' id='mqtt_client_id' name='mqtt_client_id' value=''>
        <label for='mqtt_topic'>MQTT Topic:</label>
        <input type='text' id='mqtt_topic' name='mqtt_topic' placeholder="z.B. freundschaft/farbe" value='freundschaft/farbe'>
        <label for='mqtt_user'>MQTT Benutzername (optional):</label>
        <input type='text' id='mqtt_user' name='mqtt_user' value=''>
        <label for='mqtt_pass'>MQTT Passwort (optional):</label>
        <input type='password' id='mqtt_pass' name='mqtt_pass' value=''>
        <label for='ota_ca'>OTA Firmware CA Zertifikat (optional):</label>
        <p class="help-text">Wird zur Verifizierung von HTTPS-Download-Servern (z.B. GitHub) verwendet.</p>
        <textarea id='ota_ca' name='ota_ca' placeholder="-----BEGIN CERTIFICATE-----\n...\n-----END CERTIFICATE-----"></textarea>
      </fieldset>
      
      <fieldset>
        <legend>Lampen-Einstellungen</legend>
        <label for='num_pixels'>Anzahl LEDs (NeoPixel Ring):</label>
        <input type='number' id='num_pixels' name='num_pixels' min='1' max='255' value='40'>
        <label for='color'>Deine Identitätsfarbe:</label>
        <p class="help-text">Diese Farbe wird gesendet, wenn du den Knopf drückst.</p>
        <input type='color' id='color' name='color' value='#0000FF'>
        <label for='effect'>Lichteffekt (beim Senden):</label>
        <select id='effect' name='effect'>
          <option value='fade'>Fade (Standard)</option>
          <option value='color_wipe'>Color Wipe</option>
          <option value='theater_chase'>Theater Chase</option>
          <option value='rainbow_cycle'>Rainbow Cycle</option>
          <option value='breathe'>Breathe / Pulse</option>
          <option value='fire'>Feuer-Effekt</option>
          <option value='comet'>Komet / Scanner</option>
        </select>
        <label for='duration'>Anzeigedauer beim Empfangen (in Sekunden):</label>
        <input type='number' id='duration' name='duration' min='1' max='3600' value='10'>
      </fieldset>

      <fieldset>
        <legend>Ruhemodus</legend>
        <p class="help-text">Die Lampe leuchtet nachts nicht auf, wenn Nachrichten empfangen werden.</p>
        <div class="checkbox-container">
          <input type='checkbox' id='quiet_enabled' name='quiet_enabled'>
          <label for='quiet_enabled'>Ruhemodus aktivieren</label>
        </div>
        <label for='quiet_start'>Startzeit (Stunde, 0-23):</label>
        <input type='number' id='quiet_start' name='quiet_start' min='0' max='23' value='22'>
        <label for='quiet_end'>Endzeit (Stunde, 0-23):</label>
        <input type='number' id='quiet_end' name='quiet_end' min='0' max='23' value='6'>
      </fieldset>

      <input type='submit' value='Speichern & Neustarten'>
    </form>
  </div>
  <script>
)rawliteral";

    // --- Dynamische Werte einfügen ---
    String script;
    script += "document.getElementById('admin_pass').value = '" + String(config.adminPassword) + "';\n";
    script += "document.getElementById('mqtt_client_id').value = '" + String(config.mqttClientId) + "';\n";
    script += "document.getElementById('num_pixels').value = '" + String(config.numPixels) + "';\n";
    script += "document.getElementById('ssid').value = '" + String(config.ssid) + "';\n";
    script += "document.getElementById('password').value = '" + String(config.password) + "';\n";
    script += "document.getElementById('mqtt').value = '" + String(config.mqttServer) + "';\n";
    script += "document.getElementById('mqtt_port').value = '" + String(config.mqttPort) + "';\n";
    script += "document.getElementById('mqtt_tls').checked = " + String(config.mqttTls ? "true" : "false") + ";\n";
    script += "document.getElementById('mqtt_ca').value = `" + String(config.mqttCaCert) + "`;\n";
    script += "document.getElementById('mqtt_topic').value = '" + String(config.mqttTopic) + "';\n";
    script += "document.getElementById('mqtt_user').value = '" + String(config.mqttUser) + "';\n";
    script += "document.getElementById('mqtt_pass').value = '" + String(config.mqttPassword) + "';\n";
    script += "document.getElementById('ota_ca').value = `" + String(config.otaCaCert) + "`;\n";
    char hexColor[8];
    sprintf(hexColor, "#%06X", config.identityColor);
    script += "document.getElementById('color').value = '" + String(hexColor) + "';\n";
    script += "document.getElementById('effect').value = '" + String(config.effect) + "';\n";
    script += "document.getElementById('duration').value = '" + String(config.duration / 1000) + "';\n";
    script += "document.getElementById('quiet_enabled').checked = " + String(config.quietModeEnabled ? "true" : "false") + ";\n";
    script += "document.getElementById('quiet_start').value = '" + String(config.quietHourStart) + "';\n";
    script += "document.getElementById('quiet_end').value = '" + String(config.quietHourEnd) + "';\n";

    html += script;
    html += R"rawliteral(
    function scanWlan() {
      fetch('/scan')
        .then(response => {
          if (response.status === 202) {
            document.getElementById('ssid_select').innerHTML = '<option value="">Suche läuft...</option>';
            setTimeout(scanWlan, 1500);
            return null;
          }
          return response.json();
        })
        .then(networks => {
          if (!networks) return;
          let select = document.getElementById('ssid_select');
          select.innerHTML = '<option value="">-- WLAN auswählen --</option>';
          
          // Dedupliziere SSIDs und sortiere nach Signalstärke
          let uniqueNetworks = [];
          let seen = new Set();
          networks.sort((a, b) => b.rssi - a.rssi).forEach(net => {
            if (net.ssid && !seen.has(net.ssid)) {
              seen.add(net.ssid);
              uniqueNetworks.push(net);
            }
          });
          
          uniqueNetworks.forEach(net => {
            let opt = document.createElement('option');
            opt.value = net.ssid;
            opt.textContent = net.ssid + ' (' + net.rssi + ' dBm)';
            select.appendChild(opt);
          });
        })
        .catch(err => {
          document.getElementById('ssid_select').innerHTML = '<option value="">Fehler beim Laden der WLANs</option>';
        });
    }
    scanWlan();
  </script>
</body>
</html>
)rawliteral";
    
    request->send(200, "text/html", html);
  });

  // Endpunkt zum Speichern der Konfiguration
  server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!checkAuth(request)) return;
    // Parameter auslesen
    if (request->hasParam("admin_pass", true)) strlcpy(config.adminPassword, request->getParam("admin_pass", true)->value().c_str(), sizeof(config.adminPassword));
    if (request->hasParam("mqtt_client_id", true)) strlcpy(config.mqttClientId, request->getParam("mqtt_client_id", true)->value().c_str(), sizeof(config.mqttClientId));
    if (request->hasParam("ssid", true)) strlcpy(config.ssid, request->getParam("ssid", true)->value().c_str(), sizeof(config.ssid));
    if (request->hasParam("password", true)) strlcpy(config.password, request->getParam("password", true)->value().c_str(), sizeof(config.password));
    if (request->hasParam("mqtt", true)) strlcpy(config.mqttServer, request->getParam("mqtt", true)->value().c_str(), sizeof(config.mqttServer));
    if (request->hasParam("mqtt_port", true)) config.mqttPort = request->getParam("mqtt_port", true)->value().toInt();
    config.mqttTls = request->hasParam("mqtt_tls", true);
    if (request->hasParam("mqtt_ca", true)) {
      String ca = trimString(request->getParam("mqtt_ca", true)->value());
      strlcpy(config.mqttCaCert, ca.c_str(), sizeof(config.mqttCaCert));
    }
    if (request->hasParam("mqtt_topic", true)) strlcpy(config.mqttTopic, request->getParam("mqtt_topic", true)->value().c_str(), sizeof(config.mqttTopic));
    if (request->hasParam("mqtt_user", true)) strlcpy(config.mqttUser, request->getParam("mqtt_user", true)->value().c_str(), sizeof(config.mqttUser));
    if (request->hasParam("mqtt_pass", true)) strlcpy(config.mqttPassword, request->getParam("mqtt_pass", true)->value().c_str(), sizeof(config.mqttPassword));
    if (request->hasParam("color", true)) {
      config.identityColor = hexToColor(request->getParam("color", true)->value().c_str());
    }
    if (request->hasParam("effect", true)) {
      strlcpy(config.effect, request->getParam("effect", true)->value().c_str(), sizeof(config.effect));
    }
    if (request->hasParam("duration", true)) {
      config.duration = request->getParam("duration", true)->value().toInt() * 1000; // convert to ms
    }
    if (request->hasParam("ota_ca", true)) {
      String ca = trimString(request->getParam("ota_ca", true)->value());
      strlcpy(config.otaCaCert, ca.c_str(), sizeof(config.otaCaCert));
    }
    config.quietModeEnabled = request->hasParam("quiet_enabled", true);
    if (request->hasParam("quiet_start", true)) config.quietHourStart = request->getParam("quiet_start", true)->value().toInt();
    if (request->hasParam("quiet_end", true)) config.quietHourEnd = request->getParam("quiet_end", true)->value().toInt();
    if (request->hasParam("num_pixels", true)) config.numPixels = request->getParam("num_pixels", true)->value().toInt();

    // Konfiguration speichern
    saveConfiguration();

    // Erfolgsseite mit Neustart-Button
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Gespeichert!</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif; background-color: #f0f2f5; color: #333; text-align: center; padding-top: 50px; }
        .container { max-width: 600px; margin: 0 auto; background-color: #fff; padding: 30px; border-radius: 8px; box-shadow: 0 4px 12px rgba(0,0,0,0.1); }
        h1 { color: #28a745; }
        p { font-size: 1.1em; margin-bottom: 30px; }
        a { padding: 12px 25px; background-color: #007bff; color: white; text-decoration: none; border-radius: 6px; font-size: 1.1em; transition: background-color 0.3s; }
        a:hover { background-color: #0056b3; }
    </style>
</head>
<body>
    <div class="container">
        <h1>Konfiguration gespeichert!</h1>
        <p>Die Lampe wird jetzt neu gestartet, um die Änderungen zu übernehmen.</p>
        <p style="font-size:0.9em; color:#666;">Dieser Tab kann in wenigen Sekunden geschlossen werden.</p>
        <a href='/'>Zurück zur Konfiguration</a>
    </div>
    <script>
        setTimeout(function(){ 
            // Send request to reboot, but don't wait for response.
            // This gives the server a moment to send this page's response.
            var xhr = new XMLHttpRequest();
            xhr.open('GET', '/reboot', true);
            xhr.send();
            
            // Visually indicate the restart is happening
            document.querySelector('h1').innerText = "Neustart...";
            document.querySelector('p').innerText = "Bitte warten, die Lampe verbindet sich gleich mit dem neuen Netzwerk.";
            document.querySelector('a').style.display = 'none';
        }, 2000);
    </script>
</body>
</html>
)rawliteral";
    request->send(200, "text/html", html);
  });

  // Endpunkt für den Neustart
  server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request){
    if (!checkAuth(request)) return;
    request->send(200, "text/plain", "Neustart wird ausgeführt...");
    delay(500);
    ESP.restart();
  });

  server.on("/scan", HTTP_GET, [](AsyncWebServerRequest *request){
    if (!checkAuth(request)) return;
    int16_t n = WiFi.scanComplete();
    if(n == -2){
      // Scan hasn't been started yet
      WiFi.scanNetworks(true);
      request->send(202, "text/plain", "Scanning...");
    } else if(n == -1){
      // Scan in progress
      request->send(202, "text/plain", "Scanning...");
    } else {
      // Scan complete
      String json = "[";
      for (int i = 0; i < n; ++i) {
        if (i > 0) json += ",";
        json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
      }
      json += "]";
      WiFi.scanDelete(); // Clean up to allow a new scan later
      request->send(200, "application/json", json);
    }
  });

  server.onNotFound([](AsyncWebServerRequest *request){
    // Handle Captive Portal
    // When a device connects to the AP, it might try to check for internet connectivity.
    // This request (e.g., to "http://connectivitycheck.gstatic.com/generate_204") should be
    // redirected to our configuration page.
    if (request->host() != WiFi.softAPIP().toString()) {
        request->redirect("http://" + WiFi.softAPIP().toString());
    } else {
        request->send(404, "text/plain", "Not found");
    }
  });


  server.begin();
  if (WiFi.getMode() == WIFI_AP) {
    Serial.println("Webserver im AP-Modus gestartet. Verbinde dich mit dem 'Freundschaftslampe-Setup' WLAN.");
  } else {
    Serial.println("Webserver gestartet. Öffne http://" + WiFi.localIP().toString() + " im Browser, um die Lampe zu konfigurieren.");
  }
}

void setupMqtt() {
  if (config.mqttTls) {
    if (strlen(config.mqttCaCert) > 0) {
      espClientSecure.setCACert(config.mqttCaCert);
      Serial.println("MQTT-Client: Nutze manuelles CA-Zertifikat aus der Konfiguration.");
    } else {
      espClientSecure.setCACertBundle(arduino_esp_certificate_bundle_get_all);
      Serial.println("MQTT-Client: Nutze ESP32 Zertifikatsbundle (setCACertBundle).");
    }
    client.setClient(espClientSecure);
    Serial.println("MQTT-Client für TLS-Verbindung eingerichtet.");
  } else {
    client.setClient(espClient);
    Serial.println("MQTT-Client für unverschlüsselte Verbindung eingerichtet.");
  }
  client.setServer(config.mqttServer, config.mqttPort);
  client.setCallback(mqttCallback);
}

void reconnectMqtt() {
  Serial.print("Versuche, MQTT-Verbindung herzustellen...");
  String clientId = String(config.mqttClientId);
  if(clientId.length() == 0) clientId = "Freundschaftslampe-" + String(random(0xffff), HEX);
  
  bool success;
  // Prüfen, ob ein Benutzername für die Verbindung verwendet werden soll
  if (strlen(config.mqttUser) > 0) {
    Serial.printf("\n  -> Verbinde mit Benutzer: %s\n", config.mqttUser);
    success = client.connect(clientId.c_str(), config.mqttUser, config.mqttPassword);
  } else {
    success = client.connect(clientId.c_str());
  }

  if (success) {
    Serial.println("verbunden!");
    // Topics abonnieren
    client.subscribe(config.mqttTopic);
    client.subscribe("freundschaft/update/trigger");
    Serial.printf("Topics '%s' und 'freundschaft/update/trigger' abonniert.\n", config.mqttTopic);

    // Aktuelle Version beim Start melden
    String statusMsg = "V" + String(FW_VERSION) + " online (" + config.mqttClientId + ")";
    client.publish("freundschaft/update/status", statusMsg.c_str());
  } else {
    Serial.print("Fehler, rc=");
    Serial.print(client.state());
    Serial.println(" Erneuter Versuch beim nächsten Durchlauf.");
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // Payload nullterminieren für Serial Print und Fallbacks
  payload[length] = '\0'; 

  Serial.print("Nachricht empfangen auf Topic: ");
  Serial.print(topic);
  Serial.print(", Nachricht: ");
  Serial.println((char*)payload);

  if (strcmp(topic, config.mqttTopic) == 0) {
    // Prüfen, ob der Ruhemodus aktiv ist
    if (isQuietTime()) {
      Serial.println("Ruhezeit ist aktiv. Empfangene Farbe wird ignoriert.");
      return;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload, length);
    
    if (!error) {
      // Wir ignorieren die eigene client_id NICHT, damit der eigene Button triggert.
      const char* senderId = doc["client_id"] | "";
      const char* colorStr = doc["color"] | "";
      
      if (strlen(colorStr) > 0) {
        uint32_t parsedColor = hexToColor(colorStr);
        Serial.printf("Farbe empfangen (JSON): %s\n", colorStr);

        const char* effectStr = doc["effect"] | "fade";
        strlcpy(receivedEffect, effectStr, sizeof(receivedEffect));

        preReceivedState_isLampOn = isLampOn;
        preReceivedState_Color = currentColor;

        inReceivedColorMode = true;
        receivedColor = parsedColor;
        long duration = doc["duration"] | RECEIVED_COLOR_DURATION;
        receivedColorEndTime = millis() + duration;
        
        Serial.println("Starte Anzeige der empfangenen Farbe...");
        effectStep = 0;
        lastEffectTime = 0;
      }
    } else {
      // Fallback auf CSV 'R,G,B' Format
      char* rgbString = (char*)payload;
      char* r_str = strtok(rgbString, ",");
      char* g_str = strtok(NULL, ",");
      char* b_str = strtok(NULL, ",");

      if (r_str && g_str && b_str) {
        uint8_t r = atoi(r_str);
        uint8_t g = atoi(g_str);
        uint8_t b = atoi(b_str);

        Serial.printf("Farbe empfangen (Fallback): R=%d, G=%d, B=%d\n", r, g, b);
        preReceivedState_isLampOn = isLampOn;
        preReceivedState_Color = currentColor;

        inReceivedColorMode = true;
        receivedColor = pixels.Color(r, g, b);
        receivedColorEndTime = millis() + RECEIVED_COLOR_DURATION;
        strlcpy(receivedEffect, "fade", sizeof(receivedEffect));

        Serial.println("Starte Anzeige der empfangenen Farbe...");
        effectStep = 0;
        lastEffectTime = 0;
      }
    }
  } else if (strcmp(topic, "freundschaft/update/trigger") == 0) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload, length);
    if (!error) {
      const char* url = doc["url"] | "";
      const char* version = doc["version"] | "";
      
      if (strlen(url) > 0 && strlen(version) > 0) {
        if (strcmp(version, FW_VERSION) != 0) {
          Serial.printf("Update-Befehl empfangen: %s (Aktuell: %s)\n", version, FW_VERSION);
          performOtaUpdate(url, version);
        } else {
          Serial.println("Update ignoriert: Version ist bereits aktuell.");
          client.publish("freundschaft/update/status", "Already up to date");
        }
      }
    }
  }
}

void setAllPixels(uint32_t color) {
  // Extrahieren der R, G, B-Komponenten
  uint8_t r = (color >> 16) & 0xFF;
  uint8_t g = (color >> 8) & 0xFF;
  uint8_t b = color & 0xFF;

  // Skalieren der Farbkomponenten mit der aktuellen Helligkeit
  r = (uint8_t)((float)r * (currentBrightness / 255.0));
  g = (uint8_t)((float)g * (currentBrightness / 255.0));
  b = (uint8_t)((float)b * (currentBrightness / 255.0));

  // Erstellen der angepassten Farbe
  uint32_t adjustedColor = pixels.Color(r, g, b);

  for (int i = 0; i < NUM_PIXELS; i++) {
    pixels.setPixelColor(i, adjustedColor);
  }
  pixels.show();
}

void handleButtonPress() {
  if (buttonPressedFlag) {
    buttonPressedFlag = false; // Reset flag
    Serial.println("Taster wurde gedrückt! Sende Identitätsfarbe via MQTT...");

    JsonDocument doc;
    String clientId = String(config.mqttClientId);
    if(clientId.length() == 0) clientId = "Freundschaftslampe-" + String(random(0xffff), HEX);
    doc["client_id"] = clientId;
    
    char hexColor[10];
    sprintf(hexColor, "#%06X", config.identityColor);
    doc["color"] = hexColor;
    doc["effect"] = config.effect;
    doc["duration"] = config.duration;

    String payloadStr;    serializeJson(doc, payloadStr);
    
    client.publish(config.mqttTopic, payloadStr.c_str());
    Serial.printf("Identitätsfarbe auf Topic '%s' gesendet (JSON): %s\n", config.mqttTopic, payloadStr.c_str());
  }
}

void handlePotentiometer() {
  if (inReceivedColorMode) return; // Deaktiviert, wenn eine empfangene Farbe angezeigt wird

  if (isLampOn) {
    int rawValue = analogRead(POTENTIOMETER_PIN);
    static float smoothedValue = rawValue; // initialisiert sich nur beim ersten Durchlauf
    smoothedValue = (0.05 * rawValue) + (0.95 * smoothedValue); // 5% neuer Wert, 95% alter
    
    // Den 12-Bit-ADC-Wert (0-4095) auf den 16-Bit-Hue-Wert (0-65535) abbilden
    uint16_t hue = map((int)smoothedValue, 0, 4095, 0, 65535);
    
    // HSV in RGB umwandeln
    uint32_t newColor = pixels.gamma32(pixels.ColorHSV(hue));

    // Nur aktualisieren, wenn sich die Farbe merklich ändert, um Flackern zu vermeiden
    if (newColor != currentColor) {
      currentColor = newColor;
      setAllPixels(currentColor);
    }
  }
}

void renderEffect() {
  unsigned long now = millis();
  
  // Grundfarben extrahieren (0-255)
  uint8_t baseR = (receivedColor >> 16) & 0xFF;
  uint8_t baseG = (receivedColor >> 8) & 0xFF;
  uint8_t baseB = receivedColor & 0xFF;
  
  // Helligkeit berechnen (0.0 - 1.0)
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
        // Helligkeit auf den bereits korrigierten Regenbogen anwenden
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
    // default/fade: Statische Farbe mit Gamma und Helligkeit
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
  if (inReceivedColorMode) {
    if (millis() >= receivedColorEndTime) {
      inReceivedColorMode = false; // Timer-Modus beenden

      // Vorherigen Zustand wiederherstellen
      isLampOn = preReceivedState_isLampOn;
      currentColor = preReceivedState_Color;

      Serial.println("Anzeige der empfangenen Farbe beendet. Kehre zum vorherigen Zustand zurück.");

      if (isLampOn) {
        setAllPixels(currentColor); // Zur Potentiometer-Farbe zurückkehren
      } else {
        setAllPixels(pixels.Color(0, 0, 0)); // Lampe wieder ausschalten
      }
    } else {
      renderEffect();
    }
  }
}


void handleTouch() {
  if (inReceivedColorMode) return;

  int touchValue = touchRead(TOUCH_PIN);
  unsigned long currentTime = millis();

  if (touchValue < TOUCH_THRESHOLD) {
      // ========== PIN IS TOUCHED ==========
      if (touchState == IDLE) {
          // Touch just started
          touchState = TOUCH_DETECTED;
          touchStartTime = currentTime;
      }

      // Check if touch becomes a long touch
      if (touchState == TOUCH_DETECTED && (currentTime - touchStartTime) >= 1000) {
          touchState = LONG_TOUCH_ACTIVE;
          Serial.println("Langer Druck erkannt. Helligkeitsänderung startet.");
          // On first detection of long press, if lamp is off, turn it on.
          if (!isLampOn) {
              isLampOn = true;
              // Set brightness to a minimum value if it's 0 to make it visible
              if (currentBrightness == 0) currentBrightness = 10;
              setAllPixels(currentColor);
          }
      }

      // If in long touch, adjust brightness
      if (touchState == LONG_TOUCH_ACTIVE) {
          if (millis() - lastBrightnessAnimTime > BRIGHTNESS_ANIM_DELAY) {
              lastBrightnessAnimTime = millis();

              int nextBrightness = currentBrightness + brightnessDirection;

              if (nextBrightness <= 10) { // Set a minimum brightness to not turn it off
                  nextBrightness = 10;
                  brightnessDirection = 1; // Change direction to brighter
              } else if (nextBrightness >= 255) {
                  nextBrightness = 255;
                  brightnessDirection = -1; // Change direction to darker
              }
              currentBrightness = (uint8_t)nextBrightness;
              setAllPixels(currentColor); // Update pixels with new brightness
          }
      }

  } else {
      // ========== PIN IS RELEASED ==========
      if (touchState == TOUCH_DETECTED) {
          // This was a short touch
          isLampOn = !isLampOn;
          if (isLampOn) {
              Serial.println("Lampe eingeschaltet (kurzer Druck).");
              // if brightness was 0, set to a default value
              if (currentBrightness == 0) currentBrightness = 255;
              setAllPixels(currentColor);

          } else {
              Serial.println("Lampe ausgeschaltet (kurzer Druck).");
              setAllPixels(pixels.Color(0, 0, 0));
          }
      } else if (touchState == LONG_TOUCH_ACTIVE) {
          // Long touch was released, brightness is already set.
          Serial.println("Helligkeitsänderung beendet.");
      }
      
      // Reset state for next touch
      touchState = IDLE;
  }
}

bool isQuietTime() {
  if (!config.quietModeEnabled) return false;
  
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Fehler: Konnte aktuelle Zeit nicht abrufen.");
    return false; // Ohne korrekte Uhrzeit lassen wir die Nachrichten sicherheitshalber durch
  }
  
  int currentHour = timeinfo.tm_hour;
  
  if (config.quietHourStart < config.quietHourEnd) {
    // Zeitraum liegt am selben Tag (z.B. 10 bis 14 Uhr)
    return (currentHour >= config.quietHourStart && currentHour < config.quietHourEnd);
  } else {
    // Zeitraum geht über Mitternacht (z.B. 22 bis 6 Uhr)
    return (currentHour >= config.quietHourStart || currentHour < config.quietHourEnd);
  }
}

String trimString(String str) {
  str.trim();
  return str;
}

void loadConfiguration() {
  preferences.begin("f-lampe", true); // "f-lampe" = namespace, true = read-only
  preferences.getString("ssid", config.ssid, sizeof(config.ssid));
  preferences.getString("password", config.password, sizeof(config.password));
  preferences.getString("mqttServer", config.mqttServer, sizeof(config.mqttServer));
  config.mqttPort = preferences.getUShort("mqttPort", 1883);
  config.mqttTls = preferences.getBool("mqttTls", false);
  preferences.getString("mqttCaCert", config.mqttCaCert, sizeof(config.mqttCaCert));
  preferences.getString("mqttTopic", config.mqttTopic, sizeof(config.mqttTopic));
  if (strlen(config.mqttTopic) == 0) {
    strcpy(config.mqttTopic, "freundschaft/farbe");
  }
  preferences.getString("mqttUser", config.mqttUser, sizeof(config.mqttUser));
  preferences.getString("mqttPassword", config.mqttPassword, sizeof(config.mqttPassword));
  config.identityColor = preferences.getUInt("identityColor", 0x0000FF); // Standard: Blau
  config.quietModeEnabled = preferences.getBool("quietEnabled", false);
  config.quietHourStart = preferences.getUChar("quietStart", 22);
  config.quietHourEnd = preferences.getUChar("quietEnd", 6);
  preferences.getString("adminPassword", config.adminPassword, sizeof(config.adminPassword));
  if (strlen(config.adminPassword) == 0) {
      strcpy(config.adminPassword, "12345678");
  }
  preferences.getString("mqttClientId", config.mqttClientId, sizeof(config.mqttClientId));
  if (strlen(config.mqttClientId) == 0) {
      String generated = "Freundschaftslampe-" + String(random(0xffff), HEX);
      strcpy(config.mqttClientId, generated.c_str());
  }
  config.numPixels = preferences.getUShort("numPixels", NUM_PIXELS);
  preferences.getString("effect", config.effect, sizeof(config.effect));
  if (strlen(config.effect) == 0) {
    strcpy(config.effect, "fade");
  }
  config.duration = preferences.getUInt("duration", 10000);
  preferences.getString("otaCaCert", config.otaCaCert, sizeof(config.otaCaCert));
  preferences.end();

  Serial.println("Konfiguration geladen:");
  Serial.printf("  SSID: %s\n", config.ssid);
  Serial.printf("  MQTT Server: %s\n", config.mqttServer);
  Serial.printf("  MQTT Port: %d\n", config.mqttPort);
  Serial.printf("  MQTT TLS: %s\n", config.mqttTls ? "Ja" : "Nein");
  Serial.printf("  MQTT Topic: %s\n", config.mqttTopic);
  Serial.printf("  MQTT User: %s\n", config.mqttUser);
  Serial.printf("  Identity Color: #%06X\n", config.identityColor);
}

void saveConfiguration() {
  preferences.begin("f-lampe", false); // "f-lampe" = namespace, false = read/write
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
  preferences.putString("otaCaCert", config.otaCaCert);
  preferences.end();

  Serial.println("Konfiguration wurde gespeichert.");
}

void performOtaUpdate(const char* url, const char* version) {
  Serial.println("OTA Update Prozess gestartet...");
  Serial.printf("Update-URL: %s\n", url);
  
  // Status via MQTT melden
  String startMsg = "Updating from " + String(FW_VERSION) + " to " + String(version);
  client.publish("freundschaft/update/status", startMsg.c_str());
  client.loop(); 

  WiFiClientSecure otaClient;
  
  if (strlen(config.otaCaCert) > 0) {
    otaClient.setCACert(config.otaCaCert);
    Serial.println("OTA: Nutze manuelles Zertifikat aus der Konfiguration (Override).");
  } else {
    otaClient.setCACertBundle(arduino_esp_certificate_bundle_get_all);
    Serial.println("OTA: Nutze ESP32 Zertifikatsbundle (setCACertBundle).");
  }

  // Ring blau leuchten lassen während des Updates
  setAllPixels(pixels.Color(0, 0, 255));

  // Fortschritts-Handling
  httpUpdate.onProgress([](int cur, int total) {
    static int lastPercent = -1;
    int percent = (cur * 100) / total;
    if (percent % 10 == 0 && percent != lastPercent) {
      lastPercent = percent;
      char buf[32];
      sprintf(buf, "Download: %d%%", percent);
      Serial.println(buf);
    }
  });

  t_httpUpdate_return ret = httpUpdate.update(otaClient, url);

  switch (ret) {
    case HTTP_UPDATE_FAILED: {
      String errorMsg = "Update failed: " + httpUpdate.getLastErrorString();
      Serial.println(errorMsg);
      client.publish("freundschaft/update/status", errorMsg.c_str());
      setAllPixels(pixels.Color(255, 0, 0));
      delay(2000);
      setAllPixels(pixels.Color(0, 0, 0));
      break;
    }
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("Keine Updates verfügbar.");
      client.publish("freundschaft/update/status", "No updates available");
      break;
    case HTTP_UPDATE_OK:
      Serial.println("Update erfolgreich! ESP32 startet neu...");
      client.publish("freundschaft/update/status", "Success! Rebooting...");
      client.loop();
      delay(1000);
      break;
  }
}
