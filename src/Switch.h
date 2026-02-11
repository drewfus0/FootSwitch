#ifndef SWITCH_H
#define SWITCH_H

#include <Arduino.h>
#include <BleKeyboard.h>

class Switch {
private:
    uint8_t _pin;
    uint8_t _id;
    BleKeyboard* _keyboard;
    
    // Config
    uint8_t _mode = 0; // 0 = Momentary, 1 = Macro
    String _payload1 = "";
    String _payload2 = "";
    String _name = "";

    // State
    int _buttonState;
    int _lastButtonState = LOW;
    unsigned long _lastDebounceTime = 0;
    unsigned long _debounceDelay = 50;

    // Macro Logic
    void runSequence(String seq);

public:
    Switch(uint8_t id, BleKeyboard* keyboard);
    
    void begin(uint8_t pin);
    void tick();
    
    // Configuration
    void updateConfig(uint8_t mode, String payload1, String payload2);
    void setPin(uint8_t pin);
    uint8_t getPin() const { return _pin; }
    uint8_t getMode() const { return _mode; }
    String getP1() const { return _payload1; }
    String getP2() const { return _payload2; }
    void setName(String name) { _name = name; }
    String getName() const { return _name; }
    
    // Actions
    void performPress();
    void performRelease();
};

#endif
