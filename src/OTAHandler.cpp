#include "OTAHandler.h"
#include "MqttManager.h"
#include <Update.h>

OTAHandler::OTAHandler(LampController& lamp) : _lamp(lamp) {}

void OTAHandler::performUpdate(const char* url, const char* version, const char* md5, const char* currentVersion, Config& config) {
    Serial.println("OTA Update Prozess gestartet...");
    Serial.printf("Update-URL: %s\n", url);
    
    if (md5 == nullptr || strlen(md5) != 32) {
        String errorMsg = "Update failed: Invalid or missing MD5 hash (must be 32 characters)";
        Serial.println(errorMsg);
        sendStatusRobust(errorMsg.c_str(), config);
        _lamp.setAllPixels(0xFF0000, 100); // Rot bei Fehler
        delay(2000);
        _lamp.setAllPixels(0, 0);
        
        delay(3000);
        String resetMsg = "V" + String(currentVersion) + ":online";
        sendStatusRobust(resetMsg.c_str(), config);
        return;
    }
    
    // Status via MQTT melden
    String startMsg = "Updating to " + String(version);
    sendStatusRobust(startMsg.c_str(), config);

    WiFiClientSecure otaClient;
    otaClient.setInsecure();

    // Ring blau leuchten lassen während des Updates
    _lamp.setAllPixels(0x0000FF, 100);

    Serial.printf("Sicherheits-Check: MD5 Hash Validierung aktiviert (%s)\n", md5);
    Update.setMD5(md5);
    httpUpdate.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    httpUpdate.onProgress([this, &config, currentVersion](int cur, int total) {
        static int lastPercent = -1;
        int percent = (cur * 100) / total;
        if (percent % 10 == 0 && percent != lastPercent) {
            lastPercent = percent;
            Serial.printf("Download: %d%%\n", percent);
            
            String progMsg = "V" + String(currentVersion) + ":Updating (" + String(percent) + "%)";
            sendStatusRobust(progMsg.c_str(), config);
        }
    });

    t_httpUpdate_return ret = httpUpdate.update(otaClient, url);

    switch (ret) {
        case HTTP_UPDATE_FAILED: {
            String errorMsg = "Update failed: " + httpUpdate.getLastErrorString();
            Serial.println(errorMsg);
            sendStatusRobust(errorMsg.c_str(), config);
            _lamp.setAllPixels(0xFF0000, 100); // Rot bei Fehler
            delay(2000);
            _lamp.setAllPixels(0, 0);
            break;
        }
        case HTTP_UPDATE_NO_UPDATES: {
            String msg = "No updates available";
            Serial.println(msg);
            sendStatusRobust(msg.c_str(), config);
            break;
        }
        case HTTP_UPDATE_OK: {
            String msg = "Success! Rebooting...";
            Serial.println(msg);
            sendStatusRobust(msg.c_str(), config);
            delay(1000);
            ESP.restart();
            break;
        }
    }
}

void OTAHandler::sendStatusRobust(const char* msg, Config& config) {
    if (_mqtt) {
        Serial.printf("MQTT Status-Update: %s\n", msg);
        _mqtt->forceReconnect(config);
        
        String statusTopic = _mqtt->getStatusTopic(config);
        
        _mqtt->publish(statusTopic.c_str(), msg, true); // Retained = true
        _mqtt->publish("freundschaftslampe/update/status", msg, false);
        
        // Den MQTT-Loop mehrfach aufrufen, um das Senden zu forcieren
        // Länger warten (2 Sek), da SSL/TLS Zeit braucht
        for (int i = 0; i < 40; i++) {
            _mqtt->loop();
            delay(50);
        }
    }
}
