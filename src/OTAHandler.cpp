#include "OTAHandler.h"

OTAHandler::OTAHandler(LampController& lamp) : _lamp(lamp) {}

void OTAHandler::performUpdate(const char* url, const char* version, const char* currentVersion) {
    Serial.println("OTA Update Prozess gestartet...");
    Serial.printf("Update-URL: %s\n", url);
    
    WiFiClientSecure otaClient;
    otaClient.setInsecure();

    // Ring blau leuchten lassen während des Updates
    _lamp.setAllPixels(_lamp.getColor() == 0 ? 0x0000FF : 0x0000FF, 100);

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
        case HTTP_UPDATE_FAILED:
            Serial.printf("Update failed: %s\n", httpUpdate.getLastErrorString().c_str());
            _lamp.setAllPixels(0xFF0000, 100); // Rot bei Fehler
            delay(2000);
            _lamp.setAllPixels(0, 0);
            break;
        case HTTP_UPDATE_NO_UPDATES:
            Serial.println("Keine Updates verfügbar.");
            break;
        case HTTP_UPDATE_OK:
            Serial.println("Update erfolgreich! ESP32 startet neu...");
            delay(1000);
            ESP.restart();
            break;
    }
}
