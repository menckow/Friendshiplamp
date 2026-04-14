#include "NetworkManager.h"
#include <time.h>

NetworkManager::NetworkManager() {}

void NetworkManager::begin(Config& config) {
    if (strlen(config.ssid) > 0 && strcmp(config.ssid, "DEIN_WLAN_SSID") != 0) {
        if (!setupWifi(config)) {
            startApMode();
        }
    } else {
        startApMode();
    }
}

void NetworkManager::update() {
    WiFiMode_t mode = WiFi.getMode();
    
    if (mode == WIFI_STA || mode == WIFI_AP_STA) {
        if (WiFi.status() != WL_CONNECTED && mode == WIFI_STA) {
            if (millis() - _lastWifiReconnectAttempt > RECONNECT_INTERVAL) {
                _lastWifiReconnectAttempt = millis();
                WiFi.reconnect();
            }
        }
    }
    
    if (mode == WIFI_AP || mode == WIFI_AP_STA) {
        _dnsServer.processNextRequest();
    }
}

bool NetworkManager::setupWifi(Config& config) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(config.ssid, config.password);
    
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
        delay(500);
        Serial.print(".");
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWLAN verbunden.");
        configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", "pool.ntp.org");
        return true;
    }
    
    Serial.println("\nWLAN Verbindung fehlgeschlagen.");
    return false;
}

void NetworkManager::startApMode() {
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP("Freundschaftslampe-Setup", "12345678");
    _dnsServer.start(53, "*", WiFi.softAPIP());
    Serial.println("AP-Modus gestartet: Freundschaftslampe-Setup");
}

bool NetworkManager::isConnected() const {
    return WiFi.status() == WL_CONNECTED;
}
