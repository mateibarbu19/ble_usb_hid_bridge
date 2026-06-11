# Troubleshooting

## The "ASUS KW100" Disconnect Bug (BTstack CCC Discovery)

**Symptoms:**
- The keyboard successfully pairs (you type the passcode and hit enter).
- The logs show `[BLE] Pairing complete! Secure link established.`
- The bridge logs `[BLE] Searching for HID services...`
- The keyboard continues to flash its pairing light, eventually goes to sleep, and the connection drops after exactly 30 seconds with an SM timeout.

**Root Cause:**
Many modern keyboards (like ASUS and some Logitechs) have complex GATT descriptor chains. Specifically, they often place additional descriptors (like Report References) *after* the Client Characteristic Configuration (CCC) descriptor.

The BTstack library includes an optimization flag (`ENABLE_GATT_FIND_INFORMATION_FOR_CCC_DISCOVERY`). When this optimization is active, the state machine assumes the CCC is the last item. When it encounters the extra descriptors on complex keyboards, the state machine incorrectly transitions and overwrites its own internal state. It fails to send the critical "enable notifications" command, resulting in a silent 30-second stall and timeout.

**The Fix:**
This project defines `ENABLE_GATT_LEGACY_CCC_DISCOVERY` in `btstack_config.h`. This bypasses the buggy optimization and forces BTstack to use the older, highly robust two-step discovery method that properly handles complex descriptor lists without hanging.

## Multicore Boot Loops (Pico 2 W)

**Symptoms:**
- The Pico connects to USB, logs `[SYS] Entering USB device main loop on Core 0...`, and then abruptly disconnects.
- The logs show `[SYS] Rebooted by Watchdog!` every 10 to 30 seconds.

**Root Cause:**
The RP2350 (Pico 2 W) has two cores sharing one wireless chip (CYW43439). If Core 0 tries to blink the status LED using the `cyw43` API at the exact same moment that Core 1 is booting up the Bluetooth stack, the shared hardware bus locks up. This causes Core 0 to hang indefinitely, failing to feed the hardware watchdog timer.

Additionally, when BTstack writes pairing keys to flash memory, it halts the other core. If the cores aren't perfectly synchronized using `flash_safe_execute_core_init()`, the system deadlocks.

**The Fix:**
This project implements **Strict Core Isolation**:
1. The `cyw43` hardware initialization is strictly bound to Core 1.
2. The Status LED polling task on Core 0 was permanently disabled, preventing any bus contention.
3. `flash_safe_execute_core_init()` is properly called on Core 0 to synchronize during flash operations.

## Multimedia & Function Keys Missing

**Symptoms:**
- Standard typing (letters, numbers) works perfectly.
- Pressing `F1-F12`, Volume Up/Down, or Play/Pause does nothing on the PC.

**Root Cause:**
Multi-device and multimedia keyboards use multiple "Report IDs" to tell the PC whether a packet contains standard keys (usually ID 1) or media keys (usually ID 2 or 3). 
If the firmware strips this ID, the PC assumes everything is a standard keystroke and ignores the media keys.

**The Fix:**
The firmware explicitly passes the `report_id` to TinyUSB's `tud_hid_report(0, buffer, ...)` function by setting the ID parameter to `0`. Since BTstack already natively prepends the correct Report ID to the raw data buffer, passing `0` tells TinyUSB to send the perfectly formed BTstack payload as-is, natively supporting F-keys without corrupting the packet.