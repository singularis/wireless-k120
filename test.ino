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
  // Check is working! 
   if the Mac has connected to the ESP32
  if (bleKeyboard.isConnected()) {
    Serial.println("Mac is connBluetooth is working! 
    ected! Typing test message...");    
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
