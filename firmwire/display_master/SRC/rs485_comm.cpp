/**
 * ============================================================================
 * RS-485 COMMUNICATION MODULE v1.1
 * Display Master - Smart Mosque Ecosystem
 * ============================================================================
 * 
 * v1.1 CHANGES:
 * - Added proper protocol includes
 * - Fixed missing constants and functions
 * - Added heartbeat monitoring
 * - Improved parser error handling
 * - Fixed CRC function calls
 * ============================================================================
 */

#include "../include/display_master.h"

// ✅ FIX: Proper protocol includes
#include "../../../shared/SmartMosqueProtocol/SmartMosqueProtocol.h"

// ✅ FIX: Define frame delimiters if not in protocol header
#ifndef FRAME_START_DELIM
#define FRAME_START_DELIM  0x7E
#define FRAME_END_DELIM    0x0A
#endif

// ============================================================================
// RS-485 HARDWARE CONTROL
// ============================================================================

HardwareSerial RS485Serial(2);

void rs485Init() {
    RS485Serial.begin(RS485_BAUD_RATE, SERIAL_8N1, RS485_RX, RS485_TX);
    pinMode(RS485_DE_RE, OUTPUT);
    digitalWrite(RS485_DE_RE, LOW);
    
    Serial.printf("✓ RS-485 initialized: %d bps on pins RX=%d, TX=%d, DE/RE=%d\n",
                  RS485_BAUD_RATE, RS485_RX, RS485_TX, RS485_DE_RE);
}

inline void rs485SetMode(bool transmit) {
    digitalWrite(RS485_DE_RE, transmit ? HIGH : LOW);
    if (transmit) {
        delayMicroseconds(10);
    }
}

// ============================================================================
// FRAME TRANSMISSION
// ============================================================================

bool rs485SendFrame(uint8_t target_addr, uint8_t opcode, 
                    const uint8_t* payload, uint8_t payload_len) {
    
    if (payload_len > MAX_PAYLOAD_SIZE) {
        Serial.printf("⚠ Payload too long: %d bytes\n", payload_len);
        return false;
    }
    
    uint8_t frame_buffer[MAX_FRAME_SIZE];
    uint8_t frame_len = 0;
    
    frame_buffer[frame_len++] = FRAME_START_DELIM;
    frame_buffer[frame_len++] = target_addr;
    frame_buffer[frame_len++] = opcode;
    frame_buffer[frame_len++] = payload_len;
    
    if (payload_len > 0 && payload != nullptr) {
        memcpy(&frame_buffer[frame_len], payload, payload_len);
        frame_len += payload_len;
    }
    
    // ✅ FIX: Use proper CRC function from protocol library
    uint16_t crc = SmartMosqueUtils::calculateCRC16(&frame_buffer[1], frame_len - 1);
    
    frame_buffer[frame_len++] = crc & 0xFF;
    frame_buffer[frame_len++] = (crc >> 8) & 0xFF;
    frame_buffer[frame_len++] = FRAME_END_DELIM;
    
    rs485SetMode(true);
    
    size_t written = RS485Serial.write(frame_buffer, frame_len);
    RS485Serial.flush();
    
    rs485SetMode(false);
    
    if (written != frame_len) {
        Serial.printf("⚠ TX error: sent %d/%d bytes\n", written, frame_len);
        return false;
    }
    
    #ifdef DEBUG_RS485_VERBOSE
    Serial.printf("📤 TX [%d bytes]: to=%d, cmd=0x%02X\n", 
                  frame_len, target_addr, opcode);
    #endif
    
    return true;
}

// ============================================================================
// FRAME RECEPTION
// ============================================================================

enum RS485ParserState {
    STATE_WAIT_START,
    STATE_READ_ADDR,
    STATE_READ_CMD,
    STATE_READ_LEN,
    STATE_READ_PAYLOAD,
    STATE_READ_CRC_L,
    STATE_READ_CRC_H,
    STATE_READ_END
};

static RS485ParserState g_parser_state = STATE_WAIT_START;
static SmartMosqueFrame g_rx_frame;
static uint8_t g_rx_payload_buffer[256];
static uint8_t g_rx_payload_index = 0;
static uint32_t g_last_rx_byte_time = 0;

#define PARSER_TIMEOUT_MS  100

void rs485ResetParser() {
    g_parser_state = STATE_WAIT_START;
    g_rx_payload_index = 0;
    g_rx_frame.payload_len = 0;
}

void rs485ProcessIncoming() {
    uint32_t now = millis();
    
    // ✅ IMPROVED: Better parser timeout handling
    if (g_parser_state != STATE_WAIT_START) {
        if (now - g_last_rx_byte_time > PARSER_TIMEOUT_MS) {
            Serial.printf("⚠ Parser timeout in state %d, reset\n", g_parser_state);
            g_stats.frames_invalid++;
            rs485ResetParser();
        }
    }
    
    while (RS485Serial.available()) {
        uint8_t byte = RS485Serial.read();
        g_last_rx_byte_time = now;
        
        switch (g_parser_state) {
            
            case STATE_WAIT_START:
                if (byte == FRAME_START_DELIM) {
                    g_parser_state = STATE_READ_ADDR;
                }
                break;
            
            case STATE_READ_ADDR:
                g_rx_frame.address = byte;
                g_parser_state = STATE_READ_CMD;
                break;
            
            case STATE_READ_CMD:
                g_rx_frame.command = byte;
                g_parser_state = STATE_READ_LEN;
                break;
            
            case STATE_READ_LEN:
                g_rx_frame.payload_len = byte;
                g_rx_payload_index = 0;
                
                if (g_rx_frame.payload_len == 0) {
                    g_parser_state = STATE_READ_CRC_L;
                } else {
                    g_parser_state = STATE_READ_PAYLOAD;
                }
                break;
            
            case STATE_READ_PAYLOAD:
                g_rx_payload_buffer[g_rx_payload_index++] = byte;
                
                if (g_rx_payload_index >= g_rx_frame.payload_len) {
                    g_parser_state = STATE_READ_CRC_L;
                }
                break;
            
            case STATE_READ_CRC_L:
                g_rx_frame.crc_low = byte;
                g_parser_state = STATE_READ_CRC_H;
                break;
            
            case STATE_READ_CRC_H:
                g_rx_frame.crc_high = byte;
                g_parser_state = STATE_READ_END;
                break;
            
            case STATE_READ_END:
                if (byte == FRAME_END_DELIM) {
                    // Frame complete! Validate & process
                    
                    uint8_t crc_buffer[260];
                    crc_buffer[0] = g_rx_frame.address;
                    crc_buffer[1] = g_rx_frame.command;
                    crc_buffer[2] = g_rx_frame.payload_len;
                    
                    if (g_rx_frame.payload_len > 0) {
                        memcpy(&crc_buffer[3], g_rx_payload_buffer, g_rx_frame.payload_len);
                    }
                    
                    // ✅ FIX: Use proper CRC function
                    uint16_t calc_crc = SmartMosqueUtils::calculateCRC16(
                        crc_buffer, 
                        3 + g_rx_frame.payload_len
                    );
                    uint16_t recv_crc = (g_rx_frame.crc_high << 8) | g_rx_frame.crc_low;
                    
                    if (calc_crc == recv_crc) {
                        g_rx_frame.payload = g_rx_payload_buffer;
                        rs485HandleFrame(&g_rx_frame);
                        g_stats.frames_received++;
                        
                    } else {
                        Serial.printf("⚠ CRC error: calc=0x%04X, recv=0x%04X\n", 
                                     calc_crc, recv_crc);
                        g_stats.frames_invalid++;
                    }
                    
                } else {
                    Serial.printf("⚠ Expected END byte, got 0x%02X\n", byte);
                    g_stats.frames_invalid++;
                }
                
                rs485ResetParser();
                break;
        }
    }
}

// ============================================================================
// FRAME HANDLING
// ============================================================================

void rs485HandleFrame(const SmartMosqueFrame* frame) {
    #ifdef DEBUG_RS485_VERBOSE
    Serial.printf("📥 RX Frame: addr=%d, cmd=0x%02X, len=%d\n",
                  frame->address, frame->command, frame->payload_len);
    #endif
    
    if (frame->address != DISPLAY_MASTER_ID && frame->address != 0x00) {
        return;
    }
    
    switch (frame->command) {
        
        // ✅ NEW: Heartbeat monitoring
        case CMD_HEARTBEAT: {
            g_last_logic_heartbeat = millis();
            
            if (!g_logic_master_online) {
                g_logic_master_online = true;
                Serial.println("✓ Logic Master back online");
            }
            
            #ifdef DEBUG_RS485_VERBOSE
            Serial.printf("♥ Heartbeat from Logic Master\n");
            #endif
            
            break;
        }
        
        case CMD_DISCOVERY_ANNOUNCE: {
            if (frame->payload_len != 7) {
                Serial.println("⚠ Invalid DISCOVERY_ANNOUNCE payload");
                break;
            }
            
            uint8_t mac[6];
            memcpy(mac, frame->payload, 6);
            uint8_t profile = frame->payload[6];
            
            if (g_pending_enrollment.active) {
                Serial.println("⚠ Already have pending enrollment");
                break;
            }
            
            memcpy(g_pending_enrollment.mac, mac, 6);
            g_pending_enrollment.profile = profile;
            g_pending_enrollment.first_seen = millis();
            g_pending_enrollment.active = true;
            
            uiEnrollmentShow(mac, profile);
            
            char mac_str[18];
            formatMAC(mac, mac_str);
            Serial.printf("🆕 New device: %s (profile=%s)\n", 
                         mac_str, getProfileName(profile));
            
            break;
        }
        
        case CMD_STATUS_REPORT: {
            if (frame->payload_len < 3) {
                Serial.println("⚠ Invalid STATUS_REPORT payload");
                break;
            }
            
            uint8_t device_id = frame->payload[0];
            uint8_t relay_state = frame->payload[1];
            uint8_t control_source = frame->payload[2];
            
            deviceManagerUpdate(device_id, relay_state, control_source);
            
            #ifdef DEBUG_RS485_VERBOSE
            Serial.printf("📊 Status: ID=%d, Relay=0x%02X, Source=%s\n",
                         device_id, relay_state, getControlSourceName(control_source));
            #endif
            
            break;
        }
        
        case CMD_ACK: {
            CommandQueueItem* cmd = commandQueuePeek();
            if (cmd && cmd->waiting_ack) {
                commandQueuePop();
                g_stats.commands_acked++;
                
                #ifdef DEBUG_RS485_VERBOSE
                Serial.printf("✓ ACK received for command to device %d\n", cmd->target_id);
                #endif
            }
            break;
        }
        
        case CMD_NACK: {
            if (frame->payload_len < 1) {
                Serial.println("⚠ Invalid NACK payload");
                break;
            }
            
            uint8_t error_code = frame->payload[0];
            
            CommandQueueItem* cmd = commandQueuePeek();
            if (cmd && cmd->waiting_ack) {
                Serial.printf("✗ NACK: Device %d, Error code 0x%02X\n", 
                             cmd->target_id, error_code);
                
                switch (error_code) {
                    case ERROR_MANUAL_MODE_ACTIVE:
                        uiDashboardShowAlert("Device in Manual Mode", 1);
                        break;
                    case ERROR_INTERLOCK_VIOLATION:
                        uiDashboardShowAlert("Interlock Violation", 2);
                        break;
                    default:
                        uiDashboardShowAlert("Command Failed", 2);
                        break;
                }
                
                commandQueuePop();
                g_stats.commands_failed++;
            }
            break;
        }
        
        // ✅ FIXED: Use constant instead of magic number
        case CMD_DEVICE_LIST_SYNC: {
            if (frame->payload_len < 1) {
                break;
            }

            uint8_t count = frame->payload[0];
            Serial.printf("📋 Received device list: %d devices\n", count);

            uint8_t offset = 1;
            for (uint8_t i = 0; i < count && i < MAX_DEVICES; i++) {
                if (offset + 41 > frame->payload_len) {
                    break;
                }

                uint8_t id = frame->payload[offset];
                offset += 7;

                strncpy(g_devices[i].name, (char*)&frame->payload[offset], 32);
                offset += 32;

                g_devices[i].id = id;
                g_devices[i].profile = frame->payload[offset++];
                g_devices[i].online = frame->payload[offset++];
                g_devices[i].relay_state = 0;
                g_devices[i].control_source = 0;
                g_devices[i].last_update = millis();
            }

            g_device_count = count;
            Serial.printf("✓ Device list synced: %d devices\n", count);

            break;
        }

        // Terima broadcast data sensor dari Logic Master
        case CMD_SENSOR_DATA: {
            if (frame->payload_len < sizeof(SensorData)) {
                Serial.println("⚠ CMD_SENSOR_DATA: payload terlalu pendek");
                break;
            }

            const SensorData* sd = (const SensorData*)frame->payload;
            if (sd->status == 0) {
                g_sensor_data.temperature    = sd->temperature_x10 / 10.0f;
                g_sensor_data.humidity       = sd->humidity_x10    / 10.0f;
                g_sensor_data.valid          = true;
                g_sensor_data.last_update_ms = millis();

                Serial.printf("🌡 Sensor: T=%.1f°C H=%.1f%%RH\n",
                              g_sensor_data.temperature, g_sensor_data.humidity);
            } else {
                Serial.printf("⚠ CMD_SENSOR_DATA: status error=%d\n", sd->status);
            }
            break;
        }
        
        default:
            Serial.printf("⚠ Unknown command: 0x%02X\n", frame->command);
            break;
    }
}

// ============================================================================
// COMMAND QUEUE PROCESSING
// ============================================================================

void commandQueueProcess() {
    CommandQueueItem* cmd = commandQueuePeek();
    if (!cmd) {
        return;
    }
    
    uint32_t now = millis();
    
    if (cmd->waiting_ack) {
        if (now - cmd->timestamp > COMMAND_TIMEOUT_MS) {
            cmd->retry_count++;
            
            if (cmd->retry_count < 3) {
                Serial.printf("⏱ Command timeout, retry %d/3\n", cmd->retry_count);
                cmd->timestamp = now;
                cmd->waiting_ack = false;
            } else {
                Serial.printf("✗ Command failed after 3 retries (device %d)\n", cmd->target_id);
                commandQueuePop();
                g_stats.commands_failed++;
                uiDashboardShowAlert("Command Timeout", 2);
            }
        }
        return;
    }
    
    bool sent = rs485SendFrame(cmd->target_id, cmd->opcode, 
                               cmd->payload, cmd->payload_len);
    
    if (sent) {
        cmd->waiting_ack = true;
        cmd->timestamp = now;
    } else {
        Serial.println("⚠ Failed to send command");
        commandQueuePop();
        g_stats.commands_failed++;
    }
}
