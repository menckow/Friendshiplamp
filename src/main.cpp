#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <PubSubClient.h>
#include <Preferences.h>
#include <DNSServer.h>

// == Globale Einstellungen =================================================
// -- Hardware-Pins --
#define POTENTIOMETER_PIN 34   // Pin für das Potentiometer (muss ein ADC-Pin sein) 1
#define BUTTON_PIN 4          // Pin für den Taster
#define NEOPIXEL_PIN 13        // Pin für den NeoPixel-Ring (geändert auf einen sichereren Pin)
#define TOUCH_PIN 32          // Kapazitiver Touch-Pin (GPIO9)
#define BRIGHTNESS_TOUCH_PIN 27 // Kapazitiver Touch-Pin für die Helligkeit (T7)

// -- NeoPixel-Einstellungen --
#define NUM_PIXELS 16         // Anzahl der LEDs im Ring

// -- Touch-Einstellungen --
#define TOUCH_THRESHOLD 40    // Schwellenwert für die Berührungserkennung
#define BRIGHTNESS_TOUCH_THRESHOLD 40 // Schwellenwert für die Helligkeitssteuerung
#define TOUCH_DEBOUNCE_DELAY 250 // Entprellzeit in ms
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
bool isLampOn = true;
uint32_t currentColor = pixels.Color(255, 255, 255);
uint8_t currentBrightness = 255;
unsigned long lastTouchTime = 0;
int buttonState;
int lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;

// -- Helligkeits-Animation --
int brightnessDirection = -1; // -1 für dunkler, 1 für heller
unsigned long lastBrightnessAnimTime = 0;

// == Timer-Zustand =========================================================
#define RECEIVED_COLOR_DURATION 10000 
bool inReceivedColorMode = false;
uint32_t receivedColor = 0;
unsigned long receivedColorEndTime = 0;
bool preReceivedState_isLampOn = false;
uint32_t preReceivedState_Color = 0;


// == Funktionsdeklarationen ===============================================
void initializeHardware();
void loadConfiguration();
void saveConfiguration();
void setupWifi();
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
uint32_t hexToColor(String hex);
void startNormalMode();
void startApMode();

// == Hauptprogramm =========================================================
void setup() {
  Serial.begin(115200);
  Serial.println("\nFreundschaftslampe startet...");

  initializeHardware(); // Hardware IMMER initialisieren

  loadConfiguration(); // Gespeicherte Einstellungen laden

  // Prüfen, ob eine SSID konfiguriert ist
  if (strlen(config.ssid) > 0 && strcmp(config.ssid, "DEIN_WLAN_SSID") != 0) {
    // SSID ist konfiguriert, starte im normalen Modus
    startNormalMode();
  } else {
    // Keine SSID konfiguriert, starte im Access-Point-Modus zur Einrichtung
    startApMode();
  }
}

void loop() {
  // Timer-Modus für empfangene Farben prüfen
  handleReceivedColorMode();

  // Nur im normalen Modus MQTT und Hardware-Interaktionen ausführen
  if (WiFi.getMode() == WIFI_STA) {
    // MQTT-Verbindung aufrechterhalten
    if (!client.connected()) {
      reconnectMqtt();
    }
    client.loop();

    // Hardware abfragen
    handleButtonPress();
    handlePotentiometer();
    handleTouch();
    handleBrightnessTouch();
  } else if (WiFi.getMode() == WIFI_AP) {
    // Im AP-Modus DNS-Anfragen für das Captive-Portal verarbeiten
    dnsServer.processNextRequest();
  }
  
  // Kurze Pause, um den ESP32 nicht zu überlasten
  delay(10); 
}

// == Funktionsdefinitionen =================================================

void initializeHardware() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pixels.begin();
  pixels.show(); // Pixel initial ausschalten
  Serial.println("Hardware initialisiert.");
}

void startNormalMode() {
  Serial.println("Starte im normalen Modus...");
  
  // Netzwerk-Dienste starten
  setupWifi();
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

  // AP-Modus starten
  WiFi.softAP(ap_ssid);
  
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
void setupWifi() {
  Serial.print("Verbinde mit WLAN: ");
  Serial.println(config.ssid);
  WiFi.begin(config.ssid, config.password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWLAN verbunden!");
  Serial.print("IP-Adresse: ");
  Serial.println(WiFi.localIP());
}

// Hilfsfunktion, um einen Hex-String (z.B. "#RRGGBB") in eine 32-Bit-Farbe umzuwandeln
uint32_t hexToColor(String hex) {
  hex.toUpperCase();
  if (hex.startsWith("#")) {
    hex = hex.substring(1);
  }
  return (uint32_t) strtoul(hex.c_str(), NULL, 16);
}

void setupWebServer() {
  // Konfigurationsseite unter der Haupt-URL "/"
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String html = "<html><head><title>Freundschaftslampe Konfiguration</title>";
    html += "<meta charset=\"UTF-8\">";
    html += "<style>";
    html += "body { font-family: sans-serif; background-color: #222; color: #eee; }";
    html += "div { margin: 0 auto; width: 80%; max-width: 600px; padding: 20px; }";
    html += "h1, h2 { text-align: center; }";
    html += "form { display: flex; flex-direction: column; }";
    html += "label { margin-top: 10px; }";
    html += "input { padding: 8px; margin-top: 5px; background-color: #333; color: #eee; border: 1px solid #555; border-radius: 4px; }";
    html += "textarea { padding: 8px; margin-top: 5px; background-color: #333; color: #eee; border: 1px solid #555; border-radius: 4px; font-family: monospace; }";
    html += "input[type='submit'] { background-color: #007bff; color: white; cursor: pointer; margin-top: 20px;}";
    html += "</style></head><body><div>";
    html += "<h1>Freundschaftslampe</h1><h2>Konfiguration</h2>";
    html += "<form action='/save' method='POST'>";
    
    html += "<h2>WLAN-Einstellungen</h2>";
    html += "<label for='ssid'>WLAN SSID:</label>";
    html += "<input type='text' id='ssid' name='ssid' value='" + String(config.ssid) + "'>";
    html += "<label for='password'>WLAN Passwort:</label>";
    html += "<input type='password' id='password' name='password' value='" + String(config.password) + "'>";
    
    html += "<h2>MQTT-Einstellungen</h2>";
    html += "<label for='mqtt'>MQTT Broker:</label>";
    html += "<input type='text' id='mqtt' name='mqtt' value='" + String(config.mqttServer) + "'>";
    html += "<label for='mqtt_port'>MQTT Port:</label>";
    html += "<input type='number' id='mqtt_port' name='mqtt_port' value='" + String(config.mqttPort) + "'>";
    html += "<label for='mqtt_tls'>MQTT mit TLS/SSL</label>";
    html += "<input type='checkbox' id='mqtt_tls' name='mqtt_tls' " + String(config.mqttTls ? "checked" : "") + ">";
    html += "<label for='mqtt_ca'>CA Zertifikat (optional):</label>";
    html += "<textarea id='mqtt_ca' name='mqtt_ca' rows='10'>" + String(config.mqttCaCert) + "</textarea>";
    html += "<label for='mqtt_topic'>MQTT Topic:</label>";
    html += "<input type='text' id='mqtt_topic' name='mqtt_topic' value='" + String(config.mqttTopic) + "'>";
    html += "<label for='mqtt_user'>MQTT Benutzername (optional):</label>";
    html += "<input type='text' id='mqtt_user' name='mqtt_user' value='" + String(config.mqttUser) + "'>";
    html += "<label for='mqtt_pass'>MQTT Passwort (optional):</label>";
    html += "<input type='password' id='mqtt_pass' name='mqtt_pass' value='" + String(config.mqttPassword) + "'>";

    html += "<h2>Lampen-Einstellungen</h2>";
    // Farbe in Hex umwandeln für den Color-Picker
    char hexColor[8];
    sprintf(hexColor, "#%06X", config.identityColor);
    html += "<label for='color'>Deine Identitätsfarbe:</label>";
    html += "<input type='color' id='color' name='color' value='" + String(hexColor) + "'>";

    html += "<input type='submit' value='Speichern & Neustarten'>";
    html += "</form>";
    html += "</div></body></html>";
    
    request->send(200, "text/html", html);
  });

  // Endpunkt zum Speichern der Konfiguration
  server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request) {
    // Parameter auslesen
    if (request->hasParam("ssid", true)) strcpy(config.ssid, request->getParam("ssid", true)->value().c_str());
    if (request->hasParam("password", true)) strcpy(config.password, request->getParam("password", true)->value().c_str());
    if (request->hasParam("mqtt", true)) strcpy(config.mqttServer, request->getParam("mqtt", true)->value().c_str());
    if (request->hasParam("mqtt_port", true)) config.mqttPort = request->getParam("mqtt_port", true)->value().toInt();
    config.mqttTls = request->hasParam("mqtt_tls", true);
    if (request->hasParam("mqtt_ca", true)) strcpy(config.mqttCaCert, request->getParam("mqtt_ca", true)->value().c_str());
    if (request->hasParam("mqtt_topic", true)) strcpy(config.mqttTopic, request->getParam("mqtt_topic", true)->value().c_str());
    if (request->hasParam("mqtt_user", true)) strcpy(config.mqttUser, request->getParam("mqtt_user", true)->value().c_str());
    if (request->hasParam("mqtt_pass", true)) strcpy(config.mqttPassword, request->getParam("mqtt_pass", true)->value().c_str());
    if (request->hasParam("color", true)) {
      config.identityColor = hexToColor(request->getParam("color", true)->value());
    }

    // Konfiguration speichern
    saveConfiguration();

    // Erfolgsseite mit Neustart-Button
    String html = "<html><head><title>Gespeichert!</title>";
    html += "<meta charset=\"UTF-8\">";
    html += "<style>";
    html += "body { font-family: sans-serif; background-color: #222; color: #eee; text-align: center; padding-top: 50px; }";
    html += "a { padding: 10px 20px; background-color: #28a745; color: white; text-decoration: none; border-radius: 5px; }";
    html += "</style></head><body>";
    html += "<h1>Konfiguration gespeichert!</h1>";
    html += "<p>Die Lampe muss neu gestartet werden, um die Änderungen zu übernehmen.</p>";
    html += "<a href='/reboot'>Jetzt Neustarten</a>";
    html += "</body></html>";
    request->send(200, "text/html", html);
  });

  // Endpunkt für den Neustart
  server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "Neustart wird ausgeführt...");
    delay(1000);
    ESP.restart();
  });

  server.begin();
  Serial.println("Webserver gestartet. Öffne http://" + WiFi.localIP().toString() + " im Browser, um die Lampe zu konfigurieren.");
}

void setupMqtt() {
  if (config.mqttTls) {
    if (strlen(config.mqttCaCert) > 0) {
      espClientSecure.setCACert(config.mqttCaCert);
      Serial.println("MQTT-Client: CA-Zertifikat wird verwendet.");
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
  // Loop bis die Verbindung wiederhergestellt ist
  while (!client.connected()) {
    Serial.print("Versuche, MQTT-Verbindung herzustellen...");
    // Eindeutige Client-ID erstellen
    String clientId = "Freundschaftslampe-";
    clientId += String(random(0xffff), HEX);
    
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
      // Topic für eingehende Farbnachrichten abonnieren
      client.subscribe(config.mqttTopic);
      Serial.printf("Topic '%s' abonniert.\n", config.mqttTopic);
    } else {
      Serial.print("Fehler, rc=");
      Serial.print(client.state());
      Serial.println(" Erneuter Versuch in 5 Sekunden");
      delay(5000);
    }
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // Eingehende Nachricht zur Verarbeitung in einen String umwandeln
  payload[length] = '\0'; // Null-Terminator hinzufügen
  String message = (char*)payload;

  Serial.print("Nachricht empfangen auf Topic: ");
  Serial.print(topic);
  Serial.print(", Nachricht: ");
  Serial.println(message);

  if (strcmp(topic, config.mqttTopic) == 0) {
    // RGB-Werte aus dem Payload extrahieren
    char* rgbString = (char*)payload;
    char* r_str = strtok(rgbString, ",");
    char* g_str = strtok(NULL, ",");
    char* b_str = strtok(NULL, ",");

    if (r_str && g_str && b_str) {
      uint8_t r = atoi(r_str);
      uint8_t g = atoi(g_str);
      uint8_t b = atoi(b_str);

      Serial.printf("Farbe empfangen: R=%d, G=%d, B=%d\n", r, g, b);

      // Aktuellen Zustand speichern, bevor er geändert wird
      preReceivedState_isLampOn = isLampOn;
      preReceivedState_Color = currentColor;

      // Timer-Modus aktivieren
      inReceivedColorMode = true;
      receivedColor = pixels.Color(r, g, b);
      receivedColorEndTime = millis() + RECEIVED_COLOR_DURATION;

      // Lampe mit der empfangenen Farbe einschalten
      Serial.println("Starte Anzeige der empfangenen Farbe...");
      setAllPixels(receivedColor);
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
  int reading = digitalRead(BUTTON_PIN);

  // Wenn sich der Zustand des Tasters geändert hat, starte den Entprell-Timer
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    // Wenn sich der Zustand nach der Entprellzeit nicht geändert hat
    if (reading != buttonState) {
      buttonState = reading;

      // Nur auf die fallende Flanke reagieren (Taster wurde gedrückt)
      if (buttonState == LOW) {
        Serial.println("Taster wurde gedrückt! Sende Identitätsfarbe via MQTT...");

        // Eigene Identitätsfarbe in R,G,B-Komponenten zerlegen
        uint8_t r = (config.identityColor >> 16) & 0xFF;
        uint8_t g = (config.identityColor >> 8) & 0xFF;
        uint8_t b = config.identityColor & 0xFF;

        // Payload als "R,G,B" String erstellen
        char payload[12];
        sprintf(payload, "%d,%d,%d", r, g, b);

        // Auf dem MQTT-Topic veröffentlichen
        client.publish(config.mqttTopic, payload);
        Serial.printf("Identitätsfarbe auf Topic '%s' gesendet: %s\n", config.mqttTopic, payload);
      }
    }
  }

  lastButtonState = reading;
}

void handlePotentiometer() {
  if (inReceivedColorMode) return; // Deaktiviert, wenn eine empfangene Farbe angezeigt wird

  if (isLampOn) {
    int potValue = analogRead(POTENTIOMETER_PIN);
    
    // Den 12-Bit-ADC-Wert (0-4095) auf den 16-Bit-Hue-Wert (0-65535) abbilden
    uint16_t hue = map(potValue, 0, 4095, 0, 65535);
    
    // HSV in RGB umwandeln
    uint32_t newColor = pixels.gamma32(pixels.ColorHSV(hue));

    // Nur aktualisieren, wenn sich die Farbe merklich ändert, um Flackern zu vermeiden
    if (newColor != currentColor) {
      currentColor = newColor;
      setAllPixels(currentColor);
    }
  }
}

void handleReceivedColorMode() {
  if (inReceivedColorMode && millis() >= receivedColorEndTime) {
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
  }
}


void handleTouch() {
  if (inReceivedColorMode) return; // Deaktiviert, wenn eine empfangene Farbe angezeigt wird

  int touchValue = touchRead(TOUCH_PIN);
  unsigned long currentTime = millis();

  if (touchValue < TOUCH_THRESHOLD && (currentTime - lastTouchTime) > TOUCH_DEBOUNCE_DELAY) {
    lastTouchTime = currentTime;
    isLampOn = !isLampOn; // Zustand umschalten

    if (isLampOn) {
      Serial.println("Lampe eingeschaltet.");
      setAllPixels(currentColor);
    } else {
      Serial.println("Lampe ausgeschaltet.");
      setAllPixels(pixels.Color(0, 0, 0)); // Ausschalten
    }
  }
}

void handleBrightnessTouch() {
  if (inReceivedColorMode || !isLampOn) return; // Nicht ausführen, wenn Farbe empfangen wird oder Lampe aus ist

  int touchValue = touchRead(BRIGHTNESS_TOUCH_PIN);

  if (touchValue < BRIGHTNESS_TOUCH_THRESHOLD) {
    // Pin wird berührt -> Animation ausführen
    if (millis() - lastBrightnessAnimTime > BRIGHTNESS_ANIM_DELAY) {
      lastBrightnessAnimTime = millis();

      // Helligkeit anpassen
      int nextBrightness = currentBrightness + brightnessDirection;

      // Grenzen prüfen und Richtung umkehren
      if (nextBrightness <= 0) {
        nextBrightness = 0;
        brightnessDirection = 1; // Richtung auf "heller" ändern
      } else if (nextBrightness >= 255) {
        nextBrightness = 255;
        brightnessDirection = -1; // Richtung auf "dunkler" ändern
      }

      currentBrightness = (uint8_t)nextBrightness;

      // Die Farbe mit der neuen Helligkeit sofort anzeigen
      setAllPixels(currentColor);
    }
  }
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
  preferences.end();

  Serial.println("Konfiguration wurde gespeichert.");
}
