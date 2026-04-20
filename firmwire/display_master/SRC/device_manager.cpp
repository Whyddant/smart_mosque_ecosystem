/**
 * ============================================================================
 * DEVICE MANAGER MODULE
 * Display Master - Smart Mosque Ecosystem v1.0
 * ============================================================================
 * 
 * Fungsi: Manage device database, online status, dan sinkronisasi
 * Data Source: Logic Master (via RS-485) atau NVS cache
 * 
 * Referensi:
 * - [KT-4.1] Device Registry
 * - [KT-2.3] Data Flow - Display Master receives status
 * 
 * ============================================================================
 */

#include "../include/display_master.h"

// ============================================================================
// DEVICE MANAGER INITIALIZATION
// ============================================================================

/**
 * Initialize device manager
 * Load device cache dari NVS atau request dari Logic Master
 */
void deviceManagerInit() {
    Serial.println("📱 Initializing device manager...");
    
    // Clear device array
    memset(g_devices, 0, sizeof(g_devices));
    g_device_count = 0;
    
    // Try load dari NVS cache
    if (g_preferences.begin("display_master", true)) {  // Read-only
        
        g_device_count = g_preferences.getUChar("dev_count", 0);
        
        if (g_device_count > 0 && g_device_count <= MAX_DEVICES) {
            Serial.printf("  Loading %d devices from NVS cache...\n", g_device_count);
            
            // Load each device
            for (uint8_t i = 0; i < g_device_count; i++) {
                String key = String("dev_") + i;
                size_t len = g_preferences.getBytes(key.c_str(), &g_devices[i], sizeof(DeviceDisplayInfo));
                
                if (len == sizeof(DeviceDisplayInfo)) {
                    // Mark as potentially offline (will be updated by STATUS_REPORT)
                    g_devices[i].online = false;
                    g_devices[i].last_update = 0;
                    
                    Serial.printf("  ✓ Loaded: ID=%d, Name=\"%s\", Profile=%s\n",
                                 g_devices[i].id, g_devices[i].name, 
                                 getProfileName(g_devices[i].profile));
                } else {
                    Serial.printf("  ⚠ Failed to load device %d\n", i);
                }
            }
        } else {
            Serial.println("  No cached devices found");
        }
        
        g_preferences.end();
    }
    
    Serial.printf("✓ Device manager initialized (%d devices cached)\n", g_device_count);
}

// ============================================================================
// DEVICE DATABASE OPERATIONS
// ============================================================================

/**
 * Get device by ID
 * @param id Device ID (1-247)
 * @return Pointer to device info atau NULL jika tidak found
 */
DeviceDisplayInfo* deviceManagerGet(uint8_t id) {
    if (id == 0 || id > 247) {
        return nullptr;
    }
    
    // Linear search (OK untuk max 247 devices)
    for (uint8_t i = 0; i < g_device_count; i++) {
        if (g_devices[i].id == id) {
            return &g_devices[i];
        }
    }
    
    return nullptr;  // Not found
}

/**
 * Get device count
 */
uint8_t deviceManagerGetCount() {
    return g_device_count;
}

/**
 * Add new device to database
 * Called saat device baru enrolled
 * 
 * @param id Device ID yang di-assign Logic Master
 * @param name Device name dari user
 * @param profile Device profile
 * @return true if added successfully
 */
bool deviceManagerAdd(uint8_t id, const char* name, uint8_t profile) {
    if (g_device_count >= MAX_DEVICES) {
        Serial.println("⚠ Device database full!");
        return false;
    }
    
    // Check if ID already exists
    if (deviceManagerGet(id) != nullptr) {
        Serial.printf("⚠ Device ID %d already exists!\n", id);
        return false;
    }
    
    // Add new device
    DeviceDisplayInfo* device = &g_devices[g_device_count];
    device->id = id;
    strncpy(device->name, name, sizeof(device->name) - 1);
    device->name[sizeof(device->name) - 1] = '\0';  // Null terminate
    device->profile = profile;
    device->online = false;  // Will be updated by STATUS_REPORT
    device->relay_state = 0;
    device->control_source = 0;
    device->last_update = millis();
    
    g_device_count++;
    
    Serial.printf("✓ Device added: ID=%d, Name=\"%s\", Profile=%s\n",
                 id, name, getProfileName(profile));
    
    // Save to NVS cache
    deviceManagerSaveCache();
    
    return true;
}

/**
 * Update device info dari STATUS_REPORT
 * Called saat terima STATUS_REPORT frame
 * 
 * @param id Device ID
 * @param relay_state Relay state bitmask (bit 0-7)
 * @param control_source Control source (0=AUTO, 1=MANUAL, 2=FAILSAFE)
 */
void deviceManagerUpdate(uint8_t id, uint8_t relay_state, uint8_t control_source) {
    DeviceDisplayInfo* device = deviceManagerGet(id);
    
    if (device == nullptr) {
        // Device not in database, bisa jadi device baru yang belum sync
        #ifdef DEBUG_DEVICE_MANAGER
        Serial.printf("⚠ Received STATUS_REPORT for unknown device %d\n", id);
        #endif
        return;
    }
    
    // Update device info
    bool was_offline = !device->online;
    bool mode_changed = (device->control_source != control_source);
    bool relay_changed = (device->relay_state != relay_state);
    
    device->relay_state = relay_state;
    device->control_source = control_source;
    device->online = true;
    device->last_update = millis();
    
    // Log significant changes
    if (was_offline) {
        Serial.printf("✓ Device %d (%s) is now ONLINE\n", id, device->name);
    }
    
    if (mode_changed) {
        Serial.printf("📢 Device %d mode changed: %s\n", 
                     id, getControlSourceName(control_source));
        
        // Show alert untuk MANUAL or FAILSAFE mode
        if (control_source == 1) {  // MANUAL
            char msg[64];
            snprintf(msg, sizeof(msg), "%s in MANUAL mode", device->name);
            uiDashboardShowAlert(msg, 1);  // Warning level
            
        } else if (control_source == 2) {  // FAILSAFE
            char msg[64];
            snprintf(msg, sizeof(msg), "%s in FAIL-SAFE!", device->name);
            uiDashboardShowAlert(msg, 2);  // Error level
        }
    }
    
    #ifdef DEBUG_DEVICE_MANAGER
    if (relay_changed) {
        Serial.printf("🔄 Device %d relay state: 0x%02X\n", id, relay_state);
    }
    #endif
}

/**
 * Check device online status
 * Mark devices offline jika tidak update >310 detik [KT-3.1]
 * Called periodically dari main loop (setiap 5 detik)
 */
void deviceManagerCheckOnlineStatus() {
    uint32_t now = millis();
    uint8_t offline_count = 0;
    
    for (uint8_t i = 0; i < g_device_count; i++) {
        DeviceDisplayInfo* device = &g_devices[i];
        
        if (device->online) {
            // Check timeout
            uint32_t age = now - device->last_update;
            
            if (age > DEVICE_OFFLINE_TIMEOUT) {
                // Mark offline
                device->online = false;
                offline_count++;
                
                Serial.printf("⚠ Device %d (%s) is now OFFLINE (last seen %u sec ago)\n",
                             device->id, device->name, age / 1000);
                
                // Show alert
                char msg[64];
                snprintf(msg, sizeof(msg), "%s offline", device->name);
                uiDashboardShowAlert(msg, 1);
            }
        }
    }
    
    if (offline_count > 0) {
        Serial.printf("📊 Online: %d/%d devices\n", 
                     g_device_count - offline_count, g_device_count);
    }
}

// ============================================================================
// SYNCHRONIZATION WITH LOGIC MASTER
// ============================================================================

/**
 * Request full device list dari Logic Master
 * Send custom command untuk sync database
 * 
 * Logic Master akan respond dengan DEVICE_LIST_SYNC frame
 */
void deviceManagerSyncFromLogicMaster() {
    Serial.println("📡 Requesting device list from Logic Master...");
    
    // Send sync request (custom command 0x10)
    // No payload needed
    bool sent = rs485SendFrame(LOGIC_MASTER_ID, 0x10, nullptr, 0);
    
    if (sent) {
        Serial.println("  ✓ Sync request sent");
        // Response akan di-handle di rs485HandleFrame()
    } else {
        Serial.println("  ⚠ Failed to send sync request");
    }
}

/**
 * Save device list to NVS cache
 * Untuk recovery saat restart
 * 
 * Called after:
 * - Device enrollment
 * - Sync dari Logic Master
 * - Periodic backup (optional)
 */
void deviceManagerSaveCache() {
    if (g_preferences.begin("display_master", false)) {  // Read-write
        
        Serial.println("💾 Saving device cache to NVS...");
        
        // Save device count
        g_preferences.putUChar("dev_count", g_device_count);
        
        // Save each device
        for (uint8_t i = 0; i < g_device_count; i++) {
            String key = String("dev_") + i;
            size_t written = g_preferences.putBytes(key.c_str(), &g_devices[i], sizeof(DeviceDisplayInfo));
            
            if (written != sizeof(DeviceDisplayInfo)) {
                Serial.printf("  ⚠ Failed to save device %d\n", i);
            }
        }
        
        g_preferences.end();
        Serial.printf("  ✓ Saved %d devices to NVS\n", g_device_count);
        
    } else {
        Serial.println("  ⚠ Failed to open NVS for writing");
    }
}

/**
 * Clear device cache
 * Untuk debugging atau reset
 */
void deviceManagerClearCache() {
    if (g_preferences.begin("display_master", false)) {
        g_preferences.clear();
        g_preferences.end();
        Serial.println("✓ Device cache cleared");
    }
}

// ============================================================================
// QUERY & STATISTICS FUNCTIONS
// ============================================================================

/**
 * Get devices by profile
 * @param profile Target profile (PROFILE_FAN_4CH, LIGHT_8CH, SOUND_8CH)
 * @param output Array untuk output (must be allocated by caller)
 * @param max_count Maximum devices to return
 * @return Number of devices found
 */
uint8_t deviceManagerGetByProfile(uint8_t profile, DeviceDisplayInfo** output, uint8_t max_count) {
    uint8_t count = 0;
    
    for (uint8_t i = 0; i < g_device_count && count < max_count; i++) {
        if (g_devices[i].profile == profile) {
            output[count++] = &g_devices[i];
        }
    }
    
    return count;
}

/**
 * Get online device count
 */
uint8_t deviceManagerGetOnlineCount() {
    uint8_t count = 0;
    
    for (uint8_t i = 0; i < g_device_count; i++) {
        if (g_devices[i].online) {
            count++;
        }
    }
    
    return count;
}

/**
 * Get device count by profile
 */
uint8_t deviceManagerGetCountByProfile(uint8_t profile) {
    uint8_t count = 0;
    
    for (uint8_t i = 0; i < g_device_count; i++) {
        if (g_devices[i].profile == profile) {
            count++;
        }
    }
    
    return count;
}

/**
 * Get device in MANUAL mode count
 */
uint8_t deviceManagerGetManualModeCount() {
    uint8_t count = 0;
    
    for (uint8_t i = 0; i < g_device_count; i++) {
        if (g_devices[i].control_source == 1) {  // MANUAL
            count++;
        }
    }
    
    return count;
}

/**
 * Get device in FAILSAFE mode count
 */
uint8_t deviceManagerGetFailSafeCount() {
    uint8_t count = 0;
    
    for (uint8_t i = 0; i < g_device_count; i++) {
        if (g_devices[i].control_source == 2) {  // FAILSAFE
            count++;
        }
    }
    
    return count;
}

/**
 * Print device database summary
 * Untuk debugging
 */
void deviceManagerPrintSummary() {
    Serial.println("\n╔═══════════════════════════════════════════════════╗");
    Serial.println("║          DEVICE DATABASE SUMMARY                  ║");
    Serial.println("╚═══════════════════════════════════════════════════╝");
    
    Serial.printf("Total Devices: %d\n", g_device_count);
    Serial.printf("  Online:  %d\n", deviceManagerGetOnlineCount());
    Serial.printf("  Offline: %d\n", g_device_count - deviceManagerGetOnlineCount());
    Serial.println();
    
    Serial.printf("By Profile:\n");
    Serial.printf("  Kipas (FAN_4CH):   %d\n", deviceManagerGetCountByProfile(PROFILE_FAN_4CH));
    Serial.printf("  Lampu (LIGHT_8CH): %d\n", deviceManagerGetCountByProfile(PROFILE_LIGHT_8CH));
    Serial.printf("  Sound (SOUND_8CH): %d\n", deviceManagerGetCountByProfile(PROFILE_SOUND_8CH));
    Serial.println();
    
    Serial.printf("By Control Mode:\n");
    Serial.printf("  AUTO:     %d\n", g_device_count - deviceManagerGetManualModeCount() - deviceManagerGetFailSafeCount());
    Serial.printf("  MANUAL:   %d\n", deviceManagerGetManualModeCount());
    Serial.printf("  FAILSAFE: %d\n", deviceManagerGetFailSafeCount());
    Serial.println();
    
    if (g_device_count > 0) {
        Serial.println("Device List:");
        Serial.println("ID | Name                             | Profile | Online | Mode");
        Serial.println("---|----------------------------------|---------|--------|----------");
        
        for (uint8_t i = 0; i < g_device_count; i++) {
            DeviceDisplayInfo* d = &g_devices[i];
            Serial.printf("%2d | %-32s | %-7s | %-6s | %s\n",
                         d->id, 
                         d->name, 
                         getProfileName(d->profile),
                         d->online ? "YES" : "NO",
                         getControlSourceName(d->control_source));
        }
    }
    
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
}

// ============================================================================
// JSON EXPORT (untuk Web API)
// ============================================================================

/**
 * Export device list ke JSON string
 * Digunakan untuk Web API response
 * 
 * @param output Output buffer
 * @param max_len Maximum buffer size
 * @return Actual JSON length (0 jika error)
 */
size_t deviceManagerExportJSON(char* output, size_t max_len) {
    // Gunakan ArduinoJson untuk build JSON
    // Estimated size: ~100 bytes per device + overhead
    const size_t capacity = JSON_ARRAY_SIZE(g_device_count) + g_device_count * JSON_OBJECT_SIZE(7) + 500;
    DynamicJsonDocument doc(capacity);
    
    JsonArray devices = doc.createNestedArray("devices");
    
    for (uint8_t i = 0; i < g_device_count; i++) {
        DeviceDisplayInfo* d = &g_devices[i];
        
        JsonObject device = devices.createNestedObject();
        device["id"] = d->id;
        device["name"] = d->name;
        device["profile"] = getProfileName(d->profile);
        device["online"] = d->online;
        device["relay_state"] = d->relay_state;
        device["control_source"] = getControlSourceName(d->control_source);
        
        // Relay detail (per-bit)
        JsonArray relays = device.createNestedArray("relays");
        for (int j = 0; j < 8; j++) {
            relays.add((d->relay_state & (1 << j)) ? true : false);
        }
    }
    
    // Add summary
    doc["total"] = g_device_count;
    doc["online"] = deviceManagerGetOnlineCount();
    doc["manual_mode"] = deviceManagerGetManualModeCount();
    doc["failsafe"] = deviceManagerGetFailSafeCount();
    
    // Serialize to output buffer
    size_t len = serializeJson(doc, output, max_len);
    
    if (len >= max_len) {
        Serial.println("⚠ JSON buffer too small");
        return 0;
    }
    
    return len;
}
