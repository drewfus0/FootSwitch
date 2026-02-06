#include "Switch.h"

Switch::Switch(uint8_t id, BleKeyboard* keyboard) {
    _id = id;
    _keyboard = keyboard;
    _pin = 0;
}

void Switch::begin(uint8_t pin) {
    _pin = pin;
    if (_pin > 0) {
        pinMode(_pin, INPUT_PULLDOWN);
    }
}

void Switch::setPin(uint8_t pin) {
    // If we're changing pins, might be good to reset the old one to input?
    // For now, just setup the new one.
    _pin = pin;
    if (_pin > 0) {
        pinMode(_pin, INPUT_PULLDOWN);
    }
}

void Switch::updateConfig(uint8_t mode, String payload1, String payload2) {
    _mode = mode;
    _payload1 = payload1;
    _payload2 = payload2;
}

void Switch::runSequence(String seq) {
    if (seq.length() == 0) return;
    
    // Parse format: "Action,Value,Action,Value..."
    int start = 0;
    while (start < seq.length()) {
        int comma = seq.indexOf(',', start);
        if (comma == -1) comma = seq.length();
        
        int action = seq.substring(start, comma).toInt();
        start = comma + 1;
        
        // Get Value
        comma = seq.indexOf(',', start);
        if (comma == -1) comma = seq.length();
        int val = seq.substring(start, comma).toInt();
        start = comma + 1;

        switch (action) {
            case 1: // TAP
                _keyboard->write(val);
                break;
            case 2: // DELAY
                delay(val);
                break;
            case 3: // PRESS
                _keyboard->press(val);
                break;
            case 4: // RELEASE
                _keyboard->release(val);
                break;
        }
    }
}

void Switch::performPress() {
    if (_mode == 0) {
        // MODE 0: MOMENTARY
        // Parse payload1 (comma separated keys) and HOLD them
        int start = 0;
        while(start < _payload1.length()) {
            int comma = _payload1.indexOf(',', start);
            if (comma == -1) comma = _payload1.length();
            int code = _payload1.substring(start, comma).toInt();
            if(code > 0) _keyboard->press(code);
            start = comma + 1;
        }
    } else {
        // MODE 1: MACRO
        runSequence(_payload1);
    }
}

void Switch::performRelease() {
    if (_mode == 0) {
        // MODE 0: Release All (Simple but effective for now)
        // Ideally we should only release keys this switch pressed, 
        // but BleKeyboard.releaseAll() is safer for avoiding stuck keys.
        _keyboard->releaseAll(); 
    } else {
        // MODE 1: MACRO
        runSequence(_payload2);
    }
}

void Switch::tick() {
    if (_pin == 0) return;

    int reading = digitalRead(_pin);

    if (reading != _lastButtonState) {
        _lastDebounceTime = millis();
    }

    if ((millis() - _lastDebounceTime) > _debounceDelay) {
        if (reading != _buttonState) {
            _buttonState = reading;

            if (_buttonState == HIGH) {
                performPress();
            } else {
                performRelease();
            }
        }
    }

    _lastButtonState = reading;
}
