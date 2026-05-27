#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <vector>
#include "ConfigManager.h"
#include "LampController.h"

class OTAHandler;

// Geraete-Typ-Konstante fuer das v2-Schema (siehe fl/.../<type>-Topics).
#define DEVICE_TYPE_LAMP "lamp"

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
    // Status-Variante fuer das v2-Schema: schreibt JSON nach fl/device/<id>/status
    // mit type/fw/state/color/families. 'extra' kann z.B. ein Update-State sein.
    void publishStatusV2(Config& config, const char* state, const char* extra = nullptr);
    void publish(const char* topic, const char* payload, bool retained = false);

    // v2-Helpers: aus config.familyIds parsen und die Familien-Signal-Topics liefern.
    std::vector<String> getFamilies(Config& config);
    // Familien als JSON-Array-Fragment (z.B. "[\"schmidt\",\"lieblings\"]").
    String getFamiliesJsonArray(Config& config);

private:
    WiFiClient _espClient;
    WiFiClientSecure _espClientSecure;
    PubSubClient _client;
    LampController& _lamp;
    OTAHandler& _ota;
    // Liste der v2-Signal-Topics (eines pro Familie), zur Laufzeit gefuellt.
    std::vector<String> _familySignalTopics;
    Config* _config = nullptr;

    unsigned long _lastMqttReconnectAttempt = 0;
    const unsigned long RECONNECT_INTERVAL = 5000;
    const char* FW_VERSION = "V2.4.0"; // v2-Schema

    void reconnect(Config& config);
    void callback(char* topic, byte* payload, unsigned int length);
    bool isFamilySignalTopic(const char* topic);

    static MqttManager* _instance;
    static void staticCallback(char* topic, byte* payload, unsigned int length);
};

#endif
