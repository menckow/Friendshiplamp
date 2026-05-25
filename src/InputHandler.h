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
    
    // Potentiometer – FIX 1: Zustand für Deadband-Filter
    uint16_t _lastHue = 0;
    unsigned long _lastPotReadTime = 0;
    
    // Touch
    enum TouchState { IDLE, TOUCH_DETECTED, LONG_TOUCH_ACTIVE };
    TouchState _touchState = IDLE;
    unsigned long _touchStartTime = 0;
    unsigned long _lastBrightnessAnimTime = 0;
    int _brightnessDirection = -1;

    // Entstör-Filter für den Touch-Sensor (gegen elektrisches Rauschen)
    bool _touchActive = false;
    uint8_t _touchBelowCount = 0;
    uint8_t _touchAboveCount = 0;

    static const unsigned long DEBOUNCE_DELAY = 50;

    // FIX 1: Potentiometer-Entprellung
    // ESP32-ADC rauscht ca. ±20–50 Counts; Deadband von 300 HSV-Schritten
    // entspricht ~0,5% des Farbraums und filtert das zuverlässig heraus.
    static const unsigned long POT_READ_INTERVAL = 50;  // ms zwischen ADC-Messungen
    static const uint16_t     POT_DEADBAND       = 300; // min. HSV-Änderung für Update

    // FIX 2: Langsameres Dimmen reduziert show()-Aufrufe und Interrupt-Blockaden
    static const unsigned long BRIGHTNESS_ANIM_DELAY = 25;  // war: 10ms

    static const uint8_t TOUCH_SAMPLES = 4;              // gemittelte Messungen pro update()
    static const uint8_t TOUCH_CONFIRM_SAMPLES = 4;      // ~40ms Bestätigung vor Zustandswechsel
    static const uint8_t TOUCH_RELEASE_HYSTERESIS = 8;   // Release-Schwelle = threshold + diesen Offset
    static const uint8_t MIN_DIM_BRIGHTNESS = 30;        // Helligkeit wird nicht dunkler als das (sichtbar bleiben)
};

#endif
