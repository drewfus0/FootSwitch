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

  // Current Key Codes (default to 'k' and 'm')
  uint8_t pressCode = 'k';
  uint8_t releaseCode = 'm';

  // Helper to parse "107,109" string to integers
  void parseAndSave(std::string value) {
    String data = String(value.c_str());
    int commaIndex = data.indexOf(',');
    
    if (commaIndex > 0) {
      String pStr = data.substring(0, commaIndex);
      String rStr = data.substring(commaIndex + 1);
      
      uint8_t p = (uint8_t)pStr.toInt();
      uint8_t r = (uint8_t)rStr.toInt();
      
      if (p > 0 && r > 0) {
        pressCode = p;
        releaseCode = r;
        
        // Save to NVS
        preferences.begin("footswitch", false);
        preferences.putUChar("press", pressCode);
        preferences.putUChar("release", releaseCode);
        preferences.end();
        
        Serial.printf("Config Saved! Press: %d, Release: %d\n", pressCode, releaseCode);
        
        // Notify the client of the update
        configCharacteristic->setValue(value);
        configCharacteristic->notify();
      }
    }
  }

  // Callback class to handle writes
  class ConfigCallback : public BLECharacteristicCallbacks {
  private:
    ConfigKeyboard* parent;
  public:
    ConfigCallback(ConfigKeyboard* p) : parent(p) {}
    
    void onWrite(BLECharacteristic* pCharacteristic) {
      std::string value = pCharacteristic->getValue();
      if (value.length() > 0) {
        parent->parseAndSave(value);
      }
    }
  };

public:
  ConfigKeyboard(std::string name = "FootSwitch", std::string manufacturer = "DIY", uint8_t batteryLevel = 100) 
    : BleKeyboard(name, manufacturer, batteryLevel) {
      // Load saved values on startup
      preferences.begin("footswitch", true); // true = read-only
      pressCode = preferences.getUChar("press", 'k');
      releaseCode = preferences.getUChar("release", 'm');
      preferences.end();
  }

  uint8_t getPressKey() { return pressCode; }
  uint8_t getReleaseKey() { return releaseCode; }

protected:
  // Override onStarted to add our custom service
  void onStarted(BLEServer *pServer) override {
    // 1. Create Service
    configService = pServer->createService(CONFIG_SERVICE_UUID);

    // 2. Create Characteristic
    configCharacteristic = configService->createCharacteristic(
        CONFIG_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    
    // 3. Set initial value (current config)
    String currentConfig = String(pressCode) + "," + String(releaseCode);
    configCharacteristic->setValue(currentConfig.c_str());
    
    // 4. Set Callback
    configCharacteristic->setCallbacks(new ConfigCallback(this));

    // 5. Start Service
    configService->start();

    // 6. Advertise this service so the Web App can find it
    pServer->getAdvertising()->addServiceUUID(CONFIG_SERVICE_UUID);
  }
};

#endif
