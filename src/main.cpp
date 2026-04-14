#include <Arduino.h>
#include <ArduinoJson.h>
#include "ConfigManager.h"
#include "LampController.h"
#include "NetworkManager.h"
#include "MqttManager.h"
#include "InputHandler.h"
#include "OTAHandler.h"
#include "WebManager.h"

// == Globale Instanzen =====================================================
ConfigManager configMgr;
LampController lamp(40, 13); // Default values, will be updated in setup
NetworkManager network;
MqttManager mqtt(lamp);
InputHandler input(lamp);
OTAHandler ota(lamp);
WebManager web(configMgr);

// == Hauptprogramm-Logik ==================================================
void setup() {
    Serial.begin(115200);
    Serial.println("Freundschaftslampe startet...");

    // 1. Konfiguration laden
    configMgr.begin();
    Config& config = configMgr.getConfig();

    // 2. Hardware initialisieren
    lamp.begin();
    input.begin(config);

    // 3. Netzwerk starten
    network.begin(config);
    
    // 4. Dienste (MQTT, Webserver)
    mqtt.begin(config);
    web.begin();

    Serial.println("System bereit.");
}

void loop() {
    Config& config = configMgr.getConfig();

    // Regelmäßige Updates der Module
    network.update();
    mqtt.update(config);
    input.update(config);
    lamp.update(config);

    // Button-Logik (Nachrichten senden)
    if (input.isButtonPressed()) {
        JsonDocument doc;
        doc["client_id"] = config.mqttClientId;
        char hexColor[8];
        sprintf(hexColor, "#%06X", config.identityColor);
        doc["color"] = hexColor;
        doc["effect"] = config.effect;
        doc["duration"] = config.duration;
        
        String payload;
        serializeJson(doc, payload);
        mqtt.publish(config.mqttTopic, payload.c_str());
        Serial.println("Signal gesendet.");
    }

    // Neustart-Check
    if (web.shouldReboot()) {
        delay(1000);
        ESP.restart();
    }

    delay(10);
}
