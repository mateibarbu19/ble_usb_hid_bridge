# BLE to USB HID Bridge (Pico W & Pico 2 W)

This firmware turns your Raspberry Pi Pico W or Pico 2 W into a **Bluetooth-to-USB Bridge**. It allows you to use a Bluetooth keyboard, mouse, or other HID device as if it were directly plugged into your PC via a USB cable. 

Because the PC sees a standard wired USB keyboard, you can use your Bluetooth keyboard inside the BIOS/UEFI, or on computers that don't have Bluetooth hardware at all.

## Supported Hardware
*   **Raspberry Pi Pico W** (RP2040)
*   **Raspberry Pi Pico 2 W** (RP2350)

## Why This Project Exists
This project builds upon the original concept by [Shiomachi Software](https://github.com/shiomachisoft/picow_ble_usb_hid_bridge) but introduces critical fixes and architectural improvements for modern hardware and complex keyboards:

1.  **Pico 2 W (RP2350) Multicore Stability**: The original project suffered from race conditions and deadlocks on the Pico 2 W due to how the two CPU cores shared the CYW43439 wireless chip. We introduced strict **Core Isolation**: the Bluetooth stack and wireless driver run exclusively on Core 1, while the USB stack and Status LED run safely on Core 0.
2.  **The "ASUS KW100" Bug (BTstack CCC Discovery)**: Many modern keyboards (like ASUS and some Logitechs) would successfully pair but then freeze and disconnect after 30 seconds. We discovered this was due to a state machine bug in the BTstack library (`ENABLE_GATT_FIND_INFORMATION_FOR_CCC_DISCOVERY`). This firmware bypasses the bug using the more robust `ENABLE_GATT_LEGACY_CCC_DISCOVERY` method.
3.  **Multimedia & Function Keys (F1-F12)**: The original firmware stripped "Report IDs" when forwarding keystrokes to the PC. This caused F-keys and volume buttons to stop working. We fixed the TinyUSB pipeline to properly forward these multi-report packets.
4.  **Massive GATT Buffers**: We increased the internal Bluetooth descriptor memory from 500 bytes to 4096 bytes to support gaming keyboards with huge feature sets.

---

## How to Build

We provide a robust Nix environment for building.

### 1. Build for Pico 2 W (RP2350)
```bash
cd src_fw/ble_usb_hid_bridge
nix develop .
rm -rf build
cmake -B build -S . -DPICO_BOARD=pico2_w
cmake --build build
```

### 2. Build for Pico W (RP2040)
```bash
cd src_fw/ble_usb_hid_bridge
nix develop .
rm -rf build
cmake -B build -S . -DPICO_BOARD=pico_w
cmake --build build
```

The resulting firmware will be located at `build/ble_usb_hid_bridge.uf2`. Hold the `BOOTSEL` button on your Pico while plugging it into your PC, and drag-and-drop the `.uf2` file onto the `RPI-RP2` drive.

---

## Advanced Build Options (CMake)

You can customize the firmware by passing these flags during the `cmake -B build ...` step:

*   `-DPICO_STDIO_TYPE=USB` (Default): Logs debug output via the same USB cable used for the keyboard (creates a Composite USB device). You can view the logs using a serial monitor (like PuTTY or `screen`) at 115200 baud.
*   `-DPICO_STDIO_TYPE=UART`: Sends debug logs to the hardware UART pins instead of USB.
*   `-DENABLE_USB_LOGGING=ON`: Prints detailed USB events (e.g., when the PC suspends the USB port or when HID reports are pushed). Default is OFF.
*   `-DENABLE_HEARTBEAT_LOGS=ON`: Prints a `[SYS] Heartbeat (Core 0 Running)` message every 5 seconds. Highly recommended if you are modifying the code and want to ensure the main loop hasn't crashed. Default is OFF.

---

## How to Use

1.  **Plug it in**: Connect the Pico to your PC. The onboard LED will start **blinking**, indicating it is scanning for Bluetooth devices.
2.  **Pairing Mode**: Put your Bluetooth keyboard or mouse into "Pairing Mode" (usually by holding the Bluetooth key until its LED flashes rapidly).
3.  **Connection**: The Pico will find the keyboard and securely pair with it. (If your keyboard requires a PIN, it will print the 6-digit PIN to the USB Serial console).
4.  **Ready**: Once the service discovery is complete, the Pico's LED will turn **solid green**. Your keyboard is now ready to use! 

The Pico saves the pairing data. The next time you turn on your keyboard, it will reconnect automatically in under a second.

---

## Guided Log Analysis (Troubleshooting)

If you connect to the Pico's serial port (115200 baud), you can see what it's thinking.

*   `[BLE] Scanning for HID devices...`: Normal startup state.
*   `[BLE] Found bonded device AA:BB... Attempting fast reconnection...`: The Pico remembers your keyboard and is waking it up.
*   `[BLE] Link layer connected. Requesting secure pairing...`: The devices are talking and setting up encryption.
*   `[BLE] Pairing failed: Connection timeout.`: Your keyboard went to sleep or you didn't accept the pairing in time. Turn the keyboard off and on again.
*   `[SYS] Rebooted by Watchdog!`: The firmware crashed (usually due to a code modification causing a multicore deadlock). The watchdog automatically resets the Pico to recover.