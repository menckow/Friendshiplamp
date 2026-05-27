#include <Arduino.h>
#include <ArduinoJson.h>
#include "ConfigManager.h"
#include "LampController.h"
#include "NetworkManager.h"
#include "MqttManager.h"
#include "InputHandler.h"
#include "OTAHandler.h"
#include "WebManager.h"
#include <time.h>

// == Globale Instanzen =====================================================
ConfigManager configMgr;
LampController lamp(40, 13); // Default values, will be updated in setup
NetworkManager network;
OTAHandler ota(lamp);
MqttManager mqtt(lamp, ota);
InputHandler input(lamp);
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
    ota.setMqttManager(&mqtt);
    web.setLamp(&lamp);
    web.setMqtt(&mqtt);
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

    // Button-Logik (Nachrichten senden) — v2-Schema: an alle Familien-Signale fanned-out.
    if (input.isButtonPressed()) {
        JsonDocument doc;
        doc["client_id"] = mqtt.getClientId(config);
        doc["sender_type"] = "lamp";
        char hexColor[8];
        sprintf(hexColor, "#%06X", config.identityColor);
        doc["color"] = hexColor;
        doc["effect"] = config.effect;
        doc["duration"] = config.duration;
        doc["ts"] = time(nullptr);

        String payload;
        serializeJson(doc, payload);

        auto families = mqtt.getFamilies(config);
        if (families.empty()) {
            Serial.println("Signal NICHT gesendet: keine Familien konfiguriert (Webkonfig pruefen).");
        } else {
            for (auto& fam : families) {
                String topic = "fl/family/" + fam + "/signal";
                mqtt.publish(topic.c_str(), payload.c_str());
                Serial.println("Signal gesendet -> " + topic);
            }
        }

        // Lokales LED-Feedback: bestaetigt den Druck visuell, auch wenn keine
        // Familie konfiguriert ist oder MQTT gerade nicht verbunden.
        lamp.startReceivedColorMode(config.identityColor, config.effect, config.duration);
    }

    // Neustart-Check
    if (web.shouldReboot()) {
        delay(1000);
        ESP.restart();
    }

    delay(10);
}
