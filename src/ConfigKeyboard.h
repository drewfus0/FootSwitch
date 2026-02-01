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

  // Key Codes and Modifiers
  // Modifiers: 0=None, 128=Ctrl, 129=Shift, 130=Alt, 131=Gui (Win/Cmd)
  uint8_t pressCode = 'k';
  uint8_t pressMod = 0;
  
  uint8_t releaseCode = 'm';
  uint8_t releaseMod = 0;

  // Helper to parse "key,mod,key,mod" (e.g. "99,128,109,0")
  void parseAndSave(std::string value) {
    String data = String(value.c_str());
    
    // Simple parser for 4 comma-separated values
    int first = data.indexOf(',');
    int second = data.indexOf(',', first + 1);
    int third = data.indexOf(',', second + 1);

    if (first > 0 && second > 0 && third > 0) {
      uint8_t pKey = data.substring(0, first).toInt();
      uint8_t pMod = data.substring(first + 1, second).toInt();
      uint8_t rKey = data.substring(second + 1, third).toInt();
      uint8_t rMod = data.substring(third + 1).toInt();

      if (pKey > 0 && rKey > 0) {
        pressCode = pKey;
        pressMod = pMod;
        releaseCode = rKey;
        releaseMod = rMod;
        
        // Save to NVS
        preferences.begin("footswitch", false);
        preferences.putUChar("press", pressCode);
        preferences.putUChar("pressMod", pressMod);
        preferences.putUChar("release", releaseCode);
        preferences.putUChar("releaseMod", releaseMod);
        preferences.end();
        
        Serial.printf("Config Saved! Press: %d (Mod %d), Release: %d (Mod %d)\n", pressCode, pressMod, releaseCode, releaseMod);
        
        // Notify client
        configCharacteristic->setValue(value);
        configCharacteristic->notify();
      }
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
      pressCode = preferences.getUChar("press", 'k');
      pressMod = preferences.getUChar("pressMod", 0);
      releaseCode = preferences.getUChar("release", 'm');
      releaseMod = preferences.getUChar("releaseMod", 0);
      preferences.end();
  }

  // Helper to execute the combination
  void sendAction(uint8_t key, uint8_t mod) {
    if (mod > 0) press(mod); // Press Modifier (Ctrl/Shift/etc)
    if (key > 0) press(key); // Press Key
    delay(10);               // Brief hold
    releaseAll();            // Release both
  }

  void performPress() { sendAction(pressCode, pressMod); }
  void performRelease() { sendAction(releaseCode, releaseMod); }

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

  // Override onStarted to add our custom service
  void onStarted(BLEServer *pServer) override {
    configService = pServer->createService(CONFIG_SERVICE_UUID);
    configCharacteristic = configService->createCharacteristic(
        CONFIG_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY
    );
    
    // Format: "key,mod,key,mod"
    String currentConfig = String(pressCode) + "," + String(pressMod) + "," + 
                           String(releaseCode) + "," + String(releaseMod);
    configCharacteristic->setValue(currentConfig.c_str());
    configCharacteristic->setCallbacks(new ConfigCallback(this));
    configService->start();

    // REMOVED to save space in the advertising packet. 
    // This allows the Name "FootSwitch" to fit and be visible on Windows.
    // pServer->getAdvertising()->addServiceUUID(CONFIG_SERVICE_UUID);
  }
};

#endif
