# FootSwitch Project Instructions

## Project Overview
This project builds a configurable, **Multi-Switch** Bluetooth Low Energy (BLE) FootSwitch using an ESP32 (Lolin D32 Pro) and a companion Web Bluetooth configuration app.

### Architecture
- **Firmware**: PlatformIO C++ project. Acts as a composite BLE device:
  - **HID Device**: Standard BLE Keyboard.
  - **GATT Server**: Custom service for configuring multiple switches dynamically.
- **Web App**: A "Manager" dashboard (`webapp/index.html`) to add, configure, and remove switches on different GPIO pins.

## Development Environment
- **Platform**: PlatformIO
- **Framework**: Arduino
- **Board**: `lolin_d32_pro`
- **Key Dependencies**: `ESP32 BLE Keyboard`

## Build & Deploy
- **Build**: `pio run`
- **Upload**: `pio run -t upload`
- **Monitor**: `pio device monitor` (115200)
- **Web App**: Open `webapp/index.html` in Chrome/Edge.

## Codebase Map

### Firmware (`src/`)
- **`Switch.h / .cpp`**:
  - Represents a single physical switch.
  - Handles `pinMode`, `digitalRead`, debounce (50ms).
  - Executes Macros/Combos via the parent `BleKeyboard` reference.
- **`ConfigKeyboard.h`**:
  - **Manager Class**: Inherits from `BleKeyboard`.
  - Holds a `std::vector<Switch*>` of active switches.
  - **NVS Storage**: Manages indexed preferences (`sw0_pin`, `sw0_mode`...).
  - **BLE Protocol**: Parses commands to Add/Delete/Config switches.
- **`main.cpp`**:
  - Simple loop calls `kb.tick()` to update all switches.
  - Handles Battery monitoring (Pin 35).

### Configuration (`webapp/`)
- **`index.html`**:
  - **Dashboard**: Lists active switches + Add/Clear buttons.
  - **Editor**: Configures Mode 0 (Momentary) or Mode 1 (Macro) for specific switches.
  - **Protocol**:
    - `ADD:PIN` : Add new switch.
    - `CFG:ID:PIN:MODE#P1#P2` : Configure switch.
    - `CLR` : Factory reset (remove all).

## Data Protocol
- **Service UUID**: `4fafc201-1fb5-459e-8fcc-c5c9c331914b`
- **Char UUID**: `beb5483e-36e1-4688-b7f5-ea07361b26a8`
- **Storage (NVS)**:
  - `count`: Number of switches.
  - `sw{i}_pin`: GPIO Pin.
  - `sw{i}_mode`: 0=Momentary, 1=Macro.
  - `sw{i}_pl1`: Payload 1 (Combos/Press Seq).
  - `sw{i}_pl2`: Payload 2 (Release Seq).

## Development Patterns
1. **Dynamic Instantiation**: Switches are created at runtime based on NVS config or BLE commands.
2. **Safe Pins**: Use ESP32 pins with internal Pull-Up/Down capabilities (Avoid 34-39).
   - Recommended: 4, 13-19, 21-23, 25-27, 32-33.
3. **Macro Logic**: 
   - Operations: 1=TAP, 2=DELAY, 3=PRESS, 4=RELEASE.
   - Format: CSV string parsed by `Switch::runSequence`.
4. **Bluetooth Stability**:
   - Advertising is **stopped** upon connection to prevent "Leaking Connection" errors in BlueZ.
   - Host OS caching (GATT Table) often requires **"Forgetting"** the device after firmware updates.

## Troubleshooting
- **"Leaking Connection" / "Services not resolved"**: The OS has a stale cache or the device advertised while connected. **Forget** the device in OS settings and re-pair.
- **Web Bluetooth on Linux**:
  - **Chrome**: Enable `#enable-experimental-web-platform-features`.
  - **Snap/Flatpak**: Sandboxing blocks Bluetooth. Use `.deb` install or grant permissions (`snap connect chromium:bluez`).
