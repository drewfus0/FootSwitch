#include <Arduino.h>
#include "ConfigKeyboard.h"

// Battery Pin (Lolin D32 Pro uses IO35 with 100k/100k divider)
const int BAT_PIN = 35;

// Name the device "FootSwitch"
ConfigKeyboard kb("FootSwitch", "DIY", 100);

unsigned long lastBatteryUpdate = 0;

void updateBattery() {
  uint32_t raw = analogRead(BAT_PIN);
  float voltage = (raw / 4095.0) * 3.3 * 2.0; // x2 for voltage divider
  
  // Simple map: 3.2V (0%) to 4.2V (100%)
  int percentage = (int)((voltage - 3.2) * 100.0);
  if (percentage < 0) percentage = 0;
  if (percentage > 100) percentage = 100;
  
  Serial.printf("Battery Voltage: %.2fV -> %d%%\n", voltage, percentage);
  kb.setBatteryLevel(percentage);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting FootSwitch BLE (Configurable)...");

  // Initialize BLE Keyboard (This loads switches from NVS and configures pins)
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

  // Process all switch logic
  // We process this continuously to ensure proper debounce state tracking
  kb.tick();
  
  // Small delay to prevent tight loop watchdog issues
  delay(10);
}
