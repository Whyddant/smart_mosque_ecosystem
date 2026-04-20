/**
 * ═══════════════════════════════════════════════════════════════
 *  LOGIC MASTER - FRAME PROCESSING
 * 
 *  RS-485 frame parser and dispatcher
 *  Routes incoming frames to appropriate handlers
 *  Manages RS-485 transmission with proper timing
 *  Referensi: [STP-11] Complete Frame Encoder/Decoder
 * ═══════════════════════════════════════════════════════════════
 */

#include "../include/logic_master.h"

// ══════════════════════════════════════════════════════════════
// FRAME PARSER STATE
// ══════════════════════════════════════════════════════════════

uint8_t rxBuffer[MAX_FRAME_SIZE];
uint16_t rxIndex = 0;

enum ParserState {
    WAIT_START,
    READ_ADDRESS,
    READ_COMMAND,
    READ_LENGTH,
    READ_PAYLOAD,
    READ_CRC_LOW,
    READ_CRC_HIGH,
    READ_END
};

ParserState parserState = WAIT_START;
MosqueFrame currentFrame;

// Statistics
struct FrameStats {
    uint32_t framesReceived;
    uint32_t framesSent;
    uint32_t crcErrors;
    uint32_t malformedFrames;
    uint32_t framesProcessed;
} frameStats;

// ══════════════════════════════════════════════════════════════
// FRAME PARSER (BYTE-BY-BYTE STATE MACHINE)
// ══════════════════════════════════════════════════════════════

void processIncomingByte(uint8_t byte) {
    static uint8_t expectedLength = 0;
    static uint8_t payloadIndex = 0;
    
    switch (parserState) {
        case WAIT_START:
            if (byte == FRAME_START_DELIM) {
                rxBuffer[0] = byte;
                rxIndex = 1;
                parserState = READ_ADDRESS;
            }
            break;
            
        case READ_ADDRESS:
            rxBuffer[rxIndex++] = byte;
            currentFrame.address = byte;
            
            // Check if this frame is for us (Logic Master or Broadcast)
            if (byte != ADDR_LOGIC_MASTER && byte != ADDR_BROADCAST) {
                // Not for us, but we listen to all traffic for monitoring
                // Continue parsing for logging purposes
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
            
            // Safety check
            if (rxIndex >= MAX_FRAME_SIZE - 3) {
                Serial.println("✗ Buffer overflow detected");
                parserState = WAIT_START;
                frameStats.malformedFrames++;
            }
            break;
            
        case READ_CRC_LOW:
            rxBuffer[rxIndex++] = byte;
            currentFrame.crc = byte;  // Low byte
            parserState = READ_CRC_HIGH;
            break;
            
        case READ_CRC_HIGH:
            rxBuffer[rxIndex++] = byte;
            currentFrame.crc |= (byte << 8);  // High byte
            parserState = READ_END;
            break;
            
        case READ_END:
            rxBuffer[rxIndex++] = byte;
            
            if (byte == FRAME_END_DELIM) {
                // Frame complete, validate
                currentFrame.valid = false;
                currentFrame.timestamp = millis();
                
                // Calculate CRC
                uint16_t calculatedCRC = SmartMosqueUtils::calculateCRC16(
                    &rxBuffer[1],
                    3 + currentFrame.length
                );
                
                if (calculatedCRC == currentFrame.crc) {
                    currentFrame.valid = true;
                    frameStats.framesReceived++;
                    processCompleteFrame();
                } else {
                    frameStats.crcErrors++;
                    Serial.printf("✗ CRC error: RX=0x%04X, CALC=0x%04X\n",
                                currentFrame.crc, calculatedCRC);
                }
            } else {
                frameStats.malformedFrames++;
                Serial.println("✗ Invalid END delimiter");
            }
            
            // Reset parser
            parserState = WAIT_START;
            rxIndex = 0;
            break;
    }
}

// ══════════════════════════════════════════════════════════════
// FRAME DISPATCHER
// ══════════════════════════════════════════════════════════════

void processCompleteFrame() {
    frameStats.framesProcessed++;
    
    // Debug log (compact)
    Serial.printf("← RX[%d]: ADDR=%d CMD=0x%02X LEN=%d\n",
                 frameStats.framesProcessed,
                 currentFrame.address,
                 currentFrame.command,
                 currentFrame.length);
    
    // Dispatch based on command
    switch (currentFrame.command) {
        case CMD_DISCOVERY_ANNOUNCE:
            handleDiscoveryAnnounce(currentFrame.payload, currentFrame.length);
            break;

        case CMD_STATUS_REPORT:
            handleStatusReport(currentFrame.address,
                             currentFrame.payload,
                             currentFrame.length);
            break;

        case CMD_ACK:
            handleACK(currentFrame.address);
            break;

        case CMD_NACK:
            handleNACK(currentFrame.address,
                      currentFrame.payload,
                      currentFrame.length);
            break;

        case CMD_HEARTBEAT:
            // Heartbeat dari Display Master (jika dual master)
            Serial.println("← Heartbeat from Display Master");
            break;

        case CMD_SENSOR_DATA:
            // Slave mengirim balik data sensor? (jarang, biasanya Master broadcast)
            // Log saja untuk keperluan debug
            Serial.printf("← CMD_SENSOR_DATA dari ID %d (unexpected)\n",
                         currentFrame.address);
            break;

        default:
            Serial.printf("⚠ Unknown command: 0x%02X from ID %d\n",
                         currentFrame.command, currentFrame.address);
            break;
    }
}

// ══════════════════════════════════════════════════════════════
// RS-485 TRANSMISSION
// ══════════════════════════════════════════════════════════════

void sendRS485Frame(uint8_t address, uint8_t command, 
                   const uint8_t* payload, uint8_t length) {
    if (length > MAX_PAYLOAD_SIZE) {
        Serial.println("✗ Payload too large");
        return;
    }
    
    // Build frame
    uint8_t frame[MAX_FRAME_SIZE];
    int idx = 0;
    
    frame[idx++] = FRAME_START_DELIM;  // 0x7E
    frame[idx++] = address;
    frame[idx++] = command;
    frame[idx++] = length;
    
    // Copy payload
    if (length > 0 && payload != nullptr) {
        memcpy(&frame[idx], payload, length);
        idx += length;
    }
    
    // Calculate CRC (from ADDRESS to end of PAYLOAD)
    uint16_t crc = SmartMosqueUtils::calculateCRC16(&frame[1], 3 + length);
    frame[idx++] = crc & 0xFF;         // CRC_LOW
    frame[idx++] = (crc >> 8) & 0xFF;  // CRC_HIGH
    
    frame[idx++] = FRAME_END_DELIM;    // 0x0A
    
    // Transmit via RS-485
    // Board ESP32-S3 IoT Logger RS485 Plus: DE/RE otomatis (onboard chip)
    Serial2.write(frame, idx);
    Serial2.flush();  // Tunggu transmisi selesai
    
    frameStats.framesSent++;
    
    // Debug log
    Serial.printf("→ TX[%d]: ADDR=%d CMD=0x%02X LEN=%d\n",
                 frameStats.framesSent, address, command, length);
}

// ══════════════════════════════════════════════════════════════
// RECEIVE TASK (MAIN LOOP)
// ══════════════════════════════════════════════════════════════

void processRS485Receive() {
    while (Serial2.available()) {
        uint8_t byte = Serial2.read();
        processIncomingByte(byte);
    }
}

// ══════════════════════════════════════════════════════════════
// STATISTICS
// ══════════════════════════════════════════════════════════════

void printFrameStats() {
    Serial.println("\n╔═══════════════════════════════════════╗");
    Serial.println("║       FRAME PROCESSING STATS          ║");
    Serial.println("╠═══════════════════════════════════════╣");
    Serial.printf("║ Frames Received:   %8lu        ║\n", frameStats.framesReceived);
    Serial.printf("║ Frames Sent:       %8lu        ║\n", frameStats.framesSent);
    Serial.printf("║ Frames Processed:  %8lu        ║\n", frameStats.framesProcessed);
    Serial.printf("║ CRC Errors:        %8lu        ║\n", frameStats.crcErrors);
    Serial.printf("║ Malformed Frames:  %8lu        ║\n", frameStats.malformedFrames);
    
    float errorRate = (frameStats.framesReceived > 0) ?
                     (float)(frameStats.crcErrors + frameStats.malformedFrames) /
                     frameStats.framesReceived * 100.0f : 0.0f;
    
    Serial.printf("║ Error Rate:        %6.2f%%         ║\n", errorRate);
    Serial.println("╚═══════════════════════════════════════╝\n");
}

// ══════════════════════════════════════════════════════════════
// INITIALIZATION
// ══════════════════════════════════════════════════════════════

void initFrameProcessing() {
    parserState = WAIT_START;
    rxIndex = 0;
    memset(&frameStats, 0, sizeof(frameStats));
    
    Serial.println("✓ Frame processing initialized");
}

/**
 * ═══════════════════════════════════════════════════════════════
 *  BUS MONITORING & DIAGNOSTICS
 * ═══════════════════════════════════════════════════════════════
 */

void analyzeFrame(const uint8_t* frame, uint16_t len) {
    Serial.println("\n═══ FRAME ANALYSIS ═══");
    
    // Hex dump
    Serial.print("HEX: ");
    for (int i = 0; i < len && i < 64; i++) {
        Serial.printf("%02X ", frame[i]);
        if ((i + 1) % 16 == 0) Serial.print("\n     ");
    }
    Serial.println();
    
    // Parse fields
    if (len >= 7) {
        Serial.printf("START:   0x%02X %s\n", frame[0], 
                     frame[0] == 0x7E ? "✓" : "✗");
        Serial.printf("ADDR:    0x%02X (%d)\n", frame[1], frame[1]);
        Serial.printf("CMD:     0x%02X\n", frame[2]);
        Serial.printf("LENGTH:  0x%02X (%d bytes)\n", frame[3], frame[3]);
        
        if (frame[3] > 0 && len >= 4 + frame[3]) {
            Serial.print("PAYLOAD: ");
            for (int i = 0; i < frame[3] && i < 32; i++) {
                Serial.printf("%02X ", frame[4 + i]);
            }
            Serial.println();
        }
        
        uint16_t rxCRC = frame[4 + frame[3]] | (frame[5 + frame[3]] << 8);
        uint16_t calcCRC = SmartMosqueUtils::calculateCRC16(&frame[1], 3 + frame[3]);
        Serial.printf("CRC:     0x%04X (calc: 0x%04X) %s\n",
                     rxCRC, calcCRC, rxCRC == calcCRC ? "✓" : "✗");
        Serial.printf("END:     0x%02X %s\n", frame[6 + frame[3]],
                     frame[6 + frame[3]] == 0x0A ? "✓" : "✗");
    }
    
    Serial.println("═══════════════════════\n");
}

/**
 * ═══════════════════════════════════════════════════════════════
 *  USAGE EXAMPLES
 * ═══════════════════════════════════════════════════════════════
 * 
 * // Initialize
 * initFrameProcessing();
 * 
 * // In main loop:
 * processRS485Receive();  // Call frequently (every iteration)
 * 
 * // Send frame:
 * uint8_t payload[] = {0xAA};
 * sendRS485Frame(ADDR_BROADCAST, CMD_HEARTBEAT, payload, 1);
 * 
 * // Debug: Analyze captured frame
 * uint8_t capturedFrame[64];
 * analyzeFrame(capturedFrame, sizeof(capturedFrame));
 * 
 * ═══════════════════════════════════════════════════════════════
 */
