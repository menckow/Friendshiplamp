#include "LampController.h"

LampController::LampController(uint16_t numPixels, uint8_t pin) 
    : _pixels(numPixels, pin, NEO_GRB + NEO_KHZ800) {
    _currentColor = _pixels.Color(255, 255, 255);
}

void LampController::begin() {
    _pixels.begin();
    setAllPixels(0);
}

void LampController::update(Config& config) {
    if (_inReceivedColorMode) {
        if (millis() >= _receivedColorEndTime) {
            _inReceivedColorMode = false;
            _isLampOn = _preReceivedState_isLampOn;
            _currentColor = _preReceivedState_Color;
            if (_isLampOn) setAllPixels(_currentColor); else setAllPixels(0);
        } else {
            renderEffect(config);
        }
    }
}

void LampController::setOn(bool on) {
    _isLampOn = on;
    if (_isLampOn) {
        if (_currentBrightness == 0) _currentBrightness = 255;
        setAllPixels(_currentColor);
    } else {
        setAllPixels(0);
    }
}

void LampController::setColor(uint32_t color) {
    _currentColor = color;
    if (_isLampOn && !_inReceivedColorMode) {
        setAllPixels(_currentColor);
    }
}

void LampController::setColorHSV(uint16_t hue) {
    uint32_t c = _pixels.gamma32(_pixels.ColorHSV(hue));
    if (c != _currentColor) {
        _currentColor = c;
        if (_isLampOn && !_inReceivedColorMode) {
            setAllPixels(_currentColor);
        }
    }
}

void LampController::setBrightness(uint8_t brightness) {
    _currentBrightness = brightness;
    if (_isLampOn && !_inReceivedColorMode) {
        setAllPixels(_currentColor);
    }
}

void LampController::startReceivedColorMode(uint32_t color, const char* effect, uint32_t duration) {
    _preReceivedState_isLampOn = _isLampOn;
    _preReceivedState_Color = _currentColor;
    
    _inReceivedColorMode = true;
    _receivedColor = color;
    _receivedColorEndTime = millis() + duration;
    strlcpy(_receivedEffect, effect, sizeof(_receivedEffect));
    
    _effectStep = 0;
    _lastEffectTime = 0;
}

void LampController::setAllPixels(uint32_t color, int brightness) {
    uint8_t effectiveBrightness = (brightness == -1) ? _currentBrightness : (uint8_t)brightness;
    uint8_t r = ((color >> 16) & 0xFF) * (effectiveBrightness / 255.0);
    uint8_t g = ((color >> 8) & 0xFF) * (effectiveBrightness / 255.0);
    uint8_t b = (color & 0xFF) * (effectiveBrightness / 255.0);
    
    for (int i = 0; i < _pixels.numPixels(); i++) {
        _pixels.setPixelColor(i, r, g, b);
    }
    _pixels.show();
}

void LampController::renderEffect(Config& config) {
    unsigned long now = millis();
    uint8_t baseR = (_receivedColor >> 16) & 0xFF;
    uint8_t baseG = (_receivedColor >> 8) & 0xFF;
    uint8_t baseB = _receivedColor & 0xFF;
    
    uint8_t effectiveBrightness = _currentBrightness;
    if (effectiveBrightness < 150) effectiveBrightness = 150;
    float bFact = effectiveBrightness / 255.0;

    if (strcmp(_receivedEffect, "color_wipe") == 0) {
        if (now - _lastEffectTime > 50) {
            _lastEffectTime = now;
            if (_effectStep < config.numPixels) {
                _pixels.setPixelColor(_effectStep, _pixels.gamma32(_pixels.Color(baseR * bFact, baseG * bFact, baseB * bFact)));
                _pixels.show();
                _effectStep++;
            }
        }
    } else if (strcmp(_receivedEffect, "theater_chase") == 0) {
        if (now - _lastEffectTime > 100) {
            _lastEffectTime = now;
            _pixels.clear();
            uint32_t color = _pixels.gamma32(_pixels.Color(baseR * bFact, baseG * bFact, baseB * bFact));
            for (int i = 0; i < config.numPixels; i += 3) {
                if (i + _effectStep < config.numPixels) {
                    _pixels.setPixelColor(i + _effectStep, color);
                }
            }
            _pixels.show();
            _effectStep = (_effectStep + 1) % 3;
        }
    } else if (strcmp(_receivedEffect, "rainbow_cycle") == 0) {
        if (now - _lastEffectTime > 20) {
            _lastEffectTime = now;
            for(int i=0; i<config.numPixels; i++) {
                int pixelHue = _effectStep + (i * 65536L / config.numPixels);
                uint32_t c = _pixels.gamma32(_pixels.ColorHSV(pixelHue));
                uint8_t r = ((c >> 16) & 0xFF) * bFact;
                uint8_t g = ((c >> 8) & 0xFF) * bFact;
                uint8_t b = (c & 0xFF) * bFact;
                _pixels.setPixelColor(i, _pixels.Color(r, g, b));
            }
            _pixels.show();
            _effectStep += 256;
            if(_effectStep >= 65536) _effectStep = 0;
        }
    } else if (strcmp(_receivedEffect, "breathe") == 0) {
        if (now - _lastEffectTime > 15) {
            _lastEffectTime = now;
            float pulse = (sin(_effectStep * 0.05) * 127 + 128) / 255.0;
            uint32_t color = _pixels.gamma32(_pixels.Color(baseR * bFact * pulse, baseG * bFact * pulse, baseB * bFact * pulse));
            for(int i=0; i<config.numPixels; i++) _pixels.setPixelColor(i, color);
            _pixels.show();
            _effectStep++;
        }
    } else if (strcmp(_receivedEffect, "fire") == 0) {
        if (now - _lastEffectTime > 50) {
            _lastEffectTime = now;
            for(int i=0; i<config.numPixels; i++) {
                float flicker = random(50, 255) / 255.0;
                _pixels.setPixelColor(i, _pixels.gamma32(_pixels.Color(baseR * bFact * flicker, baseG * bFact * flicker, baseB * bFact * flicker)));
            }
            _pixels.show();
            _effectStep++;
        }
    } else if (strcmp(_receivedEffect, "comet") == 0) {
        if (now - _lastEffectTime > 30) {
            _lastEffectTime = now;
            for(int i=0; i<config.numPixels; i++) {
                uint32_t c = _pixels.getPixelColor(i);
                uint8_t r = ((c >> 16) & 0xFF) * 0.8;
                uint8_t g = ((c >> 8) & 0xFF) * 0.8;
                uint8_t b = (c & 0xFF) * 0.8;
                _pixels.setPixelColor(i, _pixels.Color(r, g, b));
            }
            uint32_t head = _pixels.gamma32(_pixels.Color(baseR * bFact, baseG * bFact, baseB * bFact));
            _pixels.setPixelColor(_effectStep % config.numPixels, head);
            _pixels.show();
            _effectStep++;
        }
    } else {
        // Default: Steady color
        if (_effectStep == 0) {
            uint32_t color = _pixels.gamma32(_pixels.Color(baseR * bFact, baseG * bFact, baseB * bFact));
            for(int i=0; i<config.numPixels; i++) _pixels.setPixelColor(i, color);
            _pixels.show();
            _effectStep = 1;
        }
    }
}
