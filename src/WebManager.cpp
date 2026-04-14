#include "WebManager.h"
#include "WebTemplates.h"
#include <WiFi.h>

WebManager* WebManager::_instance = nullptr;

WebManager::WebManager(ConfigManager& configMgr) : _configMgr(configMgr), _server(80) {
    _instance = this;
}

void WebManager::begin() {
    setupRoutes();
    _server.begin();
}

void WebManager::setupRoutes() {
    _server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request){
        if (!checkAuth(request)) return;
        request->send_P(200, "text/html", INDEX_HTML, templateProcessor);
    });

    _server.on("/save", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if (!checkAuth(request)) return;
        Config& config = _configMgr.getConfig();
        
        if (request->hasParam("admin_pass", true)) strlcpy(config.adminPassword, request->getParam("admin_pass", true)->value().c_str(), sizeof(config.adminPassword));
        if (request->hasParam("mqtt_client_id", true)) strlcpy(config.mqttClientId, request->getParam("mqtt_client_id", true)->value().c_str(), sizeof(config.mqttClientId));
        if (request->hasParam("ssid", true)) strlcpy(config.ssid, request->getParam("ssid", true)->value().c_str(), sizeof(config.ssid));
        if (request->hasParam("password", true)) strlcpy(config.password, request->getParam("password", true)->value().c_str(), sizeof(config.password));
        if (request->hasParam("mqtt", true)) strlcpy(config.mqttServer, request->getParam("mqtt", true)->value().c_str(), sizeof(config.mqttServer));
        if (request->hasParam("mqtt_port", true)) config.mqttPort = request->getParam("mqtt_port", true)->value().toInt();
        config.mqttTls = request->hasParam("mqtt_tls", true);
        if (request->hasParam("mqtt_ca", true)) strlcpy(config.mqttCaCert, request->getParam("mqtt_ca", true)->value().c_str(), sizeof(config.mqttCaCert));
        if (request->hasParam("mqtt_topic", true)) strlcpy(config.mqttTopic, request->getParam("mqtt_topic", true)->value().c_str(), sizeof(config.mqttTopic));
        if (request->hasParam("mqtt_user", true)) strlcpy(config.mqttUser, request->getParam("mqtt_user", true)->value().c_str(), sizeof(config.mqttUser));
        if (request->hasParam("mqtt_pass", true)) strlcpy(config.mqttPassword, request->getParam("mqtt_pass", true)->value().c_str(), sizeof(config.mqttPassword));
        if (request->hasParam("color", true)) config.identityColor = hexToColor(request->getParam("color", true)->value().c_str());
        if (request->hasParam("effect", true)) strlcpy(config.effect, request->getParam("effect", true)->value().c_str(), sizeof(config.effect));
        if (request->hasParam("duration", true)) config.duration = request->getParam("duration", true)->value().toInt() * 1000;
        config.quietModeEnabled = request->hasParam("quiet_enabled", true);
        if (request->hasParam("quiet_start", true)) config.quietHourStart = request->getParam("quiet_start", true)->value().toInt();
        if (request->hasParam("quiet_end", true)) config.quietHourEnd = request->getParam("quiet_end", true)->value().toInt();
        if (request->hasParam("num_pixels", true)) config.numPixels = request->getParam("num_pixels", true)->value().toInt();
        if (request->hasParam("touch_threshold", true)) config.touchThreshold = request->getParam("touch_threshold", true)->value().toInt();
        
        _configMgr.save();
        request->send_P(200, "text/html", SUCCESS_HTML);
    });

    _server.on("/reboot", HTTP_GET, [this](AsyncWebServerRequest *request){
        if (!checkAuth(request)) return;
        request->send(200, "text/plain", "Rebooting...");
        _shouldReboot = true;
    });

    _server.on("/scan", HTTP_GET, [this](AsyncWebServerRequest *request){
        if (!checkAuth(request)) return;
        int16_t n = WiFi.scanComplete();
        if(n == -2){ WiFi.scanNetworks(true); request->send(202); }
        else if(n == -1) request->send(202);
        else {
            String json = "[";
            for (int i = 0; i < n; i++) {
                if (i > 0) json += ",";
                json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
            }
            json += "]";
            WiFi.scanDelete(); 
            request->send(200, "application/json", json);
        }
    });

    _server.onNotFound([](AsyncWebServerRequest *request){
        if (request->host() != WiFi.softAPIP().toString()) request->redirect("http://" + WiFi.softAPIP().toString());
        else request->send(404);
    });
}

bool WebManager::checkAuth(AsyncWebServerRequest *request) {
    if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) return true;
    Config& config = _configMgr.getConfig();
    if (strlen(config.adminPassword) > 0 && strcmp(config.adminPassword, "none") != 0) {
        if (!request->authenticate("admin", config.adminPassword)) {
            request->requestAuthentication();
            return false;
        }
    }
    return true;
}

uint32_t WebManager::hexToColor(const char* hexStr) {
    if (!hexStr) return 0;
    if (hexStr[0] == '#') hexStr++;
    return (uint32_t) strtoul(hexStr, NULL, 16);
}

String WebManager::templateProcessor(const String& var) {
    if (!_instance) return String();
    Config& config = _instance->_configMgr.getConfig();
    
    if (var == "ADMIN_PASS") return String(config.adminPassword);
    if (var == "SSID") return String(config.ssid);
    if (var == "WIFI_PASS") return String(config.password);
    if (var == "MQTT_SERVER") return String(config.mqttServer);
    if (var == "MQTT_PORT") return String(config.mqttPort);
    if (var == "MQTT_TLS_CHECKED") return config.mqttTls ? "checked" : "";
    if (var == "MQTT_CA") return String(config.mqttCaCert);
    if (var == "CLIENT_ID") return String(config.mqttClientId);
    if (var == "TOPIC") return String(config.mqttTopic);
    if (var == "MQTT_USER") return String(config.mqttUser);
    if (var == "MQTT_PASS") return String(config.mqttPassword);
    if (var == "NUM_PIXELS") return String(config.numPixels);
    if (var == "TOUCH_THRESHOLD") return String(config.touchThreshold);
    if (var == "COLOR") { char h[8]; sprintf(h, "#%06X", config.identityColor); return String(h); }
    if (var == "DURATION") return String(config.duration / 1000);
    if (var == "QUIET_CHECKED") return config.quietModeEnabled ? "checked" : "";
    if (var == "QUIET_START") return String(config.quietHourStart);
    if (var == "QUIET_END") return String(config.quietHourEnd);
    if (var == "FX_FADE") return strcmp(config.effect, "fade") == 0 ? "selected" : "";
    if (var == "FX_WIPE") return strcmp(config.effect, "color_wipe") == 0 ? "selected" : "";
    if (var == "FX_CHASE") return strcmp(config.effect, "theater_chase") == 0 ? "selected" : "";
    if (var == "FX_RAINBOW") return strcmp(config.effect, "rainbow_cycle") == 0 ? "selected" : "";
    if (var == "FX_BREATHE") return strcmp(config.effect, "breathe") == 0 ? "selected" : "";
    if (var == "FX_FIRE") return strcmp(config.effect, "fire") == 0 ? "selected" : "";
    if (var == "FX_COMET") return strcmp(config.effect, "comet") == 0 ? "selected" : "";
    return String();
}
