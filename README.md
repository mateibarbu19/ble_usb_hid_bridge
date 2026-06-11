# Raspberry Pi Pico W / Pico 2 W BLE to USB HID Bridge Adapter

Convert any Bluetooth Low Energy (BLE) keyboard or mouse into a wired USB KVM-compatible device using a **Raspberry Pi Pico W** (RP2040) or **Pico 2 W** (RP2350).

This firmware acts as a BLE Central (Host) adapter. It pairs with your Bluetooth peripheral and forwards the input data directly to your host PC via USB. Because your PC sees a standard wired USB HID device, you can use your Bluetooth keyboard inside BIOS/UEFI environments, KVM switches, or on computers completely lacking Bluetooth hardware. **Zero drivers required.**

<img width="716" height="391" alt="image" src="https://github.com/user-attachments/assets/6d4410d5-2912-4bd5-93dc-8aef206fb2b0" />

## ⚡ Key Features

*   **Ultra Low Latency:** Utilizes strict multi-core processing. Core 0 exclusively handles 1ms-interval high-speed USB polling, while Core 1 is isolated to process the Bluetooth stack and wireless driver interrupts.
*   **True Report Pass-through:** Natively passes the raw HID Report Descriptor and Input Reports directly from the BLE device to the PC. This guarantees flawless support for Multimedia keys, Volume controls, and F1-F12 Function keys on complex keyboards.
*   **Smart Scan & Auto-Reconnect:** Automatically toggles between quickly reconnecting to bonded devices and scanning for new peripherals in pairing mode.
*   **Massive Device Compatibility:** Features a massive 4096-byte GATT Attribute Database and enhanced BTstack limits to support complex gaming and multi-device keyboards (e.g., ASUS, Logitech) without stalling.

## 🛠️ How to Build

We provide a robust Nix Flake environment for perfectly reproducible builds. If you prefer using Visual Studio Code natively, please see the [VSCode Setup Guide](docs/vscode_instructions.md).

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

## ⚙️ Advanced Build Options (CMake)

Customize your firmware by passing these flags during the `cmake ..` step:

*   `-DPICO_STDIO_TYPE=USB` *(Default)*: Enables composite USB logging. Your Pico will appear as a composite device (HID Keyboard + CDC Serial Port), allowing you to read debug logs via a serial monitor (e.g., PuTTY, `screen`) at 115200 baud using the same USB cable.
*   `-DPICO_STDIO_TYPE=UART`: Sends debug logs to the hardware UART pins instead of USB.
*   `-DENABLE_USB_LOGGING=ON`: Prints highly detailed USB events (e.g., PC suspend/resume states, HID report transmissions). Default is OFF.
*   `-DENABLE_HEARTBEAT_LOGS=ON`: Prints a `[SYS] Heartbeat (Core 0 Running)` message every 5 seconds to verify the main loop is stable. Default is OFF.

## 🚀 How to Use

1.  **Plug it in**: Connect the Pico to your PC's USB port.
2.  **Pairing Mode**: Put your Bluetooth keyboard or mouse into "Pairing Mode" (usually by holding the Bluetooth key until its LED flashes rapidly).
3.  **Connection**: The Pico will automatically find the keyboard and securely pair with it. *(Note: If your keyboard requires a PIN, it will print the 6-digit PIN to the USB Serial console).*
4.  **Ready**: Once the service discovery is complete, your keyboard is immediately ready to use!

The Pico securely saves the pairing data in its flash memory. The next time you turn on your keyboard, it will reconnect automatically in under a second.

## 🐛 Documentation & Troubleshooting

If you are experiencing connection timeouts (especially with ASUS or Logitech keyboards), boot loops, or missing function keys on your specific fork, please read our [Troubleshooting Guide](docs/troubleshooting.md). It contains detailed explanations of how this firmware solves common BLE bridge bugs.

## 🙏 Acknowledgements
This project is built upon the foundational concepts established by [Shiomachi Software](https://github.com/shiomachisoft/picow_ble_usb_hid_bridge), merging sample code from **TinyUSB** (`dev_hid_composite`) and **BTstack** (`hog_host_demo`). It introduces substantial architectural rewrites to support the Pico 2 W multicore environment, fix upstream BTstack bugs, and natively pass composite media keys.