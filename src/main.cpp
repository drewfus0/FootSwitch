#include <Arduino.h>
#include "ConfigKeyboard.h"

// Battery Pin (Lolin D32 Pro uses IO35 with 100k/100k divider)
const int BAT_PIN = 35;

// Name the device "FootSwitch"
ConfigKeyboard kb("FootSwitch", "DIY", 100);

unsigned long lastBatteryUpdate = 0;

void updateBattery() {
  // Use analogReadMilliVolts for better accuracy with ESP32 ADC factory calibration
  uint32_t mv = analogReadMilliVolts(BAT_PIN);
  
  // The Lolin D32 Pro uses a 100k/100k voltage divider.
  // However, the high impedance (100k) combined with the ESP32 ADC input impedance
  // often results in a voltage drop during sampling, reading lower than actual.
  // A Compensation Factor of ~1.1 is often needed if reading is too low.
  // (e.g. Reading 3.9V when battery is 4.2V -> 4.2/3.9 = 1.077)
  float compensationFactor = 1.08; 
  
  float voltage = (mv * 2.0 * compensationFactor) / 1000.0;
  
  // Simple map: 3.2V (0%) to 4.2V (100%)
  int percentage = (int)((voltage - 3.2) * 100.0);
  if (percentage < 0) percentage = 0;
  if (percentage > 100) percentage = 100;
  
  Serial.printf("Battery Voltage: %.2fV (raw: %d) -> %d%%\n", voltage, mv, percentage);
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
