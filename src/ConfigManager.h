#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>

struct Config {
  char ssid[32];
  char password[64];
  char mqttServer[64];
  uint16_t mqttPort;
  bool mqttTls;
  bool useStandardCa;
  char mqttCaCert[2048];
  char mqttTopic[64];        // [deprecated v1] altes Signal-Topic - bleibt fuer Preferences-Kompat erhalten
  char mqttUser[32];
  char mqttPassword[64];
  uint32_t identityColor;
  bool quietModeEnabled;
  uint8_t quietHourStart;
  uint8_t quietHourEnd;
  char adminPassword[64];
  char mqttClientId[64];
  uint16_t numPixels;
  char effect[32];
  uint32_t duration;
  uint16_t touchThreshold;
  // --- v2 -------------------------------------------------------------
  // Kommaseparierte Liste von Familien-IDs, z.B. "schmidt,lieblings".
  // Whitespace wird beim Parsen getrimmt.
  char familyIds[256];
};

class ConfigManager {
public:
    ConfigManager();
    void begin();
    void load();
    void save();
    Config& getConfig();

private:
    Config _config;
    Preferences _preferences;
};

#endif
