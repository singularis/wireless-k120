/*
 * Wireless K120 — final firmware
 *
 * K120 USB (GPIO 19/20) → ESP32-S3 USB Host → BLE HID keyboard
 *
 * Features (see DESIGN.md):
Bluetooth is working! 
Bluetooth is working! 
 *  - Caps/Num Lock LED sync from the BLE host
 *  - LED status sequences (boot / connecting / connected)
 *  - 30 min idle → keyboard LEDs off (power stays on)
 *  - Power off via hardware battery kill switch
 *  - DEBUG_KEYBOARD dumps every HID signal to Serial
 *
 * Board: ESP32S3 Dev Module
 *  - USB CDC On Boot: Disabled  (USB Host owns the native PHY)
 *  - USB Mode: Hardware CDC and JTAG
 *
 * Serial debug: 115200 baud on UART0 (TX/RX). While USB Host is running,
 * do not plug the ESP USB-C data lines into a PC — they share GPIO 19/20
 * with the K120. Flash in bootloader mode (BOOT + RESET) as usual.
 */

// Set to 1 to print every keyboard HID signal to Serial
#ifndef DEBUG_KEYBOARD
#define DEBUG_KEYBOARD 1
#endif

#include <BleKeyboard.h>
#include <EspUsbHost.h>

// ---------------------------------------------------------------------------
// Config
// ---------------------------------------------------------------------------
static const char *kBleName = "logi_retro_keyboard";
static const char *kBleManufacturer = "Logitech";

static const uint32_t kIdleTimeoutMs = 30UL * 60UL * 1000UL;
static const uint32_t kLedBlinkMs = 500;

// ---------------------------------------------------------------------------
// BLE keyboard with LED output-report callback
// ---------------------------------------------------------------------------
class WirelessBleKeyboard : public BleKeyboard {
 public:
  using LedHandler = void (*)(uint8_t leds);

  WirelessBleKeyboard(std::string name, std::string manufacturer, uint8_t battery)
      : BleKeyboard(std::move(name), std::move(manufacturer), battery) {}

  void setLedHandler(LedHandler handler) { ledHandler_ = handler; }

 protected:
  void onWrite(BLECharacteristic *characteristic, NimBLEConnInfo &connInfo) override {
    const std::string &value = characteristic->getValue();
    if (!value.empty() && ledHandler_) {
      ledHandler_(static_cast<uint8_t>(value[0]));
    }
    BleKeyboard::onWrite(characteristic, connInfo);
  }

 private:
  LedHandler ledHandler_ = nullptr;
};

EspUsbHost usbHost;
WirelessBleKeyboard bleKeyboard(kBleName, kBleManufacturer, 100);

// ---------------------------------------------------------------------------
// Runtime state
// ---------------------------------------------------------------------------
volatile uint32_t lastActivityMs = 0;
uint8_t lastLedState = 0;
bool ledForcedOff = false;
bool k120Present = false;
bool wasBleConnected = false;

enum class LedAnim : uint8_t {
  None = 0,
  Boot,
  BleSearch,
  BleConnected,
};

LedAnim ledAnim = LedAnim::None;
uint32_t ledAnimStartMs = 0;
uint8_t ledAnimStep = 0;

// ---------------------------------------------------------------------------
// Debug helpers
// ---------------------------------------------------------------------------
#if DEBUG_KEYBOARD
static void debugPrintModifiers(uint8_t modifiers) {
  static const char *names[] = {"LCTRL", "LSHIFT", "LALT", "LGUI",
                                "RCTRL", "RSHIFT", "RALT", "RGUI"};
  if (modifiers == 0) {
    Serial.print("none");
    return;
  }
  bool first = true;
  for (int i = 0; i < 8; i++) {
    if (modifiers & (1 << i)) {
      if (!first) Serial.print('+');
      Serial.print(names[i]);
      first = false;
    }
  }
}

static void debugRawReport(const char *tag, const uint8_t *data, size_t length) {
  Serial.printf("[DBG] %s len=%u raw:", tag, static_cast<unsigned>(length));
  for (size_t i = 0; i < length; i++) {
    Serial.printf(" %02X", data[i]);
  }
  Serial.println();
}
#endif

// ---------------------------------------------------------------------------
// K120 LED control (USB HID output report via host)
// ---------------------------------------------------------------------------
static void applyKeyboardLeds(uint8_t leds) {
  const bool num = leds & 0x01;
  const bool caps = leds & 0x02;
  const bool scroll = leds & 0x04;
  usbHost.setKeyboardLeds(num, caps, scroll);
}

static void setLedsRaw(uint8_t leds) {
  applyKeyboardLeds(leds);
}

static void restoreHostLeds() {
  if (!ledForcedOff) {
    applyKeyboardLeds(lastLedState);
  }
}

static void onBleLedsChanged(uint8_t leds) {
  lastLedState = leds;
#if DEBUG_KEYBOARD
  Serial.printf("[DBG] BLE LED report: 0x%02X (Num=%d Caps=%d Scroll=%d)\n", leds,
                (leds & 0x01) != 0, (leds & 0x02) != 0, (leds & 0x04) != 0);
#endif
  if (!ledForcedOff && ledAnim == LedAnim::None) {
    applyKeyboardLeds(leds);
  }
}

// ---------------------------------------------------------------------------
// LED status sequences
// ---------------------------------------------------------------------------
static void startLedAnim(LedAnim anim) {
  ledAnim = anim;
  ledAnimStartMs = millis();
  ledAnimStep = 0;
}

static void serviceLedAnim() {
  if (ledAnim == LedAnim::None) return;

  const uint32_t now = millis();
  const uint32_t elapsed = now - ledAnimStartMs;

  switch (ledAnim) {
    case LedAnim::Boot:
      // Num → Caps → both OFF
      if (ledAnimStep == 0) {
        setLedsRaw(0x01);
        ledAnimStep = 1;
        ledAnimStartMs = now;
      } else if (ledAnimStep == 1 && elapsed >= 200) {
        setLedsRaw(0x02);
        ledAnimStep = 2;
        ledAnimStartMs = now;
      } else if (ledAnimStep == 2 && elapsed >= 200) {
        setLedsRaw(0x00);
        ledAnim = LedAnim::None;
        restoreHostLeds();
      }
      break;

    case LedAnim::BleSearch:
      // Alternate Num/Caps every 500 ms
      if ((elapsed / kLedBlinkMs) != ledAnimStep) {
        ledAnimStep = elapsed / kLedBlinkMs;
        setLedsRaw((ledAnimStep & 1) ? 0x02 : 0x01);
      }
      break;

    case LedAnim::BleConnected:
      // Both flash together × 2, then restore
      if (ledAnimStep == 0) {
        setLedsRaw(0x03);
        ledAnimStep = 1;
        ledAnimStartMs = now;
      } else if (ledAnimStep == 1 && elapsed >= 150) {
        setLedsRaw(0x00);
        ledAnimStep = 2;
        ledAnimStartMs = now;
      } else if (ledAnimStep == 2 && elapsed >= 150) {
        setLedsRaw(0x03);
        ledAnimStep = 3;
        ledAnimStartMs = now;
      } else if (ledAnimStep == 3 && elapsed >= 150) {
        setLedsRaw(0x00);
        ledAnim = LedAnim::None;
        restoreHostLeds();
      }
      break;

    default:
      break;
  }
}

// ---------------------------------------------------------------------------
// Boot-protocol report bridge
// ---------------------------------------------------------------------------
static bool extractBootReport(const uint8_t *data, size_t length,
                              const uint8_t **outReport) {
  if (data == nullptr || length < 8) return false;
  // Some keyboards prefix a report ID
  if (length >= 9) {
    *outReport = data + 1;
    return true;
  }
  *outReport = data;
  return true;
}

static void forwardBootReport(const uint8_t *boot) {
  KeyReport report;
  report.modifiers = boot[0];
  report.reserved = 0;
  memcpy(report.keys, &boot[2], 6);

  const bool hasActivity = report.modifiers != 0 || report.keys[0] != 0 ||
                           report.keys[1] != 0 || report.keys[2] != 0 ||
                           report.keys[3] != 0 || report.keys[4] != 0 ||
                           report.keys[5] != 0;

  if (hasActivity) {
    lastActivityMs = millis();
    if (ledForcedOff) {
      ledForcedOff = false;
      if (ledAnim == LedAnim::None) {
        applyKeyboardLeds(lastLedState);
      }
    }
  }

  if (!bleKeyboard.isConnected()) {
#if DEBUG_KEYBOARD
    Serial.println("[DBG] BLE not connected — report not forwarded");
#endif
    return;
  }

  bleKeyboard.sendReport(&report);
}

// ---------------------------------------------------------------------------
// USB Host callbacks
// ---------------------------------------------------------------------------
static void onUsbDeviceConnected(const EspUsbHostDeviceInfo &device) {
  k120Present = true;
  lastActivityMs = millis();
#if DEBUG_KEYBOARD
  Serial.print("[DBG] USB device connected: ");
  espUsbHostPrint(device);
#else
  (void)device;
  Serial.println("K120 connected over USB Host");
#endif
}

static void onUsbDeviceDisconnected(const EspUsbHostDeviceInfo &device) {
  k120Present = false;
#if DEBUG_KEYBOARD
  Serial.print("[DBG] USB device disconnected: ");
  espUsbHostPrint(device);
#else
  (void)device;
  Serial.println("K120 disconnected");
#endif
  if (bleKeyboard.isConnected()) {
    bleKeyboard.releaseAll();
  }
}

static void onUsbHidInput(const EspUsbHostHIDInput &input) {
#if DEBUG_KEYBOARD
  debugRawReport("HID", input.data, input.length);
  Serial.printf("[DBG] HID iface=%u subclass=%u protocol=%u vid=%04X pid=%04X\n",
                input.interfaceNumber, input.subclass, input.protocol, input.vid,
                input.pid);
#endif

  // Boot keyboard protocol reports (or 8/9-byte boot-shaped reports)
  const bool bootKeyboard = (input.protocol == 1) || (input.length == 8) ||
                            (input.length == 9);
  if (!bootKeyboard || input.data == nullptr) return;

  const uint8_t *boot = nullptr;
  if (!extractBootReport(input.data, input.length, &boot)) return;
  forwardBootReport(boot);
}

static void onUsbKeyboard(const EspUsbHostKeyboardEvent &event) {
#if DEBUG_KEYBOARD
  const char display =
      (event.ascii >= 0x20 && event.ascii != 0x7F) ? static_cast<char>(event.ascii) : '.';
  Serial.printf("[DBG] KEY %s keycode=0x%02X ascii=0x%02X('%c') modifiers=",
                event.pressed ? "press " : "release", event.keycode, event.ascii,
                display);
  debugPrintModifiers(event.modifiers);
  Serial.printf(" locks(N/C/S)=%d/%d/%d\n", event.numLock, event.capsLock,
                event.scrollLock);
#else
  (void)event;
#endif
}

// ---------------------------------------------------------------------------
// Setup / loop
// ---------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(300);

  Serial.println();
  Serial.println("=== Wireless K120 ===");
  Serial.println("Always-on — use battery kill switch to power off");
#if DEBUG_KEYBOARD
  Serial.println("DEBUG_KEYBOARD=1 — dumping all keyboard HID signals");
#endif

  lastActivityMs = millis();

  bleKeyboard.setLedHandler(onBleLedsChanged);
  bleKeyboard.begin();
  Serial.printf("BLE advertising as \"%s\"\n", kBleName);

  usbHost.onDeviceConnected(onUsbDeviceConnected);
  usbHost.onDeviceDisconnected(onUsbDeviceDisconnected);
  usbHost.onHIDInput(onUsbHidInput);
  usbHost.onKeyboard(onUsbKeyboard);

  if (!usbHost.begin()) {
    Serial.printf("USB Host begin failed: %s\n", usbHost.lastErrorName());
  } else {
    Serial.println("USB Host ready — waiting for K120 on GPIO 19/20");
  }

  startLedAnim(LedAnim::Boot);
}

void loop() {
  serviceLedAnim();

  const bool bleConnected = bleKeyboard.isConnected();

  // BLE connection LED sequences
  if (bleConnected && !wasBleConnected) {
    Serial.println("BLE connected");
    startLedAnim(LedAnim::BleConnected);
  } else if (!bleConnected && wasBleConnected) {
    Serial.println("BLE disconnected — searching...");
    startLedAnim(LedAnim::BleSearch);
  } else if (!bleConnected && ledAnim == LedAnim::None) {
    startLedAnim(LedAnim::BleSearch);
  } else if (bleConnected && ledAnim == LedAnim::BleSearch) {
    startLedAnim(LedAnim::BleConnected);
  }
  wasBleConnected = bleConnected;

  const uint32_t now = millis();
  const uint32_t idleMs = now - lastActivityMs;

  // Idle LED timeout only — board stays powered until kill switch
  if (!ledForcedOff && idleMs > kIdleTimeoutMs) {
    ledForcedOff = true;
    if (ledAnim == LedAnim::None) {
      setLedsRaw(0x00);
    }
#if DEBUG_KEYBOARD
    Serial.println("[DBG] LED idle timeout — LEDs forced off");
#endif
  }

  delay(5);
}
