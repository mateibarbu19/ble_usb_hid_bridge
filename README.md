# BLE to USB HID Bridge (Pico W & Pico 2 W)

This firmware turns your Raspberry Pi Pico W or Pico 2 W into a **Bluetooth-to-USB Bridge**. It allows you to use a Bluetooth keyboard, mouse, or other HID device as if it were directly plugged into your PC via a USB cable. 

Because the PC sees a standard wired USB keyboard, you can use your Bluetooth keyboard inside the BIOS/UEFI, or on computers that don't have Bluetooth hardware at all.

<img width="716" height="391" alt="image" src="https://github.com/user-attachments/assets/6d4410d5-2912-4bd5-93dc-8aef206fb2b0" />

## Supported Hardware
*   **Raspberry Pi Pico W** (RP2040)
*   **Raspberry Pi Pico 2 W** (RP2350)

## How to Build

We provide a robust Nix environment for building. If you prefer using Visual Studio Code, please see the [VSCode Setup Guide](docs/vscode_instructions.md).

### 1. Build for Pico 2 W (RP2350)
```bash
cd src
nix develop .
mkdir build && cd build
cmake .. -DPICO_BOARD=pico2_w
make
```

### 2. Build for Pico W (RP2040)
```bash
cd src
nix develop .
mkdir build && cd build
cmake .. -DPICO_BOARD=pico_w
make
```

The resulting firmware will be located at `build/ble_usb_hid_bridge.uf2`. Hold the `BOOTSEL` button on your Pico while plugging it into your PC, and drag-and-drop the `.uf2` file onto the `RPI-RP2` drive.

---

## Advanced Build Options (CMake)

You can customize the firmware by passing these flags during the `cmake ..` step:

*   `-DPICO_STDIO_TYPE=USB` (Default): Logs debug output via the same USB cable used for the keyboard (creates a Composite USB device). You can view the logs using a serial monitor (like PuTTY or `screen`) at 115200 baud.
*   `-DPICO_STDIO_TYPE=UART`: Sends debug logs to the hardware UART pins instead of USB.
*   `-DENABLE_USB_LOGGING=ON`: Prints detailed USB events (e.g., when the PC suspends the USB port or when HID reports are pushed). Default is OFF.
*   `-DENABLE_HEARTBEAT_LOGS=ON`: Prints a `[SYS] Heartbeat (Core 0 Running)` message every 5 seconds. Highly recommended if you are modifying the code and want to ensure the main loop hasn't crashed. Default is OFF.

---

## How to Use

1.  **Plug it in**: Connect the Pico to your PC.
2.  **Pairing Mode**: Put your Bluetooth keyboard or mouse into "Pairing Mode" (usually by holding the Bluetooth key until its LED flashes rapidly).
3.  **Connection**: The Pico will find the keyboard and securely pair with it. (If your keyboard requires a PIN, it will print the 6-digit PIN to the USB Serial console).
4.  **Ready**: Once the service discovery is complete, your keyboard is ready to use! 

The Pico saves the pairing data in its flash memory. The next time you turn on your keyboard, it will reconnect automatically in under a second.

---

## Documentation & Troubleshooting

If you are experiencing connection timeouts (e.g. with ASUS or Logitech keyboards), boot loops, or missing function keys, please read the [Troubleshooting Guide](docs/troubleshooting.md) for detailed explanations of how this firmware solves common BLE bridge bugs.

## Acknowledgements
This project is built upon the foundational concepts established by [Shiomachi Software](https://github.com/shiomachisoft/picow_ble_usb_hid_bridge). It introduces substantial architectural rewrites to support the Pico 2 W multicore environment, fix upstream BTstack bugs, and natively pass composite media keys.