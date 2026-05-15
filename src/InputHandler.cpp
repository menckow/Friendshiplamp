#include "InputHandler.h"

volatile bool InputHandler::_buttonPressed = false;
unsigned long InputHandler::_lastInterruptTime = 0;

InputHandler::InputHandler(LampController& lamp) : _lamp(lamp) {}

void IRAM_ATTR InputHandler::isr() {
    unsigned long interruptTime = millis();
    if (interruptTime - _lastInterruptTime > DEBOUNCE_DELAY) {
        _buttonPressed = true;
    }
    _lastInterruptTime = interruptTime;
}

void InputHandler::begin(Config& config) {
    pinMode(4, INPUT_PULLUP); // BUTTON_PIN
    attachInterrupt(digitalPinToInterrupt(4), isr, FALLING);
}

void InputHandler::update(Config& config) {
    if (_lamp.isInReceivedColorMode()) return;
    
    // Potentiometer
    if (_lamp.isOn()) {
        int raw = analogRead(34); // POTENTIOMETER_PIN
        uint16_t hue = map(raw, 0, 4095, 0, 65535);
        _lamp.setColorHSV(hue);
    }
    
    // Touch (Mittelung + Hysterese + Bestätigungs-Samples gegen elektrisches Rauschen)
    uint32_t sum = 0;
    for (uint8_t i = 0; i < TOUCH_SAMPLES; i++) sum += touchRead(32); // TOUCH_PIN
    int val = sum / TOUCH_SAMPLES;

    uint16_t engageThreshold = config.touchThreshold;
    uint16_t releaseThreshold = config.touchThreshold + TOUCH_RELEASE_HYSTERESIS;
    bool rawBelow = _touchActive ? (val < releaseThreshold) : (val < engageThreshold);

    if (rawBelow) {
        _touchAboveCount = 0;
        if (_touchBelowCount < TOUCH_CONFIRM_SAMPLES) _touchBelowCount++;
    } else {
        _touchBelowCount = 0;
        if (_touchAboveCount < TOUCH_CONFIRM_SAMPLES) _touchAboveCount++;
    }
    if (!_touchActive && _touchBelowCount >= TOUCH_CONFIRM_SAMPLES) _touchActive = true;
    else if (_touchActive && _touchAboveCount >= TOUCH_CONFIRM_SAMPLES) _touchActive = false;

    unsigned long now = millis();
    if (_touchActive) {
        if (_touchState == IDLE) { _touchState = TOUCH_DETECTED; _touchStartTime = now; }
        if (_touchState == TOUCH_DETECTED && (now - _touchStartTime) >= 1000) {
            _touchState = LONG_TOUCH_ACTIVE;
            if (!_lamp.isOn()) { _lamp.setOn(true); }
        }
        if (_touchState == LONG_TOUCH_ACTIVE && (now - _lastBrightnessAnimTime > BRIGHTNESS_ANIM_DELAY)) {
            _lastBrightnessAnimTime = now;
            int next = _lamp.getBrightness() + _brightnessDirection;
            // Helligkeits-Untergrenze, damit ein versehentlich getriggerter Dimm-Vorgang
            // die Lampe nicht "aus" wirken lässt
            if (next <= MIN_DIM_BRIGHTNESS) { next = MIN_DIM_BRIGHTNESS; _brightnessDirection = 1; }
            else if (next >= 255) { next = 255; _brightnessDirection = -1; }
            _lamp.setBrightness(next);
        }
    } else {
        if (_touchState == TOUCH_DETECTED) {
            _lamp.setOn(!_lamp.isOn());
        }
        _touchState = IDLE;
    }
}
