#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include <Arduino.h>
#include "ConfigManager.h"
#include "LampController.h"

class InputHandler {
public:
    InputHandler(LampController& lamp);
    void begin(Config& config);
    void update(Config& config);
    
    bool isButtonPressed() { bool p = _buttonPressed; _buttonPressed = false; return p; }

private:
    LampController& _lamp;
    
    // Button
    static volatile bool _buttonPressed;
    static unsigned long _lastInterruptTime;
    static void IRAM_ATTR isr();
    
    // Potentiometer
    uint32_t _lastPotColor = 0;
    
    // Touch
    enum TouchState { IDLE, TOUCH_DETECTED, LONG_TOUCH_ACTIVE };
    TouchState _touchState = IDLE;
    unsigned long _touchStartTime = 0;
    unsigned long _lastBrightnessAnimTime = 0;
    int _brightnessDirection = -1;
    
    static const unsigned long DEBOUNCE_DELAY = 50;
    static const unsigned long BRIGHTNESS_ANIM_DELAY = 10;
};

#endif
