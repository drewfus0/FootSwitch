# FootSwitch Project Instructions

## Project Overview
This project builds a configurable Bluetooth Low Energy (BLE) FootSwitch using an ESP32 (Lolin D32 Pro) and a companion Web Bluetooth configuration app.

### Architecture
- **Firmware**: PlatformIO C++ project using the Arduino framework. Acts as a composite BLE device:
  - **HID Device**: Standard BLE Keyboard for sending keystrokes.
  - **GATT Server**: Custom service for receiving configuration data from the web app.
- **Web App**: A single-file HTML/JS application (`webapp/index.html`) using the Web Bluetooth API to configure the device wirelessly.

## Development Environment
- **Platform**: PlatformIO (VS Code Extension)
- **Framework**: Arduino
- **Board**: `lolin_d32_pro` (Check `platformio.ini`)
- **Key Dependencies**: `ESP32 BLE Keyboard`

## Build & Deploy
- **Build Firmware**: `pio run`
- **Upload Firmware**: `pio run -t upload`
- **Monitor Serial**: `pio device monitor` (Baud: 115200)
- **Run Web App**: Open `webapp/index.html` in a BLE-supported browser (Chrome, Edge, Bluefy). No build step required.

## Codebase Map

### Firmware (`src/`)
- **`main.cpp`**:
  - Handles physical IO (Pin 23).
  - Implements debounce logic (50ms).
  - Triggers press/release events on the `kb` object.
- **`ConfigKeyboard.h`**:
  - Inherits from `BleKeyboard`.
  - Manages **Preferences** (NVS) to save keys/macros across reboots.
  - **mode 0 (Momentary)**: Simple key combos.
  - **mode 1 (Macro)**: Complex sequences (Tap, Delay, Press, Release).
  - **Data Protocol**: Parses custom string payload `"Action,Value,..."`.

### Configuration (`webapp/`)
- **`index.html`**:
  - Connects via `navigator.bluetooth`.
  - Service UUID: `4fafc201-1fb5-459e-8fcc-c5c9c331914b`
  - Characteristic UUID: `beb5483e-36e1-4688-b7f5-ea07361b26a8`
  - Encodes UI selections into the custom protocol string before writing.

## Development Patterns
1. **Header-Only Logic**: Much of the complexity is inline in `ConfigKeyboard.h`. When modifying logic, check this file first.
2. **Pull-Down Input**: `SWITCH_PIN` (23) is configured as `INPUT_PULLDOWN`. Ensure hardware places 3.3V on the pin when pressed.
3. **Macro Parsing**: The macro engine `runSequence()` manually parses a CSV-like string.
   - Action Codes: 1=TAP, 2=DELAY, 3=PRESS, 4=RELEASE.
4. **Web Bluetooth**: The web app must challenge the user for device selection. It cannot auto-connect silently.

## Troubleshooting
- **Device Not Found**: Ensure no other device is connected. BLE allows only one concurrent central connection.
- **Serial Output**: Use `Serial.printf` for debugging. Output is visible only when USB is connected.
- **Configuration Storage**: Settings are stored in NVS. If fields are corrupt, consider adding a way to clear NVS `preferences.clear()`.
