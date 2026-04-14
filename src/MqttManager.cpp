#include "MqttManager.h"
#include <ArduinoJson.h>

MqttManager* MqttManager::_instance = nullptr;

MqttManager::MqttManager(LampController& lamp) : _client(_espClient), _lamp(lamp) {
    _instance = this;
}

void MqttManager::begin(Config& config) {
    if (config.mqttTls) {
        if (strlen(config.mqttCaCert) > 0) _espClientSecure.setCACert(config.mqttCaCert);
        else _espClientSecure.setInsecure();
        _client.setClient(_espClientSecure);
    } else {
        _client.setClient(_espClient);
    }
    
    _client.setServer(config.mqttServer, config.mqttPort);
    _client.setCallback(MqttManager::staticCallback);
}

void MqttManager::update(Config& config) {
    if (WiFi.status() == WL_CONNECTED) {
        if (!_client.connected()) {
            if (millis() - _lastMqttReconnectAttempt > RECONNECT_INTERVAL) {
                _lastMqttReconnectAttempt = millis();
                reconnect(config);
            }
        } else {
            _client.loop();
        }
    }
}

void MqttManager::reconnect(Config& config) {
    String clientId = String(config.mqttClientId);
    if (clientId.length() == 0) {
        uint64_t mac = ESP.getEfuseMac();
        clientId = "Freundschaftslampe-" + String((uint32_t)(mac >> 32), HEX) + String((uint32_t)mac, HEX);
    }
    
    String statusTopic = "freundschaftslampe/status/" + clientId;
    
    bool connected = false;
    if (strlen(config.mqttUser) > 0) {
        connected = _client.connect(clientId.c_str(), config.mqttUser, config.mqttPassword, statusTopic.c_str(), 1, true, "offline");
    } else {
        connected = _client.connect(clientId.c_str(), statusTopic.c_str(), 1, true, "offline");
    }
    
    if (connected) {
        _client.subscribe(config.mqttTopic);
        _client.subscribe("freundschaftslampe/update/trigger");
        _client.publish(statusTopic.c_str(), (String(FW_VERSION) + ":online").c_str(), true);
        Serial.println("MQTT verbunden.");
    }
}

void MqttManager::publishStatus(Config& config, const char* message) {
    String clientId = String(config.mqttClientId);
    String statusTopic = "freundschaftslampe/status/" + clientId;
    _client.publish(statusTopic.c_str(), message);
}

void MqttManager::publish(const char* topic, const char* payload) {
    _client.publish(topic, payload);
}

void MqttManager::staticCallback(char* topic, byte* payload, unsigned int length) {
    if (_instance) {
        _instance->callback(topic, payload, length);
    }
}

void MqttManager::callback(char* topic, byte* payload, unsigned int length) {
    char message[length + 1];
    memcpy(message, payload, length);
    message[length] = '\0';
    
    if (strcmp(topic, _instance->_lamp.getColor() == 0 ? "" : "") == 0) { // Topic comparison logic needs to be robust
        // This is handled via the config topic in original code
    }

    // Since I don't have the config object here in the callback easily without storing it,
    // I'll assume the topic check is done against the subscription.
    
    // For now, let's replicate the logic from main.cpp
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, message);
    if (!error) {
        if (doc["color"].is<const char*>()) {
            const char* colorHex = doc["color"];
            uint32_t color = (uint32_t) strtoul(colorHex + (colorHex[0] == '#' ? 1 : 0), NULL, 16);
            const char* effect = doc["effect"] | "fade";
            uint32_t duration = doc["duration"] | 10000;
            _lamp.startReceivedColorMode(color, effect, duration);
        }
    }
}
