# Wireless K120 — Design Document

> **Project:** Convert a wired Logitech K120 USB keyboard into a wireless Bluetooth Low Energy keyboard with deep sleep, LED sync, and dual-mode (wired + wireless) operation.
>
> **Status:** Awaiting parts from AliExpress. Design finalised.

---

## 1. Overview

The Logitech K120 is a standard wired USB membrane keyboard. This project embeds an
ESP32-S3 Zero microcontroller inside the keyboard casing to act as a bridge between the
K120's internal USB controller and a host PC via Bluetooth Low Energy (BLE).

The keyboard is used **a few hours per week** for gaming on a Windows PC (hostname: GPU).
The rest of the time the PC runs as a Linux server accessed remotely from a MacBook Air.
This usage pattern drives every design decision below.

### Host PC Hardware
- **Motherboard:** Gigabyte B560M DS3H
- **WiFi/BT Card:** Intel Wi-Fi 6E AX210 160MHz (Bluetooth 5.3)
- **External antennas:** Connected, positioned straight, outside metal shielding ✅
- **OS (gaming):** Windows 11 Pro 24H2

---

## 2. Bill of Materials

All parts ordered from AliExpress.

| # | Component | Quantity | Est. Price | Purpose |
|---|-----------|----------|-----------|---------|
| 1 | ESP32-S3 Zero (SAMIROB) | 2 | £6.74 | Main MCU (1 spare) |
| 2 | TP4056 USB-C with protection | 2 | £1.48 | LiPo charging + protection |
| 3 | MT3608 DC-DC Boost Converter | 1 | £0.93 | 3.7V → 5V for K120 power |
| 4 | LiPo Battery 801350 3.7V 500mAh | 1 | £5.05 | Power source |
| 5 | Solder Wire Sn99.3Cu0.7 0.8mm 50g | 1 | £4.89 | Lead-free solder (hand soldering) |
| 6 | USB Logic Analyzer 24MHz 8CH | 1 | £4.19 | Protocol debugging (PulseView/sigrok) |
| 7 | Slide switch (SPST, mini) | 1 | ~£0.50 | Hidden battery kill switch (hard reset) |
| | **Total** | | **~£25** | |

### Already Owned
- Logitech K120 keyboard
- Soldering iron
- Solder wick / desoldering braid
- Jumper wires
- Mastech multimeter

---

## 3. Architecture

### 3.1 Signal Flow

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

### 3.2 Power Architecture (UPS-Style)

The keyboard uses an **Uninterruptible Power Supply (UPS) design**: the battery is
**always** the power source. USB-C only charges — it never directly powers the system.

```
USB-C PLUGGED IN:                        USB-C UNPLUGGED:
  USB 5V → TP4056 → charges battery       (nothing changes!)
  Battery → MT3608 → 5V → everything      Battery → MT3608 → 5V → everything
```

**Key benefit:** Plugging or unplugging USB-C causes **zero power glitch**. The ESP32-S3
never reboots, never loses state. It only checks USB **data lines** to switch between
wired and BLE mode. The power path is fully automatic and hardware-managed.

### 3.3 USB-C Port (Single Port Design)

The ESP32-S3 Zero's onboard USB-C port is exposed through the **original K120 cable
hole** (no new hole needed for this). This single port serves **three functions**:

1. **Wired keyboard mode** — plug into PC for BIOS / Linux server access
2. **Battery charging** — 5V from USB-C is tapped via the ESP32-S3's 5V pin to the TP4056
3. **Firmware upload** — connect to PC for Arduino IDE programming during development

### 3.4 Charging Path

```
USB-C cable → ESP32-S3 USB-C port → 5V pin → TP4056 IN+ → LiPo battery
                                     GND    → TP4056 IN-
```

The TP4056 has a factory-installed RPROG resistor that self-limits charging current.
Any charger works safely:
- MacBook USB-C power adapter ✅
- Phone charger ✅
- PC USB 3.0 port (simultaneous wired keyboard + charging) ✅

### 3.5 Casing Modifications

| Modification | Location | Type |
|-------------|----------|------|
| USB-C port access | Back panel (original cable hole) | ♻️ **Reused** — no drilling |
| Kill switch | Underside (hidden, pen-tip accessible) | 🆕 **One small hole** |

**From the outside, the keyboard looks 100% stock.**

---

## 4. Pin Assignments

| ESP32-S3 Zero Pin | Connected To | Function |
|-------------------|-------------|----------|
| GPIO 19 | K120 internal USB white wire | USB Host D- (native PHY) |
| GPIO 20 | K120 internal USB green wire | USB Host D+ (native PHY) |
| GPIO 4 | Scroll Lock key matrix pad | Deep Sleep wake interrupt |
| 5V pin | TP4056 IN+ | Battery charging from USB-C |
| GND | TP4056 IN-, K120 black wire | Common ground |
| USB-C port | External cable | Wired HID / Charging / Programming |

### K120 Internal Cable Colors (Standard USB)

| Wire Color | Function | Destination |
|-----------|----------|-------------|
| Red | VCC (5V) | MT3608 output (5V from boost converter) |
| White | USB D- | GPIO 19 |
| Green | USB D+ | GPIO 20 |
| Black | GND | ESP32-S3 GND |

---

## 5. Firmware Specification

### 5.1 Dual Operating Modes

The firmware auto-detects which mode to use on boot:

| Condition | Mode | Protocol |
|-----------|------|----------|
| USB-C cable plugged into PC | **Wired** | USB HID Device |
| USB-C cable unplugged | **Wireless** | Bluetooth Low Energy HID |

```cpp
void setup() {
    if (USB.isConnected()) {
        // Wired mode — USB HID keyboard device
        USB.begin();
        usbKeyboard.begin();
    } else {
        // Wireless mode — BLE keyboard
        bleKeyboard.begin();
    }
}
```

**Wired mode works in:**
- BIOS ✅ (standard USB HID, no drivers)
- Linux ✅
- Windows ✅

### 5.2 Bluetooth Configuration

| Parameter | Value |
|-----------|-------|
| Device name | Custom (user's choice, e.g. `"Raccoon KB"`) |
| Manufacturer | Custom (e.g. `"Custom DIY"`) |
| Protocol | BLE HID (standard, no drivers required) |
| Compatible with | Intel AX210 (BT 5.3) ✅, any standard BT adapter ✅ |

### 5.3 Scroll Lock Key — Wake + Suppression

Scroll Lock is repurposed as the **keyboard power key**:

| State | Scroll Lock Behaviour |
|-------|----------------------|
| **Deep sleep** | Physical press on GPIO 4 wakes the ESP32-S3 |
| **Active (any mode)** | Scan code is **silently dropped** — never forwarded to Windows |
| **All modes** | Scroll Lock key is fully suppressed at firmware level |

```cpp
// In the USB Host report processing loop:
if (scanCode == HID_KEY_SCROLL_LOCK) {
    // Swallow silently — never forward to BLE or USB HID output
    return;
}
```

### 5.4 LED Sync (Caps Lock + Num Lock)

LEDs are synchronised with the real Windows state via standard BLE HID Output Reports:

```cpp
bleKeyboard.setLEDsChangedHandler([](uint8_t leds) {
    lastLedState = leds;
    if (!ledForcedOff) {
        send_usb_hid_led_report(leds);  // Forward to K120 over USB Host
    }
});
```

**When does Windows send the LED state?**
- On first BLE connection (current state pushed automatically)
- On every Caps Lock / Num Lock toggle

No polling or custom drivers required — standard BLE HID handles it.

### 5.5 LED Status Sequences

Visual feedback using the K120's Caps Lock and Num Lock LEDs:

| Event | LED Sequence | Meaning |
|-------|-------------|---------|
| **Waking up** | Num → Caps → both OFF | "I'm alive" |
| **BLE connecting** | Alternate blink (Num/Caps, 500ms) | "Searching for PC..." |
| **BLE connected** | Both flash together × 2, then restore real state | "Ready!" |
| **Going to sleep** | Both slow blink × 3 → OFF | "Goodnight" |

### 5.6 Power Management

#### Idle LED Timeout (30 minutes)
After 30 minutes of no keypresses, all LEDs are turned off to save power:

```cpp
#define LED_IDLE_TIMEOUT_MS  (30UL * 60UL * 1000UL)

if (!ledForcedOff && (now - lastKeypressAt > LED_IDLE_TIMEOUT_MS)) {
    ledForcedOff = true;
    send_usb_hid_led_report(0x00);  // All LEDs off
}
```

On first keypress after idle, the real LED state is instantly restored from the
cached `lastLedState` value.

#### Deep Sleep (30 minutes idle)
After 30 minutes of no keypresses, the ESP32-S3 enters deep sleep:

```cpp
esp_sleep_enable_ext0_wakeup(GPIO_NUM_4, 0);  // Wake on Scroll Lock press
esp_deep_sleep_start();
```

| Parameter | Value |
|-----------|-------|
| Idle timeout | 30 minutes |
| Deep sleep current | ~0.02 mA |
| Standby battery life (500mAh) | **Months** |
| Wake source | Scroll Lock key (GPIO 4) |
| Reconnect time after wake | ~3 seconds |

### 5.7 Libraries

| Library | Purpose |
|---------|---------|
| `usb/usb_host.h` (Espressif) | Read K120 keystrokes as USB Host |
| `ESP32-BLE-Keyboard` (T-vK) | BLE HID keyboard emulation |
| `USB.h` + `USBHIDKeyboard.h` | Wired USB HID Device mode |

---

## 6. Hardware Assembly Notes

### 6.1 Internal Layout
The K120 has a large empty plastic shell with plenty of room for components.
Place components to keep the ESP32-S3 antenna area clear of any metal.

### 6.2 Antenna
- The ESP32-S3 Zero has a **built-in PCB trace antenna** — no external antenna needed.
- ABS plastic casing is RF-transparent at 2.4 GHz.
- **Do NOT place** the antenna area against any metal support plates inside the keyboard.

### 6.3 Scroll Lock Wake Wire
- Open the K120 and identify the Scroll Lock key's contact pad on the membrane PCB.
- Solder **one thin wire** from the Scroll Lock matrix contact to GPIO 4 on the ESP32-S3.
- This wire is the deep sleep wake interrupt source.

### 6.4 Battery Safety
- Solder LiPo wires directly to TP4056 BAT+/BAT- pads (no JST connector needed).
- **Solder quickly** (1–2 seconds per joint) when working directly on LiPo terminals.
- The TP4056 module with protection board prevents over-discharge, over-charge, and short circuit.

### 6.5 USB-C Port Exposure
- Route the ESP32-S3 Zero's USB-C port through the **original K120 cable hole** — no new drilling needed.
- This is the only external port on the finished keyboard.

### 6.6 Hidden Battery Kill Switch (Hard Reset)
- Wire a **mini SPST slide switch** in series on the **battery positive line** between the LiPo BAT+ and the TP4056 BAT+ pad.
- Mount in a discreet but accessible location (e.g., behind a sticker on the underside, or inside the battery compartment reachable with a pen tip).
- **Switch OFF** = battery fully disconnected → complete power kill → hard reset.
- **Switch ON** = normal operation.
- Use case: firmware crash, BLE pairing stuck, or any unrecoverable state where Scroll Lock wake does not respond.
- This is a **last resort** — the firmware should handle all normal recovery via deep sleep wake and BLE reconnection.

---

## 7. Development Environment

### 7.1 Arduino IDE Setup
1. Add ESP32 board URL: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
2. Install `esp32` board package by Espressif.
3. Select board: **ESP32S3 Dev Module**
4. Settings:
   - USB CDC On Boot: **Enabled**
   - Upload Mode: **UART/Hardware CDC**

### 7.2 Flashing (Bootloader Mode)
1. Hold **BOOT** button on ESP32-S3 Zero.
2. Press and release **RESET** button.
3. Release **BOOT** button.
4. Select COM port in Arduino IDE → Upload.

No external programmer needed — USB-C cable only.

### 7.3 Debugging Tools
- **USB Logic Analyzer 24MHz** + **PulseView (sigrok)** — UART / I2C / SPI protocol decode
- **OpenHantek6022** — if oscilloscope mode ever needed (not purchased)
- **Arduino Serial Monitor** — via USB-C CDC
- **Mastech Multimeter** — voltage verification

---

## 8. Software to Install on PC

| Software | Purpose | Notes |
|----------|---------|-------|
| PulseView (sigrok) | Logic analyzer protocol decode | Free, open-source |
| Zadig | USB driver replacement | Needed once to enable sigrok on Windows |
| Arduino IDE | Firmware development | Already installed |

---

## 9. PC-Side Configuration

### 9.1 Intel AX210 Bluetooth Power Management
Disable Bluetooth sleep to prevent 2-second reconnect lag:

1. Open **Device Manager**
2. Expand **Bluetooth**
3. Right-click **Intel(R) Wireless Bluetooth(R)** → Properties
4. **Power Management** tab → Uncheck *"Allow the computer to turn off this device to save power"*

### 9.2 Gigabyte B560M DS3H BIOS
- Ensure **USB Legacy Support** is **Enabled** (default on Gigabyte boards).
- This ensures the wired USB HID keyboard works during BIOS/POST.

---

## 10. Usage Scenarios

### 10.1 Gaming Session (Weekly)
1. Sit down at PC.
2. Press **Scroll Lock** on the K120.
3. LED wake sequence plays (Num → Caps blink).
4. BLE connects to Windows in ~3 seconds.
5. LED connected sequence plays (both flash × 2).
6. Game. 🎮
7. Walk away.
8. After 30 minutes idle → deep sleep automatically.

### 10.2 BIOS / Linux Server Access (Rare)
1. Plug USB-C cable from K120 into PC rear USB port.
2. ESP32-S3 auto-detects wired mode.
3. Works instantly as standard USB keyboard — BIOS, GRUB, Linux, everything.
4. When done, unplug cable.

### 10.3 Charging (Monthly or less)
1. Plug USB-C cable from K120 into MacBook charger (or any USB charger).
2. TP4056 charges LiPo safely via tapped 5V line.
3. Blue LED on TP4056 = charging, Green LED = full.
4. Unplug.

---

## 11. Known Limitations & Accepted Trade-offs

| Limitation | Status | Notes |
|-----------|--------|-------|
| Scroll Lock key disabled | ✅ Accepted | Repurposed as wake key, never used otherwise |
| ~3 second reconnect after deep sleep | ✅ Accepted | Only happens at start of gaming session |
| BLE does not work in BIOS | ✅ Solved | Wired USB-C mode auto-detected |
| Logitech Unifying/Bolt receivers incompatible | ✅ Accepted | Proprietary protocols — BLE is superior |
| Lead-free solder (not 63/37) | ✅ Accepted | Ordered Sn99.3Cu0.7 — works fine for this project |

---

## 12. Future Enhancements (Optional)

- [ ] Battery level reporting to Windows via BLE HID battery service
- [ ] Multi-device pairing (switch between Windows PC and MacBook via key combo)
- [ ] Custom key remapping (e.g., remap Scroll Lock LED to show BLE connection status)
- [ ] OTA firmware updates over BLE (avoid opening the casing)
