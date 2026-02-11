#ifndef CONFIG_KEYBOARD_H
#define CONFIG_KEYBOARD_H

#include <BleKeyboard.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <Preferences.h>
#include <BLE2902.h>
#include <vector>
#include "Switch.h"

// UUIDs for the Configuration Service
#define CONFIG_SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CONFIG_CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

class ConfigKeyboard : public BleKeyboard {
private:
  BLEService* configService;
  BLECharacteristic* configCharacteristic;
  Preferences preferences;

  std::vector<Switch*> switches;

  // Helper to load settings for a specific switch index
  void loadSwitch(int i) {
    char key[16];

    sprintf(key, "sw%d_pin", i);
    uint8_t pin = preferences.getUChar(key, 0);

    sprintf(key, "sw%d_mode", i);
    uint8_t m = preferences.getUChar(key, 0);

    sprintf(key, "sw%d_pl1", i);
    String p1 = preferences.getString(key, "");

    sprintf(key, "sw%d_pl2", i);
    String p2 = preferences.getString(key, "");

    Switch* s = new Switch(i, this);
    if (pin > 0) s->begin(pin);
    s->updateConfig(m, p1, p2);
    switches.push_back(s);
    
    Serial.printf("Loaded Switch %d: Pin %d\n", i, pin);
  }

  // Save specific switch
  void saveSwitch(int i, uint8_t pin, uint8_t mode, String p1, String p2) {
    char key[16];
    
    sprintf(key, "sw%d_pin", i);
    preferences.putUChar(key, pin);
    
    sprintf(key, "sw%d_mode", i);
    preferences.putUChar(key, mode);
    
    sprintf(key, "sw%d_pl1", i);
    preferences.putString(key, p1);

    sprintf(key, "sw%d_pl2", i);
    preferences.putString(key, p2);
  }

  // Parse New Protocol
  void parseAndSave(std::string value) {
    String data = String(value.c_str());
    Serial.println("Received: " + data);
    
    preferences.begin("footswitch", false);

    if (data.startsWith("ADD:")) {
        int pin = data.substring(4).toInt();
        if (pin > 0) {
            int newIdx = switches.size();
            
            // Save Switch Default
            saveSwitch(newIdx, pin, 0, "", "");
            // Save New Count
            preferences.putUChar("count", newIdx + 1);
            
            // Live Update
            Switch* s = new Switch(newIdx, this);
            s->begin(pin);
            switches.push_back(s);
            
            Serial.printf("Added Switch %d on Pin %d\n", newIdx, pin);
        }
    } 
    else if (data.startsWith("del:")) {
        // Implementing simple clear for now as full delete requires shifting indices
        Serial.println("Delete not fully impl, use clr");
    }
    else if (data.startsWith("clr") || data.startsWith("CLR")) {
        preferences.clear();
        for(auto s : switches) delete s;
        switches.clear();
        preferences.putUChar("count", 0);
        Serial.println("Cleared All Switches");
    }
    else if (data.startsWith("CFG:")) {
        // CFG:INDEX:PIN:MODE#PL1#PL2
        // Find separators
        int firstColon = 3; // "CFG"
        int secondColon = data.indexOf(':', firstColon + 1); // Index
        int thirdColon = data.indexOf(':', secondColon + 1); // Pin
        
        if (secondColon > 0 && thirdColon > 0) {
            int idx = data.substring(firstColon + 1, secondColon).toInt();
            int pin = data.substring(secondColon + 1, thirdColon).toInt();
            
            // Mode and Payloads are separated by #
            // Remaining string starting after thirdColon
            String rest = data.substring(thirdColon + 1);
            int hash1 = rest.indexOf('#');
            int hash2 = rest.indexOf('#', hash1 + 1);
            
            uint8_t mode = 0;
            String pl1 = "", pl2 = "";

            if (hash1 > 0) {
                mode = rest.substring(0, hash1).toInt();
                if (hash2 > 0) {
                    pl1 = rest.substring(hash1 + 1, hash2);
                    pl2 = rest.substring(hash2 + 1);
                } else {
                    pl1 = rest.substring(hash1 + 1);
                }
            } else {
                // If no hash, maybe just mode?
                mode = rest.toInt();
            }

            if (idx >= 0 && idx < switches.size()) {
                saveSwitch(idx, pin, mode, pl1, pl2);
                
                // Live Update
                switches[idx]->setPin(pin);
                switches[idx]->updateConfig(mode, pl1, pl2);
                
                Serial.printf("Configured Switch %d\n", idx);
            }
            else if (idx == switches.size()) {
                // Allow implicit Add
                saveSwitch(idx, pin, mode, pl1, pl2);
                preferences.putUChar("count", idx + 1);
                
                Switch* s = new Switch(idx, this);
                s->begin(pin);
                s->updateConfig(mode, pl1, pl2);
                switches.push_back(s);
            }
        }
    }
    
    preferences.end();
    
    // Notify client of echo?
    configCharacteristic->setValue(value);
    configCharacteristic->notify();
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
  }

  void begin() {
    preferences.begin("footswitch", true); // Read Only
    int count = preferences.getUChar("count", 0);
    
    // Migration Logic: If count is 0 but "mode" exists
    if (count == 0 && preferences.isKey("mode")) {
        Serial.println("Migrating Legacy Config...");
        preferences.end();
        preferences.begin("footswitch", false);
        
        uint8_t m = preferences.getUChar("mode", 0);
        String p1 = preferences.getString("pl1", "");
        String p2 = preferences.getString("pl2", "");
        
        preferences.putUChar("count", 1);
        saveSwitch(0, 23, m, p1, p2); // Default to pin 23
        
        count = 1;
    }
    
    preferences.end();
    
    preferences.begin("footswitch", true);
    for(int i=0; i<count; i++) {
        loadSwitch(i);
    }
    preferences.end();

    Serial.println("ConfigKeyboard::begin() called");
    BleKeyboard::begin();
    
    // Advertising
    // Wait for onStarted to add service UUID?
    // BleKeyboard::begin() calls onStarted() internally BEFORE starting advertising.
    // However, it starts advertising with HID UUIDs.
    // We want to update it.
    Serial.println("Updating BLE Advertising...");
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->stop();
    delay(10);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06); // 7.5ms
    pAdvertising->setMaxPreferred(0x12); // 22.5ms
    pAdvertising->start();
  }
  
  void tick() {
      for(auto s : switches) {
          s->tick();
      }
  }

  // Get current config as string (for reading)
  // Format: "COUNT:N;SW:0:PIN:MODE;SW:1:..."
  String getConfigString() {
      String s = "COUNT:" + String(switches.size()) + ";";
      for(int i=0; i<switches.size(); i++) {
          s += "SW:" + String(i) + ":" + String(switches[i]->getPin()) + ";";
      }
      return s;
  }

protected:
  void onConnect(BLEServer* pServer) override {
    Serial.println("Client Connected");
    BleKeyboard::onConnect(pServer); 
    pServer->getAdvertising()->start();
  }

  void onStarted(BLEServer *pServer) override {
    Serial.println("BLE Server Started");
    configService = pServer->createService(CONFIG_SERVICE_UUID);
    configCharacteristic = configService->createCharacteristic(
        CONFIG_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY
    );
    configCharacteristic->addDescriptor(new BLE2902());
    
    // Set Initial Value
    configCharacteristic->setValue(getConfigString().c_str());
    
    configCharacteristic->setCallbacks(new ConfigCallback(this));
    configService->start();
    Serial.println("Updating BLE Advertising...(2)");
    BLEAdvertising* pAdvertising = pServer->getAdvertising();
    pAdvertising->addServiceUUID(CONFIG_SERVICE_UUID);
  }
};

#endif
