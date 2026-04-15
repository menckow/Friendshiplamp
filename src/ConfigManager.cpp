#include "ConfigManager.h"

ConfigManager::ConfigManager() {
    // Standardwerte setzen
    memset(&_config, 0, sizeof(Config));
    _config.mqttPort = 1883;
    _config.identityColor = 0x0000FF;
    _config.quietHourStart = 22;
    _config.quietHourEnd = 6;
    _config.numPixels = 40;
    _config.duration = 10000;
    _config.touchThreshold = 40;
    strlcpy(_config.effect, "fade", sizeof(_config.effect));
}

void ConfigManager::begin() {
    load();
}

void ConfigManager::load() {
    _preferences.begin("f-lampe", true);
    _preferences.getString("ssid", _config.ssid, 32);
    _preferences.getString("password", _config.password, 64);
    _preferences.getString("mqttServer", _config.mqttServer, 64);
    _config.mqttPort = _preferences.getUShort("mqttPort", 1883);
    _config.mqttTls = _preferences.getBool("mqttTls", false);
    _config.useStandardCa = _preferences.getBool("useStandardCa", true);
    _preferences.getString("mqttCaCert", _config.mqttCaCert, 2048);
    _preferences.getString("mqttTopic", _config.mqttTopic, 64);
    _preferences.getString("mqttUser", _config.mqttUser, 32);
    _preferences.getString("mqttPassword", _config.mqttPassword, 64);
    _config.identityColor = _preferences.getUInt("identityColor", 0x0000FF);
    _config.quietModeEnabled = _preferences.getBool("quietEnabled", false);
    _config.quietHourStart = _preferences.getUChar("quietStart", 22);
    _config.quietHourEnd = _preferences.getUChar("quietEnd", 6);
    _preferences.getString("adminPassword", _config.adminPassword, 64);
    _preferences.getString("mqttClientId", _config.mqttClientId, 64);
    _config.numPixels = _preferences.getUShort("numPixels", 40);
    _preferences.getString("effect", _config.effect, 32);
    _config.duration = _preferences.getUInt("duration", 10000);
    _config.touchThreshold = _preferences.getUShort("touch_threshold", 40);
    _preferences.end();
}

void ConfigManager::save() {
    _preferences.begin("f-lampe", false);
    _preferences.putString("ssid", _config.ssid);
    _preferences.putString("password", _config.password);
    _preferences.putString("mqttServer", _config.mqttServer);
    _preferences.putUShort("mqttPort", _config.mqttPort);
    _preferences.putBool("mqttTls", _config.mqttTls);
    _preferences.putBool("useStandardCa", _config.useStandardCa);
    _preferences.putString("mqttCaCert", _config.mqttCaCert);
    _preferences.putString("mqttTopic", _config.mqttTopic);
    _preferences.putString("mqttUser", _config.mqttUser);
    _preferences.putString("mqttPassword", _config.mqttPassword);
    _preferences.putUInt("identityColor", _config.identityColor);
    _preferences.putBool("quietEnabled", _config.quietModeEnabled);
    _preferences.putUChar("quietStart", _config.quietHourStart);
    _preferences.putUChar("quietEnd", _config.quietHourEnd);
    _preferences.putString("adminPassword", _config.adminPassword);
    _preferences.putString("mqttClientId", _config.mqttClientId);
    _preferences.putUShort("numPixels", _config.numPixels);
    _preferences.putString("effect", _config.effect);
    _preferences.putUInt("duration", _config.duration);
    _preferences.putUShort("touch_threshold", _config.touchThreshold);
    _preferences.end();
}

Config& ConfigManager::getConfig() {
    return _config;
}
