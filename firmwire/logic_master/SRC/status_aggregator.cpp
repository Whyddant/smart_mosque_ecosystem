/**
 * ═══════════════════════════════════════════════════════════════
 *  LOGIC MASTER - STATUS AGGREGATOR
 * 
 *  Collects and processes status reports from all slaves
 *  Updates device registry and provides system-wide status
 *  Referensi: [STP-4.2.5] STATUS_REPORT
 *             [KT-4.1] Device Registry
 * ═══════════════════════════════════════════════════════════════
 */

#include "../include/logic_master.h"
#include <ArduinoJson.h>

// ══════════════════════════════════════════════════════════════
// STATISTICS
// ══════════════════════════════════════════════════════════════

struct StatusStats {
    uint32_t reportsReceived;
    uint32_t parseErrors;
    uint32_t unknownDevices;
    uint32_t statusUpdates;
    uint32_t modeChanges;
    uint32_t failSafeAlerts;
} statusStats;

// ══════════════════════════════════════════════════════════════
// STATUS REPORT HANDLER
// ══════════════════════════════════════════════════════════════

void handleStatusReport(uint8_t fromID, const uint8_t* payload, uint8_t length) {
    statusStats.reportsReceived++;
    
    // Find device in registry
    DeviceRecord* device = getDeviceByID(fromID);
    if (device == nullptr) {
        Serial.printf("⚠ Status from unknown device ID %d\n", fromID);
        statusStats.unknownDevices++;
        return;
    }
    
    // Parse JSON payload
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, payload, length);
    
    if (error) {
        Serial.printf("✗ Status JSON parse error: %s\n", error.c_str());
        statusStats.parseErrors++;
        return;
    }
    
    // Extract fields
    uint8_t deviceID = doc["device_id"];
    const char* controlSourceStr = doc["control_source"];
    JsonArray relayArray = doc["relay_states"];
    
    if (deviceID != fromID) {
        Serial.printf("⚠ Device ID mismatch: frame=%d, payload=%d\n", 
                     fromID, deviceID);
    }
    
    // Parse control source
    uint8_t oldControlSource = device->control_source;
    if (controlSourceStr != nullptr) {
        if (strcmp(controlSourceStr, "AUTO") == 0) {
            device->control_source = CONTROL_AUTO;
        } else if (strcmp(controlSourceStr, "MANUAL") == 0) {
            device->control_source = CONTROL_MANUAL;
        } else if (strcmp(controlSourceStr, "FAILSAFE") == 0) {
            device->control_source = CONTROL_FAILSAFE;
        }
    }
    
    // Check for control source changes
    if (device->control_source != oldControlSource) {
        Serial.printf("→ Device %d control source changed: %s → %s\n",
                     fromID,
                     SmartMosqueUtils::controlSourceToString(oldControlSource),
                     SmartMosqueUtils::controlSourceToString(device->control_source));
        statusStats.modeChanges++;
        
        // Alert if entering fail-safe
        if (device->control_source == CONTROL_FAILSAFE) {
            Serial.printf("⚠⚠⚠ ALERT: Device %d (%s) entered FAIL-SAFE mode!\n",
                         fromID, device->name);
            statusStats.failSafeAlerts++;
            
            // TODO: Send alert to Display Master
        }
        
        // Alert if entering manual mode
        if (device->control_source == CONTROL_MANUAL) {
            Serial.printf("⚠ Device %d (%s) switched to MANUAL mode\n",
                         fromID, device->name);
            
            // TODO: Update Display Master UI
        }
    }
    
    // Parse relay states
    uint8_t oldRelayState = device->relay_state;
    uint8_t newRelayState = 0;
    
    if (!relayArray.isNull() && relayArray.size() >= 8) {
        for (int i = 0; i < 8; i++) {
            if (relayArray[i]) {
                newRelayState |= (1 << i);
            }
        }
    }
    
    device->relay_state = newRelayState;
    
    // Log relay state changes
    if (newRelayState != oldRelayState) {
        Serial.printf("→ Device %d relay state: 0x%02X → 0x%02X\n",
                     fromID, oldRelayState, newRelayState);
    }
    
    // Update last_seen timestamp
    device->last_seen = millis() / 1000;  // Convert to seconds
    device->online = true;
    
    // Mark device as dirty for NVS flush
    markDeviceDirty(fromID);
    
    statusStats.statusUpdates++;
    
    // Profile-specific processing
    if (device->profile == PROFILE_FAN_4CH) {
        handleFanStatus(device, doc);
    }
    
    // Debug output (compact)
    Serial.printf("← Status[%d]: CS=%s, Relays=0x%02X\n",
                 fromID,
                 SmartMosqueUtils::controlSourceToString(device->control_source),
                 device->relay_state);
}

// ══════════════════════════════════════════════════════════════
// PROFILE-SPECIFIC HANDLERS
// ══════════════════════════════════════════════════════════════

void handleFanStatus(DeviceRecord* device, const JsonDocument& doc) {
    const char* fanSpeedStr = doc["fan_speed"];
    const char* modeSwitchStr = doc["mode_switch"];
    
    if (fanSpeedStr != nullptr) {
        Serial.printf("  Fan speed: %s\n", fanSpeedStr);
    }
    
    if (modeSwitchStr != nullptr) {
        Serial.printf("  Mode switch: %s\n", modeSwitchStr);
        
        // Validate consistency
        if (strcmp(modeSwitchStr, "MANUAL") == 0 && 
            device->control_source != CONTROL_MANUAL) {
            Serial.println("  ⚠ Inconsistency: Mode switch MANUAL but control_source AUTO");
        }
    }
}

// ══════════════════════════════════════════════════════════════
// SYSTEM-WIDE STATUS
// ══════════════════════════════════════════════════════════════

void getSystemStatus(JsonDocument& doc) {
    doc.clear();
    
    // Count devices by state
    int totalDevices = 0;
    int onlineDevices = 0;
    int manualMode = 0;
    int failSafeMode = 0;
    int autoMode = 0;
    
    for (int i = 1; i <= 247; i++) {
        DeviceRecord* device = getDeviceByID(i);
        if (device != nullptr) {
            totalDevices++;
            
            if (device->online) {
                onlineDevices++;
            }
            
            switch (device->control_source) {
                case CONTROL_AUTO:
                    autoMode++;
                    break;
                case CONTROL_MANUAL:
                    manualMode++;
                    break;
                case CONTROL_FAILSAFE:
                    failSafeMode++;
                    break;
            }
        }
    }
    
    // Build summary
    doc["total_devices"] = totalDevices;
    doc["online"] = onlineDevices;
    doc["offline"] = totalDevices - onlineDevices;
    doc["auto_mode"] = autoMode;
    doc["manual_mode"] = manualMode;
    doc["failsafe_mode"] = failSafeMode;
    
    // System health
    bool systemHealthy = (failSafeMode == 0) && 
                        (onlineDevices == totalDevices);
    doc["healthy"] = systemHealthy;
    
    // Uptime
    doc["uptime"] = millis() / 1000;
}

void printSystemStatus() {
    StaticJsonDocument<512> doc;
    getSystemStatus(doc);
    
    Serial.println("\n╔════════════════════════════════════════════════════╗");
    Serial.println("║              SYSTEM STATUS                         ║");
    Serial.println("╠════════════════════════════════════════════════════╣");
    Serial.printf("║ Total Devices:    %3d                             ║\n", 
                 (int)doc["total_devices"]);
    Serial.printf("║ Online:           %3d                             ║\n", 
                 (int)doc["online"]);
    Serial.printf("║ Offline:          %3d                             ║\n", 
                 (int)doc["offline"]);
    Serial.println("║                                                    ║");
    Serial.printf("║ AUTO Mode:        %3d                             ║\n", 
                 (int)doc["auto_mode"]);
    Serial.printf("║ MANUAL Mode:      %3d                             ║\n", 
                 (int)doc["manual_mode"]);
    Serial.printf("║ FAIL-SAFE Mode:   %3d                             ║\n", 
                 (int)doc["failsafe_mode"]);
    Serial.println("║                                                    ║");
    
    bool healthy = doc["healthy"];
    Serial.printf("║ System Health:    %s                         ║\n",
                 healthy ? "HEALTHY ✓" : "WARNING ⚠");
    
    Serial.printf("║ Uptime:           %6lu seconds                  ║\n",
                 (unsigned long)doc["uptime"]);
    Serial.println("╚════════════════════════════════════════════════════╝\n");
}

// ══════════════════════════════════════════════════════════════
// DEVICE LIST WITH STATUS
// ══════════════════════════════════════════════════════════════

void printDeviceList() {
    Serial.println("\n╔═══════════════════════════════════════════════════════════════════════╗");
    Serial.println("║                          DEVICE LIST                                  ║");
    Serial.println("╠═══════════════════════════════════════════════════════════════════════╣");
    Serial.println("║ ID │ Name                │ Profile │ Status │ Mode    │ Relays ║");
    Serial.println("╠═══════════════════════════════════════════════════════════════════════╣");
    
    int count = 0;
    for (int i = 1; i <= 247; i++) {
        DeviceRecord* device = getDeviceByID(i);
        if (device != nullptr) {
            count++;
            
            const char* statusStr = device->online ? "ONLINE " : "OFFLINE";
            const char* modeStr = SmartMosqueUtils::controlSourceToString(device->control_source);
            const char* profileStr = SmartMosqueUtils::profileToString(device->profile);
            
            Serial.printf("║ %2d │ %-19s │ %-7s │ %-6s │ %-7s │ 0x%02X   ║\n",
                         device->id,
                         device->name,
                         profileStr,
                         statusStr,
                         modeStr,
                         device->relay_state);
        }
    }
    
    if (count == 0) {
        Serial.println("║                         No devices registered                         ║");
    }
    
    Serial.println("╚═══════════════════════════════════════════════════════════════════════╝\n");
}

// ══════════════════════════════════════════════════════════════
// ONLINE STATUS MONITORING
// ══════════════════════════════════════════════════════════════

void updateOnlineStatus() {
    // Check all devices for timeout
    // Device is considered offline if last_seen > 310 seconds ago
    // (300s fail-safe + 10s margin)
    
    uint32_t currentTime = millis() / 1000;  // seconds
    const uint32_t OFFLINE_THRESHOLD = 310;   // seconds
    
    for (int i = 1; i <= 247; i++) {
        DeviceRecord* device = getDeviceByID(i);
        if (device != nullptr && device->online) {
            uint32_t age = currentTime - device->last_seen;
            
            if (age > OFFLINE_THRESHOLD) {
                Serial.printf("⚠ Device %d (%s) considered OFFLINE (last seen %lu s ago)\n",
                             device->id, device->name, age);
                
                device->online = false;
                markDeviceDirty(i);
                
                // TODO: Notify Display Master
            }
        }
    }
}

// ══════════════════════════════════════════════════════════════
// ALERTS AND NOTIFICATIONS
// ══════════════════════════════════════════════════════════════

void checkSystemAlerts() {
    // Check for devices in fail-safe
    for (int i = 1; i <= 247; i++) {
        DeviceRecord* device = getDeviceByID(i);
        if (device != nullptr && device->online) {
            if (device->control_source == CONTROL_FAILSAFE) {
                // Device in fail-safe, log periodically
                static uint32_t lastAlert = 0;
                if (millis() - lastAlert > 60000) {  // Alert every minute
                    Serial.printf("⚠⚠⚠ PERSISTENT FAIL-SAFE: Device %d (%s)\n",
                                 device->id, device->name);
                    lastAlert = millis();
                }
            }
        }
    }
}

// ══════════════════════════════════════════════════════════════
// JSON EXPORT FOR DISPLAY MASTER
// ══════════════════════════════════════════════════════════════

void exportDeviceListJSON(String& output) {
    StaticJsonDocument<4096> doc;
    JsonArray devices = doc.createNestedArray("devices");
    
    for (int i = 1; i <= 247; i++) {
        DeviceRecord* device = getDeviceByID(i);
        if (device != nullptr) {
            JsonObject obj = devices.createNestedObject();
            
            obj["id"] = device->id;
            obj["name"] = device->name;
            obj["profile"] = SmartMosqueUtils::profileToString(device->profile);
            obj["online"] = device->online;
            obj["control_source"] = SmartMosqueUtils::controlSourceToString(device->control_source);
            obj["relay_state"] = device->relay_state;
            
            // MAC address
            char macStr[18];
            SmartMosqueUtils::macToString(device->mac, macStr);
            obj["mac"] = macStr;
        }
    }
    
    serializeJson(doc, output);
}

// ══════════════════════════════════════════════════════════════
// STATISTICS
// ══════════════════════════════════════════════════════════════

void printStatusStats() {
    Serial.println("\n╔═══════════════════════════════════════╗");
    Serial.println("║     STATUS AGGREGATOR STATISTICS     ║");
    Serial.println("╠═══════════════════════════════════════╣");
    Serial.printf("║ Reports Received:  %8lu        ║\n", statusStats.reportsReceived);
    Serial.printf("║ Parse Errors:      %8lu        ║\n", statusStats.parseErrors);
    Serial.printf("║ Unknown Devices:   %8lu        ║\n", statusStats.unknownDevices);
    Serial.printf("║ Status Updates:    %8lu        ║\n", statusStats.statusUpdates);
    Serial.printf("║ Mode Changes:      %8lu        ║\n", statusStats.modeChanges);
    Serial.printf("║ Fail-Safe Alerts:  %8lu        ║\n", statusStats.failSafeAlerts);
    Serial.println("╚═══════════════════════════════════════╝\n");
}

// ══════════════════════════════════════════════════════════════
// INITIALIZATION
// ══════════════════════════════════════════════════════════════

void initStatusAggregator() {
    memset(&statusStats, 0, sizeof(statusStats));
    
    Serial.println("✓ Status aggregator initialized");
}

/**
 * ═══════════════════════════════════════════════════════════════
 *  USAGE EXAMPLES
 * ═══════════════════════════════════════════════════════════════
 * 
 * // Initialize
 * initStatusAggregator();
 * 
 * // Handle status report from slave
 * if (frame.command == CMD_STATUS_REPORT) {
 *     handleStatusReport(frame.address, frame.payload, frame.length);
 * }
 * 
 * // Periodic tasks (every 10 seconds in main loop)
 * updateOnlineStatus();
 * checkSystemAlerts();
 * 
 * // Get system status
 * StaticJsonDocument<512> doc;
 * getSystemStatus(doc);
 * 
 * // Print device list
 * printDeviceList();
 * 
 * // Export to Display Master
 * String json;
 * exportDeviceListJSON(json);
 * 
 * ═══════════════════════════════════════════════════════════════
 */
