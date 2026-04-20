#ifndef SMART_MOSQUE_PROTOCOL_H
#define SMART_MOSQUE_PROTOCOL_H

#include <Arduino.h>

/**
 * ═══════════════════════════════════════════════════════════════
 *  SMART MOSQUE PROTOCOL DEFINITIONS v2.1
 *  Berdasarkan:
 *  - Kebenaran Tunggal v1.4 (KT)
 *  - Spesifikasi Teknis Protokol v1.3 (STP)
 *
 *  Changelog v2.1:
 *  - Tambah PROFILE_SENSOR_TEMP untuk sensor SHT20 XY-MD02 (RS485 Modbus RTU)
 *  - PROFILE_LIGHT_8CH & PROFILE_SOUND_8CH: node relay 8ch Modbus RTU eksternal
 *  - Tambah CMD_SENSOR_READ & CMD_SENSOR_DATA
 *  - Tambah struct SensorData (suhu & kelembaban)
 *  - Update PROFILE_NAMES & validateConfig
 * ═══════════════════════════════════════════════════════════════
 */

// ══════════════════════════════════════════════════════════════
// SECTION 1: FRAME CONSTANTS
// Referensi: [STP-3.1] Frame Structure Byte-by-Byte
// ══════════════════════════════════════════════════════════════

#define FRAME_START_DELIM   0x7E  // Start marker '~'
#define FRAME_END_DELIM     0x0A  // End marker '\n'
#define MIN_FRAME_SIZE      7     // Minimum: START+ADDR+CMD+LEN+CRC+CRC+END
#define MAX_FRAME_SIZE      262   // Maximum: 7 overhead + 255 payload
#define MAX_PAYLOAD_SIZE    255   // Maximum payload length

// ══════════════════════════════════════════════════════════════
// SECTION 2: ADDRESSING SCHEME
// Referensi: [STP-3.2.2] ADDRESS Field Specifications
// ══════════════════════════════════════════════════════════════

#define ADDR_BROADCAST      0x00  // Broadcast to all slaves
#define ADDR_MIN_SLAVE      0x01  // Minimum slave ID
#define ADDR_MAX_SLAVE      0xF7  // Maximum slave ID (247)
#define ADDR_LOGIC_MASTER   0xFE  // Logic Master fixed ID (254)
#define ADDR_DISPLAY_MASTER 0xFF  // Display Master fixed ID (255)

// ══════════════════════════════════════════════════════════════
// SECTION 3: COMMAND OPCODES
// Referensi: [STP-3.2.3] COMMAND Field Specifications
// ══════════════════════════════════════════════════════════════

#define CMD_DISCOVERY_ANNOUNCE  0x01  // Slave→Master: Request enrollment
#define CMD_DISCOVERY_RESPONSE  0x02  // Master→Slave: Assign ID
#define CMD_SET_RELAY           0x03  // Master→Slave: Control relays
#define CMD_HEARTBEAT           0x04  // Master→All: Alive signal
#define CMD_STATUS_REPORT       0x05  // Slave→Master: Status update
#define CMD_FORCE_RESET         0x06  // Master→Slave: Factory reset (optional)
#define CMD_ACK                 0x07  // Bidirectional: Acknowledge
#define CMD_NACK                0x08  // Bidirectional: Negative acknowledge
#define CMD_SENSOR_READ         0x09  // Master→Sensor: Request sensor reading
#define CMD_SENSOR_DATA         0x0A  // Sensor→Master: Temperature & humidity data

// ══════════════════════════════════════════════════════════════
// SECTION 4: DEVICE PROFILES
// Referensi: [KT-6.3] Profile Type Mapping
// ══════════════════════════════════════════════════════════════

enum DeviceProfile : uint8_t {
    PROFILE_UNDEFINED   = 0,
    PROFILE_FAN_4CH     = 1,  // HwThinker 4-relay ESP32: 3 relay speed + 1 relay swing
    PROFILE_LIGHT_8CH   = 2,  // Modbus RTU RS485 Relay 8Ch: node saklar lampu
    PROFILE_SOUND_8CH   = 3,  // Modbus RTU RS485 Relay 8Ch: node sound sistem
    PROFILE_SENSOR_TEMP = 4   // SHT20 XY-MD02 RS485: sensor suhu & kelembaban
};

// Profile string names for JSON
static const char* PROFILE_NAMES[] = {
    "UNDEFINED",
    "FAN_4CH",
    "LIGHT_8CH",
    "SOUND_8CH",
    "SENSOR_TEMP"
};

// ══════════════════════════════════════════════════════════════
// SECTION 5: ERROR CODES
// Referensi: [STP-7.4] Error Codes (NACK Payload)
// ══════════════════════════════════════════════════════════════

enum ErrorCode : uint8_t {
    ERROR_OK                    = 0x00,  // No error
    ERROR_INVALID_CRC           = 0x01,  // CRC mismatch
    ERROR_PAYLOAD_TOO_LONG      = 0x02,  // Length > 255
    ERROR_INVALID_COMMAND       = 0x03,  // Unknown opcode
    ERROR_MANUAL_MODE_ACTIVE    = 0x04,  // Kipas in MANUAL mode
    ERROR_INTERLOCK_VIOLATION   = 0x05,  // Multiple speed relay ON
    ERROR_NVS_WRITE_ERROR       = 0x06,  // Flash write failed
    ERROR_PROFILE_MISMATCH      = 0x07,  // Command not for this profile
    ERROR_DEVICE_NOT_FOUND      = 0x08,  // ID not registered
    ERROR_TIMEOUT               = 0x09,  // Response too slow
    ERROR_BUFFER_OVERFLOW       = 0x0A   // Too many commands
};

// ══════════════════════════════════════════════════════════════
// SECTION 6: CONTROL SOURCE
// Referensi: [KT-3.3] State Machine Slave
// ══════════════════════════════════════════════════════════════

enum ControlSource : uint8_t {
    CONTROL_AUTO     = 0,  // Normal remote control
    CONTROL_MANUAL   = 1,  // Manual override (kipas only)
    CONTROL_FAILSAFE = 2   // Emergency fail-safe mode
};

static const char* CONTROL_SOURCE_NAMES[] = {
    "AUTO",
    "MANUAL",
    "FAILSAFE"
};

// ══════════════════════════════════════════════════════════════
// SECTION 7: SLAVE STATE MACHINE
// Referensi: [STP-5.1] Slave State Machine
// ══════════════════════════════════════════════════════════════

enum SlaveState : uint8_t {
    STATE_INIT,          // Initializing hardware
    STATE_DISCOVERY,     // ID=0, waiting for enrollment
    STATE_OPERATIONAL,   // ID assigned, normal operation
    STATE_AUTO,          // Operational + AUTO mode
    STATE_MANUAL,        // Operational + MANUAL mode (kipas only)
    STATE_FAILSAFE       // No heartbeat >300s
};

// ══════════════════════════════════════════════════════════════
// SECTION 8: FAN SPEED DEFINITIONS
// Referensi: [KT-3.2.2] Kipas (DPDT Changeover)
// ══════════════════════════════════════════════════════════════

enum FanSpeed : uint8_t {
    FAN_OFF  = 0,
    FAN_LOW  = 1,
    FAN_MED  = 2,
    FAN_HIGH = 3
};

static const char* FAN_SPEED_NAMES[] = {
    "OFF",
    "LOW",
    "MED",
    "HIGH"
};

// ══════════════════════════════════════════════════════════════
// SECTION 9: DATA STRUCTURES
// ══════════════════════════════════════════════════════════════

/**
 * Complete RS-485 Frame Structure
 * Referensi: [STP-3.1] Frame Structure
 */
struct MosqueFrame {
    uint8_t  start_delim;      // Always 0x7E
    uint8_t  address;          // Target or source ID
    uint8_t  command;          // Command opcode
    uint8_t  length;           // Payload length (0-255)
    uint8_t  payload[MAX_PAYLOAD_SIZE];  // Variable payload
    uint16_t crc;              // CRC16-MODBUS (little-endian)
    uint8_t  end_delim;        // Always 0x0A
    
    // Metadata (not transmitted)
    bool     valid;            // Frame validation flag
    uint32_t timestamp;        // Reception timestamp (millis)
};

/**
 * Slave Configuration (stored in NVS)
 * Referensi: [KT-4.2] Slave Configuration
 */
struct SlaveConfig {
    uint8_t device_id;         // 0=unassigned, 1-247=assigned
    char    device_name[32];   // User-assigned name
    uint8_t profile;           // DeviceProfile enum
    uint8_t mac[6];            // MAC address
    uint16_t crc;              // Config checksum
} __attribute__((packed));

/**
 * Device Record (Master database)
 * Referensi: [KT-4.1] Device Registry
 */
struct DeviceRecord {
    uint8_t  id;               // 1-247
    uint8_t  mac[6];           // MAC address
    char     name[32];         // User-friendly name
    uint8_t  profile;          // DeviceProfile enum
    uint32_t last_seen;        // Epoch seconds of last heartbeat/report
    bool     online;           // Derived from (now() - last_seen < 310)
    uint8_t  relay_state;      // Bitmask 8 relay (bit 0-7)
    uint8_t  control_source;   // ControlSource enum
} __attribute__((packed));

/**
 * SET_RELAY Command Payload (Binary format)
 * Referensi: [STP-4.2.3] SET_RELAY
 */
struct SetRelayPayload {
    uint8_t relay_mask;        // Bitmask which relays to change
    uint8_t values[8];         // Values for relay 1-8 (0x00=OFF, 0x01=ON)
} __attribute__((packed));

/**
 * Sensor Data Payload (CMD_SENSOR_DATA)
 * Dikirim oleh Logic Master setelah membaca SHT20 XY-MD02 via Modbus RTU
 * Referensi: SHT20 XY-MD02 register map (holding register 0x0001 & 0x0002)
 */
struct SensorData {
    int16_t  temperature_x10;  // Suhu dalam 0.1°C (contoh: 275 = 27.5°C)
    uint16_t humidity_x10;     // Kelembaban dalam 0.1% RH (contoh: 650 = 65.0%)
    uint8_t  sensor_id;        // Modbus address sensor (default: 0x01)
    uint8_t  status;           // 0=OK, 1=CRC Error, 2=Timeout, 3=Out of Range
    uint32_t timestamp;        // millis() saat pembacaan
} __attribute__((packed));

// ══════════════════════════════════════════════════════════════
// SECTION 10: TIMING CONSTANTS
// Referensi: [STP-2.2.1] Timing Requirements
// ══════════════════════════════════════════════════════════════

#define HEARTBEAT_INTERVAL      60000     // 60 seconds
#define HEARTBEAT_TOLERANCE     5000      // ±5 seconds
#define FAILSAFE_TIMEOUT        300000    // 300 seconds (5 minutes)
#define DISCOVERY_MIN_INTERVAL  1000      // 1 second
#define DISCOVERY_MAX_INTERVAL  3000      // 3 seconds
#define COMMAND_TIMEOUT         3000      // 3 seconds
#define DE_RE_SETUP_TIME        1         // 1 ms
#define INTER_FRAME_GAP         1         // 1 ms (simplified from 303μs)
#define FAN_DEAD_TIME           200       // 200 ms for relay switching

// ══════════════════════════════════════════════════════════════
// SECTION 11: UTILITY CLASS
// ══════════════════════════════════════════════════════════════

class SmartMosqueUtils {
public:
    /**
     * Calculate CRC16-MODBUS
     * Referensi: [STP-6.2] Reference Implementation
     * 
     * @param data Pointer to data buffer
     * @param length Number of bytes to process
     * @return 16-bit CRC value (little-endian)
     */
    static uint16_t calculateCRC16(const uint8_t *data, uint16_t length) {
        uint16_t crc = 0xFFFF;  // Initial value
        
        for (uint16_t i = 0; i < length; i++) {
            crc ^= (uint16_t)data[i];  // XOR byte into CRC
            
            for (uint8_t j = 0; j < 8; j++) {
                if (crc & 0x0001) {
                    crc >>= 1;
                    crc ^= 0xA001;  // Polynomial (0x8005 reversed)
                } else {
                    crc >>= 1;
                }
            }
        }
        
        return crc;  // No final XOR (XorOut = 0x0000)
    }
    
    /**
     * Convert profile enum to string
     */
    static const char* profileToString(uint8_t profile) {
        if (profile > PROFILE_SENSOR_TEMP) {
            return PROFILE_NAMES[0];  // UNDEFINED
        }
        return PROFILE_NAMES[profile];
    }
    
    /**
     * Convert control source to string
     */
    static const char* controlSourceToString(uint8_t source) {
        if (source > CONTROL_FAILSAFE) {
            return "UNKNOWN";
        }
        return CONTROL_SOURCE_NAMES[source];
    }
    
    /**
     * Convert fan speed to string
     */
    static const char* fanSpeedToString(uint8_t speed) {
        if (speed > FAN_HIGH) {
            return "UNKNOWN";
        }
        return FAN_SPEED_NAMES[speed];
    }
    
    /**
     * Get MAC address as string
     * 
     * @param mac 6-byte MAC address array
     * @param output Output string buffer (min 18 chars)
     */
    static void macToString(const uint8_t mac[6], char* output) {
        sprintf(output, "%02X:%02X:%02X:%02X:%02X:%02X",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
    
    /**
     * Parse MAC string to bytes
     * 
     * @param str MAC string (format: "XX:XX:XX:XX:XX:XX")
     * @param mac Output 6-byte array
     * @return true if successful
     */
    static bool stringToMac(const char* str, uint8_t mac[6]) {
        int values[6];
        int count = sscanf(str, "%x:%x:%x:%x:%x:%x",
                          &values[0], &values[1], &values[2],
                          &values[3], &values[4], &values[5]);
        
        if (count == 6) {
            for (int i = 0; i < 6; i++) {
                mac[i] = (uint8_t)values[i];
            }
            return true;
        }
        return false;
    }
    
    /**
     * Calculate config CRC for validation
     * 
     * @param config Pointer to SlaveConfig
     * @return CRC16 checksum
     */
    static uint16_t calculateConfigCRC(const SlaveConfig* config) {
        // Calculate CRC over all fields except the CRC field itself
        return calculateCRC16((const uint8_t*)config, 
                             sizeof(SlaveConfig) - sizeof(uint16_t));
    }
    
    /**
     * Validate slave config
     * 
     * @param config Pointer to SlaveConfig
     * @return true if valid
     */
    static bool validateConfig(const SlaveConfig* config) {
        // Check ID range
        if (config->device_id > ADDR_MAX_SLAVE) {
            return false;
        }
        
        // Check profile
        if (config->profile > PROFILE_SENSOR_TEMP) {
            return false;
        }
        
        // Validate CRC
        uint16_t calculated = calculateConfigCRC(config);
        if (calculated != config->crc) {
            return false;
        }
        
        return true;
    }
};

// ══════════════════════════════════════════════════════════════
// SECTION 12: DEBUG HELPERS
// ══════════════════════════════════════════════════════════════

#ifdef DEBUG_PROTOCOL
    #define PROTOCOL_LOG(fmt, ...) Serial.printf("[PROTO] " fmt "\n", ##__VA_ARGS__)
    #define PROTOCOL_HEX(data, len) printHex(data, len)
    
    static void printHex(const uint8_t* data, uint16_t len) {
        Serial.print("[HEX] ");
        for (uint16_t i = 0; i < len; i++) {
            Serial.printf("%02X ", data[i]);
            if ((i + 1) % 16 == 0) Serial.print("\n      ");
        }
        Serial.println();
    }
#else
    #define PROTOCOL_LOG(fmt, ...)
    #define PROTOCOL_HEX(data, len)
#endif

// ══════════════════════════════════════════════════════════════
// SECTION 13: VERSION INFO
// ══════════════════════════════════════════════════════════════

#define PROTOCOL_VERSION_MAJOR  2
#define PROTOCOL_VERSION_MINOR  1
#define PROTOCOL_VERSION_PATCH  0
#define PROTOCOL_VERSION_STRING "2.1.0"

#endif // SMART_MOSQUE_PROTOCOL_H
