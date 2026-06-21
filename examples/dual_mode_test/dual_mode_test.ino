#define KeyReport BleKeyReport
#define MediaKeyReport BleMediaKeyReport
#include <BleKeyboard.h>
#undef KeyReport
#undef MediaKeyReport

#include "USB.h"
#include "USBHIDKeyboard.h"

// Initialize Native USB Keyboard
USBHIDKeyboard usbKeyboard;

// Initialize Bluetooth Keyboard
BleKeyboard bleKeyboard("logi_retro_keyboard", "Logitech", 100);

// Use the BOOT button on the ESP32-S3 as a trigger (GPIO 0 is standard for the BOOT button)
const int BUTTON_PIN = 0; 

// Track previous button state for edge detection
bool lastButtonState = HIGH;

void setup() {
  Serial.begin(115200);
  
  // Set up the button pin with an internal pullup resistor
  // The BOOT button pulls to LOW when pressed.
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.println("Starting Dual-Mode Keyboard...");

  // Initialize Native USB
  usbKeyboard.begin();
  USB.begin();

  // Initialize Bluetooth
  bleKeyboard.begin();

  Serial.println("Ready! Plug into USB, connect via Bluetooth, or both.");
  Serial.println("Press the BOOT button on the ESP32-S3 to type the test message.");
}

void loop() {
  // Read the current button state
  bool currentButtonState = digitalRead(BUTTON_PIN);

  // Check if the button was just pressed (transition from HIGH to LOW)
  if (lastButtonState == HIGH && currentButtonState == LOW) {
    Serial.println("Button pressed! Sending message via all connected interfaces...");

    const char* message = "Dual-Mode Keyboard works!\n";
    
    // 1. Send via USB
    // Note: USB doesn't have a reliable 'isConnected()' check for HID endpoints in Arduino,
    // so we just blindly send it. If a host is listening, it will receive it.
    usbKeyboard.print(message);
    
    // 2. Send via Bluetooth
    if (bleKeyboard.isConnected()) {
      bleKeyboard.print(message);
    } else {
      Serial.println("(Bluetooth not connected, skipped BLE sending)");
    }

    // Debounce the button
    delay(200);
  }

  // Save the state for the next loop
  lastButtonState = currentButtonState;

  // Small delay for stability
  delay(10);
}
