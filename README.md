# Wireless K120 Bluetooth Keyboard Mod

Convert a stock wired Logitech K120 USB keyboard into a wireless Bluetooth Low Energy (BLE) keyboard with hardware-driven LED status synchronization, dual-mode (wired + wireless) auto-detection, and always-on power controlled by a battery kill switch.

---

## Overview

The Logitech K120 is a ubiquitous, reliable wired USB membrane keyboard. This project embeds an **ESP32-S3 Zero** microcontroller inside the spacious keyboard casing, transforming it into a wireless BLE keyboard while keeping its stock appearance and maintaining the ability to work as a wired keyboard when connected via USB-C.

The board stays **always powered** while the kill switch is on. There is no deep sleep and no Scroll Lock wake — turn power off with the hardware battery switch.

---

## Key Features

*   **Dual-Mode Operation:** Auto-detects whether a USB-C cable is connected to a PC host. Works as standard USB HID in wired mode (essential for BIOS access or Linux server terminals) and automatically switches to BLE mode when unplugged.
*   **UPS-Style Power Architecture:** The system is continuously powered from the battery boosted to 5V (via MT3608). The USB-C port is only tapped to charge the battery via a TP4056 charger module. This prevents any power drop-outs or reboots when plugging/unplugging the cable.
*   **Always-On + Kill Switch:** Firmware does not enter deep sleep. Power on/off is the hidden mini SPST slide switch on the battery positive line (underside, pen-tip accessible).
*   **Idle LED Timeout:** After 30 minutes with no keypresses, Caps/Num Lock LEDs turn off to save a little power; the ESP32-S3 and BLE keep running. LEDs restore on the next keypress.
*   **Normal Scroll Lock:** Scroll Lock is forwarded to the host like any other key (not used as a wake key).
*   **LED Synchronization:** Syncs Caps Lock and Num Lock LEDs from the host OS status via BLE output reports without needing custom PC drivers.

---

## Architecture & Signal Flow

```
┌──────────────────────────────────────────────────────────────────┐
│                      LOGITECH K120 CASING                        │
│                                                                   │
│  ┌──────────┐      USB D-/D+      ┌───────────────┐              │
│  │  K120    │ ───────────────→    │  ESP32-S3     │              │
│  │ Internal │     (GPIO 19/20)    │  Zero         │              │
│  │ USB Ctrl │                     │               │──→ BLE ──→ PC │
│  └──────────┘                     │  USB Host     │              │
│       ↑                           │  + BLE HID    │              │
│       │ 5V power                  │  + USB Device │              │
│       │                           └───────┬───────┘              │
│  ┌────┴────┐                              │                      │
│  │ MT3608  │←── 3.7V ───┐                 │ USB-C port           │
│  │ Boost   │            │                 │ (reuses original     │
│  │ 3.7→5V  │            │                 │  K120 cable hole)    │
│  └────┬────┘            │                 │                      │
│       │                 │                 │                      │
│       └──→ 5V ──→ ESP32-S3 (5V pin)      │                      │
│                         │                 │                      │
│              ┌──────────┴──────────┐      │                      │
│              │   LiPo 3.7V 500mAh  │      │                      │
│              │        BT1          │      │                      │
│              └──────────┬──────────┘      │                      │
│                 ┌───────┤                 │                      │
│            ┌────┴────┐  │                 │                      │
│            │ SW1     │  │                 │                      │
│            │ KILL SW │  │  ← power on/off │                      │
│            └────┬────┘  │                 │                      │
│            ┌────┴────┐  │                 │                      │
│            │ TP4056  │←─┼── 5V (from ESP32-S3 5V pin,           │
│            │ Charger │  │      only when USB-C plugged in)       │
│            └─────────┘  │                 │                      │
└──────────────────────────────────────────────────────────────────┘
```

For a detailed circuit schematic, view [schematic.html](schematic.html).

---

## Bill of Materials (BOM)

| Component | Quantity | Purpose |
| :--- | :---: | :--- |
| **ESP32-S3 Zero** | 1 | Main MCU with native USB-OTG and BLE support |
| **TP4056 USB-C Charger** | 1 | LiPo battery charging with built-in protection |
| **MT3608 Boost Converter** | 1 | DC-DC Step Up (3.7V → 5V) to power the K120 controller |
| **LiPo Battery (801350)** | 1 | 3.7V 500mAh battery |
| **Mini SPST Slide Switch** | 1 | Battery power on/off (primary power control) |
| **Logitech K120** | 1 | Target wired USB keyboard |

---

## Pin Assignments

| ESP32-S3 Zero Pin | Connected To | Function |
| :--- | :--- | :--- |
| **GPIO 19** | K120 USB White Wire | USB Host D- (native PHY) |
| **GPIO 20** | K120 USB Green Wire | USB Host D+ (native PHY) |
| **5V Pin** | TP4056 IN+ | Charges battery when USB-C is plugged in |
| **GND** | TP4056 IN-, K120 Black | Common ground bus |
| **USB-C Port** | External PC Host/Charger | Wired HID / Power Charging / Programming |

GPIO 4 / Scroll Lock wake wiring is **not used**.

### K120 Internal Cable Colors
*   **Red:** VCC (5V from MT3608 Boost output)
*   **White:** USB D- (to GPIO 19)
*   **Green:** USB D+ (to GPIO 20)
*   **Black:** GND (to ESP32-S3 common GND)

---

## Firmware Details

Main sketch: [`wireless_k120/wireless_k120.ino`](wireless_k120/wireless_k120.ino)

The firmware bridges USB Host reports from the K120 to BLE HID:

*   **EspUsbHost** — USB Host HID input from GPIO 19/20
*   **ESP32-BLE-Keyboard** — BLE HID keyboard output
*   Optional dual-mode USB Device HID (see examples / DESIGN.md)

### Power behaviour

```cpp
// Always-on while the battery kill switch is ON.
// No esp_deep_sleep_start() / no Scroll Lock wake on GPIO 4.
// After 30 minutes idle: keyboard LEDs off only; MCU stays running.
```

### Debug

Set `DEBUG_KEYBOARD` to `1` in the sketch to dump every HID signal to Serial (115200).

---

## Installation & Customization

### PC configuration
To avoid reconnect lag on Windows machines using an **Intel AX210 Bluetooth card**:
1. Open **Device Manager** -> **Bluetooth** -> **Intel(R) Wireless Bluetooth(R)**.
2. In the **Power Management** tab, uncheck *"Allow the computer to turn off this device to save power"*.

### BIOS Setup
Enable **USB Legacy Support** in your motherboard settings (e.g. Gigabyte B560M DS3H) to ensure that the wired USB-C mode works during POST / GRUB.

---

## Documentation Links
*   Detailed technical explainer: [keyboard_mod_explainer.html](keyboard_mod_explainer.html)
*   Full hardware design notes: [DESIGN.md](DESIGN.md)
*   Visual interactive schematic: [schematic.html](schematic.html)
*   Arduino IDE Setup & Tools: [docs/ARDUINO_SETUP.md](docs/ARDUINO_SETUP.md)
*   Dual-Mode Implementation Details: [docs/DUAL_MODE_HANDOVER.md](docs/DUAL_MODE_HANDOVER.md)
*   Final firmware: [wireless_k120/wireless_k120.ino](wireless_k120/wireless_k120.ino)
*   Example Dual-Mode Sketch: [examples/dual_mode_test/dual_mode_test.ino](examples/dual_mode_test/dual_mode_test.ino)
