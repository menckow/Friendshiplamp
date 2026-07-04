#include "MqttManager.h"
#include <ArduinoJson.h>
#include "StandardCAs.h"
#include "OTAHandler.h"
#include <time.h>

MqttManager* MqttManager::_instance = nullptr;

MqttManager::MqttManager(LampController& lamp, OTAHandler& ota) : _client(_espClient), _lamp(lamp), _ota(ota) {
    _instance = this;
}

// --- v2 Helper -----------------------------------------------------------

std::vector<String> MqttManager::getFamilies(Config& config) {
    std::vector<String> result;
    String raw(config.familyIds);
    int start = 0;
    while (start <= (int)raw.length()) {
        int comma = raw.indexOf(',', start);
        String part = (comma < 0) ? raw.substring(start) : raw.substring(start, comma);
        part.trim();
        if (part.length() > 0) result.push_back(part);
        if (comma < 0) break;
        start = comma + 1;
    }
    return result;
}

String MqttManager::getFamiliesJsonArray(Config& config) {
    String out = "[";
    auto fams = getFamilies(config);
    for (size_t i = 0; i < fams.size(); i++) {
        if (i > 0) out += ",";
        out += "\"";
        out += fams[i];
        out += "\"";
    }
    out += "]";
    return out;
}

bool MqttManager::isFamilySignalTopic(const char* topic) {
    for (auto& t : _familySignalTopics) {
        if (strcmp(topic, t.c_str()) == 0) return true;
    }
    return false;
}

void MqttManager::begin(Config& config) {
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
    // Default PubSubClient-Buffer ist 256 Bytes (Topic+Payload+Header). Fuer das
    // v2-JSON-Status-Topic mit families-Array reichen wir das auf 1024 hoch.
    _client.setBufferSize(1024);
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
    return "fl/device/" + getClientId(config) + "/status";
}

void MqttManager::reconnect(Config& config) {
    String clientId = getClientId(config);
    String statusTopic = getStatusTopic(config);

    // LWT als JSON: state=offline. families mitgeben, damit das Dashboard die
    // Lampe beim Offline-Gang in derselben Familien-Gruppe anzeigen kann.
    char hexColor[10];
    sprintf(hexColor, "#%06X", config.identityColor);
    String lwt = String("{\"type\":\"") + DEVICE_TYPE_LAMP +
                 "\",\"fw\":\"" + FW_VERSION +
                 "\",\"state\":\"offline\"" +
                 ",\"color\":\"" + hexColor + "\"" +
                 ",\"families\":" + getFamiliesJsonArray(config) +
                 "}";

    bool connected = false;
    if (strlen(config.mqttUser) > 0) {
        connected = _client.connect(clientId.c_str(), config.mqttUser, config.mqttPassword, statusTopic.c_str(), 1, true, lwt.c_str());
    } else {
        connected = _client.connect(clientId.c_str(), statusTopic.c_str(), 1, true, lwt.c_str());
    }

    if (!connected) return;

    // --- v2 Subscribes ---------------------------------------------------
    // Familien-Signal- und Familien-OTA-Topics einsammeln, Bestandsliste ablegen.
    _familySignalTopics.clear();
    auto fams = getFamilies(config);
    for (auto& fam : fams) {
        String sig = "fl/family/" + fam + "/signal";
        String otaFam = "fl/family/" + fam + "/update/trigger/" + DEVICE_TYPE_LAMP;
        _client.subscribe(sig.c_str());
        _client.subscribe(otaFam.c_str());
        _familySignalTopics.push_back(sig);
    }
    // Per-Device-OTA (immer eindeutig)
    String otaDevice = "fl/device/" + clientId + "/update/trigger";
    _client.subscribe(otaDevice.c_str());
    // Globaler Notfall-Push fuer Lampen
    _client.subscribe("fl/_global/update/trigger/" DEVICE_TYPE_LAMP);

    // --- v2 Online-Status veroeffentlichen ------------------------------
    publishStatusV2(config, "online");

    // --- Migration-Hygiene: alten v1-Status-Topic einmalig leeren -------
    // (idempotent; nach Cutover bleibt das Dashboard sauber)
    String oldStatusTopic = "freundschaftslampe/status/" + clientId;
    _client.publish(oldStatusTopic.c_str(), "", true);

    Serial.println("MQTT verbunden (v2-Schema).");
    Serial.printf("  Familien: %u\n", (unsigned)fams.size());
    for (auto& fam : fams) Serial.println("    - " + fam);
}

void MqttManager::publishStatus(Config& config, const char* message) {
    // Legacy-Pfad: Erlaubt freie Statustexte, nutzt aber das v2-Topic.
    // Wir verpacken den Text als JSON-State, damit das Dashboard ihn parsen kann.
    publishStatusV2(config, message);
}

void MqttManager::publishStatusV2(Config& config, const char* state, const char* extra) {
    String clientId = getClientId(config);
    char hexColor[10];
    sprintf(hexColor, "#%06X", config.identityColor);

    String payload;
    payload.reserve(256);
    payload += "{\"type\":\"" DEVICE_TYPE_LAMP "\",";
    payload += "\"fw\":\""; payload += FW_VERSION; payload += "\",";
    payload += "\"state\":\""; payload += state; payload += "\",";
    payload += "\"color\":\""; payload += hexColor; payload += "\",";
    payload += "\"families\":"; payload += getFamiliesJsonArray(config);
    if (extra && *extra) {
        payload += ",\"info\":\""; payload += extra; payload += "\"";
    }
    payload += "}";

    String topic = getStatusTopic(config);
    _client.publish(topic.c_str(), payload.c_str(), true);
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

    if (_instance == nullptr || _instance->_config == nullptr) return;
    Config& cfg = *(_instance->_config);

    // --- Familien-Signal-Topic? --------------------------------------------
    if (_instance->isFamilySignalTopic(topic)) {
        // Ruhemodus
        if (cfg.quietModeEnabled) {
            time_t now = time(nullptr);
            struct tm* timeinfo = localtime(&now);
            uint8_t currentHour = timeinfo->tm_hour;
            uint8_t start = cfg.quietHourStart;
            uint8_t end = cfg.quietHourEnd;
            bool inQuietTime = (start < end)
                ? (currentHour >= start && currentHour < end)
                : (currentHour >= start || currentHour < end);
            if (inQuietTime) {
                Serial.printf("Signal ignoriert: Ruhemodus aktiv (%02d:00-%02d:00, jetzt %02d:00)\n", start, end, currentHour);
                return;
            }
        }

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, message);
        if (error) return;

        // NTP-Schutz
        time_t now = time(nullptr);
        const time_t NTP_VALID_AFTER = 1700000000;
        if (now < NTP_VALID_AFTER) {
            Serial.println("Signal ignoriert: Zeit noch nicht via NTP synchronisiert");
            return;
        }
        if (doc["ts"].is<time_t>()) {
            time_t msgTs = doc["ts"];
            if (msgTs > 0) {
                long age = (long)now - (long)msgTs;
                if (abs(age) > 60) {
                    Serial.printf("Signal ignoriert: Veraltet (Alter: %lds)\n", age);
                    return;
                }
            }
        }

        // Self-Filter (wichtig bei Mehrfachmitgliedschaft)
        if (doc["client_id"].is<const char*>()) {
            const char* senderId = doc["client_id"];
            String myId = _instance->getClientId(cfg);
            if (myId == senderId) {
                Serial.println("Signal ignoriert: eigener Druck");
                return;
            }
        }

        // Lampen reagieren nur auf Signale von anderen Lampen, nicht auf
        // PIR-Trigger einer Box. Wir filtern per Praefix "box", damit alle
        // Box-Varianten (box, box-a1s, ...) gemeinsam ausgeschlossen sind.
        // Empty sender_type (= aeltere v2-Sender ohne Typkennung) wird
        // zugelassen, damit nichts unbemerkt verloren geht.
        if (doc["sender_type"].is<const char*>()) {
            String senderType = String((const char*)doc["sender_type"]);
            if (senderType.startsWith("box")) {
                Serial.println("Signal ignoriert: Sender ist eine Box (" + senderType + ")");
                return;
            }
        }

        if (doc["color"].is<const char*>()) {
            const char* colorHex = doc["color"];
            uint32_t color = (uint32_t) strtoul(colorHex + (colorHex[0] == '#' ? 1 : 0), NULL, 16);
            const char* effect = doc["effect"] | "fade";
            uint32_t duration = doc["duration"] | 10000;
            _instance->_lamp.startReceivedColorMode(color, effect, duration);
        }
        return;
    }

    // --- OTA-Topics --------------------------------------------------------
    String myId = _instance->getClientId(cfg);
    String perDeviceOta = "fl/device/" + myId + "/update/trigger";
    String globalOta    = String("fl/_global/update/trigger/") + DEVICE_TYPE_LAMP;

    bool isPerDeviceOta = (strcmp(topic, perDeviceOta.c_str()) == 0);
    bool isGlobalOta    = (strcmp(topic, globalOta.c_str()) == 0);

    // Familien-OTA-Topic? (fl/family/<f>/update/trigger/lamp)
    bool isFamilyOta = false;
    {
        String prefix = "fl/family/";
        String suffix = String("/update/trigger/") + DEVICE_TYPE_LAMP;
        String t(topic);
        if (t.startsWith(prefix) && t.endsWith(suffix)) {
            isFamilyOta = true;
        }
    }

    if (!isPerDeviceOta && !isGlobalOta && !isFamilyOta) {
        return; // unbekanntes Topic, ignorieren
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, message);
    if (error) {
        Serial.println("OTA: payload nicht parsebar");
        return;
    }

    // Defense-in-depth: optionaler target_type-Check
    if (doc["target_type"].is<const char*>()) {
        const char* tt = doc["target_type"];
        if (strcmp(tt, DEVICE_TYPE_LAMP) != 0) {
            Serial.printf("OTA: ignoriert (target_type=%s passt nicht zu %s)\n", tt, DEVICE_TYPE_LAMP);
            return;
        }
    }

    const char* url = doc["url"] | "";
    const char* version = doc["version"] | "";
    const char* md5 = doc["md5"] | "";
    if (strlen(url) > 0 && strcmp(version, _instance->FW_VERSION) != 0) {
        _instance->_ota.performUpdate(url, version, md5, _instance->FW_VERSION, cfg);
    } else if (strcmp(version, _instance->FW_VERSION) == 0) {
        Serial.println("OTA: bereits aktuell");
    }
}
