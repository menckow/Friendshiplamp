#include "MqttManager.h"
#include <ArduinoJson.h>
#include "StandardCAs.h"
#include "OTAHandler.h"

MqttManager* MqttManager::_instance = nullptr;

MqttManager::MqttManager(LampController& lamp, OTAHandler& ota) : _client(_espClient), _lamp(lamp), _ota(ota) {
    _instance = this;
    memset(_mqttTopic, 0, sizeof(_mqttTopic));
}

void MqttManager::begin(Config& config) {
    strlcpy(_mqttTopic, config.mqttTopic, sizeof(_mqttTopic));
    if (config.mqttTls) {
        if (config.useStandardCa) {
            _espClientSecure.setCACert(ISRG_ROOT_X1);
        } else if (strlen(config.mqttCaCert) > 0) {
            _espClientSecure.setCACert(config.mqttCaCert);
        } else {
            _espClientSecure.setInsecure();
        }
        _client.setClient(_espClientSecure);
    } else {
        _client.setClient(_espClient);
    }
    
    _client.setServer(config.mqttServer, config.mqttPort);
    _client.setCallback(MqttManager::staticCallback);
}

void MqttManager::update(Config& config) {
    _config = &config;
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

void MqttManager::loop() {
    _client.loop();
}

void MqttManager::forceReconnect(Config& config) {
    if (!_client.connected()) {
        reconnect(config);
    }
}

String MqttManager::getClientId(Config& config) {
    if (strlen(config.mqttClientId) > 0) return String(config.mqttClientId);
    uint64_t mac = ESP.getEfuseMac();
    return "Freundschaftslampe-" + String((uint32_t)(mac >> 32), HEX) + String((uint32_t)mac, HEX);
}

String MqttManager::getStatusTopic(Config& config) {
    return "freundschaftslampe/status/" + getClientId(config);
}

void MqttManager::reconnect(Config& config) {
    String clientId = getClientId(config);
    String statusTopic = getStatusTopic(config);
    
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
    publish(getStatusTopic(config).c_str(), message, true);
}

void MqttManager::publish(const char* topic, const char* payload, bool retained) {
    _client.publish(topic, payload, retained);
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
    
    if (strcmp(topic, _instance->_mqttTopic) == 0) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, message);
        if (!error) {
            if (doc["color"].is<const char*>()) {
                const char* colorHex = doc["color"];
                uint32_t color = (uint32_t) strtoul(colorHex + (colorHex[0] == '#' ? 1 : 0), NULL, 16);
                const char* effect = doc["effect"] | "fade";
                uint32_t duration = doc["duration"] | 10000;
                _instance->_lamp.startReceivedColorMode(color, effect, duration);
            }
        }
    } else if (strcmp(topic, "freundschaftslampe/update/trigger") == 0) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, message);
        if (!error) {
            const char* url = doc["url"] | "";
            const char* version = doc["version"] | "";
            const char* md5 = doc["md5"] | "";
            if (strlen(url) > 0 && strcmp(version, _instance->FW_VERSION) != 0 && _instance->_config != nullptr) {
                _instance->_ota.performUpdate(url, version, md5, _instance->FW_VERSION, *(_instance->_config));
            }
        }
    }
}
