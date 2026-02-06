#include <Arduino.h>
#include "ConfigKeyboard.h"

// Lolin D32 Pro - Pin 23
const int SWITCH_PIN = 23;
// Battery Pin (Lolin D32 Pro uses IO35 with 100k/100k divider)
const int BAT_PIN = 35;

// Name the device "FootSwitch"
ConfigKeyboard kb("FootSwitch", "DIY", 100);


// Variable to store the last known state of the switch
int lastState = -1;
unsigned long lastBatteryUpdate = 0;

void updateBattery() {
  uint32_t raw = analogRead(BAT_PIN);
  float voltage = (raw / 4095.0) * 3.3 * 2.0; // x2 for voltage divider
  
  // Simple map: 3.2V (0%) to 4.2V (100%)
  int percentage = (int)((voltage - 3.2) * 100.0);
  if (percentage < 0) percentage = 0;
  if (percentage > 100) percentage = 100;
  
  // Serial.printf("Battery: %.2fV (%d%%)\n", voltage, percentage);
  Serial.printf("Battery Voltage: %.2fV -> %d%%\n", voltage, percentage);
  kb.setBatteryLevel(percentage);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting FootSwitch BLE (Configurable)...");

  // ESP32 supports internal Pull-Down resistors.
  // This ensures the pin is LOW when the switch is open,
  // and goes HIGH when the switch connects to 3.3V.
  pinMode(SWITCH_PIN, INPUT_PULLDOWN);

  // Initialize BLE Keyboard
  kb.begin();
  
  // Initial battery read
  updateBattery();
}

void loop() {
  // Update battery every 30 seconds
  if (millis() - lastBatteryUpdate > 30000) {
      updateBattery();
      lastBatteryUpdate = millis();
  }

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
