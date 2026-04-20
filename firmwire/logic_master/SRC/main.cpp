/**
 * ═══════════════════════════════════════════════════════════════
 *  LOGIC MASTER MAIN.CPP — PATCH v1.2
 *
 *  INSTRUKSI INTEGRASI:
 *  Terapkan perubahan berikut ke firmware/logic_master/src/main.cpp
 *
 *  Perubahan yang diperlukan:
 *  1. Update komentar header → v1.2
 *  2. Ubah inisialisasi Serial2 (tambah komentar Bus A)
 *  3. initModbus() sekarang juga init Serial1 (Bus B) — tidak ada
 *     perubahan kode di main.cpp, sudah dihandle di modbus_handler.cpp
 *  4. Update printSystemInfo() agar tampilkan Bus A dan Bus B
 *
 *  DIFF — Tempel perubahan ini ke main.cpp:
 * ═══════════════════════════════════════════════════════════════
 */

// ── PERUBAHAN 1: Header komentar (ganti baris * SMART MOSQUE - LOGIC MASTER FIRMWARE v1.1)
/*
 *  SMART MOSQUE - LOGIC MASTER FIRMWARE v1.2
 *
 *  Perubahan v1.2:
 *  - Pisahkan bus RS485:
 *      Bus A: Serial2 (GPIO15/16) @ 115200 = Smart Mosque Protocol (kipas)
 *      Bus B: Serial1 (GPIO1/2)   @ 9600   = Modbus RTU (X8 relay + SHT20)
 *  - Node lampu & sound sekarang ESP32 X8 Relay (303e32dc812)
 *    dikontrol via Bus B dengan protokol FC15/FC02
 */

// ── PERUBAHAN 2: Bagian setup() — ganti blok "Initialize RS-485" ──
/*
    // Initialize RS-485 Bus A — Smart Mosque Protocol (Slave Kipas)
    // Serial2: TX=GPIO15, RX=GPIO16 @ 115200 bps, auto DE/RE onboard
    Serial.println("→ Initializing RS-485 Bus A (Smart Mosque Protocol)...");
    Serial2.begin(SMART_MOSQUE_BAUD_RATE, SERIAL_8N1, PIN_RS485_RX, PIN_RS485_TX);
    Serial.println("✓ Bus A ready: Serial2 TX=GPIO15 RX=GPIO16 @ 115200 bps");

    // Bus B (Modbus RTU) diinisialisasi di dalam initModbus() → Serial1
    // TX=GPIO1, RX=GPIO2, DE/RE=GPIO5 @ 9600 bps
*/

// ── PERUBAHAN 3: printSystemInfo() — ganti blok RS485 ──
/*
    Serial.printf( "║ Board:          ESP32-S3 N16R8 IoT Logger    ║\n");
    Serial.printf( "║ Bus A Serial2:  TX=GPIO15 RX=GPIO16 @115200  ║\n");
    Serial.printf( "║   → Smart Mosque Protocol (slave kipas)      ║\n");
    Serial.printf( "║ Bus B Serial1:  TX=GPIO1  RX=GPIO2  @9600    ║\n");
    Serial.printf( "║   DE/RE=GPIO5 → Modbus RTU (relay + sensor)  ║\n");
    Serial.printf( "║ RTC DS3231M:    SDA=GPIO8,  SCL=GPIO9        ║\n");
    Serial.printf( "║ MicroSD SPI:    SS=10 MOSI=11 SCK=12 MISO=13║\n");
    Serial.printf( "║ Modbus Sensor:  addr=0x%02X (SHT20 @ Bus B)   ║\n", MODBUS_ADDR_SENSOR_DEFAULT);
    Serial.printf( "║ X8 Lampu:       addr=0x%02X (ESP32 @ Bus B)   ║\n", MODBUS_ADDR_RELAY_LIGHT_DEFAULT);
    Serial.printf( "║ X8 Sound:       addr=0x%02X (ESP32 @ Bus B)   ║\n", MODBUS_ADDR_RELAY_SOUND_DEFAULT);
*/

// ══════════════════════════════════════════════════════════════
// VERSI LENGKAP main.cpp v1.2 (siap pakai, ganti seluruh file)
// ══════════════════════════════════════════════════════════════

/*
 * Salin seluruh isi di bawah ini ke main.cpp
 * (menggantikan versi v1.1)
 */

#include "logic_master.h"

// ── Global Variables ──────────────────────────────────────────
MasterDeviceRecord deviceRegistry[MAX_DEVICES] = {0};
uint8_t deviceCount = 0;
PendingDevice pendingDevices[10] = {0};
uint8_t pendingCount = 0;
SystemStats stats = {0};
SensorReading sensorData = {0.0f, 0.0f, MODBUS_ADDR_SENSOR_DEFAULT, 0, false, 0, 0};
uint32_t lastHeartbeat   = 0;
uint32_t lastNVSFlush    = 0;
uint32_t lastSensorRead  = 0;
uint32_t bootTime        = 0;
MosqueFrame currentFrame;
uint8_t rxBuffer[MAX_FRAME_SIZE];
uint16_t rxIndex = 0;
ParserState parserState = WAIT_START;
Preferences preferences;

// ── Setup ─────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n\n");
    Serial.println("═══════════════════════════════════════════");
    Serial.println("  SMART MOSQUE LOGIC MASTER v1.2");
    Serial.println("  Board: ESP32-S3 N16R8 IoT Logger RS485+");
    Serial.println("  Device ID: 254 (0xFE)");
    Serial.println("═══════════════════════════════════════════");

    bootTime = millis();

    // ── Bus A: Smart Mosque Protocol (slave kipas) ──────────
    // Serial2: TX2=GPIO15, RX2=GPIO16 @ 115200 bps
    // DE/RE otomatis oleh chip RS485 onboard
    Serial.println("→ Initializing RS-485 Bus A (Smart Mosque Protocol)...");
    Serial2.begin(SMART_MOSQUE_BAUD_RATE, SERIAL_8N1, PIN_RS485_RX, PIN_RS485_TX);
    Serial.println("✓ Bus A: Serial2 TX=GPIO15 RX=GPIO16 @ 115200 bps (auto DE/RE)");

    // ── RTC DS3231M ─────────────────────────────────────────
    Serial.println("→ Initializing RTC DS3231M (SDA=GPIO8, SCL=GPIO9)...");
    initRTC();

    // ── MicroSD ─────────────────────────────────────────────
    Serial.println("→ Initializing MicroSD (SS=GPIO10, MOSI=GPIO11)...");
    initSD();

    // ── Bus B: Modbus RTU (Serial1) ─────────────────────────
    // Serial1: TX=GPIO1, RX=GPIO2, DE/RE=GPIO5 @ 9600 bps
    // Node lampu (ESP32 X8 addr=0x02), sound (ESP32 X8 addr=0x03),
    // sensor SHT20 (addr=0x01)
    Serial.println("→ Initializing Modbus RTU Bus B (X8 Relay + SHT20)...");
    initModbus();  // inisialisasi Serial1 ada di sini

    // ── Device Registry ──────────────────────────────────────
    Serial.println("→ Initializing device registry...");
    initDeviceRegistry();

    if (loadRegistryFromNVS()) {
        Serial.printf("✓ Registry loaded: %d devices\n", deviceCount);
        printDeviceRegistry();
    } else {
        Serial.println("⚠ No existing registry, starting fresh");
    }

    // ── Subsystems ───────────────────────────────────────────
    Serial.println("→ Initializing subsystems...");
    initHeartbeat();
    initDiscovery();
    Serial.println("✓ Subsystems initialized");

    lastHeartbeat  = millis();
    lastNVSFlush   = millis();
    lastSensorRead = millis();

    printSystemInfo();
    Serial.println("\n✓ Setup complete, entering main loop\n");
}

// ── Main Loop ─────────────────────────────────────────────────
void loop() {
    // Task 1: Proses frame Bus A (Smart Mosque Protocol) — prioritas tertinggi
    while (Serial2.available()) {
        uint8_t byte = Serial2.read();
        processIncomingByte(byte);
    }

    // Task 2: Heartbeat ke slave kipas (Bus A)
    sendHeartbeatIfDue();

    // Task 3: Baca sensor SHT20 via Modbus RTU Bus B (tiap 10 detik)
    pollSensorIfDue();

    // Task 4: Proses enrollment pending
    if (pendingCount > 0) {
        processPendingDiscovery();
    }

    // Task 5: Update status online/offline
    static uint32_t lastOnlineCheck = 0;
    if (millis() - lastOnlineCheck > 10000) {
        updateDeviceOnlineStatus();
        lastOnlineCheck = millis();
    }

    // Task 6: Flush device registry ke NVS (tiap 5 menit)
    if (millis() - lastNVSFlush > NVS_FLUSH_INTERVAL) {
        Serial.println("→ Flushing device registry to NVS...");
        flushDirtyDevices();
        lastNVSFlush = millis();
    }

    // Task 7: Sync ke Display Master (tiap 5 detik)
    static uint32_t lastSync = 0;
    if (millis() - lastSync > 5000) {
        syncToDisplayMaster();
        lastSync = millis();
    }

    // Task 8: Log sensor ke MicroSD (tiap 60 detik)
    static uint32_t lastSDLog = 0;
    if (millis() - lastSDLog > 60000 && sensorData.valid) {
        logSensorData(&sensorData);
        lastSDLog = millis();
    }

    // Task 9: Statistics (tiap 1 menit)
    static uint32_t lastStatsUpdate = 0;
    if (millis() - lastStatsUpdate > 60000) {
        stats.uptime_seconds = (millis() - bootTime) / 1000;
        printStats();
        lastStatsUpdate = millis();
    }

    // Task 10: Watchdog
    feedWatchdog();

    delay(1);
}

// ── Utility ───────────────────────────────────────────────────
uint32_t getEpochTime() {
    return getRTCEpoch();
}

void feedWatchdog() {
    yield();
}

void printSystemInfo() {
    Serial.println("\n╔══════════════════════════════════════════════╗");
    Serial.println("║          SYSTEM INFORMATION v1.2            ║");
    Serial.println("╠══════════════════════════════════════════════╣");
    Serial.printf( "║ Firmware:       v%-26s ║\n", FIRMWARE_VERSION);
    Serial.printf( "║ Device ID:      %-27d ║\n", DEVICE_ID);
    Serial.println("╠══════════════════════════════════════════════╣");
    Serial.printf( "║ Board:          ESP32-S3 N16R8 IoT Logger    ║\n");
    Serial.printf( "║ Bus A Serial2:  TX=GPIO15 RX=GPIO16 @115200  ║\n");
    Serial.printf( "║   Smart Mosque Protocol → slave kipas        ║\n");
    Serial.printf( "║ Bus B Serial1:  TX=GPIO1  RX=GPIO2  @9600    ║\n");
    Serial.printf( "║   Modbus RTU → X8 relay lampu/sound + SHT20 ║\n");
    Serial.printf( "║   DE/RE Bus B: GPIO%d                        ║\n", PIN_MODBUS_DE_RE);
    Serial.println("╠══════════════════════════════════════════════╣");
    Serial.printf( "║ RTC DS3231M:    SDA=GPIO8,  SCL=GPIO9        ║\n");
    Serial.printf( "║ MicroSD SPI:    SS=10 MOSI=11 SCK=12 MISO=13║\n");
    Serial.println("╠══════════════════════════════════════════════╣");
    Serial.printf( "║ Sensor SHT20:  addr=0x%02X  (Bus B FC03)      ║\n", MODBUS_ADDR_SENSOR_DEFAULT);
    Serial.printf( "║ X8 Lampu:      addr=0x%02X  (Bus B FC15/FC02) ║\n", MODBUS_ADDR_RELAY_LIGHT_DEFAULT);
    Serial.printf( "║ X8 Sound:      addr=0x%02X  (Bus B FC15/FC02) ║\n", MODBUS_ADDR_RELAY_SOUND_DEFAULT);
    Serial.println("╠══════════════════════════════════════════════╣");
    Serial.printf( "║ Devices:        %d registered                 ║\n", deviceCount);
    Serial.printf( "║ Free Heap:      %6d KB                   ║\n", ESP.getFreeHeap() / 1024);
    Serial.printf( "║ PSRAM:          %6d KB                   ║\n", ESP.getPsramSize() / 1024);
    Serial.println("╚══════════════════════════════════════════════╝");
}

void printStats() {
    Serial.println("\n╔══════════════════════════════════════════════╗");
    Serial.println("║            RUNTIME STATISTICS                ║");
    Serial.println("╠══════════════════════════════════════════════╣");
    Serial.printf( "║ Uptime:           %8lu s              ║\n", stats.uptime_seconds);
    Serial.printf( "║ Heartbeats:       %8lu                ║\n", stats.heartbeats_sent);
    Serial.printf( "║ Frames RX Bus A:  %8lu                ║\n", stats.frames_received);
    Serial.printf( "║ Frames TX Bus A:  %8lu                ║\n", stats.frames_sent);
    Serial.printf( "║ Commands Routed:  %8lu                ║\n", stats.commands_routed);
    Serial.printf( "║ CRC Errors:       %8lu                ║\n", stats.crc_errors);
    Serial.printf( "║ Enrollments:      %8lu                ║\n", stats.enrollments_completed);
    Serial.println("╠══════════════════════════════════════════════╣");
    Serial.printf( "║ Modbus Reads OK:  %8lu (Bus B)        ║\n", stats.modbus_reads_ok);
    Serial.printf( "║ Modbus Reads ERR: %8lu (Bus B)        ║\n", stats.modbus_reads_err);
    Serial.printf( "║ Relay Cmds Sent:  %8lu (Bus B)        ║\n", stats.relay_commands_sent);
    Serial.println("╠══════════════════════════════════════════════╣");
    Serial.printf( "║ Devices Online:   %8d                ║\n", stats.devices_online);
    Serial.printf( "║ Devices Offline:  %8d                ║\n", stats.devices_offline);
    if (sensorData.valid) {
        Serial.printf("║ Suhu:             %6.1f °C               ║\n", sensorData.temperature);
        Serial.printf("║ Kelembaban:       %6.1f %%RH              ║\n", sensorData.humidity);
    } else {
        Serial.println("║ Sensor:           Belum ada data           ║");
    }
    Serial.println("╚══════════════════════════════════════════════╝\n");
}
