#ifndef CONFIG_KEYBOARD_H
#define CONFIG_KEYBOARD_H

#include <BleKeyboard.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <Preferences.h>

// UUIDs for the Configuration Service
#define CONFIG_SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CONFIG_CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

class ConfigKeyboard : public BleKeyboard {
private:
  BLEService* configService;
  BLECharacteristic* configCharacteristic;
  Preferences preferences;

  // New State Variables
  uint8_t mode = 0; // 0 = Momentary/Combo, 1 = Macro
  String payload1 = ""; // Combo Keys (Mode 0) or Press Sequence (Mode 1)
  String payload2 = ""; // Unused (Mode 0) or Release Sequence (Mode 1)

  // Helper to run a macro sequence
  // Strings format: "Actions,Value,Action,Value..."
  // Actions: 1=TAP, 2=DELAY
  void runSequence(String seq) {
    if (seq.length() == 0) return;
    
    int start = 0;
    while(start < seq.length()) {
        // Parse Action
        int comma1 = seq.indexOf(',', start);
        if(comma1 == -1) break; // Invalid format
        int action = seq.substring(start, comma1).toInt();
        
        // Parse Value
        int comma2 = seq.indexOf(',', comma1 + 1);
        if(comma2 == -1) comma2 = seq.length();
        int val = seq.substring(comma1 + 1, comma2).toInt();
        
        // Execute
        if (action == 1) { // TAP
            // write() sends a press and release
            write(val);
        } else if (action == 2) { // DELAY
            delay(val);
        }
        
        start = comma2 + 1;
    }
  }

  // Parse "MODE#PAYLOAD1#PAYLOAD2"
  void parseAndSave(std::string value) {
    String data = String(value.c_str());
    
    int firstHash = data.indexOf('#');
    int secondHash = data.indexOf('#', firstHash + 1);

    // Basic validity check
    if (firstHash > 0) {
      mode = data.substring(0, firstHash).toInt();
      
      if (secondHash > 0) {
          payload1 = data.substring(firstHash + 1, secondHash);
          payload2 = data.substring(secondHash + 1);
      } else {
          // Fallback if second hash missing
          payload1 = data.substring(firstHash + 1);
          payload2 = "";
      }

      // Save to NVS
      preferences.begin("footswitch", false);
      preferences.putUChar("mode", mode);
      preferences.putString("pl1", payload1);
      preferences.putString("pl2", payload2);
      preferences.end();
      
      Serial.printf("Config Saved! Mode: %d\nPL1: %s\nPL2: %s\n", mode, payload1.c_str(), payload2.c_str());
      
      // Notify client
      configCharacteristic->setValue(value);
      configCharacteristic->notify();
    }
  }

  class ConfigCallback : public BLECharacteristicCallbacks {
  private:
    ConfigKeyboard* parent;
  public:
    ConfigCallback(ConfigKeyboard* p) : parent(p) {}
    void onWrite(BLECharacteristic* pCharacteristic) {
      std::string value = pCharacteristic->getValue();
      if (value.length() > 0) parent->parseAndSave(value);
    }
  };

public:
  ConfigKeyboard(std::string name = "FootSwitch", std::string manufacturer = "DIY", uint8_t batteryLevel = 100) 
    : BleKeyboard(name, manufacturer, batteryLevel) {
      preferences.begin("footswitch", true);
      mode = preferences.getUChar("mode", 0);
      payload1 = preferences.getString("pl1", "");
      payload2 = preferences.getString("pl2", "");
      preferences.end();
  }

  void performPress() {
    if (mode == 0) {
        // MODE 0: MOMENTARY (Combo)
        // Parse payload1 (comma separated keys) and HOLD them
        int start = 0;
        while(start < payload1.length()) {
            int comma = payload1.indexOf(',', start);
            if (comma == -1) comma = payload1.length();
            int code = payload1.substring(start, comma).toInt();
            if(code > 0) press(code);
            start = comma + 1;
        }
    } else {
        // MODE 1: MACRO
        // Run specific sequence
        runSequence(payload1);
    }
  }

  void performRelease() {
    if (mode == 0) {
        // MODE 0: Release Everything
        releaseAll();
    } else {
        // MODE 1: MACRO
        // Run release sequence
        runSequence(payload2);
    }
  }

protected:
  // Override onConnect to RESTART advertising so a 2nd device (App) can connect
  // even while the PC is connected as a Keyboard.
  void onConnect(BLEServer* pServer) override {
    // Call parent method so BleKeyboard knows we are connected
    BleKeyboard::onConnect(pServer); 
    
    // Restart Advertising immediately
    pServer->getAdvertising()->start();
    Serial.println("Device connected. Advertising restarted for multi-connect.");
  }

public: 
  void begin(void) {
    BleKeyboard::begin();

    configService = pServer->createService(CONFIG_SERVICE_UUID);
    configCharacteristic = configService->createCharacteristic(
        CONFIG_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY
    );
    
    // Send current config as default value
    String currentConfig = String(mode) + "#" + payload1 + "#" + payload2;
    
    configCharacteristic->setValue(currentConfig.c_str());
    configCharacteristic->setCallbacks(new ConfigCallback(this));
    configService->start();
    
    // Update advertising? Default advertising only shows HID. 
    // But we can just connect to the device and discover this service.
    // For specific discovery, we'd need to add the UUID to advertising.
    BLEAdvertising* pAdvertising = pServer->getAdvertising();
    pAdvertising->addServiceUUID(CONFIG_SERVICE_UUID);
    // Restart advertising to pick up the new UUID
    pAdvertising->stop();
    pAdvertising->start();
  }
};

#endif
