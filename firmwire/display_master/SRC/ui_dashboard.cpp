/**
 * ============================================================================
 * UI DASHBOARD MODULE
 * Display Master - Smart Mosque Ecosystem v1.0
 * ============================================================================
 * 
 * Fungsi: Main dashboard UI dengan LVGL
 * Layout: Device overview, statistics, quick controls
 * 
 * Referensi:
 * - LVGL v8.3.9 API
 * - [KT-2.3] Display Master UI requirements
 * 
 * ============================================================================
 */

#include "../include/display_master.h"

// ============================================================================
// UI COMPONENTS - GLOBAL OBJECTS
// ============================================================================

// Screens
static lv_obj_t* screen_dashboard = nullptr;

// Header
static lv_obj_t* label_title = nullptr;
static lv_obj_t* label_time = nullptr;
static lv_obj_t* label_status = nullptr;

// Status cards
static lv_obj_t* card_devices = nullptr;
static lv_obj_t* label_device_count = nullptr;
static lv_obj_t* label_online_count = nullptr;

static lv_obj_t* card_kipas = nullptr;
static lv_obj_t* label_kipas_count = nullptr;

static lv_obj_t* card_lampu = nullptr;
static lv_obj_t* label_lampu_count = nullptr;

static lv_obj_t* card_sound = nullptr;
static lv_obj_t* label_sound_count = nullptr;

// Kartu sensor suhu & kelembaban
static lv_obj_t* card_sensor = nullptr;
static lv_obj_t* label_temp  = nullptr;
static lv_obj_t* label_humid = nullptr;

// Alert panel
static lv_obj_t* panel_alert = nullptr;
static lv_obj_t* label_alert = nullptr;
static uint32_t alert_show_time = 0;

// Device list table
static lv_obj_t* table_devices = nullptr;

// Quick control buttons
static lv_obj_t* btn_all_on = nullptr;
static lv_obj_t* btn_all_off = nullptr;
static lv_obj_t* btn_kipas_on = nullptr;
static lv_obj_t* btn_kipas_off = nullptr;
static lv_obj_t* btn_refresh = nullptr;

// ============================================================================
// LVGL STYLES
// ============================================================================

static lv_style_t style_card;
static lv_style_t style_header;
static lv_style_t style_alert_info;
static lv_style_t style_alert_warning;
static lv_style_t style_alert_error;

/**
 * Initialize custom styles
 */
void uiDashboardInitStyles() {
    // Card style
    lv_style_init(&style_card);
    lv_style_set_radius(&style_card, 10);
    lv_style_set_bg_color(&style_card, lv_color_hex(0x2C3E50));
    lv_style_set_border_width(&style_card, 2);
    lv_style_set_border_color(&style_card, lv_color_hex(0x34495E));
    lv_style_set_pad_all(&style_card, 15);
    
    // Header style
    lv_style_init(&style_header);
    lv_style_set_bg_color(&style_header, lv_color_hex(0x1ABC9C));
    lv_style_set_text_color(&style_header, lv_color_white());
    lv_style_set_pad_all(&style_header, 10);
    
    // Alert styles
    lv_style_init(&style_alert_info);
    lv_style_set_bg_color(&style_alert_info, lv_color_hex(0x3498DB));
    
    lv_style_init(&style_alert_warning);
    lv_style_set_bg_color(&style_alert_warning, lv_color_hex(0xF39C12));
    
    lv_style_init(&style_alert_error);
    lv_style_set_bg_color(&style_alert_error, lv_color_hex(0xE74C3C));
}

// ============================================================================
// EVENT HANDLERS
// ============================================================================

/**
 * Button event handler untuk quick controls
 */
void uiDashboardButtonHandler(lv_event_t* e) {
    lv_obj_t* btn = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    
    if (code == LV_EVENT_CLICKED) {
        
        if (btn == btn_all_on) {
            Serial.println("🔘 Button: ALL ON");
            sendBroadcastCommand(true, 0);  // ALL devices, ALL relays ON
            
        } else if (btn == btn_all_off) {
            Serial.println("🔘 Button: ALL OFF");
            sendBroadcastCommand(false, 0);  // ALL devices, ALL relays OFF
            
        } else if (btn == btn_kipas_on) {
            Serial.println("🔘 Button: KIPAS ON");
            sendBroadcastCommand(true, PROFILE_FAN_4CH);  // Kipas only
            
        } else if (btn == btn_kipas_off) {
            Serial.println("🔘 Button: KIPAS OFF");
            sendBroadcastCommand(false, PROFILE_FAN_4CH);  // Kipas only
            
        } else if (btn == btn_refresh) {
            Serial.println("🔘 Button: REFRESH");
            deviceManagerSyncFromLogicMaster();
        }
    }
}

/**
 * Device table click handler
 * Open device control screen
 */
void uiDashboardTableHandler(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    
    if (code == LV_EVENT_CLICKED) {
        uint16_t row, col;
        lv_table_get_selected_cell(table_devices, &row, &col);
        
        if (row > 0 && row <= g_device_count) {
            // Get device ID dari row
            uint8_t device_id = g_devices[row - 1].id;
            Serial.printf("🔘 Table click: Device %d\n", device_id);
            
            // Show control screen
            uiDeviceControlShow(device_id);
        }
    }
}

// ============================================================================
// UI CREATION FUNCTIONS
// ============================================================================

/**
 * Create header panel
 */
void uiDashboardCreateHeader(lv_obj_t* parent) {
    // Header container
    lv_obj_t* header = lv_obj_create(parent);
    lv_obj_set_size(header, 800, 60);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_add_style(header, &style_header, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    
    // Title
    label_title = lv_label_create(header);
    lv_label_set_text(label_title, LV_SYMBOL_HOME " SMART MOSQUE DASHBOARD");
    lv_obj_set_style_text_font(label_title, &lv_font_montserrat_24, 0);
    lv_obj_align(label_title, LV_ALIGN_LEFT_MID, 10, 0);
    
    // Time (will be updated)
    label_time = lv_label_create(header);
    lv_label_set_text(label_time, "00:00:00");
    lv_obj_set_style_text_font(label_time, &lv_font_montserrat_20, 0);
    lv_obj_align(label_time, LV_ALIGN_RIGHT_MID, -150, 0);
    
    // Status indicator
    label_status = lv_label_create(header);
    lv_label_set_text(label_status, LV_SYMBOL_WIFI " ONLINE");
    lv_obj_align(label_status, LV_ALIGN_RIGHT_MID, -10, 0);
}

/**
 * Create status cards (device count, profile breakdown)
 */
void uiDashboardCreateStatusCards(lv_obj_t* parent) {
    int card_width = 180;
    int card_height = 120;
    int y_pos = 70;
    
    // Card 1: Total Devices
    card_devices = lv_obj_create(parent);
    lv_obj_set_size(card_devices, card_width, card_height);
    lv_obj_align(card_devices, LV_ALIGN_TOP_LEFT, 10, y_pos);
    lv_obj_add_style(card_devices, &style_card, 0);
    
    lv_obj_t* label = lv_label_create(card_devices);
    lv_label_set_text(label, LV_SYMBOL_LIST " TOTAL DEVICES");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 5);
    
    label_device_count = lv_label_create(card_devices);
    lv_label_set_text(label_device_count, "0");
    lv_obj_set_style_text_font(label_device_count, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(label_device_count, lv_color_hex(0x1ABC9C), 0);
    lv_obj_align(label_device_count, LV_ALIGN_CENTER, 0, 10);
    
    label_online_count = lv_label_create(card_devices);
    lv_label_set_text(label_online_count, "0 online");
    lv_obj_align(label_online_count, LV_ALIGN_BOTTOM_MID, 0, -5);
    
    // Card 2: Kipas
    card_kipas = lv_obj_create(parent);
    lv_obj_set_size(card_kipas, card_width, card_height);
    lv_obj_align(card_kipas, LV_ALIGN_TOP_LEFT, 200, y_pos);
    lv_obj_add_style(card_kipas, &style_card, 0);
    
    label = lv_label_create(card_kipas);
    lv_label_set_text(label, LV_SYMBOL_REFRESH " KIPAS");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 5);
    
    label_kipas_count = lv_label_create(card_kipas);
    lv_label_set_text(label_kipas_count, "0");
    lv_obj_set_style_text_font(label_kipas_count, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(label_kipas_count, lv_color_hex(0x3498DB), 0);
    lv_obj_align(label_kipas_count, LV_ALIGN_CENTER, 0, 10);
    
    // Card 3: Lampu
    card_lampu = lv_obj_create(parent);
    lv_obj_set_size(card_lampu, card_width, card_height);
    lv_obj_align(card_lampu, LV_ALIGN_TOP_LEFT, 390, y_pos);
    lv_obj_add_style(card_lampu, &style_card, 0);
    
    label = lv_label_create(card_lampu);
    lv_label_set_text(label, LV_SYMBOL_IMAGE " LAMPU");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 5);
    
    label_lampu_count = lv_label_create(card_lampu);
    lv_label_set_text(label_lampu_count, "0");
    lv_obj_set_style_text_font(label_lampu_count, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(label_lampu_count, lv_color_hex(0xF39C12), 0);
    lv_obj_align(label_lampu_count, LV_ALIGN_CENTER, 0, 10);
    
    // Card 4: Sound
    card_sound = lv_obj_create(parent);
    lv_obj_set_size(card_sound, card_width, card_height);
    lv_obj_align(card_sound, LV_ALIGN_TOP_LEFT, 580, y_pos);
    lv_obj_add_style(card_sound, &style_card, 0);

    label = lv_label_create(card_sound);
    lv_label_set_text(label, LV_SYMBOL_AUDIO " SOUND");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 5);

    label_sound_count = lv_label_create(card_sound);
    lv_label_set_text(label_sound_count, "0");
    lv_obj_set_style_text_font(label_sound_count, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(label_sound_count, lv_color_hex(0x9B59B6), 0);
    lv_obj_align(label_sound_count, LV_ALIGN_CENTER, 0, 10);

    // Kartu sensor suhu & kelembaban (baris kedua, full width)
    card_sensor = lv_obj_create(parent);
    lv_obj_set_size(card_sensor, 760, 55);
    lv_obj_align(card_sensor, LV_ALIGN_TOP_MID, 0, y_pos + card_height + 8);
    lv_obj_add_style(card_sensor, &style_card, 0);

    label = lv_label_create(card_sensor);
    lv_label_set_text(label, LV_SYMBOL_SETTINGS " SENSOR SUHU & KELEMBABAN (SHT20 XY-MD02)");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 8, 0);

    label_temp = lv_label_create(card_sensor);
    lv_label_set_text(label_temp, "T: --.-°C");
    lv_obj_set_style_text_font(label_temp, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(label_temp, lv_color_hex(0xE74C3C), 0);
    lv_obj_align(label_temp, LV_ALIGN_RIGHT_MID, -120, 0);

    label_humid = lv_label_create(card_sensor);
    lv_label_set_text(label_humid, "H: --.-%RH");
    lv_obj_set_style_text_font(label_humid, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(label_humid, lv_color_hex(0x3498DB), 0);
    lv_obj_align(label_humid, LV_ALIGN_RIGHT_MID, -10, 0);
}

/**
 * Create alert panel (hidden by default)
 */
void uiDashboardCreateAlert(lv_obj_t* parent) {
    panel_alert = lv_obj_create(parent);
    lv_obj_set_size(panel_alert, 760, 50);
    lv_obj_align(panel_alert, LV_ALIGN_TOP_MID, 0, 200);
    lv_obj_add_flag(panel_alert, LV_OBJ_FLAG_HIDDEN);  // Hidden by default
    
    label_alert = lv_label_create(panel_alert);
    lv_label_set_text(label_alert, "");
    lv_obj_set_style_text_font(label_alert, &lv_font_montserrat_16, 0);
    lv_obj_center(label_alert);
}

/**
 * Create device list table
 */
void uiDashboardCreateDeviceTable(lv_obj_t* parent) {
    table_devices = lv_table_create(parent);
    lv_obj_set_size(table_devices, 760, 180);
    lv_obj_align(table_devices, LV_ALIGN_TOP_MID, 0, 260);
    
    // Table header
    lv_table_set_col_cnt(table_devices, 5);
    lv_table_set_row_cnt(table_devices, 9);  // 1 header + 8 devices visible
    
    lv_table_set_col_width(table_devices, 0, 50);   // ID
    lv_table_set_col_width(table_devices, 1, 300);  // Name
    lv_table_set_col_width(table_devices, 2, 120);  // Profile
    lv_table_set_col_width(table_devices, 3, 100);  // Status
    lv_table_set_col_width(table_devices, 4, 140);  // Mode
    
    // Header row
    lv_table_set_cell_value(table_devices, 0, 0, "ID");
    lv_table_set_cell_value(table_devices, 0, 1, "Device Name");
    lv_table_set_cell_value(table_devices, 0, 2, "Profile");
    lv_table_set_cell_value(table_devices, 0, 3, "Status");
    lv_table_set_cell_value(table_devices, 0, 4, "Mode");
    
    // Style header row
    for (int i = 0; i < 5; i++) {
        lv_table_set_cell_value_fmt(table_devices, 0, i, "%s", lv_table_get_cell_value(table_devices, 0, i));
    }
    
    // Add click handler
    lv_obj_add_event_cb(table_devices, uiDashboardTableHandler, LV_EVENT_CLICKED, nullptr);
}

/**
 * Create quick control buttons
 */
void uiDashboardCreateControls(lv_obj_t* parent) {
    int btn_width = 140;
    int btn_height = 40;
    int y_pos = 450;
    
    // ALL ON button
    btn_all_on = lv_btn_create(parent);
    lv_obj_set_size(btn_all_on, btn_width, btn_height);
    lv_obj_align(btn_all_on, LV_ALIGN_BOTTOM_LEFT, 20, y_pos);
    lv_obj_set_style_bg_color(btn_all_on, lv_color_hex(0x27AE60), 0);
    lv_obj_add_event_cb(btn_all_on, uiDashboardButtonHandler, LV_EVENT_CLICKED, nullptr);
    
    lv_obj_t* label = lv_label_create(btn_all_on);
    lv_label_set_text(label, LV_SYMBOL_POWER " ALL ON");
    lv_obj_center(label);
    
    // ALL OFF button
    btn_all_off = lv_btn_create(parent);
    lv_obj_set_size(btn_all_off, btn_width, btn_height);
    lv_obj_align(btn_all_off, LV_ALIGN_BOTTOM_LEFT, 170, y_pos);
    lv_obj_set_style_bg_color(btn_all_off, lv_color_hex(0xE74C3C), 0);
    lv_obj_add_event_cb(btn_all_off, uiDashboardButtonHandler, LV_EVENT_CLICKED, nullptr);
    
    label = lv_label_create(btn_all_off);
    lv_label_set_text(label, LV_SYMBOL_POWER " ALL OFF");
    lv_obj_center(label);
    
    // KIPAS ON button
    btn_kipas_on = lv_btn_create(parent);
    lv_obj_set_size(btn_kipas_on, btn_width, btn_height);
    lv_obj_align(btn_kipas_on, LV_ALIGN_BOTTOM_LEFT, 320, y_pos);
    lv_obj_set_style_bg_color(btn_kipas_on, lv_color_hex(0x3498DB), 0);
    lv_obj_add_event_cb(btn_kipas_on, uiDashboardButtonHandler, LV_EVENT_CLICKED, nullptr);
    
    label = lv_label_create(btn_kipas_on);
    lv_label_set_text(label, LV_SYMBOL_REFRESH " KIPAS ON");
    lv_obj_center(label);
    
    // KIPAS OFF button
    btn_kipas_off = lv_btn_create(parent);
    lv_obj_set_size(btn_kipas_off, btn_width, btn_height);
    lv_obj_align(btn_kipas_off, LV_ALIGN_BOTTOM_LEFT, 470, y_pos);
    lv_obj_set_style_bg_color(btn_kipas_off, lv_color_hex(0x95A5A6), 0);
    lv_obj_add_event_cb(btn_kipas_off, uiDashboardButtonHandler, LV_EVENT_CLICKED, nullptr);
    
    label = lv_label_create(btn_kipas_off);
    lv_label_set_text(label, LV_SYMBOL_REFRESH " KIPAS OFF");
    lv_obj_center(label);
    
    // REFRESH button
    btn_refresh = lv_btn_create(parent);
    lv_obj_set_size(btn_refresh, btn_width, btn_height);
    lv_obj_align(btn_refresh, LV_ALIGN_BOTTOM_RIGHT, -20, y_pos);
    lv_obj_set_style_bg_color(btn_refresh, lv_color_hex(0x34495E), 0);
    lv_obj_add_event_cb(btn_refresh, uiDashboardButtonHandler, LV_EVENT_CLICKED, nullptr);
    
    label = lv_label_create(btn_refresh);
    lv_label_set_text(label, LV_SYMBOL_REFRESH " REFRESH");
    lv_obj_center(label);
}

// ============================================================================
// PUBLIC API
// ============================================================================

/**
 * Create main dashboard screen
 * Called dari setup()
 */
void uiDashboardCreate() {
    Serial.println("🎨 Creating dashboard UI...");
    
    // Initialize styles
    uiDashboardInitStyles();
    
    // Create screen
    screen_dashboard = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(screen_dashboard, lv_color_hex(0x1C2833), 0);
    
    // Create UI components
    uiDashboardCreateHeader(screen_dashboard);
    uiDashboardCreateStatusCards(screen_dashboard);
    uiDashboardCreateAlert(screen_dashboard);
    uiDashboardCreateDeviceTable(screen_dashboard);
    uiDashboardCreateControls(screen_dashboard);
    
    // Set as global reference
    g_main_screen = screen_dashboard;
    
    Serial.println("✓ Dashboard UI created");
}

/**
 * Update dashboard data
 * Called periodically dari main loop (500ms)
 */
void uiDashboardUpdate() {
    // Update time (system uptime)
    char time_str[16];
    uint32_t seconds = millis() / 1000;
    uint32_t hours = seconds / 3600;
    seconds %= 3600;
    uint32_t minutes = seconds / 60;
    seconds %= 60;
    snprintf(time_str, sizeof(time_str), "%02u:%02u:%02u", hours, minutes, seconds);
    lv_label_set_text(label_time, time_str);
    
    // Update device counts
    lv_label_set_text_fmt(label_device_count, "%d", g_device_count);
    lv_label_set_text_fmt(label_online_count, "%d online", deviceManagerGetOnlineCount());
    
    lv_label_set_text_fmt(label_kipas_count, "%d", deviceManagerGetCountByProfile(PROFILE_FAN_4CH));
    lv_label_set_text_fmt(label_lampu_count, "%d", deviceManagerGetCountByProfile(PROFILE_LIGHT_8CH));
    lv_label_set_text_fmt(label_sound_count, "%d", deviceManagerGetCountByProfile(PROFILE_SOUND_8CH));

    // Update kartu sensor suhu & kelembaban
    if (g_sensor_data.valid) {
        lv_label_set_text_fmt(label_temp,  "T: %.1f°C",  g_sensor_data.temperature);
        lv_label_set_text_fmt(label_humid, "H: %.1f%%RH", g_sensor_data.humidity);
    } else {
        lv_label_set_text(label_temp,  "T: --.-°C");
        lv_label_set_text(label_humid, "H: --.-%RH");
    }
    
    // Update device table (first 8 devices)
    for (int i = 0; i < 8; i++) {
        int row = i + 1;
        
        if (i < g_device_count) {
            DeviceDisplayInfo* dev = &g_devices[i];
            
            lv_table_set_cell_value_fmt(table_devices, row, 0, "%d", dev->id);
            lv_table_set_cell_value(table_devices, row, 1, dev->name);
            lv_table_set_cell_value(table_devices, row, 2, getProfileName(dev->profile));
            lv_table_set_cell_value(table_devices, row, 3, dev->online ? "ONLINE" : "OFFLINE");
            lv_table_set_cell_value(table_devices, row, 4, getControlSourceName(dev->control_source));
            
        } else {
            // Clear row
            lv_table_set_cell_value(table_devices, row, 0, "");
            lv_table_set_cell_value(table_devices, row, 1, "");
            lv_table_set_cell_value(table_devices, row, 2, "");
            lv_table_set_cell_value(table_devices, row, 3, "");
            lv_table_set_cell_value(table_devices, row, 4, "");
        }
    }
    
    // Hide alert setelah 5 detik
    if (alert_show_time > 0 && millis() - alert_show_time > 5000) {
        lv_obj_add_flag(panel_alert, LV_OBJ_FLAG_HIDDEN);
        alert_show_time = 0;
    }
}

/**
 * Show alert notification
 * @param message Alert message
 * @param level 0=info, 1=warning, 2=error
 */
void uiDashboardShowAlert(const char* message, uint8_t level) {
    // Set message
    lv_label_set_text(label_alert, message);
    
    // Set color by level
    lv_obj_remove_style_all(panel_alert);
    if (level == 0) {
        lv_obj_add_style(panel_alert, &style_alert_info, 0);
    } else if (level == 1) {
        lv_obj_add_style(panel_alert, &style_alert_warning, 0);
    } else {
        lv_obj_add_style(panel_alert, &style_alert_error, 0);
    }
    
    // Show alert
    lv_obj_clear_flag(panel_alert, LV_OBJ_FLAG_HIDDEN);
    alert_show_time = millis();
    
    Serial.printf("🔔 Alert [%d]: %s\n", level, message);
}
