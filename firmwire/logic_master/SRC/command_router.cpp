/**
 * ═══════════════════════════════════════════════════════════════
 *  LOGIC MASTER - COMMAND ROUTER
 * 
 *  Routes commands from Display Master to appropriate slaves
 *  Handles relay control, status requests, and system commands
 *  Referensi: [STP-4.2.3] SET_RELAY
 *             [KT-2.2] Alur Data Pokok - Kontrol Normal
 * ═══════════════════════════════════════════════════════════════
 */

#include "../include/logic_master.h"
#include <ArduinoJson.h>

// ══════════════════════════════════════════════════════════════
// COMMAND QUEUE
// ══════════════════════════════════════════════════════════════

struct CommandQueueItem {
    uint8_t targetID;
    uint8_t command;
    uint8_t payload[MAX_PAYLOAD_SIZE];
    uint8_t length;
    uint32_t timestamp;
    uint8_t retries;
    bool waitingACK;
};

CommandQueueItem commandQueue[MAX_COMMAND_QUEUE];
uint8_t queueHead = 0;
uint8_t queueTail = 0;
uint8_t queueCount = 0;

// Statistics
struct RouterStats {
    uint32_t commandsSent;
    uint32_t commandsQueued;
    uint32_t commandsDropped;
    uint32_t acksReceived;
    uint32_t nacksReceived;
    uint32_t timeouts;
    uint32_t retries;
} routerStats;

// ══════════════════════════════════════════════════════════════
// COMMAND QUEUEING
// ══════════════════════════════════════════════════════════════

bool queueCommand(uint8_t targetID, uint8_t command, 
                 const uint8_t* payload, uint8_t length) {
    // Check if queue is full
    if (queueCount >= MAX_COMMAND_QUEUE) {
        Serial.println("✗ Command queue full, dropping command");
        routerStats.commandsDropped++;
        return false;
    }
    
    // Add to queue
    CommandQueueItem& item = commandQueue[queueTail];
    item.targetID = targetID;
    item.command = command;
    item.length = length;
    if (length > 0 && payload != nullptr) {
        memcpy(item.payload, payload, length);
    }
    item.timestamp = millis();
    item.retries = 0;
    item.waitingACK = true;
    
    queueTail = (queueTail + 1) % MAX_COMMAND_QUEUE;
    queueCount++;
    
    routerStats.commandsQueued++;
    
    return true;
}

CommandQueueItem* dequeueCommand() {
    if (queueCount == 0) {
        return nullptr;
    }
    
    CommandQueueItem* item = &commandQueue[queueHead];
    queueHead = (queueHead + 1) % MAX_COMMAND_QUEUE;
    queueCount--;
    
    return item;
}

CommandQueueItem* peekQueue() {
    if (queueCount == 0) {
        return nullptr;
    }
    return &commandQueue[queueHead];
}

// ══════════════════════════════════════════════════════════════
// SET RELAY COMMAND
// ══════════════════════════════════════════════════════════════

bool sendSetRelay(uint8_t slaveID, uint8_t relayMask, const uint8_t* values) {
    Serial.printf("\n→ Sending SET_RELAY to slave %d\n", slaveID);
    
    // Validate slave exists
    DeviceRecord* device = getDeviceByID(slaveID);
    if (device == nullptr) {
        Serial.println("✗ Device not found");
        return false;
    }
    
    // Check if device is online
    if (!device->online) {
        Serial.println("✗ Device offline");
        return false;
    }
    
    // Build binary payload (9 bytes)
    uint8_t payload[9];
    payload[0] = relayMask;
    memcpy(&payload[1], values, 8);
    
    Serial.printf("  Mask: 0x%02X\n", relayMask);
    Serial.print("  Values: [");
    for (int i = 0; i < 8; i++) {
        Serial.printf("%d", values[i]);
        if (i < 7) Serial.print(",");
    }
    Serial.println("]");
    
    // Validate for FAN profile (interlock check)
    if (device->profile == PROFILE_FAN_4CH) {
        int speedCount = 0;
        if (values[0]) speedCount++;  // LOW
        if (values[1]) speedCount++;  // MED
        if (values[2]) speedCount++;  // HIGH
        
        if (speedCount > 1) {
            Serial.println("✗ Interlock violation: Multiple speeds requested");
            return false;
        }
    }
    
    // Queue or send immediately
    if (queueCount == 0) {
        // Queue is empty, send immediately
        sendRS485Frame(slaveID, CMD_SET_RELAY, payload, 9);
        routerStats.commandsSent++;
        
        // Add to queue for ACK tracking
        queueCommand(slaveID, CMD_SET_RELAY, payload, 9);
    } else {
        // Queue is busy, add to queue
        if (!queueCommand(slaveID, CMD_SET_RELAY, payload, 9)) {
            return false;
        }
        Serial.println("  → Queued (will send when available)");
    }
    
    return true;
}

// ══════════════════════════════════════════════════════════════
// CONVENIENCE FUNCTIONS
// ══════════════════════════════════════════════════════════════

bool setRelayChannel(uint8_t slaveID, uint8_t channel, bool state) {
    if (channel >= 8) {
        Serial.println("✗ Invalid channel (0-7)");
        return false;
    }
    
    uint8_t mask = (1 << channel);
    uint8_t values[8] = {0};
    values[channel] = state ? 1 : 0;
    
    return sendSetRelay(slaveID, mask, values);
}

bool setAllRelays(uint8_t slaveID, uint8_t bitmask) {
    uint8_t mask = 0xFF;  // All channels
    uint8_t values[8];
    
    for (int i = 0; i < 8; i++) {
        values[i] = (bitmask & (1 << i)) ? 1 : 0;
    }
    
    return sendSetRelay(slaveID, mask, values);
}

bool setFanSpeed(uint8_t slaveID, FanSpeed speed) {
    // Validasi: harus device FAN_4CH
    DeviceRecord* device = getDeviceByID(slaveID);
    if (device == nullptr || device->profile != PROFILE_FAN_4CH) {
        Serial.println("✗ Not a FAN_4CH device");
        return false;
    }

    // Layout relay HwThinker 4-relay ESP32:
    //   values[0] = Relay1 (GPIO23) = SWING
    //   values[1] = Relay2 (GPIO5)  = Speed LOW
    //   values[2] = Relay3 (GPIO4)  = Speed MED
    //   values[3] = Relay4 (GPIO13) = Speed HIGH
    uint8_t mask = 0x0F;  // 4 relay (bit 0-3)
    uint8_t values[8] = {0};

    switch (speed) {
        case FAN_LOW:
            values[0] = 1;  // swing ON
            values[1] = 1;  // LOW ON
            break;
        case FAN_MED:
            values[0] = 1;  // swing ON
            values[2] = 1;  // MED ON
            break;
        case FAN_HIGH:
            values[0] = 1;  // swing ON
            values[3] = 1;  // HIGH ON
            break;
        case FAN_OFF:
        default:
            // Semua 0 = semua OFF termasuk swing
            break;
    }

    Serial.printf("→ Setting fan speed: %s (slave %d)\n",
                 SmartMosqueUtils::fanSpeedToString(speed), slaveID);

    return sendSetRelay(slaveID, mask, values);
}

// ══════════════════════════════════════════════════════════════
// BROADCAST COMMANDS
// ══════════════════════════════════════════════════════════════

bool broadcastAllOff() {
    Serial.println("\n→ Broadcasting ALL OFF to all slaves");
    
    uint8_t mask = 0xFF;
    uint8_t values[8] = {0};  // All OFF
    
    return sendSetRelay(ADDR_BROADCAST, mask, values);
}

bool broadcastAllOn() {
    Serial.println("\n→ Broadcasting ALL ON to all slaves");
    
    uint8_t mask = 0xFF;
    uint8_t values[8] = {1,1,1,1,1,1,1,1};  // All ON
    
    return sendSetRelay(ADDR_BROADCAST, mask, values);
}

// ══════════════════════════════════════════════════════════════
// ACK/NACK HANDLING
// ══════════════════════════════════════════════════════════════

void handleACK(uint8_t fromID) {
    Serial.printf("← ACK from slave %d\n", fromID);
    
    routerStats.acksReceived++;
    
    // Find command in queue waiting for ACK
    CommandQueueItem* item = peekQueue();
    if (item != nullptr && item->targetID == fromID && item->waitingACK) {
        Serial.println("  ✓ Command confirmed");
        item->waitingACK = false;
        
        // Remove from queue
        dequeueCommand();
    }
}

void handleNACK(uint8_t fromID, const uint8_t* payload, uint8_t length) {
    Serial.printf("← NACK from slave %d\n", fromID);
    
    routerStats.nacksReceived++;
    
    // Parse error code from JSON payload
    if (length > 0) {
        StaticJsonDocument<128> doc;
        DeserializationError error = deserializeJson(doc, payload, length);
        
        if (!error) {
            uint8_t errorCode = doc["code"];
            const char* message = doc["msg"];
            
            Serial.printf("  Error code: 0x%02X\n", errorCode);
            Serial.printf("  Message: %s\n", message ? message : "N/A");
            
            // Handle specific errors
            switch (errorCode) {
                case ERROR_MANUAL_MODE_ACTIVE:
                    Serial.println("  → Device in MANUAL mode");
                    // Update device control source
                    DeviceRecord* device = getDeviceByID(fromID);
                    if (device) {
                        device->control_source = CONTROL_MANUAL;
                    }
                    break;
                    
                case ERROR_INTERLOCK_VIOLATION:
                    Serial.println("  → Interlock violation detected");
                    break;
                    
                case ERROR_INVALID_CRC:
                    Serial.println("  → CRC error, will retry");
                    break;
            }
        }
    }
    
    // Remove from queue (don't retry for most errors)
    CommandQueueItem* item = peekQueue();
    if (item != nullptr && item->targetID == fromID) {
        dequeueCommand();
    }
}

// ══════════════════════════════════════════════════════════════
// COMMAND QUEUE PROCESSING
// ══════════════════════════════════════════════════════════════

void processCommandQueue() {
    CommandQueueItem* item = peekQueue();
    if (item == nullptr) {
        return;  // Queue empty
    }
    
    // Check if waiting for ACK
    if (item->waitingACK) {
        // Check timeout
        if (millis() - item->timestamp > COMMAND_TIMEOUT) {
            Serial.printf("⚠ Command timeout for slave %d\n", item->targetID);
            routerStats.timeouts++;
            
            // Retry?
            if (item->retries < MAX_COMMAND_RETRIES) {
                item->retries++;
                item->timestamp = millis();
                routerStats.retries++;
                
                Serial.printf("  → Retry %d/%d\n", item->retries, MAX_COMMAND_RETRIES);
                
                // Resend
                sendRS485Frame(item->targetID, item->command, 
                             item->payload, item->length);
                routerStats.commandsSent++;
            } else {
                Serial.println("  ✗ Max retries reached, giving up");
                
                // Mark device as potentially offline
                DeviceRecord* device = getDeviceByID(item->targetID);
                if (device) {
                    device->online = false;
                }
                
                // Remove from queue
                dequeueCommand();
            }
        }
        return;  // Still waiting
    }
    
    // Command completed, remove and process next
    dequeueCommand();
    
    // Process next command if available
    item = peekQueue();
    if (item != nullptr) {
        sendRS485Frame(item->targetID, item->command, 
                     item->payload, item->length);
        routerStats.commandsSent++;
    }
}

// ══════════════════════════════════════════════════════════════
// DISPLAY MASTER COMMANDS
// ══════════════════════════════════════════════════════════════

bool handleDisplayCommand(const uint8_t* payload, uint8_t length) {
    // Parse JSON command from Display Master
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, payload, length);
    
    if (error) {
        Serial.printf("✗ JSON parse error: %s\n", error.c_str());
        return false;
    }
    
    const char* action = doc["action"];
    if (action == nullptr) {
        Serial.println("✗ No action specified");
        return false;
    }
    
    Serial.printf("\n← Display command: %s\n", action);
    
    // Route based on action
    if (strcmp(action, "set_relay") == 0) {
        uint8_t slaveID = doc["slave_id"];
        uint8_t channel = doc["channel"];
        bool state = doc["state"];
        
        return setRelayChannel(slaveID, channel, state);
    }
    else if (strcmp(action, "set_fan_speed") == 0) {
        uint8_t slaveID = doc["slave_id"];
        const char* speedStr = doc["speed"];
        
        FanSpeed speed = FAN_OFF;
        if (strcmp(speedStr, "LOW") == 0) speed = FAN_LOW;
        else if (strcmp(speedStr, "MED") == 0) speed = FAN_MED;
        else if (strcmp(speedStr, "HIGH") == 0) speed = FAN_HIGH;
        
        return setFanSpeed(slaveID, speed);
    }
    else if (strcmp(action, "set_all_relays") == 0) {
        uint8_t slaveID = doc["slave_id"];
        uint8_t bitmask = doc["bitmask"];
        
        return setAllRelays(slaveID, bitmask);
    }
    else if (strcmp(action, "broadcast_off") == 0) {
        return broadcastAllOff();
    }
    else if (strcmp(action, "broadcast_on") == 0) {
        return broadcastAllOn();
    }
    else {
        Serial.printf("✗ Unknown action: %s\n", action);
        return false;
    }
}

// ══════════════════════════════════════════════════════════════
// STATISTICS
// ══════════════════════════════════════════════════════════════

void printRouterStats() {
    Serial.println("\n╔═══════════════════════════════════════╗");
    Serial.println("║     COMMAND ROUTER STATISTICS         ║");
    Serial.println("╠═══════════════════════════════════════╣");
    Serial.printf("║ Commands Sent:     %8lu        ║\n", routerStats.commandsSent);
    Serial.printf("║ Commands Queued:   %8lu        ║\n", routerStats.commandsQueued);
    Serial.printf("║ Commands Dropped:  %8lu        ║\n", routerStats.commandsDropped);
    Serial.printf("║ ACKs Received:     %8lu        ║\n", routerStats.acksReceived);
    Serial.printf("║ NACKs Received:    %8lu        ║\n", routerStats.nacksReceived);
    Serial.printf("║ Timeouts:          %8lu        ║\n", routerStats.timeouts);
    Serial.printf("║ Retries:           %8lu        ║\n", routerStats.retries);
    Serial.printf("║ Queue Size:        %8d        ║\n", queueCount);
    Serial.println("╚═══════════════════════════════════════╝\n");
}

// ══════════════════════════════════════════════════════════════
// INITIALIZATION
// ══════════════════════════════════════════════════════════════

void initCommandRouter() {
    queueHead = 0;
    queueTail = 0;
    queueCount = 0;
    memset(&routerStats, 0, sizeof(routerStats));
    
    Serial.println("✓ Command router initialized");
    Serial.printf("  Queue size: %d commands\n", MAX_COMMAND_QUEUE);
    Serial.printf("  Timeout: %d ms\n", COMMAND_TIMEOUT);
    Serial.printf("  Max retries: %d\n", MAX_COMMAND_RETRIES);
}

/**
 * ═══════════════════════════════════════════════════════════════
 *  USAGE EXAMPLES
 * ═══════════════════════════════════════════════════════════════
 * 
 * // Initialize
 * initCommandRouter();
 * 
 * // Set single relay
 * setRelayChannel(5, 2, true);  // Slave 5, channel 2, ON
 *
 * // Set fan speed (HwThinker 4-relay)
 * setFanSpeed(3, FAN_MED);  // Slave 3, medium speed
 *
 * // Set relay Modbus 8ch (node lampu/sound)
 * setModbusRelay8ch(MODBUS_ADDR_RELAY_LIGHT_DEFAULT, 0, true);  // Lampu 1 ON
 * setAllModbusRelays8ch(MODBUS_ADDR_RELAY_SOUND_DEFAULT, 0x0F); // Sound 1-4 ON
 *
 * // Set all relays at once
 * setAllRelays(14, 0xFF);  // Slave 14, all ON
 *
 * // Broadcast commands
 * broadcastAllOff();
 *
 * ═══════════════════════════════════════════════════════════════
 */

// ══════════════════════════════════════════════════════════════
// ESP32 X8 RELAY ROUTING — NODE LAMPU & SOUND
// ══════════════════════════════════════════════════════════════

/**
 * Kontrol semua relay ESP32 X8 sekaligus dengan bitmask.
 * Ini adalah fungsi utama — gunakan ini untuk kontrol lampu/sound.
 *
 * @param modbusAddr  MODBUS_ADDR_RELAY_LIGHT_DEFAULT (0x02) atau
 *                    MODBUS_ADDR_RELAY_SOUND_DEFAULT (0x03)
 * @param mask        Bitmask: bit0=relay1 ... bit7=relay8
 *                    0x00 = semua OFF, 0xFF = semua ON
 * @return true jika berhasil
 */
bool setX8Relay(uint8_t modbusAddr, uint8_t mask) {
    bool ok = modbusX8WriteRelays(modbusAddr, mask);
    if (ok) {
        // Update device registry
        for (uint8_t i = 0; i < deviceCount; i++) {
            MasterDeviceRecord* dev = &deviceRegistry[i];
            if ((dev->profile == PROFILE_LIGHT_8CH &&
                 modbusAddr == MODBUS_ADDR_RELAY_LIGHT_DEFAULT) ||
                (dev->profile == PROFILE_SOUND_8CH &&
                 modbusAddr == MODBUS_ADDR_RELAY_SOUND_DEFAULT)) {
                dev->relay_state = mask;
                dev->dirty = true;
                break;
            }
        }
    }
    return ok;
}

/**
 * Kontrol satu relay ESP32 X8, pertahankan state relay lainnya.
 * Baca state saat ini → update bit → tulis kembali.
 *
 * @param modbusAddr  Alamat Modbus node
 * @param relayIndex  Index relay 0-7 (relay 1 = index 0)
 * @param state       true=ON, false=OFF
 * @return true jika berhasil
 */
bool setX8RelayOne(uint8_t modbusAddr, uint8_t relayIndex, bool state) {
    if (relayIndex > 7) {
        Serial.println("✗ setX8RelayOne: index harus 0-7");
        return false;
    }

    // Ambil state saat ini dari device registry
    uint8_t currentMask = 0x00;
    for (uint8_t i = 0; i < deviceCount; i++) {
        MasterDeviceRecord* dev = &deviceRegistry[i];
        if ((dev->profile == PROFILE_LIGHT_8CH &&
             modbusAddr == MODBUS_ADDR_RELAY_LIGHT_DEFAULT) ||
            (dev->profile == PROFILE_SOUND_8CH &&
             modbusAddr == MODBUS_ADDR_RELAY_SOUND_DEFAULT)) {
            currentMask = dev->relay_state;
            break;
        }
    }

    // Jika registry kosong, baca langsung dari device
    // (fallback: gunakan FC02 read status)
    uint8_t liveVal = 0;
    if (currentMask == 0 && modbusX8ReadStatus(modbusAddr, 0x01, &liveVal)) {
        currentMask = liveVal;
    }

    // Update bit
    if (state) {
        currentMask |= (1 << relayIndex);
    } else {
        currentMask &= ~(1 << relayIndex);
    }

    return setX8Relay(modbusAddr, currentMask);
}

/**
 * Baca state relay dari ESP32 X8 Relay via FC02.
 *
 * @param modbusAddr  Alamat Modbus node
 * @param outMask     Pointer ke byte output (bitmask relay)
 * @return true jika berhasil
 */
bool readX8RelayStatus(uint8_t modbusAddr, uint8_t* outMask) {
    // memaddr 0x01 = baca val_out (relay state)
    return modbusX8ReadStatus(modbusAddr, 0x01, outMask);
}

// ══════════════════════════════════════════════════════════════
// LEGACY WRAPPERS (kompatibilitas kode lama)
// ══════════════════════════════════════════════════════════════

/**
 * Legacy: kontrol satu relay via index.
 * Sekarang menggunakan setX8RelayOne() di balik layar.
 */
bool setModbusRelay8ch(uint8_t modbusAddr, uint8_t relayIndex, bool state) {
    return setX8RelayOne(modbusAddr, relayIndex, state);
}

/**
 * Legacy: kontrol semua relay via bitmask.
 * Sekarang menggunakan setX8Relay() di balik layar.
 */
bool setAllModbusRelays8ch(uint8_t modbusAddr, uint8_t mask) {
    return setX8Relay(modbusAddr, mask);
}
