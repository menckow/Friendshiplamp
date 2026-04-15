#include "OTAHandler.h"
#include "MqttManager.h"

OTAHandler::OTAHandler(LampController& lamp) : _lamp(lamp) {}

void OTAHandler::performUpdate(const char* url, const char* version, const char* currentVersion, Config& config) {
    Serial.println("OTA Update Prozess gestartet...");
    Serial.printf("Update-URL: %s\n", url);
    
    // Status via MQTT melden
    if (_mqtt) {
        String clientId = String(config.mqttClientId);
        String statusTopic = "freundschaftslampe/status/" + clientId;
        String startMsg = "Updating to " + String(version);
        
        _mqtt->publish(statusTopic.c_str(), (String(currentVersion) + ":" + startMsg).c_str());
        _mqtt->publish("freundschaftslampe/update/status", startMsg.c_str());
    }

    WiFiClientSecure otaClient;
    otaClient.setInsecure();

    // Ring blau leuchten lassen während des Updates
    _lamp.setAllPixels(0x0000FF, 100);

    httpUpdate.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
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
            if (_mqtt) {
                String clientId = String(config.mqttClientId);
                String statusTopic = "freundschaftslampe/status/" + clientId;
                _mqtt->publish(statusTopic.c_str(), errorMsg.c_str());
                _mqtt->publish("freundschaftslampe/update/status", errorMsg.c_str());
                _mqtt->loop();
            }
            _lamp.setAllPixels(0xFF0000, 100); // Rot bei Fehler
            delay(2000);
            _lamp.setAllPixels(0, 0);
            break;
        }
        case HTTP_UPDATE_NO_UPDATES: {
            String msg = "No updates available";
            Serial.println(msg);
            if (_mqtt) {
                String clientId = String(config.mqttClientId);
                String statusTopic = "freundschaftslampe/status/" + clientId;
                _mqtt->publish(statusTopic.c_str(), msg.c_str());
                _mqtt->publish("freundschaftslampe/update/status", msg.c_str());
                _mqtt->loop();
            }
            break;
        }
        case HTTP_UPDATE_OK: {
            String msg = "Success! Rebooting...";
            Serial.println(msg);
            if (_mqtt) {
                String clientId = String(config.mqttClientId);
                String statusTopic = "freundschaftslampe/status/" + clientId;
                _mqtt->publish(statusTopic.c_str(), msg.c_str());
                _mqtt->publish("freundschaftslampe/update/status", msg.c_str());
                _mqtt->loop();
            }
            delay(1000);
            ESP.restart();
            break;
        }
    }
}
