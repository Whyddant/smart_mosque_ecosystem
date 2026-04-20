/**
 * ============================================================================
 * UI DEVICE CONTROL MODULE
 * Display Master - Smart Mosque Ecosystem v1.0
 * ============================================================================
 * 
 * Fungsi: Detail control page untuk satu device
 * Features: Individual relay control, status display, mode indicator
 * 
 * Referensi:
 * - LVGL v8.3.9 API
 * - [KT-2.3] Device Control UI
 * - [STP-3.3] SET_RELAY command
 * 
 * ============================================================================
 */

#include "../include/display_master.h"

// ============================================================================
// UI COMPONENTS - GLOBAL OBJECTS
// ============================================================================

// Screen
static lv_obj_t* screen_control = nullptr;

// Current device being controlled
static uint8_t current_device_id = 0;

// Header
static lv_obj_t* label_device_name = nullptr;
static lv_obj_t* label_device_info = nullptr;
static lv_obj_t* btn_back = nullptr;

// Status panel
static lv_obj_t* panel_status = nullptr;
static lv_obj_t* label_online_status = nullptr;
static lv_obj_t* label_control_mode = nullptr;
static lv_obj_t* label_last_update = nullptr;

// Relay control buttons (8 relays max)
static lv_obj_t* relay_buttons[8] = {nullptr};
static lv_obj_t* relay_labels[8] = {nullptr};

// Quick control buttons
static lv_obj_t* btn_all_relays_on = nullptr;
static lv_obj_t* btn_all_relays_off = nullptr;

// Fan speed control (for FAN_4CH profile)
static lv_obj_t* panel_fan_speed = nullptr;
static lv_obj_t* btn_fan_off = nullptr;
static lv_obj_t* btn_fan_low = nullptr;
static lv_obj_t* btn_fan_med = nullptr;
static lv_obj_t* btn_fan_high = nullptr;

// ============================================================================
// EVENT HANDLERS
// ============================================================================

/**
 * Back button handler
 */
void uiDeviceControlBackHandler(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    
    if (code == LV_EVENT_CLICKED) {
        Serial.println("🔙 Back to dashboard");
        lv_scr_load(g_main_screen);
        current_device_id = 0;
    }
}

/**
 * Relay button handler
 * Toggle individual relay ON/OFF
 */
void uiDeviceControlRelayHandler(lv_event_t* e) {
    lv_obj_t* btn = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    
    if (code == LV_EVENT_CLICKED) {
        // Find relay index
        int relay_index = -1;
        for (int i = 0; i < 8; i++) {
            if (relay_buttons[i] == btn) {
                relay_index = i;
                break;
            }
        }
        
        if (relay_index < 0) return;
        
        // Get current device
        DeviceDisplayInfo* device = deviceManagerGet(current_device_id);
        if (!device) return;
        
        // Toggle state
        bool current_state = (device->relay_state & (1 << relay_index)) != 0;
        bool new_state = !current_state;
        
        Serial.printf("🔘 Relay %d: %s -> %s\n", relay_index,
                     current_state ? "ON" : "OFF",
                     new_state ? "ON" : "OFF");
        
        // Send command
        sendRelayCommand(current_device_id, relay_index, new_state);
    }
}

/**
 * All relays ON/OFF handler
 */
void uiDeviceControlAllRelaysHandler(lv_event_t* e) {
    lv_obj_t* btn = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    
    if (code == LV_EVENT_CLICKED) {
        bool turn_on = (btn == btn_all_relays_on);
        
        Serial.printf("🔘 All relays: %s\n", turn_on ? "ON" : "OFF");
        
        DeviceDisplayInfo* device = deviceManagerGet(current_device_id);
        if (!device) return;
        
        int relay_count = (device->profile == PROFILE_FAN_4CH) ? 4 : 8;
        
        // Build proper SET_RELAY payload
        SetRelayPayload payload;
        payload.relay_mask = (1 << relay_count) - 1;
        for (int i = 0; i < relay_count; i++) {
            payload.values[i] = turn_on ? 1 : 0;
        }
        memset(&payload.values[relay_count], 0, 8 - relay_count);
        
        commandQueueAdd(current_device_id, CMD_SET_RELAY, 
                       (uint8_t*)&payload, sizeof(SetRelayPayload));
    }
}

/**
 * Fan speed button handler (FAN_4CH profile only)
 */
void uiDeviceControlFanSpeedHandler(lv_event_t* e) {
    lv_obj_t* btn = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    
    if (code == LV_EVENT_CLICKED) {
        uint8_t speed = 0;  // OFF
        if (btn == btn_fan_low) speed = 1;
        else if (btn == btn_fan_med) speed = 2;
        else if (btn == btn_fan_high) speed = 3;
        
        const char* speed_names[] = {"OFF", "LOW", "MED", "HIGH"};
        Serial.printf("🌀 Fan speed: %s\n", speed_names[speed]);
        
        // Build SET_RELAY payload with interlock
        SetRelayPayload payload;
        payload.relay_mask = 0x07;  // Bit 0-2 (3 speed relays)
        memset(payload.values, 0, 8);
        
        if (speed > 0) {
            payload.values[speed - 1] = 1;
        }
        
        commandQueueAdd(current_device_id, CMD_SET_RELAY, 
                       (uint8_t*)&payload, sizeof(SetRelayPayload));
    }
}

// ============================================================================
// UI CREATION FUNCTIONS
// ============================================================================

/**
 * Create header with device info and back button
 */
void uiDeviceControlCreateHeader(lv_obj_t* parent) {
    // Back button
    btn_back = lv_btn_create(parent);
    lv_obj_set_size(btn_back, 100, 50);
    lv_obj_align(btn_back, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_set_style_bg_color(btn_back, lv_color_hex(0x34495E), 0);
    lv_obj_add_event_cb(btn_back, uiDeviceControlBackHandler, LV_EVENT_CLICKED, nullptr);
    
    lv_obj_t* label = lv_label_create(btn_back);
    lv_label_set_text(label, LV_SYMBOL_LEFT " BACK");
    lv_obj_center(label);
    
    // Device name
    label_device_name = lv_label_create(parent);
    lv_label_set_text(label_device_name, "Device Name");
    lv_obj_set_style_text_font(label_device_name, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(label_device_name, lv_color_hex(0x1ABC9C), 0);
    lv_obj_align(label_device_name, LV_ALIGN_TOP_MID, 0, 15);
    
    // Device info (ID & Profile)
    label_device_info = lv_label_create(parent);
    lv_label_set_text(label_device_info, "ID: 0 | Profile: UNKNOWN");
    lv_obj_align(label_device_info, LV_ALIGN_TOP_MID, 0, 45);
}

/**
 * Create status panel
 */
void uiDeviceControlCreateStatus(lv_obj_t* parent) {
    panel_status = lv_obj_create(parent);
    lv_obj_set_size(panel_status, 760, 80);
    lv_obj_align(panel_status, LV_ALIGN_TOP_MID, 0, 75);
    lv_obj_set_style_bg_color(panel_status, lv_color_hex(0x2C3E50), 0);
    lv_obj_set_style_radius(panel_status, 10, 0);
    
    // Online status
    label_online_status = lv_label_create(panel_status);
    lv_label_set_text(label_online_status, LV_SYMBOL_WIFI " OFFLINE");
    lv_obj_set_style_text_font(label_online_status, &lv_font_montserrat_18, 0);
    lv_obj_align(label_online_status, LV_ALIGN_LEFT_MID, 20, -10);
    
    // Control mode
    label_control_mode = lv_label_create(panel_status);
    lv_label_set_text(label_control_mode, "Mode: AUTO");
    lv_obj_set_style_text_font(label_control_mode, &lv_font_montserrat_18, 0);
    lv_obj_align(label_control_mode, LV_ALIGN_LEFT_MID, 20, 15);
    
    // Last update
    label_last_update = lv_label_create(panel_status);
    lv_label_set_text(label_last_update, "Last update: --");
    lv_obj_align(label_last_update, LV_ALIGN_RIGHT_MID, -20, 0);
}

/**
 * Create relay control buttons (8 buttons)
 */
void uiDeviceControlCreateRelayButtons(lv_obj_t* parent) {
    int btn_width = 170;
    int btn_height = 60;
    int start_y = 170;
    int spacing_x = 190;
    int spacing_y = 80;
    
    for (int i = 0; i < 8; i++) {
        int row = i / 4;
        int col = i % 4;
        
        relay_buttons[i] = lv_btn_create(parent);
        lv_obj_set_size(relay_buttons[i], btn_width, btn_height);
        lv_obj_align(relay_buttons[i], LV_ALIGN_TOP_LEFT, 
                     20 + col * spacing_x, start_y + row * spacing_y);
        lv_obj_set_style_bg_color(relay_buttons[i], lv_color_hex(0x7F8C8D), 0);  // Gray (OFF)
        lv_obj_add_event_cb(relay_buttons[i], uiDeviceControlRelayHandler, LV_EVENT_CLICKED, nullptr);
        
        relay_labels[i] = lv_label_create(relay_buttons[i]);
        lv_label_set_text_fmt(relay_labels[i], "RELAY %d\nOFF", i + 1);
        lv_obj_set_style_text_align(relay_labels[i], LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_center(relay_labels[i]);
    }
}

/**
 * Create all relays ON/OFF buttons
 */
void uiDeviceControlCreateQuickButtons(lv_obj_t* parent) {
    // ALL ON button
    btn_all_relays_on = lv_btn_create(parent);
    lv_obj_set_size(btn_all_relays_on, 200, 50);
    lv_obj_align(btn_all_relays_on, LV_ALIGN_BOTTOM_LEFT, 20, -20);
    lv_obj_set_style_bg_color(btn_all_relays_on, lv_color_hex(0x27AE60), 0);
    lv_obj_add_event_cb(btn_all_relays_on, uiDeviceControlAllRelaysHandler, LV_EVENT_CLICKED, nullptr);
    
    lv_obj_t* label = lv_label_create(btn_all_relays_on);
    lv_label_set_text(label, LV_SYMBOL_POWER " ALL ON");
    lv_obj_center(label);
    
    // ALL OFF button
    btn_all_relays_off = lv_btn_create(parent);
    lv_obj_set_size(btn_all_relays_off, 200, 50);
    lv_obj_align(btn_all_relays_off, LV_ALIGN_BOTTOM_LEFT, 230, -20);
    lv_obj_set_style_bg_color(btn_all_relays_off, lv_color_hex(0xE74C3C), 0);
    lv_obj_add_event_cb(btn_all_relays_off, uiDeviceControlAllRelaysHandler, LV_EVENT_CLICKED, nullptr);
    
    label = lv_label_create(btn_all_relays_off);
    lv_label_set_text(label, LV_SYMBOL_POWER " ALL OFF");
    lv_obj_center(label);
}

/**
 * Create fan speed control panel (FAN_4CH only)
 * Hidden by default, shown when FAN device selected
 */
void uiDeviceControlCreateFanSpeedPanel(lv_obj_t* parent) {
    panel_fan_speed = lv_obj_create(parent);
    lv_obj_set_size(panel_fan_speed, 500, 80);
    lv_obj_align(panel_fan_speed, LV_ALIGN_BOTTOM_RIGHT, -20, -20);
    lv_obj_set_style_bg_color(panel_fan_speed, lv_color_hex(0x34495E), 0);
    lv_obj_set_style_radius(panel_fan_speed, 10, 0);
    lv_obj_add_flag(panel_fan_speed, LV_OBJ_FLAG_HIDDEN);  // Hidden by default
    
    lv_obj_t* label = lv_label_create(panel_fan_speed);
    lv_label_set_text(label, LV_SYMBOL_REFRESH " FAN SPEED:");
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 10, 0);
    
    int btn_width = 90;
    int btn_height = 50;
    int start_x = 140;
    
    // OFF button
    btn_fan_off = lv_btn_create(panel_fan_speed);
    lv_obj_set_size(btn_fan_off, btn_width, btn_height);
    lv_obj_align(btn_fan_off, LV_ALIGN_LEFT_MID, start_x, 0);
    lv_obj_set_style_bg_color(btn_fan_off, lv_color_hex(0x7F8C8D), 0);
    lv_obj_add_event_cb(btn_fan_off, uiDeviceControlFanSpeedHandler, LV_EVENT_CLICKED, nullptr);
    
    label = lv_label_create(btn_fan_off);
    lv_label_set_text(label, "OFF");
    lv_obj_center(label);
    
    // LOW button
    btn_fan_low = lv_btn_create(panel_fan_speed);
    lv_obj_set_size(btn_fan_low, btn_width, btn_height);
    lv_obj_align(btn_fan_low, LV_ALIGN_LEFT_MID, start_x + 100, 0);
    lv_obj_set_style_bg_color(btn_fan_low, lv_color_hex(0x3498DB), 0);
    lv_obj_add_event_cb(btn_fan_low, uiDeviceControlFanSpeedHandler, LV_EVENT_CLICKED, nullptr);
    
    label = lv_label_create(btn_fan_low);
    lv_label_set_text(label, "LOW");
    lv_obj_center(label);
    
    // MED button
    btn_fan_med = lv_btn_create(panel_fan_speed);
    lv_obj_set_size(btn_fan_med, btn_width, btn_height);
    lv_obj_align(btn_fan_med, LV_ALIGN_LEFT_MID, start_x + 200, 0);
    lv_obj_set_style_bg_color(btn_fan_med, lv_color_hex(0xF39C12), 0);
    lv_obj_add_event_cb(btn_fan_med, uiDeviceControlFanSpeedHandler, LV_EVENT_CLICKED, nullptr);
    
    label = lv_label_create(btn_fan_med);
    lv_label_set_text(label, "MED");
    lv_obj_center(label);
    
    // HIGH button
    btn_fan_high = lv_btn_create(panel_fan_speed);
    lv_obj_set_size(btn_fan_high, btn_width, btn_height);
    lv_obj_align(btn_fan_high, LV_ALIGN_LEFT_MID, start_x + 300, 0);
    lv_obj_set_style_bg_color(btn_fan_high, lv_color_hex(0xE74C3C), 0);
    lv_obj_add_event_cb(btn_fan_high, uiDeviceControlFanSpeedHandler, LV_EVENT_CLICKED, nullptr);
    
    label = lv_label_create(btn_fan_high);
    lv_label_set_text(label, "HIGH");
    lv_obj_center(label);
}

// ============================================================================
// PUBLIC API
// ============================================================================

/**
 * Create device control screen
 * Called dari setup()
 */
void uiDeviceControlCreate() {
    Serial.println("🎨 Creating device control UI...");
    
    // Create screen
    screen_control = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(screen_control, lv_color_hex(0x1C2833), 0);
    
    // Create UI components
    uiDeviceControlCreateHeader(screen_control);
    uiDeviceControlCreateStatus(screen_control);
    uiDeviceControlCreateRelayButtons(screen_control);
    uiDeviceControlCreateQuickButtons(screen_control);
    uiDeviceControlCreateFanSpeedPanel(screen_control);
    
    // Set as global reference
    g_control_screen = screen_control;
    
    Serial.println("✓ Device control UI created");
}

/**
 * Show control screen untuk device tertentu
 * Called dari dashboard (table click)
 * 
 * @param device_id Target device ID
 */
void uiDeviceControlShow(uint8_t device_id) {
    DeviceDisplayInfo* device = deviceManagerGet(device_id);
    if (!device) {
        Serial.printf("⚠ Device %d not found\n", device_id);
        return;
    }
    
    // Set current device
    current_device_id = device_id;
    
    // Update header
    lv_label_set_text(label_device_name, device->name);
    lv_label_set_text_fmt(label_device_info, "ID: %d | Profile: %s", 
                          device->id, getProfileName(device->profile));
    
    // Show/hide fan speed panel based on profile
    if (device->profile == PROFILE_FAN_4CH) {
        lv_obj_clear_flag(panel_fan_speed, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(panel_fan_speed, LV_OBJ_FLAG_HIDDEN);
    }
    
    // Update display
    uiDeviceControlUpdate();
    
    // Load screen
    lv_scr_load(screen_control);
    
    Serial.printf("📱 Showing control for device %d (%s)\n", device_id, device->name);
}

/**
 * Update control screen
 * Called periodically atau setelah command
 */
void uiDeviceControlUpdate() {
    if (current_device_id == 0) return;
    
    DeviceDisplayInfo* device = deviceManagerGet(current_device_id);
    if (!device) return;
    
    // Update status panel
    if (device->online) {
        lv_label_set_text(label_online_status, LV_SYMBOL_WIFI " ONLINE");
        lv_obj_set_style_text_color(label_online_status, lv_color_hex(0x27AE60), 0);
    } else {
        lv_label_set_text(label_online_status, LV_SYMBOL_WIFI " OFFLINE");
        lv_obj_set_style_text_color(label_online_status, lv_color_hex(0xE74C3C), 0);
    }
    
    // Update control mode
    lv_label_set_text_fmt(label_control_mode, "Mode: %s", 
                          getControlSourceName(device->control_source));
    
    // Color by mode
    if (device->control_source == 0) {  // AUTO
        lv_obj_set_style_text_color(label_control_mode, lv_color_hex(0x1ABC9C), 0);
    } else if (device->control_source == 1) {  // MANUAL
        lv_obj_set_style_text_color(label_control_mode, lv_color_hex(0xF39C12), 0);
    } else {  // FAILSAFE
        lv_obj_set_style_text_color(label_control_mode, lv_color_hex(0xE74C3C), 0);
    }
    
    // Update last update time
    if (device->last_update > 0) {
        uint32_t age = (millis() - device->last_update) / 1000;
        lv_label_set_text_fmt(label_last_update, "Last update: %u sec ago", age);
    } else {
        lv_label_set_text(label_last_update, "Last update: --");
    }
    
    // Update relay buttons
    int relay_count = (device->profile == PROFILE_FAN_4CH) ? 4 : 8;
    
    for (int i = 0; i < 8; i++) {
        if (i < relay_count) {
            // Check relay state
            bool state = (device->relay_state & (1 << i)) != 0;
            
            // Update button color
            if (state) {
                lv_obj_set_style_bg_color(relay_buttons[i], lv_color_hex(0x27AE60), 0);  // Green
            } else {
                lv_obj_set_style_bg_color(relay_buttons[i], lv_color_hex(0x7F8C8D), 0);  // Gray
            }
            
            // Update label
            const char* relay_name = "RELAY";
            if (device->profile == PROFILE_FAN_4CH) {
                const char* names[] = {"LOW", "MED", "HIGH", "RES"};
                relay_name = names[i];
            }
            
            lv_label_set_text_fmt(relay_labels[i], "%s %d\n%s", 
                                 relay_name, i + 1, state ? "ON" : "OFF");
            
            // Enable button
            lv_obj_clear_state(relay_buttons[i], LV_STATE_DISABLED);
            
        } else {
            // Hide unused relays
            lv_obj_set_style_bg_color(relay_buttons[i], lv_color_hex(0x34495E), 0);
            lv_label_set_text(relay_labels[i], "--");
            lv_obj_add_state(relay_buttons[i], LV_STATE_DISABLED);
        }
    }
    
    // Disable controls jika device offline atau MANUAL mode
    bool controls_enabled = device->online && (device->control_source == 0);  // AUTO only
    
    if (!controls_enabled) {
        // Disable all control buttons
        for (int i = 0; i < relay_count; i++) {
            lv_obj_add_state(relay_buttons[i], LV_STATE_DISABLED);
        }
        lv_obj_add_state(btn_all_relays_on, LV_STATE_DISABLED);
        lv_obj_add_state(btn_all_relays_off, LV_STATE_DISABLED);
        
        if (device->profile == PROFILE_FAN_4CH) {
            lv_obj_add_state(btn_fan_off, LV_STATE_DISABLED);
            lv_obj_add_state(btn_fan_low, LV_STATE_DISABLED);
            lv_obj_add_state(btn_fan_med, LV_STATE_DISABLED);
            lv_obj_add_state(btn_fan_high, LV_STATE_DISABLED);
        }
        
    } else {
        // Enable controls
        for (int i = 0; i < relay_count; i++) {
            lv_obj_clear_state(relay_buttons[i], LV_STATE_DISABLED);
        }
        lv_obj_clear_state(btn_all_relays_on, LV_STATE_DISABLED);
        lv_obj_clear_state(btn_all_relays_off, LV_STATE_DISABLED);
        
        if (device->profile == PROFILE_FAN_4CH) {
            lv_obj_clear_state(btn_fan_off, LV_STATE_DISABLED);
            lv_obj_clear_state(btn_fan_low, LV_STATE_DISABLED);
            lv_obj_clear_state(btn_fan_med, LV_STATE_DISABLED);
            lv_obj_clear_state(btn_fan_high, LV_STATE_DISABLED);
        }
    }
}
