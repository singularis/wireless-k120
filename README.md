# Wireless K120 Bluetooth Keyboard Mod

Convert a stock wired Logitech K120 USB keyboard into a wireless Bluetooth Low Energy (BLE) keyboard with deep sleep support, hardware-driven LED status synchronization, and dual-mode (wired + wireless) auto-detection.

---

## 📌 Overview

The Logitech K120 is a ubiquitous, reliable wired USB membrane keyboard. This project embeds an **ESP32-S3 Zero** microcontroller inside the spacious keyboard casing, transforming it into a wireless BLE keyboard while keeping its stock appearance and maintaining the ability to work as a wired keyboard when connected via USB-C.

---

## ⚡ Key Features

*   **Dual-Mode Operation:** Auto-detects whether a USB-C cable is connected to a PC host. Works as standard USB HID in wired mode (essential for BIOS access or Linux server terminals) and automatically switches to BLE mode when unplugged.
*   **UPS-Style Power Architecture:** The system is continuously powered from the battery boosted to 5V (via MT3608). The USB-C port is only tapped to charge the battery via a TP4056 charger module. This prevents any power drop-outs or reboots when plugging/unplugging the cable.
*   **Deep Sleep Power Management:** Automatically transitions to deep sleep after 30 minutes of idle time (~0.02 mA draw). Standby battery life from a 500mAh LiPo lasts for months.
*   **Scroll Lock Wake:** Repurposes the Scroll Lock matrix pad to act as the hardware wake trigger (GPIO 4). The key's scan code is completely suppressed at the firmware level so it never registers on the host computer.
*   **LED Synchronization:** Syncs Caps Lock and Num Lock LEDs from the host OS status via BLE output reports without needing custom PC drivers.
*   **Hidden Kill Switch:** Includes a mini SPST slide switch on the battery positive line, accessible via a pen-tip on the underside, to completely power-cycle the board in case of recovery/development lockups.

---

## 📐 Architecture & Signal Flow

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
│            │ KILL SW │  │                 │                      │
│            └────┬────┘  │                 │                      │
│            ┌────┴────┐  │                 │                      │
│            │ TP4056  │←─┼── 5V (from ESP32-S3 5V pin,           │
│            │ Charger │  │      only when USB-C plugged in)       │
│            └─────────┘  │                 │                      │
│                         │                 │                      │
│  ┌──────────────┐       │                 │                      │
│  │ Scroll Lock  │── wire to GPIO 4 ── Deep Sleep wake source    │
│  │ key matrix   │                                                │
│  └──────────────┘                                                │
└──────────────────────────────────────────────────────────────────┘
```

For a detailed circuit schematic, view [schematic.html](schematic.html).

---

## 🛠️ Bill of Materials (BOM)

| Component | Quantity | Purpose |
| :--- | :---: | :--- |
| **ESP32-S3 Zero** | 1 | Main MCU with native USB-OTG and BLE support |
| **TP4056 USB-C Charger** | 1 | LiPo battery charging with built-in protection |
| **MT3608 Boost Converter** | 1 | DC-DC Step Up (3.7V → 5V) to power the K120 controller |
| **LiPo Battery (801350)** | 1 | 3.7V 500mAh battery |
| **Mini SPST Slide Switch** | 1 | Hard reset/power kill switch |
| **Logitech K120** | 1 | Target wired USB keyboard |

---

## 📌 Pin Assignments

| ESP32-S3 Zero Pin | Connected To | Function |
| :--- | :--- | :--- |
| **GPIO 19** | K120 USB White Wire | USB Host D- (native PHY) |
| **GPIO 20** | K120 USB Green Wire | USB Host D+ (native PHY) |
| **GPIO 4** | Scroll Lock membrane pad | Deep Sleep wake interrupt |
| **5V Pin** | TP4056 IN+ | Charges battery when USB-C is plugged in |
| **GND** | TP4056 IN-, K120 Black | Common ground bus |
| **USB-C Port** | External PC Host/Charger | Wired HID / Power Charging / Programming |

### K120 Internal Cable Colors
*   **Red:** VCC (5V from MT3608 Boost output)
*   **White:** USB D- (to GPIO 19)
*   **Green:** USB D+ (to GPIO 20)
*   **Black:** GND (to ESP32-S3 common GND)

---

## 💾 Firmware Details

The firmware utilizes standard Espressif and community libraries to bridge the USB host reports to BLE keyboard packets. 

*   `usb/usb_host.h` parses reports from the native USB controller pins.
*   `ESP32-BLE-Keyboard` processes and sends Bluetooth HID signals.
*   `USB.h` + `USBHIDKeyboard.h` manage wired USB emulation.

### Operating Mode Auto-Detection

```cpp
void setup() {
    if (USB.isConnected()) {
        // Wired mode — standard USB HID keyboard device
        USB.begin();
        usbKeyboard.begin();
    } else {
        // Wireless mode — BLE keyboard
        bleKeyboard.begin();
    }
}
```

### Sleep Configuration

The controller enables ext0 sleep trigger on GPIO 4 (Scroll Lock contact pad):
```cpp
esp_sleep_enable_ext0_wakeup(GPIO_NUM_4, 0); // Wake on Scroll Lock matrix press
esp_deep_sleep_start();
```

---

## 🔧 Installation & Customization

### PC configuration
To avoid reconnect lag on Windows machines using an **Intel AX210 Bluetooth card**:
1. Open **Device Manager** -> **Bluetooth** -> **Intel(R) Wireless Bluetooth(R)**.
2. In the **Power Management** tab, uncheck *"Allow the computer to turn off this device to save power"*.

### BIOS Setup
Enable **USB Legacy Support** in your motherboard settings (e.g. Gigabyte B560M DS3H) to ensure that the wired USB-C mode works during POST / GRUB.

---

## 📄 Documentation Links
*   Detailed technical explainer: [keyboard_mod_explainer.html](keyboard_mod_explainer.html)
*   Full hardware design notes: [DESIGN.md](DESIGN.md)
*   Visual interactive schematic: [schematic.html](schematic.html)
*   Arduino IDE Setup & Tools: [docs/ARDUINO_SETUP.md](docs/ARDUINO_SETUP.md)
*   Dual-Mode Implementation Details: [docs/DUAL_MODE_HANDOVER.md](docs/DUAL_MODE_HANDOVER.md)
*   Example Dual-Mode Sketch: [examples/dual_mode_test/dual_mode_test.ino](examples/dual_mode_test/dual_mode_test.ino)
