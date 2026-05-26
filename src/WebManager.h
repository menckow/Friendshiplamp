#ifndef WEB_MANAGER_H
#define WEB_MANAGER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "ConfigManager.h"

class LampController;
class MqttManager;

class WebManager {
public:
    WebManager(ConfigManager& configMgr);
    void begin();
    bool shouldReboot() const { return _shouldReboot; }

    // Optionale Anbindung an Hardware/Status fuer Web-Upload-OTA.
    // Beide sind nicht zwingend; ohne sie laeuft das Update ohne LED-/MQTT-Status.
    void setLamp(LampController* lamp) { _lamp = lamp; }
    void setMqtt(MqttManager* mqtt) { _mqtt = mqtt; }

private:
    ConfigManager& _configMgr;
    AsyncWebServer _server;
    bool _shouldReboot = false;

    // Web-Upload-OTA State
    LampController* _lamp = nullptr;
    MqttManager* _mqtt = nullptr;
    bool _updateInProgress = false;
    bool _updateFinishedOk = false;
    bool _updateAuthOk = false;
    String _updateError;

    void setupRoutes();
    static String templateProcessor(const String& var);
    bool checkAuth(AsyncWebServerRequest *request);
    bool authenticateUpload(AsyncWebServerRequest *request);
    uint32_t hexToColor(const char* hexStr);

    static WebManager* _instance;
};

#endif
