# Changelog

All notable changes to this project will be documented in this file.

## [Unreleased]

### Added
- **Pico 2 W Support**: Unified codebase now supports both the Raspberry Pi Pico W (RP2040) and Pico 2 W (RP2350) via the `PICO_BOARD` CMake parameter.
- **Nix Flake**: Added a `flake.nix` for a reproducible, isolated development environment.
- **USB Stdio & Logging**: Added `PICO_STDIO_TYPE` (USB, UART, BOTH) CMake option to allow serial logging over USB (CDC) simultaneously with the HID bridge. Added `ENABLE_USB_LOGGING` and `ENABLE_HEARTBEAT_LOGS` flags.
- **Docs Directory**: Created a `docs/` folder containing advanced troubleshooting guides and VSCode setup instructions.

### Changed
- **Project Structure**: Moved all firmware source code from `src_fw/ble_usb_hid_bridge/` to a standard root `src/` directory. Removed compiled binaries from the repository.
- **Increased Resources**: Boosted `MAX_ATT_DB_SIZE` to 4096 bytes and doubled L2CAP channels and HIDS clients in `btstack_config.h` to support massive gaming keyboards.

### Fixed
- **BTstack CCC Bug (ASUS/Logitech Keyboards)**: Enabled `ENABLE_GATT_LEGACY_CCC_DISCOVERY` to fix a BTstack internal state machine bug where keyboards with complex descriptors would hang indefinitely during discovery.
- **Multicore Stability**: Fixed a watchdog boot loop race condition on the Pico 2 W by isolating the CYW43 driver to Core 1, disabling the conflicting LED task on Core 0, and restoring `flash_safe_execute_core_init()`.
- **Function/Media Keys**: Reverted `tud_hid_report` parameter to `0` to prevent TinyUSB from double-prepending Report IDs, correctly restoring F1-F12 and multimedia key support for composite keyboards.