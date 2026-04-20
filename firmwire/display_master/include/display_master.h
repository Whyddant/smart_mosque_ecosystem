/**
 * ============================================================================
 * DISPLAY MASTER - HEADER FILE v1.1
 * Smart Mosque Ecosystem
 * ============================================================================
 * 
 * v1.1 CHANGES:
 * - Added SmartMosqueFrame struct definition
 * - Added CMD_DEVICE_LIST_SYNC constant
 * - Added COMMAND_QUEUE_SIZE constant
 * - Fixed queue size references
 * ============================================================================
 */

#ifndef DISPLAY_MASTER_H
#define DISPLAY_MASTER_H

#include <Arduino.h>
#include <HardwareSerial.h>
#include <Preferences.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <lvgl.h>

// Include protocol library
#include "../../../shared/SmartMosqueProtocol/SmartMosqueProtocol.h"

// ============================================================================
// KONSTANTA SISTEM [KT-2.2]
// ============================================================================

#define DISPLAY_MASTER_ID        255        // Fixed ID per [KT-2.2]
#define LOGIC_MASTER_ID          254        // Logic Master ID
#define MAX_DEVICES              247        // Maximum slave devices

// RS-485 Pinout Display Master
// Board: HwThinker 4-relay ESP32 (sesuai label ModBUS TX/RX)
// Sesuai tabel pin Image 2: ModBUS TX=GPIO19, ModBUS RX=GPIO18
#define RS485_RX                 18   // ModBUS RX (GPIO18)
#define RS485_TX                 19   // ModBUS TX (GPIO19)
#define RS485_DE_RE              -1   // Tidak digunakan (chip onboard auto DE/RE)

// Timing
#define RS485_BAUD_RATE          115200
#define COMMAND_TIMEOUT_MS       2000
#define UI_UPDATE_INTERVAL_MS    500
#define DEVICE_OFFLINE_TIMEOUT   310000

// WiFi
#define WIFI_AP_SSID            "SmartMosque-Display"
#define WIFI_AP_PASSWORD        "masjid2026"
#define WEB_SERVER_PORT         80

// ✅ NEW: Custom protocol commands
#define CMD_DEVICE_LIST_SYNC    0x10

// ✅ NEW: Queue configuration  
#define COMMAND_QUEUE_SIZE      10

// ============================================================================
// STRUKTUR DATA
// ============================================================================

/**
 * ✅ NEW: RS-485 Frame Structure (for parser)
 */
struct SmartMosqueFrame {
    uint8_t address;
    uint8_t command;
    uint8_t payload_len;
    uint8_t* payload;
    uint8_t crc_low;
    uint8_t crc_high;
};

struct DeviceDisplayInfo {
    uint8_t id;
    char name[32];
    uint8_t profile;
    bool online;
    uint8_t relay_state;
    uint8_t control_source;
    uint32_t last_update;
};

// Data sensor suhu & kelembaban dari broadcast Logic Master (CMD_SENSOR_DATA)
struct SensorDisplayData {
    float    temperature;       // Suhu dalam °C
    float    humidity;          // Kelembaban dalam % RH
    bool     valid;             // true jika sudah menerima data
    uint32_t last_update_ms;    // millis() saat terima data terakhir
};

struct PendingEnrollment {
    uint8_t mac[6];
    uint8_t profile;
    uint32_t first_seen;
    bool active;
};

struct CommandQueueItem {
    uint8_t target_id;
    uint8_t opcode;
    uint8_t payload[32];
    uint8_t payload_len;
    uint32_t timestamp;
    uint8_t retry_count;
    bool waiting_ack;
};

struct DisplayStatistics {
    uint32_t commands_sent;
    uint32_t commands_acked;
    uint32_t commands_failed;
    uint32_t frames_received;
    uint32_t frames_invalid;
    uint32_t ui_updates;
    uint32_t uptime_seconds;
};

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================

extern DeviceDisplayInfo g_devices[MAX_DEVICES];
extern uint8_t g_device_count;
extern SensorDisplayData g_sensor_data;
extern PendingEnrollment g_pending_enrollment;
extern CommandQueueItem g_command_queue[COMMAND_QUEUE_SIZE];  // ✅ FIXED
extern uint8_t g_queue_head, g_queue_tail;
extern DisplayStatistics g_stats;
extern Preferences g_preferences;
extern AsyncWebServer g_webServer;

extern lv_obj_t* g_main_screen;
extern lv_obj_t* g_control_screen;
extern lv_obj_t* g_enrollment_popup;

// ✅ NEW: Logic Master health tracking
extern uint32_t g_last_logic_heartbeat;
extern bool g_logic_master_online;

// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================

// RS-485 Communication
void rs485Init();
bool rs485SendFrame(uint8_t target_addr, uint8_t opcode, 
                    const uint8_t* payload, uint8_t payload_len);
void rs485ProcessIncoming();
void rs485HandleFrame(const SmartMosqueFrame* frame);

// Device Manager
void deviceManagerInit();
void deviceManagerUpdate(uint8_t id, uint8_t relay_state, uint8_t control_source);
void deviceManagerCheckOnlineStatus();
DeviceDisplayInfo* deviceManagerGet(uint8_t id);
uint8_t deviceManagerGetCount();
void deviceManagerSyncFromLogicMaster();
void deviceManagerSaveCache();

// UI Modules
void uiDashboardCreate();
void uiDashboardUpdate();
void uiDashboardShowAlert(const char* message, uint8_t level);
void uiDeviceControlCreate();
void uiDeviceControlShow(uint8_t device_id);
void uiDeviceControlUpdate();
void uiDeviceControlHandleRelayButton(uint8_t device_id, uint8_t relay_index, bool state);
void uiEnrollmentCreate();
void uiEnrollmentShow(const uint8_t* mac, uint8_t profile);
void uiEnrollmentHide();
void uiEnrollmentHandleApproval(const char* device_name);
void uiEnrollmentHandleRejection();

// WiFi & Web Server
void wifiInit();
void webServerInit();
void webApiGetDevices(AsyncWebServerRequest* request);
void webApiControlRelay(AsyncWebServerRequest* request, uint8_t* data, 
                        size_t len, size_t index, size_t total);
void webApiGetStats(AsyncWebServerRequest* request);
void webSocketBroadcast(const char* json_data);

// Command Queue
inline bool commandQueueAdd(uint8_t target_id, uint8_t opcode, 
                            const uint8_t* payload, uint8_t payload_len) {
    uint8_t next_tail = (g_queue_tail + 1) % COMMAND_QUEUE_SIZE;  // ✅ FIXED
    if (next_tail == g_queue_head) {
        Serial.println("⚠ Command queue full!");
        uiDashboardShowAlert("Queue Full, Try Again", 1);  // ✅ IMPROVED
        return false;
    }
    
    g_command_queue[g_queue_tail].target_id = target_id;
    g_command_queue[g_queue_tail].opcode = opcode;
    g_command_queue[g_queue_tail].payload_len = payload_len;
    if (payload_len > 0) {
        memcpy(g_command_queue[g_queue_tail].payload, payload, payload_len);
    }
    g_command_queue[g_queue_tail].timestamp = millis();
    g_command_queue[g_queue_tail].retry_count = 0;
    g_command_queue[g_queue_tail].waiting_ack = true;
    
    g_queue_tail = next_tail;
    return true;
}

inline CommandQueueItem* commandQueuePeek() {
    if (g_queue_head == g_queue_tail) {
        return nullptr;
    }
    return &g_command_queue[g_queue_head];
}

inline void commandQueuePop() {
    if (g_queue_head != g_queue_tail) {
        g_queue_head = (g_queue_head + 1) % COMMAND_QUEUE_SIZE;  // ✅ FIXED
    }
}

void commandQueueProcess();

// Utilities
inline const char* getProfileName(uint8_t profile) {
    switch (profile) {
        case PROFILE_FAN_4CH:   return "KIPAS";
        case PROFILE_LIGHT_8CH: return "LAMPU";
        case PROFILE_SOUND_8CH: return "SOUND";
        case PROFILE_SENSOR_TEMP: return "SENSOR";
        default:                return "UNKNOWN";
    }
}

inline const char* getControlSourceName(uint8_t source) {
    switch (source) {
        case 0: return "AUTO";
        case 1: return "MANUAL";
        case 2: return "FAILSAFE";
        default: return "UNKNOWN";
    }
}

inline void formatMAC(const uint8_t* mac, char* output) {
    sprintf(output, "%02X:%02X:%02X:%02X:%02X:%02X",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

inline void getUptimeString(char* output, size_t max_len) {
    uint32_t seconds = millis() / 1000;
    uint32_t days = seconds / 86400;
    seconds %= 86400;
    uint32_t hours = seconds / 3600;
    seconds %= 3600;
    uint32_t minutes = seconds / 60;
    seconds %= 60;
    
    snprintf(output, max_len, "%ud %02u:%02u:%02u", days, hours, minutes, seconds);
}

#endif // DISPLAY_MASTER_H