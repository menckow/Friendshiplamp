#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include "ConfigManager.h"

class NetworkManager {
public:
    NetworkManager();
    void begin(Config& config);
    void update();
    bool setupWifi(Config& config);
    void startApMode();
    bool isConnected() const;

private:
    DNSServer _dnsServer;
    unsigned long _lastWifiReconnectAttempt = 0;
    const unsigned long RECONNECT_INTERVAL = 5000;
};

#endif
