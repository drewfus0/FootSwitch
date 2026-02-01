#include <Arduino.h>
#include "ConfigKeyboard.h"

// Lolin D32 Pro - Pin 23
const int SWITCH_PIN = 23;

// Name the device "FootSwitch"
ConfigKeyboard kb("FootSwitch", "DIY", 100);


// Variable to store the last known state of the switch
int lastState = -1;

void setup() {
  Serial.begin(115200);
  Serial.println("Starting FootSwitch BLE (Configurable)...");

  // ESP32 supports internal Pull-Down resistors.
  // This ensures the pin is LOW when the switch is open,
  // and goes HIGH when the switch connects to 3.3V.
  pinMode(SWITCH_PIN, INPUT_PULLDOWN);

  // Initialize BLE Keyboard
  kb.begin();
}

void loop() {
  // Only process input if Bluetooth is connected to a device
  if(kb.isConnected()) {
    
    int currentState = digitalRead(SWITCH_PIN);

    // Check for state change
    if (currentState != lastState) {
        // Debounce
        delay(50);
        
        // Confirm state after debounce
        if (digitalRead(SWITCH_PIN) == currentState) {
            
            if (currentState == HIGH) {
                // Switch Depressed (Transition to HIGH)
                // Use the configured key code
                Serial.printf("Depressed -> Action\n");
                kb.performPress(); 
            } else {
                // Switch Released (Transition to LOW)
                // Use the configured key code
                Serial.printf("Released -> Action\n");
                kb.performRelease();
            }
            
            // Update last state
            lastState = currentState;
        }
    }
  }
}
