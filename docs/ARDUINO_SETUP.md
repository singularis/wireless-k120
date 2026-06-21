# Arduino IDE Setup — ESP32-S3 Zero BLE Keyboard

## Step 1: Install Arduino IDE

Download from: https://www.arduino.cc/en/software
(Get Arduino IDE 2.x — the newer version)

---

## Step 2: Add ESP32 Board Support

1. Open Arduino IDE
2. Go to **Arduino IDE → Settings** (or `Cmd + ,`)
3. In **"Additional boards manager URLs"**, paste this URL:

```
https://dl.espressif.com/dl/package_esp32_index.json
```

4. Click **OK**

5. Go to **Tools → Board → Boards Manager**
6. Search for **"esp32"**
7. Install **"esp32 by Espressif Systems"** (version 2.x or 3.x)
8. Wait for download (~200MB, takes a few minutes)

---

## Step 3: Install the BLE Keyboard Library

The **ESP32 BLE Keyboard** library by T-vK is not available in the Library Manager, so we'll install it from a ZIP file:

1. Download the library ZIP here: [ESP32-BLE-Keyboard.zip](https://github.com/T-vK/ESP32-BLE-Keyboard/archive/refs/heads/master.zip)
2. In Arduino IDE, go to **Sketch → Include Library → Add .ZIP Library...**
3. Select the downloaded `ESP32-BLE-Keyboard-master.zip` file.
4. Now, install its required dependency: Go to **Sketch → Include Library → Manage Libraries...**
5. Search for **"NimBLE-Arduino"**
6. Install **NimBLE-Arduino** by h2zero

---

## Step 4: Select Your Board

1. Go to **Tools → Board → esp32** → select:
   - **"ESP32S3 Dev Module"**

2. Set these settings under **Tools** menu:
   
   | Setting | Value |
   |---------|-------|
   | Board | ESP32S3 Dev Module |
   | USB CDC On Boot | **Enabled** |
   | Flash Size | **4MB** (or 8MB if your board has more) |
   | Partition Scheme | **Default 4MB with spiffs** |
   | Upload Mode | **UART0 / Hardware CDC** |
   | USB Mode | **Hardware CDC and JTAG** |

---

## Step 5: Connect Your ESP32-S3 Zero

1. Plug the ESP32-S3 Zero into your Mac via **USB-C data cable**
2. Go to **Tools → Port** → select the port that appears
   - It will look like: `/dev/cu.usbmodem*` or `/dev/cu.wchusbserial*`
   
   **⚠️ No port showing up?**
   - Make sure it's a **data cable**, not charge-only
   - Try holding **BOOT button** while plugging in
   - Try a different USB-C port on your Mac

---

## Step 6: Open the Sketch

1. Go to **File → Open**
2. Navigate to: `~/esp32/logi_retro_keyboard/logi_retro_keyboard.ino`
3. Open it

---

## Step 7: Upload (Flash)

1. Click the **→ Upload** button (arrow icon in top-left)
2. Watch the output console at the bottom
3. If it says **"Hard resetting via RTS pin..."** — it worked! ✅

   **⚠️ Upload failing?**
   - Hold the **BOOT button** on the ESP32-S3 Zero
   - While holding BOOT, click Upload
   - Release BOOT after you see "Connecting..."

---

## Step 8: Pair with Mac

1. Open **System Settings → Bluetooth** on your Mac
2. You should see **"logi_retro_keyboard"** in the device list
3. Click **Connect**
4. Open **TextEdit** or any text editor
5. Watch it type: **Hello World!** 🎉

---

## Step 9: Serial Monitor (Optional)

1. Go to **Tools → Serial Monitor**
2. Set baud rate to **115200** (bottom-right dropdown)
3. You'll see status messages from the ESP32

---

## Troubleshooting

| Problem | Solution |
|---------|----------|
| No port in Tools menu | Use a data cable, not charge-only |
| Upload fails | Hold BOOT button during upload |
| BLE not appearing | Reset the board (press RST button) |
| Types garbage | Make sure Mac keyboard layout is US/English |
| Compilation error "bad CPU type" | Open Terminal and run: `softwareupdate --install-rosetta --agree-to-license` |
