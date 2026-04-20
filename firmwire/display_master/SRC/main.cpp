/**
 * ============================================================================
 * DISPLAY MASTER - MAIN PROGRAM v1.1
 * Smart Mosque Ecosystem
 * ============================================================================
 * 
 * v1.1 CHANGES:
 * - Added Logic Master health monitoring
 * - Improved error handling
 * ============================================================================
 */

#include <Arduino.h>
#include "LGFX_Config.h"
#include <lvgl.h>
#include "../include/display_master.h"

// ============================================================================
// GLOBAL VARIABLES DEFINITION
// ============================================================================

// Device database
DeviceDisplayInfo g_devices[MAX_DEVICES];
uint8_t g_device_count = 0;
SensorDisplayData g_sensor_data = {0.0f, 0.0f, false, 0};

// Pending enrollment
PendingEnrollment g_pending_enrollment = {
    .active = false
};

// Command queue
CommandQueueItem g_command_queue[COMMAND_QUEUE_SIZE];  // ✅ FIXED
uint8_t g_queue_head = 0;
uint8_t g_queue_tail = 0;

// Statistics
DisplayStatistics g_stats = {0};

// NVS storage
Preferences g_preferences;

// Web server
AsyncWebServer g_webServer(WEB_SERVER_PORT);

// ✅ NEW: Logic Master health tracking
uint32_t g_last_logic_heartbeat = 0;
bool g_logic_master_online = true;

// LVGL display & touch
LGFX_SmartMosque lcd;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[800 * 40];

// UI screens
lv_obj_t* g_main_screen = nullptr;
lv_obj_t* g_control_screen = nullptr;
lv_obj_t* g_enrollment_popup = nullptr;

// ============================================================================
// LVGL INTEGRATION FUNCTIONS
// ============================================================================

void lvgl_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    
    lcd.startWrite();
    lcd.setAddrWindow(area->x1, area->y1, w, h);
    lcd.pushPixels((uint16_t *)&color_p->full, w * h, true);
    lcd.endWrite();
    
    lv_disp_flush_ready(disp);
}

void lvgl_touchpad_read(lv_indev_drv_t * indev_driver, lv_indev_data_t * data) {
    uint16_t touchX, touchY;
    bool touched = lcd.getTouch(&touchX, &touchY);
    
    if (!touched) {
        data->state = LV_INDEV_STATE_REL;
    } else {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = touchX;
        data->point.y = touchY;
    }
}

// ============================================================================
// SYSTEM INITIALIZATION
// ============================================================================

void initDisplay() {
    Serial.println("🖥️ Initializing display...");
    
    lcd.init();
    lcd.setRotation(1);
    lcd.fillScreen(TFT_BLACK);
    lcd.setTextColor(TFT_WHITE);
    lcd.setTextSize(2);
    lcd.setCursor(10, 10);
    lcd.println("SMART MOSQUE DISPLAY");
    lcd.println("Initializing...");
    
    lv_init();
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, 800 * 40);
    
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 800;
    disp_drv.ver_res = 480;
    disp_drv.flush_cb = lvgl_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
    
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = lvgl_touchpad_read;
    lv_indev_drv_register(&indev_drv);
    
    Serial.println("✓ Display initialized");
}

void initNVS() {
    Serial.println("💾 Initializing NVS...");
    
    if (!g_preferences.begin("display_master", false)) {
        Serial.println("⚠ NVS initialization failed!");
        return;
    }
    
    g_device_count = g_preferences.getUChar("dev_count", 0);
    Serial.printf("✓ NVS initialized (cached devices: %d)\n", g_device_count);
}

void printBanner() {
    Serial.println("\n╔═══════════════════════════════════════════════════╗");
    Serial.println("║                                                   ║");
    Serial.println("║       SMART MOSQUE ECOSYSTEM - DISPLAY MASTER    ║");
    Serial.println("║                    v1.1                           ║");
    Serial.println("║                                                   ║");
    Serial.println("╚═══════════════════════════════════════════════════╝");
    Serial.printf("\n🆔 Device ID: %d (Display Master)\n", DISPLAY_MASTER_ID);
    Serial.printf("📡 RS-485: %d bps\n", RS485_BAUD_RATE);
    Serial.printf("🖥️  Display: 800x480 capacitive touch\n");
    Serial.printf("📶 WiFi AP: %s\n", WIFI_AP_SSID);
    Serial.println();
}

// ============================================================================
// MAIN SETUP
// ============================================================================

void setup() {
    Serial.begin(115200);
    delay(500);
    
    printBanner();
    
    initDisplay();
    initNVS();
    
    rs485Init();
    Serial.println("✓ RS-485 initialized");
    
    deviceManagerInit();
    Serial.println("✓ Device manager initialized");
    
    wifiInit();
    Serial.println("✓ WiFi AP started");
    
    webServerInit();
    Serial.println("✓ Web server started");
    
    uiDashboardCreate();
    Serial.println("✓ Dashboard UI created");
    
    uiDeviceControlCreate();
    Serial.println("✓ Control UI created");
    
    uiEnrollmentCreate();
    Serial.println("✓ Enrollment UI created");
    
    lv_scr_load(g_main_screen);
    
    Serial.println("📡 Syncing device list from Logic Master...");
    deviceManagerSyncFromLogicMaster();
    
    // ✅ NEW: Initialize Logic Master heartbeat tracking
    g_last_logic_heartbeat = millis();
    g_logic_master_online = true;
    
    Serial.println("\n🎉 Display Master ready!");
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    Serial.printf("📱 Dashboard: http://%s\n", WiFi.softAPIP().toString().c_str());
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
    static uint32_t lastUIUpdate = 0;
    static uint32_t lastOnlineCheck = 0;
    static uint32_t lastStatsUpdate = 0;
    static uint32_t lastLogicCheck = 0;  // ✅ NEW
    
    uint32_t now = millis();
    
    lv_timer_handler();
    rs485ProcessIncoming();
    commandQueueProcess();
    
    if (now - lastUIUpdate >= UI_UPDATE_INTERVAL_MS) {
        lastUIUpdate = now;
        
        lv_obj_t* active_screen = lv_scr_act();
        if (active_screen == g_main_screen) {
            uiDashboardUpdate();
        } else if (active_screen == g_control_screen) {
            uiDeviceControlUpdate();
        }
        
        g_stats.ui_updates++;
    }
    
    if (now - lastOnlineCheck >= 5000) {
        lastOnlineCheck = now;
        deviceManagerCheckOnlineStatus();
    }
    
    // ✅ NEW: Check Logic Master health (every 5 seconds)
    if (now - lastLogicCheck >= 5000) {
        lastLogicCheck = now;
        
        if (now - g_last_logic_heartbeat > DEVICE_OFFLINE_TIMEOUT) {
            if (g_logic_master_online) {
                g_logic_master_online = false;
                Serial.println("⚠⚠⚠ Logic Master OFFLINE!");
                uiDashboardShowAlert("Logic Master Offline!", 2);
            }
        }
    }
    
    if (now - lastStatsUpdate >= 1000) {
        lastStatsUpdate = now;
        g_stats.uptime_seconds = now / 1000;
    }
    
    delay(5);
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

bool sendRelayCommand(uint8_t device_id, uint8_t relay_index, bool state) {
    // Validate device ID
    if (device_id == 0 || device_id > 247) {
        Serial.printf("⚠ Invalid device ID: %d\n", device_id);
        return false;
    }
    
    // Validate relay index
    if (relay_index > 7) {
        Serial.printf("⚠ Invalid relay index: %d\n", relay_index);
        return false;
    }
    
    // Get device info
    DeviceDisplayInfo* device = deviceManagerGet(device_id);
    if (!device) {
        Serial.printf("⚠ Device %d not found\n", device_id);
        return false;
    }
    
    // Check if device is online
    if (!device->online) {
        Serial.printf("⚠ Device %d is offline\n", device_id);
        uiDashboardShowAlert("Device Offline", 2);
        return false;
    }
    
    // Check if device is in MANUAL mode
    if (device->control_source == 1) {
        Serial.printf("⚠ Device %d in MANUAL mode\n", device_id);
        uiDashboardShowAlert("Device in Manual Mode", 1);
        return false;
    }
    
    // Build proper SET_RELAY payload
    SetRelayPayload payload;
    payload.relay_mask = (1 << relay_index);
    memset(payload.values, 0, 8);
    payload.values[relay_index] = state ? 1 : 0;
    
    // Add to command queue
    bool queued = commandQueueAdd(device_id, CMD_SET_RELAY, 
                                  (uint8_t*)&payload, sizeof(SetRelayPayload));
    
    if (queued) {
        Serial.printf("✓ Command queued: Device %d, Relay %d -> %s\n", 
                     device_id, relay_index, state ? "ON" : "OFF");
        g_stats.commands_sent++;
    } else {
        Serial.println("⚠ Command queue full!");
    }
    
    return queued;
}

bool sendBroadcastCommand(bool state, uint8_t profile) {
    uint8_t payload[3];
    payload[0] = 0xFF;
    payload[1] = state ? 1 : 0;
    payload[2] = profile;
    
    bool queued = commandQueueAdd(0x00, CMD_SET_RELAY, payload, 3);
    
    if (queued) {
        Serial.printf("✓ Broadcast command queued: %s (profile=%d)\n", 
                     state ? "ALL ON" : "ALL OFF", profile);
        g_stats.commands_sent++;
    }
    
    return queued;
}

bool approveEnrollment(const char* device_name) {
    if (!g_pending_enrollment.active) {
        Serial.println("⚠ No pending enrollment");
        return false;
    }
    
    uint8_t payload[39];
    memcpy(payload, g_pending_enrollment.mac, 6);
    strncpy((char*)&payload[6], device_name, 32);
    payload[38] = g_pending_enrollment.profile;
    
    bool queued = commandQueueAdd(LOGIC_MASTER_ID, CMD_DISCOVERY_RESPONSE, payload, 39);
    
    if (queued) {
        char mac_str[18];
        formatMAC(g_pending_enrollment.mac, mac_str);
        Serial.printf("✓ Enrollment approved: %s -> %s\n", mac_str, device_name);
        
        g_pending_enrollment.active = false;
        uiEnrollmentHide();
        
        return true;
    }
    
    return false;
}

bool rejectEnrollment() {
    if (!g_pending_enrollment.active) {
        return false;
    }
    
    char mac_str[18];
    formatMAC(g_pending_enrollment.mac, mac_str);
    Serial.printf("✗ Enrollment rejected: %s\n", mac_str);
    
    g_pending_enrollment.active = false;
    uiEnrollmentHide();
    
    return true;
}