/**
 * ============================================================================
 * UI ENROLLMENT MODULE
 * Display Master - Smart Mosque Ecosystem v1.0
 * ============================================================================
 * 
 * Fungsi: Auto-enrollment popup untuk approve device baru
 * Flow: Device announce → Show popup → User input name → Approve/Reject
 * 
 * Referensi:
 * - [KT-2.3] Auto-Enrollment Flow
 * - [STP-3.1] DISCOVERY_ANNOUNCE
 * - [STP-3.2] DISCOVERY_RESPONSE
 * 
 * ============================================================================
 */

#include "../include/display_master.h"

// ============================================================================
// UI COMPONENTS - GLOBAL OBJECTS
// ============================================================================

// Modal background (dark overlay)
static lv_obj_t* modal_bg = nullptr;

// Popup panel
static lv_obj_t* popup_panel = nullptr;

// Labels
static lv_obj_t* label_title = nullptr;
static lv_obj_t* label_mac = nullptr;
static lv_obj_t* label_profile = nullptr;
static lv_obj_t* label_instruction = nullptr;

// Text input for device name
static lv_obj_t* textarea_name = nullptr;
static lv_obj_t* keyboard = nullptr;

// Buttons
static lv_obj_t* btn_approve = nullptr;
static lv_obj_t* btn_reject = nullptr;

// ============================================================================
// EVENT HANDLERS
// ============================================================================

/**
 * Textarea focus event
 * Show keyboard saat textarea di-click
 */
void uiEnrollmentTextareaHandler(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    
    if (code == LV_EVENT_FOCUSED) {
        if (keyboard) {
            lv_obj_clear_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
        }
    } else if (code == LV_EVENT_DEFOCUSED) {
        if (keyboard) {
            lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

/**
 * Approve button handler
 * Send device name ke Logic Master untuk enrollment
 */
void uiEnrollmentApproveHandler(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    
    if (code == LV_EVENT_CLICKED) {
        // Get device name dari textarea
        const char* device_name = lv_textarea_get_text(textarea_name);
        
        // Validate name (min 3 characters)
        if (strlen(device_name) < 3) {
            Serial.println("⚠ Device name too short");
            uiDashboardShowAlert("Device name must be at least 3 characters", 1);
            return;
        }
        
        Serial.printf("✓ Approving enrollment: %s\n", device_name);
        
        // Call approval function (defined in main.cpp)
        bool success = approveEnrollment(device_name);
        
        if (success) {
            uiDashboardShowAlert("Device enrolled successfully", 0);
        } else {
            uiDashboardShowAlert("Enrollment failed", 2);
        }
    }
}

/**
 * Reject button handler
 * Reject device enrollment
 */
void uiEnrollmentRejectHandler(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    
    if (code == LV_EVENT_CLICKED) {
        Serial.println("✗ Enrollment rejected by user");
        
        // Call rejection function (defined in main.cpp)
        rejectEnrollment();
        
        uiDashboardShowAlert("Device enrollment rejected", 1);
    }
}

// ============================================================================
// UI CREATION FUNCTIONS
// ============================================================================

/**
 * Create enrollment popup
 * Called dari setup()
 */
void uiEnrollmentCreate() {
    Serial.println("🎨 Creating enrollment popup...");
    
    // Modal background (semi-transparent overlay)
    modal_bg = lv_obj_create(lv_scr_act());
    lv_obj_set_size(modal_bg, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(modal_bg, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(modal_bg, LV_OPA_50, 0);
    lv_obj_clear_flag(modal_bg, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(modal_bg, LV_OBJ_FLAG_HIDDEN);  // Hidden by default
    
    // Popup panel (centered)
    popup_panel = lv_obj_create(modal_bg);
    lv_obj_set_size(popup_panel, 600, 450);
    lv_obj_center(popup_panel);
    lv_obj_set_style_bg_color(popup_panel, lv_color_hex(0x2C3E50), 0);
    lv_obj_set_style_border_width(popup_panel, 3, 0);
    lv_obj_set_style_border_color(popup_panel, lv_color_hex(0x1ABC9C), 0);
    lv_obj_set_style_radius(popup_panel, 15, 0);
    
    // Title
    label_title = lv_label_create(popup_panel);
    lv_label_set_text(label_title, LV_SYMBOL_PLUS " NEW DEVICE DETECTED");
    lv_obj_set_style_text_font(label_title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(label_title, lv_color_hex(0x1ABC9C), 0);
    lv_obj_align(label_title, LV_ALIGN_TOP_MID, 0, 20);
    
    // MAC address label
    label_mac = lv_label_create(popup_panel);
    lv_label_set_text(label_mac, "MAC: 00:00:00:00:00:00");
    lv_obj_set_style_text_font(label_mac, &lv_font_montserrat_16, 0);
    lv_obj_align(label_mac, LV_ALIGN_TOP_MID, 0, 60);
    
    // Profile label
    label_profile = lv_label_create(popup_panel);
    lv_label_set_text(label_profile, "Profile: UNKNOWN");
    lv_obj_set_style_text_font(label_profile, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(label_profile, lv_color_hex(0xF39C12), 0);
    lv_obj_align(label_profile, LV_ALIGN_TOP_MID, 0, 90);
    
    // Instruction label
    label_instruction = lv_label_create(popup_panel);
    lv_label_set_text(label_instruction, "Please enter a name for this device:");
    lv_obj_align(label_instruction, LV_ALIGN_TOP_MID, 0, 130);
    
    // Textarea for device name
    textarea_name = lv_textarea_create(popup_panel);
    lv_obj_set_size(textarea_name, 550, 60);
    lv_obj_align(textarea_name, LV_ALIGN_TOP_MID, 0, 165);
    lv_textarea_set_placeholder_text(textarea_name, "Device Name");
    lv_textarea_set_one_line(textarea_name, true);
    lv_textarea_set_max_length(textarea_name, 31);  // Max 31 chars (留1个给null)
    lv_obj_set_style_text_font(textarea_name, &lv_font_montserrat_18, 0);
    lv_obj_add_event_cb(textarea_name, uiEnrollmentTextareaHandler, LV_EVENT_ALL, nullptr);
    
    // Keyboard (hidden by default)
    keyboard = lv_keyboard_create(popup_panel);
    lv_obj_set_size(keyboard, 550, 160);
    lv_obj_align(keyboard, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_keyboard_set_textarea(keyboard, textarea_name);
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    
    // Approve button
    btn_approve = lv_btn_create(popup_panel);
    lv_obj_set_size(btn_approve, 250, 60);
    lv_obj_align(btn_approve, LV_ALIGN_BOTTOM_LEFT, 20, -180);
    lv_obj_set_style_bg_color(btn_approve, lv_color_hex(0x27AE60), 0);
    lv_obj_add_event_cb(btn_approve, uiEnrollmentApproveHandler, LV_EVENT_CLICKED, nullptr);
    
    lv_obj_t* label = lv_label_create(btn_approve);
    lv_label_set_text(label, LV_SYMBOL_OK " APPROVE");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);
    lv_obj_center(label);
    
    // Reject button
    btn_reject = lv_btn_create(popup_panel);
    lv_obj_set_size(btn_reject, 250, 60);
    lv_obj_align(btn_reject, LV_ALIGN_BOTTOM_RIGHT, -20, -180);
    lv_obj_set_style_bg_color(btn_reject, lv_color_hex(0xE74C3C), 0);
    lv_obj_add_event_cb(btn_reject, uiEnrollmentRejectHandler, LV_EVENT_CLICKED, nullptr);
    
    label = lv_label_create(btn_reject);
    lv_label_set_text(label, LV_SYMBOL_CLOSE " REJECT");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);
    lv_obj_center(label);
    
    // Set as global reference
    g_enrollment_popup = modal_bg;
    
    Serial.println("✓ Enrollment popup created");
}

// ============================================================================
// PUBLIC API
// ============================================================================

/**
 * Show enrollment popup
 * Called saat terima DISCOVERY_ANNOUNCE
 * 
 * @param mac MAC address device baru
 * @param profile Profile device
 */
void uiEnrollmentShow(const uint8_t* mac, uint8_t profile) {
    if (!modal_bg) return;
    
    // Format MAC address
    char mac_str[20];
    formatMAC(mac, mac_str);
    
    // Update labels
    lv_label_set_text_fmt(label_mac, "MAC: %s", mac_str);
    lv_label_set_text_fmt(label_profile, "Profile: %s", getProfileName(profile));
    
    // Clear textarea
    lv_textarea_set_text(textarea_name, "");
    
    // Hide keyboard
    if (keyboard) {
        lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    }
    
    // Show modal
    lv_obj_clear_flag(modal_bg, LV_OBJ_FLAG_HIDDEN);
    
    // Focus textarea
    lv_obj_add_state(textarea_name, LV_STATE_FOCUSED);
    
    Serial.printf("📱 Showing enrollment popup: %s (%s)\n", 
                 mac_str, getProfileName(profile));
}

/**
 * Hide enrollment popup
 * Called setelah approve/reject
 */
void uiEnrollmentHide() {
    if (!modal_bg) return;
    
    // Hide keyboard
    if (keyboard) {
        lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    }
    
    // Hide modal
    lv_obj_add_flag(modal_bg, LV_OBJ_FLAG_HIDDEN);
    
    // Clear textarea
    lv_textarea_set_text(textarea_name, "");
    
    Serial.println("📱 Enrollment popup hidden");
}

/**
 * Check if enrollment popup is currently shown
 * @return true if visible
 */
bool uiEnrollmentIsShown() {
    if (!modal_bg) return false;
    return !lv_obj_has_flag(modal_bg, LV_OBJ_FLAG_HIDDEN);
}
