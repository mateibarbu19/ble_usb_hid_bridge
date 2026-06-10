/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "bsp/board_api.h"
#include "tusb.h"
#include "hardware/watchdog.h"
// @@add
// =====>
#include "Common.h"
// <=====

//--------------------------------------------------------------------+
// MACROS
//--------------------------------------------------------------------+
// @@add
// =====>
#define USB_REINIT_STABILIZATION_DELAY 100 // ms
#define LED_BLINKING_INTERVAL 200 // ms

#ifdef ENABLE_USB_LOGGING
#define USB_LOG(...) printf("[USB] " __VA_ARGS__)
#else
#define USB_LOG(...)
#endif

#define SYS_LOG(...) printf("[SYS] " __VA_ARGS__)
// <=====
//--------------------------------------------------------------------+
// GLOBAL VARIABLES
//--------------------------------------------------------------------+
// @@add
// =====>
volatile bool g_usb_reinit_request = false; // Flag to request USB re-initialization when BLE HID connection is established
extern volatile bool g_cyw43_initialized;
// <=====

//--------------------------------------------------------------------+
// FUNCTION PROTOTYPES
//--------------------------------------------------------------------+
// @@add
// =====>
void usb_dev_main(void);
void hid_task(void);
void led_blinking_task(void);
bool send_hid_report(void);

extern bool is_ble_app_state_ready(void);
extern void ble_host_main(void);
// <=====

/*------------- MAIN -------------*/
int main(void)
{
    board_init();  

    // init device stack on configured roothub port
    tud_init(BOARD_TUD_RHPORT);

    if (board_init_after_tusb) {
        board_init_after_tusb();
    }      
    
    // @@chg
    // =====>
    stdio_init_all();
    CMN_Init(); 

    // Preferred stdio might need a small delay or waiting for connection if USB
#ifdef LIB_PICO_STDIO_USB
    // Wait for USB connection for a short time to allow developer to see boot logs
    // but don't block forever if it's a production use case.
    uint32_t start_wait = board_millis();
    while (!tud_cdc_connected() && (board_millis() - start_wait < 2000)) {
        tud_task();
    }
#endif

    SYS_LOG("BLE to USB HID Bridge starting...\n");
#ifdef PICO_BOARD
    SYS_LOG("Board: %s\n", PICO_BOARD);
#endif

    // Enable watchdog with a 10-second timeout for robustness
    if (watchdog_caused_reboot()) {
        SYS_LOG("Rebooted by Watchdog!\n");
    }
    watchdog_enable(10000, 1); // 10000ms timeout

    // Initialize to lock out CPU Core 0 when btstack writes to flash memory on CPU Core 1.
    // This is required for RP2350 multicore flash safety.
    flash_safe_execute_core_init();

    SYS_LOG("Launching BLE host on Core 1...\n");
    multicore_launch_core1(ble_host_main);

    SYS_LOG("Entering USB device main loop on Core 0...\n");
    usb_dev_main();

    return 0;
    // <=====
}

// @@add
// =====>
//--------------------------------------------------------------------+
// Main loop for the USB device (runs on Core0).
//--------------------------------------------------------------------+
// This function loops indefinitely, handling USB events, LED blinking, and HID tasks.
// It also handles USB re-initialization requests from Core1.
void usb_dev_main(void)
{    
    uint32_t last_heartbeat = 0;
    SYS_LOG("USB Main Loop Started\n");
    while (1) 
    {
        watchdog_update(); // Feed the watchdog

        if (board_millis() - last_heartbeat > 5000) {
            last_heartbeat = board_millis();
#ifdef ENABLE_HEARTBEAT_LOGS
            SYS_LOG("Heartbeat (Core 0 Running)\n");
#endif
        }

        // Check for USB re-initialization request from Core1 (BLE host)
        if (g_usb_reinit_request) {
            g_usb_reinit_request = false; 
            SYS_LOG("USB Re-init requested\n");
            if (tud_mounted()) {
                tud_disconnect(); // Disconnect the USB device
                board_delay(USB_REINIT_STABILIZATION_DELAY); // Wait a bit for stabilization
            }
            // Clear any pending HID reports from the queue before reconnecting.
            CMN_ClearQueue(CMN_QUE_KIND_HID_RPT);
            tud_connect();
        }

        tud_task();          // Run TinyUSB device task
        watchdog_update();   // Feed again after potentially long library calls
        
        hid_task();          // Run HID report sending task
    }
}
// <=====

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void)
{
    USB_LOG("Device mounted\n");
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
    USB_LOG("Device unmounted\n");
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
    USB_LOG("Bus suspended (remote_wakeup_en=%d)\n", remote_wakeup_en);
    (void) remote_wakeup_en;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
    USB_LOG("Bus resumed\n");
}

//--------------------------------------------------------------------+
// USB HID
//--------------------------------------------------------------------+

// @@chg
// =====>
// Dequeue and send one HID report from the queue to the USB host.
// return true if a report was successfully sent, false otherwise.
bool send_hid_report(void)
{
    static ST_HID_RPT stHidRpt; // Change local variable to static to use static memory (data area) instead of stack, preventing stack overflow.
    bool bRet = false;

    // Peek at the next report in the queue without removing it yet
    if (CMN_PeekQueue(CMN_QUE_KIND_HID_RPT, &stHidRpt)) {
        // If the host is suspended, wake it up and exit.
        // The report will be sent on a subsequent call after the host resumes.
        if ( tud_suspended()) {
            tud_remote_wakeup();
            return bRet;
        }                 
        // If the HID interface is ready, try to send the report
        if (tud_hid_ready()) {      
            // BTstack already prepends the Report ID to the stHidRpt.report buffer.
            // Passing 0 tells TinyUSB to send the buffer exactly as-is.
            // Passing a non-zero ID here would cause TinyUSB to prepend a second ID,
            // shifting the bytes and breaking F-keys and standard keystrokes.
            if (tud_hid_report(0, stHidRpt.report, stHidRpt.report_len)) {
                USB_LOG("HID report sent (len=%d)\n", stHidRpt.report_len);
                // If sent successfully, remove the report from the queue
                CMN_AdvanceQueue(CMN_QUE_KIND_HID_RPT);
                bRet = true;
            }  
        }
    }
 
    return bRet;
}
// <=====

//--------------------------------------------------------------------+
// HID TASK
//--------------------------------------------------------------------+
void hid_task(void)
{
    // Dequeue and send one HID report.
    (void)send_hid_report();
}

// Invoked when sent REPORT successfully to host
// Application can use this to send the next report
// Note: For composite reports, report[0] is report ID
void tud_hid_report_complete_cb(uint8_t instance, uint8_t const* report, uint16_t len)
{
    (void) instance;
    (void) len;
    (void) report;
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
    // TODO not Implemented
    (void) instance;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) reqlen;

    return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
    USB_LOG("HID set report: instance=%d, id=%d, type=%d, size=%d\n", instance, report_id, report_type, bufsize);
    (void) instance;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) bufsize;
}

//--------------------------------------------------------------------+
// BLINKING TASK
//--------------------------------------------------------------------+
void led_blinking_task(void)
{
    static uint32_t start_ms = 0;
    static bool led_state = false;

    // Prevent Core 0 from accessing the wireless SPI bus until Core 1 has initialized it.
    if (!g_cyw43_initialized) return;

    const uint32_t blink_interval = LED_BLINKING_INTERVAL;

    if (is_ble_app_state_ready()) {
        // Turn on LED when in READY state
        if (!led_state) {
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, true);
            led_state = true;
        }
    } else {
        // Blink every 200ms when not in READY state
        if ( board_millis() - start_ms < blink_interval) return; // not enough time
        start_ms += blink_interval;

        led_state = !led_state;
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_state);
    }
}
