#ifndef LOGIC_MASTER_H
#define LOGIC_MASTER_H

#include <Arduino.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <SmartMosqueProtocol.h>

/**
 * ═══════════════════════════════════════════════════════════════
 *  LOGIC MASTER - COMMON DEFINITIONS
 *
 *  Referensi: [KT-2.2] Logic Master Specifications
 *
 *  Perubahan v1.2:
 *  - Pisahkan bus RS485:
 *      Serial2 (GPIO15/16) @ 115200 = Smart Mosque Protocol (kipas)
 *      Serial1 (GPIO1/2)   @ 9600   = Modbus RTU (X8 relay + SHT20)
 *  - Node lampu & sound sekarang pakai ESP32 X8 Relay Modbus
 *    (303e32dc812) dengan protokol FC15/FC02 custom (bukan FC05)
 *  - Tambah Serial1 untuk bus Modbus terpisah
 * ═══════════════════════════════════════════════════════════════
 */

// ══════════════════════════════════════════════════════════════
// FIRMWARE VERSION
// ══════════════════════════════════════════════════════════════

#define FIRMWARE_VERSION "1.2.0"
#define BUILD_DATE __DATE__
#define BUILD_TIME __TIME__
#define DEVICE_ID ADDR_LOGIC_MASTER  // Fixed: 254 (0xFE)

// ══════════════════════════════════════════════════════════════
// HARDWARE PINOUT - LOGIC MASTER
// Board: ESP32-S3 N16R8 IoT Logger RS485 Plus
// ══════════════════════════════════════════════════════════════

// ── BUS A: Smart Mosque Protocol (Serial2) ───────────────────
// Slave kipas HwThinker @ 115200 bps
// Sesuai label board: TX2=GPIO15, RX2=GPIO16
#define PIN_RS485_TX        15   // Serial2 TX → Bus A (kipas)
#define PIN_RS485_RX        16   // Serial2 RX ← Bus A (kipas)
#define PIN_RS485_DE_RE     -1   // Auto onboard chip

// ── BUS B: Modbus RTU (Serial1) ──────────────────────────────
// Node lampu (ESP32 X8 Relay), node sound (ESP32 X8 Relay),
// dan sensor SHT20 XY-MD02 @ 9600 bps
// Menggunakan GPIO bebas di board
#define PIN_MODBUS_TX       1    // Serial1 TX → Bus B (Modbus RTU)
#define PIN_MODBUS_RX       2    // Serial1 RX ← Bus B (Modbus RTU)
#define PIN_MODBUS_DE_RE    5    // DE/RE MAX485 Bus B (GPIO5 tersedia)
// Catatan: Jika board Anda sudah punya RS485 onboard untuk Serial1,
//          set PIN_MODBUS_DE_RE = -1 dan hapus pinMode di initModbus()

// ── MicroSD Card (SPI) ───────────────────────────────────────
#define PIN_SD_SS           10
#define PIN_SD_MOSI         11
#define PIN_SD_SCK          12
#define PIN_SD_MISO         13

// ── RTC DS3231M (I2C) ────────────────────────────────────────
#define PIN_RTC_SDA         8
#define PIN_RTC_SCL         9
#define RTC_I2C_ADDRESS     0x68

// ── Status LED ───────────────────────────────────────────────
#define PIN_LED_STATUS      15

// ══════════════════════════════════════════════════════════════
// MODBUS RTU — BUS B
// ══════════════════════════════════════════════════════════════

// Baud rate Bus B (Modbus RTU) — harus sama dengan semua device di bus ini
// ESP32 X8 Relay default 9600, SHT20 XY-MD02 default 9600
#define MODBUS_BAUD_RATE    9600

// Modbus address
#define MODBUS_ADDR_SENSOR_DEFAULT       0x01  // SHT20 XY-MD02
#define MODBUS_ADDR_RELAY_LIGHT_DEFAULT  0x02  // ESP32 X8 Relay — Node Lampu
#define MODBUS_ADDR_RELAY_SOUND_DEFAULT  0x03  // ESP32 X8 Relay — Node Sound

// Register SHT20 XY-MD02 (FC03 Holding Registers)
#define MODBUS_REG_HUMIDITY         0x0001  // Kelembaban × 10
#define MODBUS_REG_TEMPERATURE      0x0002  // Suhu × 10

// Coil base relay 8ch (untuk FC05 / legacy, tidak dipakai untuk X8)
#define MODBUS_COIL_RELAY_BASE      0x0000

// ── Protokol ESP32 X8 Relay (303e32dc812) ────────────────────
// Board ini menggunakan protokol custom (bukan Modbus standar FC05):
//   FC15 (0x0F) Write Multiple Coils → kontrol relay (1 byte bitmask)
//   FC02 (0x02) Read Discrete Inputs → baca status relay / input
//
// Format frame FC15 dari Master ke X8:
//   [addr][0x0F][0x00][0x00][0x00][0x08][0x01][MASK][CRC_L][CRC_H]
//   MASK = bitmask 8 relay: bit0=relay1 ... bit7=relay8
//
// Format frame FC02 dari Master ke X8:
//   [addr][0x02][0x00][memaddr][0x00][0x01][CRC_L][CRC_H]
//   memaddr 0x00 = baca val_in (input optocoupler)
//   memaddr 0x01 = baca val_out (status relay saat ini)

// Timing Modbus
#define MODBUS_TIMEOUT_MS           500     // Timeout respons
#define MODBUS_INTER_FRAME_GAP_MS   4       // Jeda antar frame @ 9600 bps
#define MODBUS_SENSOR_INTERVAL_MS   10000   // Baca sensor tiap 10 detik

// ══════════════════════════════════════════════════════════════
// SMART MOSQUE BUS A — BAUD RATE
// ══════════════════════════════════════════════════════════════

#define SMART_MOSQUE_BAUD_RATE  115200  // Bus A: protokol custom

// ══════════════════════════════════════════════════════════════
// SYSTEM CONSTANTS
// ══════════════════════════════════════════════════════════════

#define MAX_DEVICES         247
#define ACTIVE_DEVICES      18
#define HEARTBEAT_INTERVAL  60000   // 60 detik
#define HEARTBEAT_TOLERANCE 5000
#define NVS_FLUSH_INTERVAL  300000  // 5 menit
#define DISCOVERY_TIMEOUT   300000
#define DEVICE_OFFLINE_THRESHOLD 310000

// ══════════════════════════════════════════════════════════════
// DEVICE REGISTRY
// ══════════════════════════════════════════════════════════════

struct MasterDeviceRecord {
    uint8_t  id;
    uint8_t  mac[6];
    char     name[32];
    uint8_t  profile;
    uint32_t last_seen;
    bool     online;
    uint8_t  relay_state;
    uint8_t  control_source;
    uint32_t first_seen;
    uint32_t last_command_sent;
    uint32_t last_status_recv;
    uint8_t  error_count;
    uint8_t  rssi;
    bool     dirty;
} __attribute__((packed));

struct PendingDevice {
    uint8_t  mac[6];
    char     profile[16];
    char     version[16];
    uint32_t timestamp;
    uint8_t  announce_count;
};

// ══════════════════════════════════════════════════════════════
// STATISTICS
// ══════════════════════════════════════════════════════════════

struct SystemStats {
    uint32_t uptime_seconds;
    uint32_t heartbeats_sent;
    uint32_t frames_received;
    uint32_t frames_sent;
    uint32_t commands_routed;
    uint32_t status_reports_recv;
    uint32_t crc_errors;
    uint32_t enrollments_completed;
    uint32_t nvs_writes;
    uint8_t  devices_online;
    uint8_t  devices_offline;
    uint32_t modbus_reads_ok;
    uint32_t modbus_reads_err;
    uint32_t relay_commands_sent;
};

// ══════════════════════════════════════════════════════════════
// SENSOR DATA
// ══════════════════════════════════════════════════════════════

struct SensorReading {
    float    temperature;
    float    humidity;
    uint8_t  modbus_addr;
    uint32_t last_read_ms;
    bool     valid;
    uint8_t  status;
    uint8_t  error_count;
};

// ══════════════════════════════════════════════════════════════
// GLOBAL STATE
// ══════════════════════════════════════════════════════════════

extern MasterDeviceRecord deviceRegistry[MAX_DEVICES];
extern uint8_t deviceCount;
extern PendingDevice pendingDevices[10];
extern uint8_t pendingCount;
extern SystemStats stats;
extern SensorReading sensorData;
extern uint32_t lastHeartbeat;
extern uint32_t lastNVSFlush;
extern uint32_t lastSensorRead;
extern uint32_t bootTime;
extern MosqueFrame currentFrame;
extern uint8_t rxBuffer[MAX_FRAME_SIZE];
extern uint16_t rxIndex;
extern ParserState parserState;
extern Preferences preferences;

// ══════════════════════════════════════════════════════════════
// FUNCTION DECLARATIONS
// ══════════════════════════════════════════════════════════════

// === device_registry.cpp ===
void     initDeviceRegistry();
bool     loadRegistryFromNVS();
bool     saveRegistryToNVS();
void     flushDirtyDevices();
MasterDeviceRecord* findDeviceByID(uint8_t id);
MasterDeviceRecord* findDeviceByMAC(const uint8_t mac[6]);
MasterDeviceRecord* addDevice(uint8_t id, const uint8_t mac[6],
                              const char* name, uint8_t profile);
bool     removeDevice(uint8_t id);
void     updateDeviceStatus(uint8_t id, const DeviceRecord& status);
void     updateDeviceOnlineStatus();
uint8_t  getNextAvailableID();
void     printDeviceRegistry();
DeviceRecord* getDeviceByID(uint8_t id);

// === heartbeat.cpp ===
void     initHeartbeat();
void     sendHeartbeatIfDue();
void     sendHeartbeat();

// === discovery.cpp ===
void     initDiscovery();
void     handleDiscoveryAnnounce(const uint8_t* payload, uint8_t length);
void     processPendingDiscovery();
void     sendDiscoveryResponse(uint8_t deviceID, const char* mac,
                               const char* name, const char* profile);
void     cleanupOldPending();

// === command_router.cpp ===
void     routeCommand(uint8_t targetID, uint8_t command,
                     const uint8_t* payload, uint8_t length);
void     broadcastCommand(uint8_t command, const uint8_t* payload, uint8_t length);
bool     waitForACK(uint8_t deviceID, uint32_t timeout);
void     handleCommandResponse(uint8_t fromID, const MosqueFrame* frame);
// Kontrol via Smart Mosque Protocol Bus A (slave kipas ESP32)
bool     sendSetRelay(uint8_t slaveID, uint8_t relayMask, const uint8_t* values);
bool     setRelayChannel(uint8_t slaveID, uint8_t channel, bool state);
bool     setAllRelays(uint8_t slaveID, uint8_t bitmask);
bool     setFanSpeed(uint8_t slaveID, FanSpeed speed);
bool     broadcastAllOff();
bool     broadcastAllOn();
// Kontrol via Modbus RTU Bus B (ESP32 X8 Relay — lampu & sound)
bool     setX8Relay(uint8_t modbusAddr, uint8_t mask);
bool     setX8RelayOne(uint8_t modbusAddr, uint8_t relayIndex, bool state);
bool     readX8RelayStatus(uint8_t modbusAddr, uint8_t* outMask);
// Legacy wrapper (tetap tersedia untuk kompatibilitas)
bool     setModbusRelay8ch(uint8_t modbusAddr, uint8_t relayIndex, bool state);
bool     setAllModbusRelays8ch(uint8_t modbusAddr, uint8_t mask);

// === status_aggregator.cpp ===
void     handleStatusReport(uint8_t deviceID, const uint8_t* payload, uint8_t length);
void     aggregateSystemStatus();
void     syncToDisplayMaster();

// === frame_processing.cpp ===
void     processIncomingByte(uint8_t byte);
void     processCompleteFrame();
void     sendFrame(uint8_t address, uint8_t command,
                  const uint8_t* payload, uint8_t length);
void     sendACK(uint8_t targetID);
void     sendNACK(uint8_t targetID, ErrorCode errorCode, const char* message = nullptr);

// === modbus_handler.cpp ===
void     initModbus();
bool     readSensorSHT20(uint8_t modbusAddr, SensorReading* out);
// Fungsi utama untuk ESP32 X8 Relay (FC15/FC02)
bool     modbusX8WriteRelays(uint8_t modbusAddr, uint8_t mask);
bool     modbusX8ReadStatus(uint8_t modbusAddr, uint8_t memaddr, uint8_t* outVal);
// Legacy relay 8ch (FC05/FC0F) — untuk kompatibilitas modul lain
bool     setRelay8ch(uint8_t modbusAddr, uint8_t relayIndex, bool state);
bool     setAllRelays8ch(uint8_t modbusAddr, uint8_t mask);
void     pollSensorIfDue();
void     broadcastSensorData();

// === rtc_handler.cpp ===
void     initRTC();
uint32_t getRTCEpoch();
bool     setRTCTime(uint32_t epoch);
void     printRTCTime();

// === sd_logger.cpp ===
void     initSD();
void     logSensorData(const SensorReading* data);
void     logEvent(const char* event);
bool     isSDReady();

// === Utilities ===
uint32_t getEpochTime();
void     feedWatchdog();
void     printSystemInfo();
void     printStats();

#endif // LOGIC_MASTER_H
