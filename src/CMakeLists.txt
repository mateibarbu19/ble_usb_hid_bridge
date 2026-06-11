# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
# Default to pico2_w if not specified, but allow override
if(NOT PICO_BOARD)
    set(PICO_BOARD pico2_w CACHE STRING "Board type")
endif()

option(ENABLE_USB_LOGGING "Enable USB interface logging" OFF)
option(ENABLE_HEARTBEAT_LOGS "Enable periodic heartbeat logs" OFF)
set(PICO_STDIO_TYPE "USB" CACHE STRING "Type of stdio: USB, UART, or BOTH")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(ble_usb_hid_bridge C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
        )

# Add executable.  
add_executable(ble_usb_hid_bridge
    main.c
    usb_descriptors.c
    hog_host_demo.c
    bt_example_common.c
    Common.c
    )  

target_link_libraries(ble_usb_hid_bridge
    pico_stdlib
    pico_multicore
    pico_flash
    hardware_sync
    pico_unique_id
    tinyusb_device
    tinyusb_board
    pico_btstack_ble
    pico_btstack_classic
    pico_btstack_cyw43
    pico_cyw43_arch_threadsafe_background
    )   

target_include_directories(ble_usb_hid_bridge PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    )

target_compile_definitions(ble_usb_hid_bridge PRIVATE 
    CYW43_LWIP=0
    )    

if(ENABLE_USB_LOGGING)
    target_compile_definitions(ble_usb_hid_bridge PRIVATE ENABLE_USB_LOGGING=1)
endif()

if(ENABLE_HEARTBEAT_LOGS)
    target_compile_definitions(ble_usb_hid_bridge PRIVATE ENABLE_HEARTBEAT_LOGS=1)
endif()

pico_btstack_make_gatt_header(ble_usb_hid_bridge PRIVATE 
    ${CMAKE_CURRENT_LIST_DIR}/hog_host_demo.gatt
    )

# Stdio configuration
if(PICO_STDIO_TYPE STREQUAL "USB")
    pico_enable_stdio_usb(ble_usb_hid_bridge 1)
    pico_enable_stdio_uart(ble_usb_hid_bridge 0)
    target_compile_definitions(ble_usb_hid_bridge PRIVATE LIB_PICO_STDIO_USB=1)
elseif(PICO_STDIO_TYPE STREQUAL "UART")
    pico_enable_stdio_usb(ble_usb_hid_bridge 0)
    pico_enable_stdio_uart(ble_usb_hid_bridge 1)
    target_compile_definitions(ble_usb_hid_bridge PRIVATE LIB_PICO_STDIO_UART=1)
elseif(PICO_STDIO_TYPE STREQUAL "BOTH")
    pico_enable_stdio_usb(ble_usb_hid_bridge 1)
    pico_enable_stdio_uart(ble_usb_hid_bridge 1)
    target_compile_definitions(ble_usb_hid_bridge PRIVATE LIB_PICO_STDIO_USB=1 LIB_PICO_STDIO_UART=1)
endif()

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(ble_usb_hid_bridge)
