# Logi Retro Keyboard - Agent Handover Document

**Project Status:** Initial Prototyping & Environment Setup Complete
**Current Goal:** Reading physical key inputs and mapping them to the dual-mode keyboard output.

This document serves as a comprehensive handover for the next agent working on this project. It details the precise environment, workarounds, and current state of the codebase.

## 1. Hardware & Environment
- **Target Board:** ESP32-S3 (specifically ESP32-S3 Zero).
- **Core Platform:** Arduino ESP32 Core v3.0.0+ (underlying ESP-IDF 5.1).
- **Compiler:** `arduino-cli` (FQBN: `esp32:esp32:esp32s3`).
- **Mac Host:** Apple Silicon (M-series). Rosetta 2 is installed to resolve previous `bad CPU type` ctags execution errors.

## 2. Dependencies & Patches
To achieve a "Dual-Mode" keyboard (Native USB HID + Bluetooth BLE HID) on the ESP32-S3 Core v3.x, significant manual patching was required because the legacy `ESP32-BLE-Keyboard` library is incompatible out-of-the-box.

### `NimBLE-Arduino`
- **Version Installed:** `2.5.0`
- **Why:** The built-in Bluedroid stack in ESP32 Core v3 crashes on boot for this library. NimBLE is required.

### `ESP32-BLE-Keyboard` (T-vK)
This library was manually patched at `/Users/dante/Documents/Arduino/libraries/ESP32_BLE_Keyboard` to support NimBLE 2.5.0:
1. **Enabled NimBLE:** Uncommented `#define USE_NIMBLE` in `BleKeyboard.h`.
2. **Missing Headers:** Added `#include "NimBLEDevice.h"` and `#include "NimBLEServer.h"` in `BleKeyboard.h`.
3. **NimBLE 2.x Signatures:** Updated `onConnect`, `onDisconnect`, and `onWrite` in both the header and `.cpp` file to accept the new `NimBLEConnInfo& connInfo` argument.
4. **NimBLE 2.x Method Names:** Patched `BleKeyboard.cpp` to use modern NimBLE 2.x API calls (e.g., `getInputReport()`, `setManufacturer()`, `setPnp()`, `setHidInfo()`, `setReportMap()`, `getHidService()`, `enableScanResponse()`).

## 3. The `logi_retro_keyboard.ino` Sketch
The current sketch successfully implements the Dual-Mode architecture:
- It instantiates both `USBHIDKeyboard` (for Native USB) and `BleKeyboard`.
- **CRITICAL WORKAROUND:** Because both libraries define `KeyReport`, `MediaKeyReport`, and various `KEY_*` macros, the sketch uses a macro renaming trick to prevent compilation conflicts:
  ```cpp
  #define KeyReport BleKeyReport
  #define MediaKeyReport BleMediaKeyReport
  #include <BleKeyboard.h>
  #undef KeyReport
  #undef MediaKeyReport
  
  #include "USB.h"
  #include "USBHIDKeyboard.h"
  ```
- **Current Behavior:** The sketch monitors `GPIO 0` (the physical BOOT button with `INPUT_PULLUP`). When pressed, it broadcasts `"Dual-Mode Keyboard works!\n"` via both Native USB and BLE simultaneously.

## 4. Next Steps
The core communication protocols (USB and BLE) are 100% stable and tested. The next logical step is to map the physical Logitech retro keyboard hardware matrix to GPIO pins, implement key scanning/debouncing, and map those hardware scans to the existing `usbKeyboard.press()` / `bleKeyboard.press()` interface.

## 5. Verified Bluetooth-Only Baseline
For reference, the following is the minimal verified script that successfully broadcasts as a Bluetooth keyboard and types a test message (which proves the NimBLE 2.5 manual patches are working):

```cpp
#include <BleKeyboard.h>

// Name the device "logi_retro_keyboard" so it's easy to find on your Mac
BleKeyboard bleKeyboard("logi_retro_keyboard", "Logitech", 100);

void setup() {
  Serial.begin(115200);
  
  Serial.println("Starting Bluetooth...");
  bleKeyboard.begin();
  Serial.println("Bluetooth is broadcasting! Open your Mac's Bluetooth settings and connect.");
}

void loop() {
  // Check if the Mac has connected to the ESP32
  if (bleKeyboard.isConnected()) {
    Serial.println("Mac is connected! Typing test message...");
    
    // Type a test message
    bleKeyboard.print("Bluetooth is working! ");
    
    // Press the Enter key
    bleKeyboard.write(KEY_RETURN);
    
    // Wait 5 seconds before typing it again
    delay(5000);
  } else {
    // If not connected, do nothing and wait
    delay(1000);
  }
}
```
