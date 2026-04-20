/**
 * ═══════════════════════════════════════════════════════════════
 *  SLAVE NODE - FRAME PROCESSING (Part 3/3)
 * 
 *  This file contains:
 *  - RS-485 frame parser (state machine)
 *  - CRC validation
 *  - Command handlers (HEARTBEAT, SET_RELAY, etc.)
 *  - Frame error handling
 * 
 *  Compliance:
 *  - Kebenaran Tunggal v1.4 (KT)
 *  - Spesifikasi Teknis Protokol v1.3 (STP)
 * 
 *  IMPROVEMENTS (v2.0.1):
 *  ✅ Added global parser reset function
 *  ✅ Enhanced buffer overflow protection
 *  ✅ Improved error recovery
 *  ✅ Payload validation strengthened
 *  ✅ Heartbeat debug throttling
 * ═══════════════════════════════════════════════════════════════
 */

// ══════════════════════════════════════════════════════════════
// PARSER UTILITY FUNCTIONS
// ══════════════════════════════════════════════════════════════

/**
 * Reset parser to initial state
 * Call this on any error or frame completion
 */
void resetParser() {
    parserState = WAIT_START;
    rxIndex = 0;
    memset(&currentFrame, 0, sizeof(MosqueFrame));
}

// ══════════════════════════════════════════════════════════════
// FRAME PARSER STATE MACHINE
// Referensi: [STP-11.2] Complete Frame Decoder
// ══════════════════════════════════════════════════════════════

/**
 * Process incoming byte through state machine parser
 * Called from main loop for every byte received on Serial2
 * 
 * @param byte Single byte from RS-485 bus
 */
void processIncomingByte(uint8_t byte) {
    static uint8_t expectedLength = 0;
    static uint8_t payloadIndex = 0;
    
    // Early buffer overflow protection
    if (rxIndex >= MAX_FRAME_SIZE - 1) {
        Serial.println("✗ Buffer overflow imminent, resetting parser");
        resetParser();
        stats.crcErrors++;
        return;
    }
    
    switch (parserState) {
        case WAIT_START:
            if (byte == FRAME_START_DELIM) {
                rxBuffer[0] = byte;
                rxIndex = 1;
                parserState = READ_ADDRESS;
                memset(&currentFrame, 0, sizeof(MosqueFrame));
                currentFrame.start_delim = byte;
            }
            break;
            
        case READ_ADDRESS:
            rxBuffer[rxIndex++] = byte;
            currentFrame.address = byte;
            
            if (byte != config.device_id && byte != ADDR_BROADCAST) {
                resetParser();
                break;
            }
            parserState = READ_COMMAND;
            break;
            
        case READ_COMMAND:
            rxBuffer[rxIndex++] = byte;
            currentFrame.command = byte;
            parserState = READ_LENGTH;
            break;
            
        case READ_LENGTH:
            rxBuffer[rxIndex++] = byte;
            currentFrame.length = byte;
            expectedLength = byte;
            payloadIndex = 0;
            
            if (expectedLength > MAX_PAYLOAD_SIZE) {
                Serial.printf("✗ Invalid length: %d (max %d)\n", 
                             expectedLength, MAX_PAYLOAD_SIZE);
                resetParser();
                stats.crcErrors++;
                break;
            }
            
            if (7 + expectedLength > MAX_FRAME_SIZE) {
                Serial.printf("✗ Frame too large: %d bytes\n", 7 + expectedLength);
                resetParser();
                stats.crcErrors++;
                break;
            }
            
            if (expectedLength == 0) {
                parserState = READ_CRC_LOW;
            } else {
                parserState = READ_PAYLOAD;
            }
            break;
            
        case READ_PAYLOAD:
            rxBuffer[rxIndex++] = byte;
            currentFrame.payload[payloadIndex++] = byte;
            
            if (payloadIndex >= expectedLength) {
                parserState = READ_CRC_LOW;
            }
            break;
            
        case READ_CRC_LOW:
            rxBuffer[rxIndex++] = byte;
            currentFrame.crc = byte;
            parserState = READ_CRC_HIGH;
            break;
            
        case READ_CRC_HIGH:
            rxBuffer[rxIndex++] = byte;
            currentFrame.crc |= (byte << 8);
            parserState = READ_END;
            break;
            
        case READ_END:
            rxBuffer[rxIndex++] = byte;
            currentFrame.end_delim = byte;
            
            if (byte == FRAME_END_DELIM) {
                currentFrame.timestamp = millis();
                validateAndProcessFrame();
            } else {
                Serial.printf("✗ Invalid end delimiter: 0x%02X (expected 0x%02X)\n", 
                             byte, FRAME_END_DELIM);
                stats.crcErrors++;
            }
            resetParser();
            break;
            
        default:
            Serial.printf("✗ Invalid parser state: %d\n", parserState);
            resetParser();
            break;
    }
}

/**
 * Validate CRC and process complete frame
 */
void validateAndProcessFrame() {
    uint16_t calculatedCRC = SmartMosqueUtils::calculateCRC16(
        &rxBuffer[1],
        3 + currentFrame.length
    );
    
    if (calculatedCRC == currentFrame.crc) {
        currentFrame.valid = true;
        stats.framesReceived++;
        
        #ifdef DEBUG_PROTOCOL
        Serial.printf("✓ RX[%d bytes]: ADDR=0x%02X CMD=0x%02X LEN=%d CRC=OK\n",
                     rxIndex, currentFrame.address, currentFrame.command, 
                     currentFrame.length);
        #endif
        
        processCompleteFrame();
        
    } else {
        currentFrame.valid = false;
        stats.crcErrors++;
        
        Serial.printf("✗ CRC Error: RX=0x%04X CALC=0x%04X\n", 
                     currentFrame.crc, calculatedCRC);
        
        if (currentState != STATE_DISCOVERY) {
            sendNACK(ERROR_INVALID_CRC, "Frame corrupted");
        }
    }
}

// ══════════════════════════════════════════════════════════════
// FRAME DISPATCHER
// ══════════════════════════════════════════════════════════════

void processCompleteFrame() {
    switch (currentFrame.command) {
        case CMD_HEARTBEAT:
            handleHeartbeat();
            break;

        case CMD_SET_RELAY:
            handleSetRelay(currentFrame.payload, currentFrame.length);
            break;

        case CMD_DISCOVERY_RESPONSE:
            handleDiscoveryResponse(currentFrame.payload, currentFrame.length);
            break;

        case CMD_FORCE_RESET:
            handleForceReset();
            break;

        case CMD_SENSOR_DATA:
            // Broadcast data suhu & kelembaban dari Logic Master
            // Digunakan untuk kontrol kecepatan kipas otomatis
            handleSensorData(currentFrame.payload, currentFrame.length);
            break;

        case CMD_ACK:
            #ifdef DEBUG_PROTOCOL
            Serial.println("✓ ACK received from Master");
            #endif
            break;

        case CMD_NACK:
            Serial.println("✗ NACK received from Master");
            break;

        default:
            Serial.printf("✗ Unknown command: 0x%02X\n", currentFrame.command);
            if (currentState != STATE_DISCOVERY) {
                sendNACK(ERROR_INVALID_COMMAND, "Unknown opcode");
            }
            stats.commandsRejected++;
            break;
    }
}

// ══════════════════════════════════════════════════════════════
// COMMAND HANDLERS
// ══════════════════════════════════════════════════════════════

void handleHeartbeat() {
    lastHeartbeatReceived = millis();
    
    if (currentState == STATE_FAILSAFE) {
        Serial.println("✓ Heartbeat restored from FAIL-SAFE");
        exitFailSafeMode();
    }
    
    // Throttled debug output
    static uint32_t lastDebugPrint = 0;
    if (millis() - lastDebugPrint > 60000) {
        Serial.printf("♥ Heartbeat OK (age: %lu s)\n", 
                     (millis() - bootTime) / 1000);
        lastDebugPrint = millis();
    }
}

void handleSetRelay(const uint8_t* payload, uint8_t length) {
    Serial.println("\n→ Processing SET_RELAY command");
    
    if (controlSource == CONTROL_MANUAL) {
        Serial.println("✗ Command rejected: Device in MANUAL mode");
        stats.commandsRejected++;
        sendNACK(ERROR_MANUAL_MODE_ACTIVE, "Flip mode switch to AUTO");
        return;
    }
    
    if (controlSource == CONTROL_FAILSAFE) {
        Serial.println("✗ Command rejected: Device in FAIL-SAFE mode");
        stats.commandsRejected++;
        return;
    }
    
    // Validate payload length
    if (length == 0 || length > 128) {
        Serial.printf("✗ Invalid payload length: %d\n", length);
        sendNACK(ERROR_PAYLOAD_TOO_LONG, "Invalid length");
        return;
    }
    
    uint8_t relayMask = 0xFF;
    uint8_t values[8] = {0};
    
    if (length == 9) {
        // Binary format
        relayMask = payload[0];
        memcpy(values, &payload[1], 8);
        
        Serial.println("  Format: Binary");
        Serial.printf("  Mask: 0x%02X\n", relayMask);
        Serial.print("  Values: [");
        for (int i = 0; i < 8; i++) {
            Serial.printf("%d", values[i]);
            if (i < 7) Serial.print(",");
        }
        Serial.println("]");
        
    } else {
        // JSON format
        StaticJsonDocument<128> doc;
        DeserializationError error = deserializeJson(doc, payload, length);
        
        if (error) {
            Serial.printf("✗ Invalid payload: not binary or JSON\n");
            sendNACK(ERROR_PAYLOAD_TOO_LONG, "Invalid payload format");
            return;
        }
        
        Serial.println("  Format: JSON");
        
        relayMask = doc["relay_mask"] | 0xFF;
        JsonArray valuesArray = doc["values"];
        
        if (valuesArray.isNull() || valuesArray.size() == 0) {
            Serial.println("✗ Missing or empty values array");
            sendNACK(ERROR_PAYLOAD_TOO_LONG, "Invalid JSON structure");
            return;
        }
        
        for (int i = 0; i < 8 && i < valuesArray.size(); i++) {
            values[i] = valuesArray[i];
        }
    }
    
    if (!validateRelayCommand(values)) {
        Serial.println("✗ Command validation failed");
        stats.commandsRejected++;
        sendNACK(ERROR_INTERLOCK_VIOLATION, "Multiple fan speeds ON");
        return;
    }
    
    Serial.println("✓ Command validated, executing...");
    applyRelayCommand(values);
    stats.commandsExecuted++;
    
    sendStatusReport();
    sendACK();
    
    Serial.println("✓ SET_RELAY completed successfully\n");
}

void handleDiscoveryResponse(const uint8_t* payload, uint8_t length) {
    Serial.println("\n→ Processing DISCOVERY_RESPONSE");
    
    if (currentState != STATE_DISCOVERY) {
        Serial.println("✗ Not in discovery mode, ignoring");
        return;
    }
    
    // Validate payload length
    if (length == 0 || length > 255) {
        Serial.printf("✗ Invalid payload length: %d\n", length);
        return;
    }
    
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, payload, length);
    
    if (error) {
        Serial.printf("✗ JSON parse error: %s\n", error.c_str());
        sendNACK(ERROR_PAYLOAD_TOO_LONG, "Invalid JSON");
        return;
    }
    
    // Validate MAC
    const char* macStr = doc["mac"];
    if (macStr == nullptr) {
        Serial.println("✗ Missing MAC field");
        return;
    }
    
    char myMacStr[18];
    SmartMosqueUtils::macToString(myMAC, myMacStr);
    
    if (strcmp(macStr, myMacStr) != 0) {
        Serial.printf("✗ MAC mismatch: expected %s, got %s\n", myMacStr, macStr);
        return;
    }
    
    Serial.println("✓ MAC address validated");
    
    // Extract assigned ID
    if (!doc.containsKey("assigned_id")) {
        Serial.println("✗ Missing assigned_id field");
        return;
    }
    
    uint8_t assignedID = doc["assigned_id"];
    
    if (assignedID < ADDR_MIN_SLAVE || assignedID > ADDR_MAX_SLAVE) {
        Serial.printf("✗ Invalid ID: %d (must be 1-247)\n", assignedID);
        sendNACK(ERROR_PAYLOAD_TOO_LONG, "Invalid ID range");
        return;
    }
    
    Serial.printf("✓ Assigned ID: %d\n", assignedID);
    
    // Extract device name
    const char* deviceName = doc["device_name"];
    if (deviceName == nullptr) {
        Serial.println("✗ Missing device_name field");
        return;
    }
    
    Serial.printf("✓ Device name: %s\n", deviceName);
    
    // Extract profile
    const char* profileStr = doc["profile"];
    if (profileStr == nullptr) {
        Serial.println("✗ Missing profile field");
        return;
    }
    
    uint8_t profile = PROFILE_UNDEFINED;
    if (strcmp(profileStr, "FAN_4CH") == 0) {
        profile = PROFILE_FAN_4CH;
    } else if (strcmp(profileStr, "LIGHT_8CH") == 0) {
        profile = PROFILE_LIGHT_8CH;
    } else if (strcmp(profileStr, "SOUND_8CH") == 0) {
        profile = PROFILE_SOUND_8CH;
    } else {
        Serial.printf("✗ Unknown profile: %s\n", profileStr);
        return;
    }
    
    Serial.printf("✓ Profile: %s\n", profileStr);
    
    // Update config
    config.device_id = assignedID;
    strncpy(config.device_name, deviceName, 31);
    config.device_name[31] = '\0';
    config.profile = profile;
    memcpy(config.mac, myMAC, 6);
    
    // Save to NVS
    Serial.println("→ Saving configuration to NVS...");
    if (saveConfigToNVS()) {
        Serial.println("✓ Configuration saved successfully");
        
        sendACK();
        delay(100);
        
        Serial.println("\n╔═══════════════════════════════════════╗");
        Serial.println("║   ENROLLMENT SUCCESSFUL               ║");
        Serial.println("╠═══════════════════════════════════════╣");
        Serial.printf("║ New ID:   %-27d ║\n", assignedID);
        Serial.printf("║ Name:     %-27s ║\n", deviceName);
        Serial.printf("║ Profile:  %-27s ║\n", profileStr);
        Serial.println("╚═══════════════════════════════════════╝");
        Serial.println("\n→ Restarting in 2 seconds...\n");
        
        delay(2000);
        ESP.restart();
        
    } else {
        Serial.println("✗ Failed to save configuration");
        sendNACK(ERROR_NVS_WRITE_ERROR, "NVS write failed");
    }
}

void handleForceReset() {
    Serial.println("\n⚠⚠⚠ FORCE_RESET COMMAND RECEIVED ⚠⚠⚠");
    Serial.println("This will erase all configuration!");
    
    sendACK();
    delay(100);
    
    Serial.println("→ Executing factory reset...");
    factoryReset();
}

// ══════════════════════════════════════════════════════════════
// DEBUG HELPERS
// ══════════════════════════════════════════════════════════════

void analyzeFrame(const MosqueFrame* frame) {
    #ifdef DEBUG_PROTOCOL
    Serial.println("\n═══ FRAME ANALYSIS ═══");
    
    Serial.print("HEX: ");
    Serial.printf("%02X ", frame->start_delim);
    Serial.printf("%02X ", frame->address);
    Serial.printf("%02X ", frame->command);
    Serial.printf("%02X ", frame->length);
    
    for (int i = 0; i < frame->length; i++) {
        Serial.printf("%02X ", frame->payload[i]);
    }
    
    Serial.printf("%02X %02X ", frame->crc & 0xFF, (frame->crc >> 8) & 0xFF);
    Serial.printf("%02X\n", frame->end_delim);
    
    Serial.printf("START:   0x%02X %s\n", 
                 frame->start_delim, 
                 frame->start_delim == FRAME_START_DELIM ? "✓" : "✗");
    Serial.printf("ADDR:    0x%02X (%d) %s\n", 
                 frame->address, frame->address,
                 (frame->address == config.device_id || 
                  frame->address == ADDR_BROADCAST) ? "✓" : "for other");
    Serial.printf("CMD:     0x%02X\n", frame->command);
    Serial.printf("LENGTH:  %d bytes\n", frame->length);
    
    if (frame->length > 0) {
        Serial.print("PAYLOAD: ");
        bool printable = true;
        for (int i = 0; i < frame->length; i++) {
            if (frame->payload[i] < 32 || frame->payload[i] > 126) {
                printable = false;
                break;
            }
        }
        
        if (printable) {
            Serial.write(frame->payload, frame->length);
        } else {
            for (int i = 0; i < frame->length; i++) {
                Serial.printf("%02X ", frame->payload[i]);
            }
        }
        Serial.println();
    }
    
    Serial.printf("CRC:     0x%04X %s\n", 
                 frame->crc, frame->valid ? "✓" : "✗");
    Serial.printf("END:     0x%02X %s\n", 
                 frame->end_delim,
                 frame->end_delim == FRAME_END_DELIM ? "✓" : "✗");
    
    Serial.println("═════════════════════\n");
    #endif
}
