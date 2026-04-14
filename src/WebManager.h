#ifndef WEB_MANAGER_H
#define WEB_MANAGER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "ConfigManager.h"

class WebManager {
public:
    WebManager(ConfigManager& configMgr);
    void begin();
    bool shouldReboot() const { return _shouldReboot; }

private:
    ConfigManager& _configMgr;
    AsyncWebServer _server;
    bool _shouldReboot = false;

    void setupRoutes();
    static String templateProcessor(const String& var);
    bool checkAuth(AsyncWebServerRequest *request);
    uint32_t hexToColor(const char* hexStr);
    
    static WebManager* _instance;
};

#endif
