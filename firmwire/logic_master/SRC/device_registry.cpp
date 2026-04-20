/**
 * ═══════════════════════════════════════════════════════════════
 *  LOGIC MASTER - DEVICE REGISTRY MANAGEMENT
 * 
 *  Handles all device database operations:
 *  - CRUD operations for device records
 *  - NVS persistence with compare-before-write
 *  - Online/offline status tracking
 *  - Database queries and searches
 * 
 *  Referensi: [KT-4.1] Device Registry (Logic Master)
 * ═══════════════════════════════════════════════════════════════
 */

#include "logic_master.h"

// ══════════════════════════════════════════════════════════════
// INITIALIZATION
// ══════════════════════════════════════════════════════════════

/**
 * Initialize device registry
 * Clear RAM cache and prepare for use
 */
void initDeviceRegistry() {
    Serial.println("→ Initializing device registry...");
    
    // Clear RAM cache
    memset(deviceRegistry, 0, sizeof(deviceRegistry));
    deviceCount = 0;
    
    // Initialize all IDs as unassigned
    for (int i = 0; i < MAX_DEVICES; i++) {
        deviceRegistry[i].id = 0;  // 0 = unassigned
    }
    
    Serial.println("✓ Device registry initialized");
}

// ══════════════════════════════════════════════════════════════
// NVS PERSISTENCE
// Referensi: [KT-4.3] Periodic Flush Strategy
// ══════════════════════════════════════════════════════════════

/**
 * Load device registry from NVS to RAM
 * @return true if successful, false if no data or error
 */
bool loadRegistryFromNVS() {
    preferences.begin("smart_mosque", true);  // Read-only
    
    // Read device count
    deviceCount = preferences.getUChar("dev_count", 0);
    
    if (deviceCount == 0) {
        preferences.end();
        Serial.println("  No devices in NVS");
        return false;
    }
    
    Serial.printf("  Loading %d devices from NVS...\\n", deviceCount);
    
    // Load each device
    int loadedCount = 0;
    for (int i = 1; i <= MAX_DEVICES; i++) {
        String key = "dev_" + String(i);
        size_t size = preferences.getBytes(key.c_str(), 
                                          &deviceRegistry[i], 
                                          sizeof(MasterDeviceRecord));
        
        if (size == sizeof(MasterDeviceRecord) && deviceRegistry[i].id != 0) {
            loadedCount++;
            deviceRegistry[i].dirty = false;  // Fresh from NVS
        }
    }
    
    preferences.end();
    
    Serial.printf("✓ Loaded %d devices successfully\\n", loadedCount);
    return (loadedCount > 0);
}

/**
 * Save entire registry to NVS
 * WARNING: This writes ALL devices, slow operation!
 * Use flushDirtyDevices() for incremental updates
 * 
 * @return true if successful
 */
bool saveRegistryToNVS() {
    Serial.println("→ Saving registry to NVS (full write)...");
    
    preferences.begin("smart_mosque", false);  // Read-write
    
    // Write device count
    preferences.putUChar("dev_count", deviceCount);
    
    // Write each device
    int writtenCount = 0;
    for (int i = 1; i <= MAX_DEVICES; i++) {
        if (deviceRegistry[i].id != 0) {
            String key = "dev_" + String(i);
            size_t written = preferences.putBytes(key.c_str(),
                                                  &deviceRegistry[i],
                                                  sizeof(MasterDeviceRecord));
            
            if (written == sizeof(MasterDeviceRecord)) {
                deviceRegistry[i].dirty = false;
                writtenCount++;
            }
        }
    }
    
    preferences.end();
    
    stats.nvs_writes++;
    Serial.printf("✓ Saved %d devices to NVS\\n", writtenCount);
    
    return (writtenCount == deviceCount);
}

/**
 * Flush only dirty devices to NVS (incremental write)
 * Referensi: [KT-4.3] Compare-before-write optimization
 */
void flushDirtyDevices() {
    int dirtyCount = 0;
    
    // Count dirty devices
    for (int i = 1; i <= MAX_DEVICES; i++) {
        if (deviceRegistry[i].id != 0 && deviceRegistry[i].dirty) {
            dirtyCount++;
        }
    }
    
    if (dirtyCount == 0) {
        Serial.println("  No dirty devices, skip NVS flush");
        return;
    }
    
    Serial.printf("→ Flushing %d dirty devices to NVS...\\n", dirtyCount);
    
    preferences.begin("smart_mosque", false);
    
    int flushedCount = 0;
    for (int i = 1; i <= MAX_DEVICES; i++) {
        if (deviceRegistry[i].id != 0 && deviceRegistry[i].dirty) {
            String key = "dev_" + String(i);
            
            // Read old record for comparison
            MasterDeviceRecord oldRecord;
            size_t size = preferences.getBytes(key.c_str(), &oldRecord, 
                                              sizeof(MasterDeviceRecord));
            
            bool needsWrite = true;
            if (size == sizeof(MasterDeviceRecord)) {
                // Compare stable fields only (exclude last_seen, dirty flag)
                if (oldRecord.id == deviceRegistry[i].id &&
                    memcmp(oldRecord.mac, deviceRegistry[i].mac, 6) == 0 &&
                    strcmp(oldRecord.name, deviceRegistry[i].name) == 0 &&
                    oldRecord.profile == deviceRegistry[i].profile &&
                    oldRecord.relay_state == deviceRegistry[i].relay_state &&
                    oldRecord.control_source == deviceRegistry[i].control_source) {
                    needsWrite = false;  // No stable field changed
                }
            }
            
            if (needsWrite) {
                preferences.putBytes(key.c_str(), &deviceRegistry[i],
                                    sizeof(MasterDeviceRecord));
                flushedCount++;
            }
            
            deviceRegistry[i].dirty = false;
        }
    }
    
    preferences.end();
    
    stats.nvs_writes++;
    Serial.printf("✓ Flushed %d devices (skipped %d unchanged)\\n", 
                 flushedCount, dirtyCount - flushedCount);
}

// ══════════════════════════════════════════════════════════════
// CRUD OPERATIONS
// ══════════════════════════════════════════════════════════════

/**
 * Find device by ID
 * @param id Device ID (1-247)
 * @return Pointer to device record, or nullptr if not found
 */
MasterDeviceRecord* findDeviceByID(uint8_t id) {
    if (id == 0 || id > MAX_DEVICES) {
        return nullptr;
    }
    
    if (deviceRegistry[id].id == id) {
        return &deviceRegistry[id];
    }
    
    return nullptr;
}

/**
 * Find device by MAC address
 * @param mac 6-byte MAC address
 * @return Pointer to device record, or nullptr if not found
 */
MasterDeviceRecord* findDeviceByMAC(const uint8_t mac[6]) {
    for (int i = 1; i <= MAX_DEVICES; i++) {
        if (deviceRegistry[i].id != 0) {
            if (memcmp(deviceRegistry[i].mac, mac, 6) == 0) {
                return &deviceRegistry[i];
            }
        }
    }
    return nullptr;
}

/**
 * Add new device to registry
 * @param id Device ID (1-247)
 * @param mac MAC address
 * @param name Device name
 * @param profile Device profile (FAN/LIGHT/SOUND)
 * @return Pointer to new device record, or nullptr if failed
 */
MasterDeviceRecord* addDevice(uint8_t id, const uint8_t mac[6], 
                              const char* name, uint8_t profile) {
    // Validate ID
    if (id == 0 || id > MAX_DEVICES) {
        Serial.printf("✗ Invalid device ID: %d\\n", id);
        return nullptr;
    }
    
    // Check if ID already used
    if (deviceRegistry[id].id != 0) {
        Serial.printf("✗ Device ID %d already exists\\n", id);
        return nullptr;
    }
    
    // Check if MAC already registered
    MasterDeviceRecord* existing = findDeviceByMAC(mac);
    if (existing != nullptr) {
        Serial.printf("✗ MAC already registered as ID %d\\n", existing->id);
        return nullptr;
    }
    
    // Create new record
    MasterDeviceRecord* device = &deviceRegistry[id];
    memset(device, 0, sizeof(MasterDeviceRecord));
    
    device->id = id;
    memcpy(device->mac, mac, 6);
    strncpy(device->name, name, 31);
    device->name[31] = '\\0';
    device->profile = profile;
    device->first_seen = getEpochTime();
    device->last_seen = getEpochTime();
    device->online = true;
    device->relay_state = 0x00;
    device->control_source = CONTROL_AUTO;
    device->error_count = 0;
    device->dirty = true;  // Needs NVS flush
    
    deviceCount++;
    
    char macStr[18];
    SmartMosqueUtils::macToString(mac, macStr);
    Serial.printf("✓ Added device: ID=%d, MAC=%s, Name=%s, Profile=%s\\n",
                 id, macStr, name, SmartMosqueUtils::profileToString(profile));
    
    return device;
}

/**
 * Remove device from registry
 * @param id Device ID
 * @return true if removed, false if not found
 */
bool removeDevice(uint8_t id) {
    MasterDeviceRecord* device = findDeviceByID(id);
    if (device == nullptr) {
        Serial.printf("✗ Device ID %d not found\\n", id);
        return false;
    }
    
    Serial.printf("→ Removing device ID %d (%s)\\n", id, device->name);
    
    // Clear from RAM
    memset(device, 0, sizeof(MasterDeviceRecord));
    device->id = 0;
    
    deviceCount--;
    
    // Clear from NVS
    preferences.begin("smart_mosque", false);
    String key = "dev_" + String(id);
    preferences.remove(key.c_str());
    preferences.putUChar("dev_count", deviceCount);
    preferences.end();
    
    stats.nvs_writes++;
    
    Serial.println("✓ Device removed");
    return true;
}

/**
 * Update device status from STATUS_REPORT
 * @param id Device ID
 * @param status Status data from slave
 */
void updateDeviceStatus(uint8_t id, const DeviceRecord& status) {
    MasterDeviceRecord* device = findDeviceByID(id);
    if (device == nullptr) {
        Serial.printf("✗ Device ID %d not in registry\\n", id);
        return;
    }
    
    // Check for changes (compare-before-write)
    bool changed = false;
    
    if (device->relay_state != status.relay_state) {
        device->relay_state = status.relay_state;
        changed = true;
    }
    
    if (device->control_source != status.control_source) {
        device->control_source = status.control_source;
        changed = true;
    }
    
    // Always update last_seen (but don't mark as dirty)
    device->last_seen = getEpochTime();
    device->last_status_recv = millis();
    device->online = true;
    device->error_count = 0;  // Reset error count on successful report
    
    if (changed) {
        device->dirty = true;  // Mark for NVS flush
    }
}

// ══════════════════════════════════════════════════════════════
// STATUS TRACKING
// ══════════════════════════════════════════════════════════════

/**
 * Update online/offline status for all devices
 * Based on last_seen timestamp
 * Referensi: [KT-4.1] online field calculation
 */
void updateDeviceOnlineStatus() {
    uint32_t now = getEpochTime();
    int onlineCount = 0;
    int offlineCount = 0;
    
    for (int i = 1; i <= MAX_DEVICES; i++) {
        if (deviceRegistry[i].id != 0) {
            uint32_t age = now - deviceRegistry[i].last_seen;
            bool wasOnline = deviceRegistry[i].online;
            deviceRegistry[i].online = (age < DEVICE_OFFLINE_THRESHOLD / 1000);
            
            if (deviceRegistry[i].online) {
                onlineCount++;
            } else {
                offlineCount++;
                
                // Log transition to offline
                if (wasOnline) {
                    Serial.printf("⚠ Device ID %d (%s) went OFFLINE\\n",
                                 deviceRegistry[i].id, deviceRegistry[i].name);
                    deviceRegistry[i].dirty = true;
                }
            }
        }
    }
    
    stats.devices_online = onlineCount;
    stats.devices_offline = offlineCount;
}

/**
 * Get next available device ID
 * Scans registry for first free ID
 * @return Next free ID (1-247), or 0 if registry full
 */
uint8_t getNextAvailableID() {
    for (uint8_t id = 1; id <= MAX_DEVICES; id++) {
        if (deviceRegistry[id].id == 0) {
            return id;
        }
    }
    
    Serial.println("✗ Device registry full (247 devices)!");
    return 0;  // Registry full
}

// ══════════════════════════════════════════════════════════════
// DISPLAY & DEBUG
// ══════════════════════════════════════════════════════════════

/**
 * Print device registry to serial
 * For debugging and status display
 */
void printDeviceRegistry() {
    Serial.println("\\n╔═══════════════════════════════════════════════════════════════╗");
    Serial.println("║                    DEVICE REGISTRY                            ║");
    Serial.println("╠═══════════════════════════════════════════════════════════════╣");
    
    if (deviceCount == 0) {
        Serial.println("║  No devices registered                                        ║");
    } else {
        for (int i = 1; i <= MAX_DEVICES; i++) {
            if (deviceRegistry[i].id != 0) {
                char macStr[18];
                SmartMosqueUtils::macToString(deviceRegistry[i].mac, macStr);
                
                Serial.printf("║ ID: %3d | %-20s | %-8s | %s ║\\n",
                             deviceRegistry[i].id,
                             deviceRegistry[i].name,
                             SmartMosqueUtils::profileToString(deviceRegistry[i].profile),
                             deviceRegistry[i].online ? "ONLINE " : "OFFLINE");
            }
        }
    }
    
    Serial.println("╠═══════════════════════════════════════════════════════════════╣");
    Serial.printf("║ Total: %d devices | Online: %d | Offline: %d              ║\\n",
                 deviceCount, stats.devices_online, stats.devices_offline);
    Serial.println("╚═══════════════════════════════════════════════════════════════╝\\n");
}

// ══════════════════════════════════════════════════════════════
// END OF DEVICE_REGISTRY.CPP
// ══════════════════════════════════════════════════════════════
