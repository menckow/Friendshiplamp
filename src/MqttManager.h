#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include "ConfigManager.h"
#include "LampController.h"

class OTAHandler;

class MqttManager {
public:
    MqttManager(LampController& lamp, OTAHandler& ota);
    void begin(Config& config);
    void update(Config& config);
    void loop();
    void forceReconnect(Config& config);
    String getClientId(Config& config);
    String getStatusTopic(Config& config);
    void publishStatus(Config& config, const char* message);
    void publish(const char* topic, const char* payload, bool retained = false);

private:
    WiFiClient _espClient;
    WiFiClientSecure _espClientSecure;
    PubSubClient _client;
    LampController& _lamp;
    OTAHandler& _ota;
    char _mqttTopic[64];
    Config* _config = nullptr;
    
    unsigned long _lastMqttReconnectAttempt = 0;
    const unsigned long RECONNECT_INTERVAL = 5000;
    const char* FW_VERSION = "V2.2.1"; // Should be synced with main.cpp or a version header

    void reconnect(Config& config);
    void callback(char* topic, byte* payload, unsigned int length);
    
    static MqttManager* _instance;
    static void staticCallback(char* topic, byte* payload, unsigned int length);
};

#endif
