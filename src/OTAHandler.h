#ifndef OTA_HANDLER_H
#define OTA_HANDLER_H

#include <Arduino.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include "LampController.h"
#include "ConfigManager.h"

class MqttManager;

class OTAHandler {
public:
    OTAHandler(LampController& lamp);
    void setMqttManager(MqttManager* mqtt) { _mqtt = mqtt; }
    void performUpdate(const char* url, const char* version, const char* md5, const char* currentVersion, Config& config);

private:
    void sendStatusRobust(const char* msg, Config& config);
    
    LampController& _lamp;
    MqttManager* _mqtt = nullptr;
};

#endif
