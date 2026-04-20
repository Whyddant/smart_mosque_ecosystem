/**
 * ═══════════════════════════════════════════════════════════════
 *  LOGIC MASTER - DISCOVERY HANDLER
 * 
 *  Handles auto-enrollment of new slave devices
 *  Referensi: [STP-4.2.1] DISCOVERY_ANNOUNCE
 *             [STP-4.2.2] DISCOVERY_RESPONSE
 *             [KT-2.3] Auto-Enrollment Flow
 * ═══════════════════════════════════════════════════════════════
 */

#include "../include/logic_master.h"
#include <ArduinoJson.h>

// ══════════════════════════════════════════════════════════════
// PENDING DEVICES MANAGEMENT
// ══════════════════════════════════════════════════════════════

// Pending devices queue (waiting for user confirmation)
PendingDevice pendingDevices[MAX_PENDING_DEVICES];
uint8_t pendingDeviceCount = 0;

// Statistics
struct DiscoveryStats {
    uint32_t totalAnnouncements;
    uint32_t newDevices;
    uint32_t duplicateAnnouncements;
    uint32_t assignmentsCompleted;
    uint32_t assignmentsFailed;
} discoveryStats;

// ══════════════════════════════════════════════════════════════
// DISCOVERY ANNOUNCE HANDLER
// ══════════════════════════════════════════════════════════════

void handleDiscoveryAnnounce(const uint8_t* payload, uint8_t length) {
    Serial.println("\n╔═══════════════════════════════════════╗");
    Serial.println("║   DISCOVERY ANNOUNCE RECEIVED         ║");
    Serial.println("╚═══════════════════════════════════════╝");
    
    discoveryStats.totalAnnouncements++;
    
    // Parse JSON payload
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, payload, length);
    
    if (error) {
        Serial.printf("✗ JSON parse error: %s\n", error.c_str());
        return;
    }
    
    // Extract fields
    const char* macStr = doc["mac"];
    const char* profileStr = doc["profile"];
    const char* version = doc["ver"];
    
    if (macStr == nullptr || profileStr == nullptr) {
        Serial.println("✗ Missing required fields (mac or profile)");
        return;
    }
    
    // Parse MAC address
    uint8_t mac[6];
    if (!SmartMosqueUtils::stringToMac(macStr, mac)) {
        Serial.println("✗ Invalid MAC address format");
        return;
    }
    
    Serial.printf("MAC:     %s\n", macStr);
    Serial.printf("Profile: %s\n", profileStr);
    Serial.printf("Version: %s\n", version ? version : "unknown");
    
    // Check if device already registered
    DeviceRecord* existing = findDeviceByMAC(mac);
    if (existing != nullptr) {
        Serial.printf("→ Device already registered as ID %d: %s\n", 
                     existing->id, existing->name);
        
        discoveryStats.duplicateAnnouncements++;
        
        // Re-send discovery response with existing ID
        sendDiscoveryResponse(existing->id, existing->name, 
                            mac, profileStr);
        return;
    }
    
    // Check if already in pending queue
    for (int i = 0; i < pendingDeviceCount; i++) {
        if (memcmp(pendingDevices[i].mac, mac, 6) == 0) {
            Serial.println("→ Device already in pending queue");
            
            // Update timestamp (device is still announcing)
            pendingDevices[i].timestamp = millis();
            return;
        }
    }
    
    // Add to pending queue
    if (pendingDeviceCount >= MAX_PENDING_DEVICES) {
        Serial.println("✗ Pending queue full, cannot add device");
        return;
    }
    
    // Create pending device entry
    PendingDevice& pending = pendingDevices[pendingDeviceCount];
    memcpy(pending.mac, mac, 6);
    strncpy(pending.macStr, macStr, 17);
    pending.macStr[17] = '\0';
    strncpy(pending.profile, profileStr, 15);
    pending.profile[15] = '\0';
    pending.timestamp = millis();
    pending.autoApprove = false;  // Requires user confirmation by default
    
    pendingDeviceCount++;
    discoveryStats.newDevices++;
    
    Serial.printf("✓ Added to pending queue (%d/%d)\n", 
                 pendingDeviceCount, MAX_PENDING_DEVICES);
    
    // Notify Display Master about new pending device
    notifyDisplayNewDevice(macStr, profileStr);
    
    // Auto-approve if enabled (optional feature)
    if (AUTO_APPROVE_ENABLED) {
        Serial.println("→ Auto-approve enabled, assigning ID...");
        
        // Generate auto name
        char autoName[32];
        generateAutoDeviceName(autoName, profileStr, mac);
        
        approveDevice(macStr, autoName);
    }
}

// ══════════════════════════════════════════════════════════════
// DEVICE APPROVAL (USER OR AUTO)
// ══════════════════════════════════════════════════════════════

bool approveDevice(const char* macStr, const char* deviceName) {
    Serial.println("\n╔═══════════════════════════════════════╗");
    Serial.println("║   APPROVING DEVICE                    ║");
    Serial.println("╚═══════════════════════════════════════╝");
    
    // Find pending device
    PendingDevice* pending = nullptr;
    int pendingIndex = -1;
    
    for (int i = 0; i < pendingDeviceCount; i++) {
        if (strcmp(pendingDevices[i].macStr, macStr) == 0) {
            pending = &pendingDevices[i];
            pendingIndex = i;
            break;
        }
    }
    
    if (pending == nullptr) {
        Serial.println("✗ Device not found in pending queue");
        return false;
    }
    
    Serial.printf("MAC:  %s\n", macStr);
    Serial.printf("Name: %s\n", deviceName);
    Serial.printf("Profile: %s\n", pending->profile);
    
    // Get next available ID
    uint8_t assignedID = getNextAvailableID();
    if (assignedID == 0) {
        Serial.println("✗ No available IDs (all 247 slots used)");
        discoveryStats.assignmentsFailed++;
        return false;
    }
    
    Serial.printf("Assigned ID: %d\n", assignedID);
    
    // Parse profile enum
    uint8_t profileEnum = PROFILE_FAN_4CH;  // Default
    if (strcmp(pending->profile, "FAN_4CH") == 0) {
        profileEnum = PROFILE_FAN_4CH;
    } else if (strcmp(pending->profile, "LIGHT_8CH") == 0) {
        profileEnum = PROFILE_LIGHT_8CH;
    } else if (strcmp(pending->profile, "SOUND_8CH") == 0) {
        profileEnum = PROFILE_SOUND_8CH;
    }
    
    // Create device record
    DeviceRecord newDevice;
    newDevice.id = assignedID;
    memcpy(newDevice.mac, pending->mac, 6);
    strncpy(newDevice.name, deviceName, 31);
    newDevice.name[31] = '\0';
    newDevice.profile = profileEnum;
    newDevice.last_seen = 0;  // Will be updated on first status report
    newDevice.online = false;
    newDevice.relay_state = 0x00;
    newDevice.control_source = CONTROL_AUTO;
    
    // Add to registry
    if (!addDevice(&newDevice)) {
        Serial.println("✗ Failed to add device to registry");
        discoveryStats.assignmentsFailed++;
        return false;
    }
    
    Serial.println("✓ Device added to registry");
    
    // Send discovery response to slave
    sendDiscoveryResponse(assignedID, deviceName, 
                         pending->mac, pending->profile);
    
    // Remove from pending queue
    for (int i = pendingIndex; i < pendingDeviceCount - 1; i++) {
        pendingDevices[i] = pendingDevices[i + 1];
    }
    pendingDeviceCount--;
    
    discoveryStats.assignmentsCompleted++;
    
    Serial.printf("✓ Assignment complete! Pending queue: %d remaining\n", 
                 pendingDeviceCount);
    
    // Notify Display Master of successful assignment
    notifyDisplayDeviceAssigned(assignedID, deviceName);
    
    return true;
}

// ══════════════════════════════════════════════════════════════
// DISCOVERY RESPONSE SENDER
// ══════════════════════════════════════════════════════════════

void sendDiscoveryResponse(uint8_t assignedID, const char* deviceName,
                          const uint8_t* mac, const char* profile) {
    Serial.println("\n→ Sending DISCOVERY_RESPONSE...");
    
    // Build JSON payload
    StaticJsonDocument<256> doc;
    
    // MAC address (for validation by slave)
    char macStr[18];
    SmartMosqueUtils::macToString(mac, macStr);
    doc["mac"] = macStr;
    
    // Assigned ID
    doc["assigned_id"] = assignedID;
    
    // Device name
    doc["device_name"] = deviceName;
    
    // Profile confirmation
    doc["profile"] = profile;
    
    // Timestamp (optional)
    doc["timestamp"] = millis() / 1000;  // seconds since boot
    
    // Serialize to string
    String jsonStr;
    serializeJson(doc, jsonStr);
    
    Serial.printf("  Payload: %s\n", jsonStr.c_str());
    Serial.printf("  Length: %d bytes\n", jsonStr.length());
    
    // Send via RS-485 (broadcast to ensure slave receives it)
    sendRS485Frame(ADDR_BROADCAST, CMD_DISCOVERY_RESPONSE,
                  (uint8_t*)jsonStr.c_str(), jsonStr.length());
    
    Serial.println("✓ DISCOVERY_RESPONSE sent");
}

// ══════════════════════════════════════════════════════════════
// AUTO NAME GENERATION
// ══════════════════════════════════════════════════════════════

void generateAutoDeviceName(char* output, const char* profile, const uint8_t* mac) {
    // Format: "ProfileType-XXXX" where XXXX is last 2 bytes of MAC
    
    const char* prefix = "Device";
    if (strcmp(profile, "FAN_4CH") == 0) {
        prefix = "Kipas";
    } else if (strcmp(profile, "LIGHT_8CH") == 0) {
        prefix = "Lampu";
    } else if (strcmp(profile, "SOUND_8CH") == 0) {
        prefix = "Sound";
    }
    
    sprintf(output, "%s-%02X%02X", prefix, mac[4], mac[5]);
}

// ══════════════════════════════════════════════════════════════
// DISPLAY MASTER NOTIFICATIONS
// ══════════════════════════════════════════════════════════════

void notifyDisplayNewDevice(const char* macStr, const char* profile) {
    // This will be implemented when Display Master communication is ready
    // For now, just log
    Serial.println("\n→ [DISPLAY NOTIFY] New device pending approval");
    Serial.printf("  MAC: %s\n", macStr);
    Serial.printf("  Profile: %s\n", profile);
    
    // TODO: Send notification to Display Master via RS-485
    // Format: JSON command to update UI with pending device popup
}

void notifyDisplayDeviceAssigned(uint8_t id, const char* name) {
    // This will be implemented when Display Master communication is ready
    Serial.println("\n→ [DISPLAY NOTIFY] Device assigned successfully");
    Serial.printf("  ID: %d\n", id);
    Serial.printf("  Name: %s\n", name);
    
    // TODO: Send notification to Display Master
    // Format: JSON command to update device list UI
}

// ══════════════════════════════════════════════════════════════
// PERIODIC TASKS
// ══════════════════════════════════════════════════════════════

void processPendingDevices() {
    // Check for stale pending devices (timeout after 5 minutes)
    uint32_t now = millis();
    const uint32_t PENDING_TIMEOUT = 300000;  // 5 minutes
    
    for (int i = pendingDeviceCount - 1; i >= 0; i--) {
        if (now - pendingDevices[i].timestamp > PENDING_TIMEOUT) {
            Serial.printf("⚠ Pending device %s timed out, removing\n", 
                         pendingDevices[i].macStr);
            
            // Remove from queue
            for (int j = i; j < pendingDeviceCount - 1; j++) {
                pendingDevices[j] = pendingDevices[j + 1];
            }
            pendingDeviceCount--;
        }
    }
}

void printPendingDevices() {
    if (pendingDeviceCount == 0) {
        Serial.println("No pending devices");
        return;
    }
    
    Serial.println("\n╔════════════════════════════════════════════════════════╗");
    Serial.println("║              PENDING DEVICES                           ║");
    Serial.println("╠════════════════════════════════════════════════════════╣");
    
    for (int i = 0; i < pendingDeviceCount; i++) {
        PendingDevice& p = pendingDevices[i];
        uint32_t age = (millis() - p.timestamp) / 1000;
        
        Serial.printf("║ %d. MAC: %-17s Profile: %-10s  ║\n", 
                     i + 1, p.macStr, p.profile);
        Serial.printf("║    Age: %lu seconds                                    ║\n", age);
    }
    
    Serial.println("╚════════════════════════════════════════════════════════╝\n");
}

// ══════════════════════════════════════════════════════════════
// STATISTICS
// ══════════════════════════════════════════════════════════════

void printDiscoveryStats() {
    Serial.println("\n╔═══════════════════════════════════════╗");
    Serial.println("║     DISCOVERY STATISTICS              ║");
    Serial.println("╠═══════════════════════════════════════╣");
    Serial.printf("║ Total Announcements:  %8lu       ║\n", 
                 discoveryStats.totalAnnouncements);
    Serial.printf("║ New Devices:          %8lu       ║\n", 
                 discoveryStats.newDevices);
    Serial.printf("║ Duplicate Announces:  %8lu       ║\n", 
                 discoveryStats.duplicateAnnouncements);
    Serial.printf("║ Assignments Done:     %8lu       ║\n", 
                 discoveryStats.assignmentsCompleted);
    Serial.printf("║ Assignments Failed:   %8lu       ║\n", 
                 discoveryStats.assignmentsFailed);
    Serial.println("╚═══════════════════════════════════════╝\n");
}

// ══════════════════════════════════════════════════════════════
// MANUAL APPROVAL VIA SERIAL COMMANDS
// ══════════════════════════════════════════════════════════════

void handleSerialCommand_Approve(const char* macStr, const char* name) {
    if (approveDevice(macStr, name)) {
        Serial.println("✓ Device approved successfully");
    } else {
        Serial.println("✗ Failed to approve device");
    }
}

void handleSerialCommand_ListPending() {
    printPendingDevices();
}

void handleSerialCommand_RejectDevice(const char* macStr) {
    // Find and remove from pending queue
    for (int i = 0; i < pendingDeviceCount; i++) {
        if (strcmp(pendingDevices[i].macStr, macStr) == 0) {
            Serial.printf("→ Rejecting device: %s\n", macStr);
            
            // Remove from queue
            for (int j = i; j < pendingDeviceCount - 1; j++) {
                pendingDevices[j] = pendingDevices[j + 1];
            }
            pendingDeviceCount--;
            
            Serial.println("✓ Device rejected and removed from queue");
            return;
        }
    }
    
    Serial.println("✗ Device not found in pending queue");
}

// ══════════════════════════════════════════════════════════════
// INITIALIZATION
// ══════════════════════════════════════════════════════════════

void initDiscovery() {
    pendingDeviceCount = 0;
    memset(&discoveryStats, 0, sizeof(discoveryStats));
    
    Serial.println("✓ Discovery handler initialized");
    Serial.printf("  Max pending: %d devices\n", MAX_PENDING_DEVICES);
    Serial.printf("  Auto-approve: %s\n", AUTO_APPROVE_ENABLED ? "ENABLED" : "DISABLED");
}

/**
 * ═══════════════════════════════════════════════════════════════
 *  USAGE EXAMPLES
 * ═══════════════════════════════════════════════════════════════
 * 
 * // In main loop, when discovery announce received:
 * if (frame.command == CMD_DISCOVERY_ANNOUNCE) {
 *     handleDiscoveryAnnounce(frame.payload, frame.length);
 * }
 * 
 * // Periodic task (every 10 seconds):
 * processPendingDevices();
 * 
 * // Manual approval via serial:
 * handleSerialCommand_Approve("A0:B1:C2:D3:E4:F5", "Kipas Saf 1");
 * 
 * // List pending devices:
 * handleSerialCommand_ListPending();
 * 
 * // Reject device:
 * handleSerialCommand_RejectDevice("A0:B1:C2:D3:E4:F5");
 * 
 * ═══════════════════════════════════════════════════════════════
 */
