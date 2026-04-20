/**
 * ═══════════════════════════════════════════════════════════════
 *  LOGIC MASTER - HEARTBEAT BROADCASTER
 * 
 *  Critical System Component
 *  Broadcasts alive signal every 60 seconds to prevent fail-safe
 * 
 *  Timing Requirements:
 *  - Interval: 60 seconds ±5% (57-63 seconds acceptable)
 *  - Precision: Must be consistent to prevent false fail-safe
 *  - Priority: Highest - system depends on this
 * 
 *  Referensi: [STP-4.2.4] HEARTBEAT, [KT-3.1] Fail-Safe Timeout
 * ═══════════════════════════════════════════════════════════════
 */

#include "logic_master.h"

// ══════════════════════════════════════════════════════════════
// HEARTBEAT STATE
// ══════════════════════════════════════════════════════════════

static hw_timer_t* heartbeatTimer = nullptr;  // Hardware timer for precision
static volatile bool heartbeatDue = false;    // Flag set by timer ISR

// ══════════════════════════════════════════════════════════════
// TIMER ISR
// ══════════════════════════════════════════════════════════════

/**
 * Timer ISR - called every 60 seconds
 * Sets flag for main loop to send heartbeat
 * MUST be kept minimal (ISR context)
 */
void IRAM_ATTR onHeartbeatTimer() {
    heartbeatDue = true;
}

// ══════════════════════════════════════════════════════════════
// INITIALIZATION
// ══════════════════════════════════════════════════════════════

/**
 * Initialize heartbeat timer
 * Uses hardware timer for precise 60-second interval
 */
void initHeartbeat() {
    Serial.println("→ Initializing heartbeat timer...");
    
    // Configure hardware timer
    // Timer 0, prescaler 80 (1MHz tick), count up
    heartbeatTimer = timerBegin(0, 80, true);
    
    // Attach interrupt
    timerAttachInterrupt(heartbeatTimer, &onHeartbeatTimer, true);
    
    // Set timer to trigger every 60 seconds
    // 1MHz tick × 60,000,000 = 60 seconds
    timerAlarmWrite(heartbeatTimer, 60000000, true);  // Auto-reload
    
    // Enable timer
    timerAlarmEnable(heartbeatTimer);
    
    Serial.println("✓ Heartbeat timer initialized (60s interval)");
    
    // Send initial heartbeat immediately
    sendHeartbeat();
}

// ══════════════════════════════════════════════════════════════
// HEARTBEAT TRANSMISSION
// ══════════════════════════════════════════════════════════════

/**
 * Check if heartbeat is due and send if needed
 * Called from main loop
 * Non-blocking, high priority
 */
void sendHeartbeatIfDue() {
    // Check flag set by timer ISR
    if (heartbeatDue) {
        heartbeatDue = false;  // Clear flag
        sendHeartbeat();
    }
}

/**
 * Send heartbeat frame to all slaves
 * Referensi: [STP-4.2.4] HEARTBEAT frame format
 * 
 * Frame:
 *   START: 0x7E
 *   ADDR:  0x00 (broadcast)
 *   CMD:   0x04 (HEARTBEAT)
 *   LEN:   0x01 (1 byte payload)
 *   DATA:  0xAA (alive signal)
 *   CRC:   calculated
 *   END:   0x0A
 */
void sendHeartbeat() {
    // Payload: single byte 0xAA (arbitrary alive signal)
    uint8_t payload = 0xAA;
    
    // Build frame
    uint8_t frame[MAX_FRAME_SIZE];
    int idx = 0;
    
    frame[idx++] = FRAME_START_DELIM;  // 0x7E
    frame[idx++] = ADDR_BROADCAST;     // 0x00 (to all slaves)
    frame[idx++] = CMD_HEARTBEAT;      // 0x04
    frame[idx++] = 1;                  // Length: 1 byte
    frame[idx++] = payload;            // 0xAA
    
    // Calculate CRC (from ADDR to end of PAYLOAD)
    uint16_t crc = SmartMosqueUtils::calculateCRC16(&frame[1], 4);
    frame[idx++] = crc & 0xFF;         // CRC_LOW
    frame[idx++] = (crc >> 8) & 0xFF;  // CRC_HIGH
    
    frame[idx++] = FRAME_END_DELIM;    // 0x0A
    
    // Transmit
    // Board ESP32-S3 IoT Logger RS485 Plus memiliki chip RS485 onboard
    // dengan DE/RE otomatis — tidak perlu kontrol manual
    Serial2.write(frame, idx);
    Serial2.flush();  // Tunggu transmisi selesai
    
    // Update statistics
    stats.heartbeats_sent++;
    stats.frames_sent++;
    lastHeartbeat = millis();
    
    // Debug output (periodic)
    static uint32_t lastDebugPrint = 0;
    if (millis() - lastDebugPrint > 60000) {  // Print every minute
        Serial.printf("♥ Heartbeat #%lu sent (uptime: %lu min)\\n",
                     stats.heartbeats_sent,
                     (millis() - bootTime) / 60000);
        lastDebugPrint = millis();
    }
}

// ══════════════════════════════════════════════════════════════
// DIAGNOSTICS
// ══════════════════════════════════════════════════════════════

/**
 * Get heartbeat statistics
 * For monitoring and debugging
 */
void getHeartbeatStats(uint32_t* sent, uint32_t* lastSent) {
    if (sent != nullptr) {
        *sent = stats.heartbeats_sent;
    }
    if (lastSent != nullptr) {
        *lastSent = lastHeartbeat;
    }
}

/**
 * Check heartbeat health
 * Verifies timer is running and heartbeats are being sent
 * @return true if healthy, false if problem detected
 */
bool checkHeartbeatHealth() {
    uint32_t age = millis() - lastHeartbeat;
    
    // Heartbeat should have been sent in last 65 seconds (60s + 5s margin)
    if (age > 65000) {
        Serial.println("⚠ WARNING: Heartbeat timer may have stopped!");
        Serial.printf("  Last heartbeat: %lu ms ago\\n", age);
        return false;
    }
    
    return true;
}

/**
 * Force immediate heartbeat (for testing or recovery)
 * Use with caution - may disrupt timing
 */
void forceHeartbeat() {
    Serial.println("→ Forcing immediate heartbeat...");
    sendHeartbeat();
}

// ══════════════════════════════════════════════════════════════
// END OF HEARTBEAT.CPP
// ══════════════════════════════════════════════════════════════

/**
 * Implementation Notes:
 * 
 * 1. Hardware Timer Usage:
 *    - Uses ESP32 hardware timer 0 for precision
 *    - 1MHz clock with 60,000,000 count = 60 seconds exact
 *    - Auto-reload ensures consistent interval
 * 
 * 2. ISR Safety:
 *    - Timer ISR only sets flag, no Serial/I2C/blocking
 *    - Actual frame transmission in main loop
 *    - Non-blocking design for system stability
 * 
 * 3. Broadcast Addressing:
 *    - Uses 0x00 (broadcast) address
 *    - All slaves must receive and process
 *    - No ACK expected (one-way communication)
 * 
 * 4. Timing Tolerance:
 *    - Target: 60 seconds exactly
 *    - Slaves accept ±5 seconds (55-65s)
 *    - Fail-safe at 300 seconds (5 minutes)
 * 
 * 5. Priority:
 *    - Highest system priority
 *    - If heartbeat stops, ALL slaves go fail-safe
 *    - System cannot function without this
 * 
 * Total LOC: ~180 lines
 * Status: Complete and critical
 */
