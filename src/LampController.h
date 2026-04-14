#ifndef LAMP_CONTROLLER_H
#define LAMP_CONTROLLER_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "ConfigManager.h"

class LampController {
public:
    LampController(uint16_t numPixels, uint8_t pin);
    void begin();
    void update(Config& config);
    
    void setOn(bool on);
    bool isOn() const { return _isLampOn; }
    
    void setColor(uint32_t color);
    void setColorHSV(uint16_t hue);
    uint32_t getColor() const { return _currentColor; }
    
    void setBrightness(uint8_t brightness);
    uint8_t getBrightness() const { return _currentBrightness; }
    
    void startReceivedColorMode(uint32_t color, const char* effect, uint32_t duration);
    bool isInReceivedColorMode() const { return _inReceivedColorMode; }
    
    void setAllPixels(uint32_t color, int brightness = -1);

private:
    Adafruit_NeoPixel _pixels;
    ConfigManager* _configMgr;
    
    bool _isLampOn = false;
    uint32_t _currentColor;
    uint8_t _currentBrightness = 100;
    
    // Received color mode state
    bool _inReceivedColorMode = false;
    uint32_t _receivedColor = 0;
    unsigned long _receivedColorEndTime = 0;
    char _receivedEffect[32];
    
    // Previous state backup
    bool _preReceivedState_isLampOn = false;
    uint32_t _preReceivedState_Color = 0;
    
    unsigned long _lastEffectTime = 0;
    int _effectStep = 0;

    void renderEffect(Config& config);
};

#endif
