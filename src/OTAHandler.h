#ifndef OTA_HANDLER_H
#define OTA_HANDLER_H

#include <Arduino.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include "LampController.h"

class OTAHandler {
public:
    OTAHandler(LampController& lamp);
    void performUpdate(const char* url, const char* version, const char* currentVersion);

private:
    LampController& _lamp;
};

#endif
