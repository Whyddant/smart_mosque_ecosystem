/**
 * ═══════════════════════════════════════════════════════════════
 *  SMART MOSQUE - SLAVE NODE FIRMWARE v2.1
 *
 *  NODE KIPAS - HwThinker 4-relay ESP32
 *  ─────────────────────────────────────
 *  Board  : HwThinker 4-channel relay ESP32 (ESP32 WROOM-32)
 *  RS485  : TX=GPIO19 (ModBUS TX), RX=GPIO18 (ModBUS RX)
 *  Relay  :
 *    Relay 1 = GPIO23 → Relay SWING (arah kipas)
 *    Relay 2 = GPIO5  → Speed LOW
 *    Relay 3 = GPIO4  → Speed MED
 *    Relay 4 = GPIO13 → Speed HIGH
 *  Input  : GPIO25 (Input1), GPIO26 (Input2),
 *           GPIO27 (Input3), GPIO33 (Input4)
 *  LED    : GPIO15
 *  Power  : 5V via USB atau terminal board
 *
 *  NODE LAMPU & SOUND — TIDAK PAKAI FIRMWARE INI:
 *  ────────────────────────────────────────────────
 *  Modul Modbus RTU RS485 Relay 8Ch dikontrol langsung oleh
 *  Logic Master via perintah Modbus RTU (FC05/FC0F).
 *  Tidak ada ESP32 slave di node tersebut.
 *
 *  Compliance:
 *  - Kebenaran Tunggal v1.4 (KT)
 *  - Spesifikasi Teknis Protokol v1.3 (STP)
 *
 *  Perubahan v2.1:
 *  - Update pinout sesuai HwThinker 4-relay ESP32
 *  - RS485: TX=GPIO19, RX=GPIO18 (sesuai label ModBUS TX/RX)
 *  - Relay: GPIO23(swing), GPIO5(low), GPIO4(med), GPIO13(high)
 *  - Tambah input sensor dari CMD_SENSOR_DATA (broadcast suhu)
 *  - Auto mode berdasarkan suhu dari sensor SHT20
 * ═══════════════════════════════════════════════════════════════
 */

#include <Arduino.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <esp_mac.h>
#include <SmartMosqueProtocol.h>

// ══════════════════════════════════════════════════════════════
// FIRMWARE VERSION
// ══════════════════════════════════════════════════════════════
#define FIRMWARE_VERSION "2.1.0"
#define BUILD_DATE __DATE__
#define BUILD_TIME __TIME__

// ══════════════════════════════════════════════════════════════
// HARDWARE PINOUT - HwThinker 4-relay ESP32
// ══════════════════════════════════════════════════════════════

// RS-485 Communication
// Sesuai label board HwThinker: ModBUS TX=GPIO19, ModBUS RX=GPIO18
#define PIN_RS485_TX        19   // ModBUS TX pada board HwThinker
#define PIN_RS485_RX        18   // ModBUS RX pada board HwThinker
// Board HwThinker tidak memiliki pin DE/RE terpisah (menggunakan chip onboard)
// Jika chip RS485 eksternal yang digunakan, koneksikan DE/RE ke GPIO bebas
#define PIN_RS485_DE_RE     -1   // -1 = tidak digunakan (chip onboard auto)

// Relay Outputs - HwThinker 4-channel
// Sesuai tabel pin board HwThinker:
//   GPIO23 = Relay 1 → SWING (pengatur arah kipas)
//   GPIO5  = Relay 2 → Speed LOW
//   GPIO4  = Relay 3 → Speed MED
//   GPIO13 = Relay 4 → Speed HIGH
const uint8_t RELAY_PINS[4] = {23, 5, 4, 13};
#define PIN_RELAY_SWING     23   // Relay 1: kontrol arah / swing kipas
#define PIN_RELAY_SPEED_LOW 5    // Relay 2: kecepatan rendah
#define PIN_RELAY_SPEED_MED 4    // Relay 3: kecepatan sedang
#define PIN_RELAY_SPEED_HIGH 13  // Relay 4: kecepatan tinggi

// Input Digital (tersedia di board HwThinker)
#define PIN_INPUT_1         25   // Input 1 (GPIO25)
#define PIN_INPUT_2         26   // Input 2 (GPIO26)
#define PIN_INPUT_3         27   // Input 3 (GPIO27 = Mode AUTO/MANUAL switch)
#define PIN_INPUT_4         33   // Input 4 (GPIO33)

// Mode Detection: GPIO27 (Input 3) digunakan sebagai saklar AUTO/MANUAL
// HIGH = AUTO mode (dikontrol remote), LOW = MANUAL mode (saklar fisik)
#define PIN_MODE_DETECT     27

// Status LED
#define PIN_LED_STATUS      15   // GPIO15 = LED onboard HwThinker

// ── KONTROL SUHU OTOMATIS ──────────────────────────────────
// Kipas auto mode: kecepatan berdasarkan suhu dari sensor SHT20
#define TEMP_THRESHOLD_LOW   27.0f   // < 27°C → FAN_OFF
#define TEMP_THRESHOLD_MED   30.0f   // 27-30°C → FAN_LOW
#define TEMP_THRESHOLD_HIGH  33.0f   // 30-33°C → FAN_MED
                                      // > 33°C  → FAN_HIGH
// ──────────────────────────────────────────────────────────────

// ══════════════════════════════════════════════════════════════
// GLOBAL VARIABLES
// ══════════════════════════════════════════════════════════════

// Configuration
Preferences preferences;
SlaveConfig config;
uint8_t myMAC[6];

// State management
SlaveState currentState = STATE_INIT;
ControlSource controlSource = CONTROL_AUTO;
uint8_t relayStates = 0x00;  // Bitmask untuk 4 relay (bit 0-3)

// Data suhu dari broadcast Logic Master (CMD_SENSOR_DATA)
float latestTemperature = 0.0f;
float latestHumidity    = 0.0f;
bool  sensorDataReceived = false;

// Timing
volatile uint32_t lastHeartbeatReceived = 0;
uint32_t lastDiscoveryAnnounce = 0;
uint32_t lastStatusReport      = 0;
uint32_t bootTime              = 0;

// Frame parser
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
struct {
    uint32_t framesReceived;
    uint32_t framesSent;
    uint32_t crcErrors;
    uint32_t commandsExecuted;
    uint32_t commandsRejected;
} stats;

// ══════════════════════════════════════════════════════════════
// FORWARD DECLARATIONS
// ══════════════════════════════════════════════════════════════

// Setup & Loop
void setup();
void loop();

// Configuration Management
bool loadConfigFromNVS();
bool saveConfigToNVS();
void factoryReset();

// Hardware Control
void initHardware();
void initRelays();
void setRelay(uint8_t index, bool state);
void setAllRelays(uint8_t mask);
FanSpeed getCurrentFanSpeed();
bool isAutoMode();  // For FAN_4CH only

// State Machine
void updateState();
void enterDiscoveryMode();
void enterOperationalMode();
void enterFailSafeMode();
void exitFailSafeMode();
void executeFailSafe();

// LED Indicators
void updateLED();
void blinkLED(uint32_t interval);

// RS-485 Communication
void sendFrame(uint8_t address, uint8_t command, const uint8_t* payload, uint8_t length);
void sendDiscoveryAnnounce();
void sendStatusReport();
void sendACK();
void sendNACK(ErrorCode errorCode, const char* message = nullptr);

// Frame Processing
void processIncomingByte(uint8_t byte);
void processCompleteFrame();
void handleHeartbeat();
void handleSetRelay(const uint8_t* payload, uint8_t length);
void handleDiscoveryResponse(const uint8_t* payload, uint8_t length);
void handleForceReset();

// Relay Control (Profile-specific)
bool validateRelayCommand(const uint8_t* values);
void applyRelayCommand(const uint8_t* values);
void applyFanSpeed(FanSpeed speed);

// Utility
void getMACAddress();
void printBootInfo();
void printStats();

// Sensor Data (dari broadcast Logic Master)
void handleSensorData(const uint8_t* payload, uint8_t length);
FanSpeed calcAutoSpeedFromTemp(float temp);

// ══════════════════════════════════════════════════════════════
// SETUP
// ══════════════════════════════════════════════════════════════

void setup() {
    // Initialize serial debug
    Serial.begin(115200);
    delay(500);

    Serial.println("\n\n");
    Serial.println("════════════════════════════════════════════");
    Serial.println("  SMART MOSQUE SLAVE NODE KIPAS v" FIRMWARE_VERSION);
    Serial.println("  Board: HwThinker 4-relay ESP32");
    Serial.println("════════════════════════════════════════════");

    // Get MAC address
    getMACAddress();
    char macStr[18];
    SmartMosqueUtils::macToString(myMAC, macStr);
    Serial.printf("MAC Address: %s\n", macStr);

    // Initialize hardware
    initHardware();
    Serial.println("✓ Hardware initialized");

    // Load configuration from NVS
    if (loadConfigFromNVS()) {
        Serial.printf("✓ Config loaded: ID=%d, Profile=%s\n",
                     config.device_id,
                     SmartMosqueUtils::profileToString(config.profile));
        currentState = STATE_OPERATIONAL;
    } else {
        Serial.println("⚠ No valid config, entering discovery mode");
        config.device_id = 0;
        config.profile   = PROFILE_FAN_4CH;
        currentState     = STATE_DISCOVERY;
    }

    // Initialize RS-485
    // HwThinker board: ModBUS TX=GPIO19, ModBUS RX=GPIO18
    // DE/RE dikelola otomatis oleh chip RS485 onboard
    Serial2.begin(115200, SERIAL_8N1, PIN_RS485_RX, PIN_RS485_TX);
    // PIN_RS485_DE_RE = -1 → tidak ada pinMode
    Serial.println("✓ RS-485 initialized (TX=GPIO19, RX=GPIO18, auto DE/RE)");

    // Initialize timing
    bootTime = millis();
    lastHeartbeatReceived = millis();

    printBootInfo();
    Serial.println("\n✓ Setup complete, entering main loop\n");
}

// ══════════════════════════════════════════════════════════════
// MAIN LOOP
// ══════════════════════════════════════════════════════════════

void loop() {
    // Update state machine
    updateState();

    // Proses frame RS-485 masuk
    while (Serial2.available()) {
        uint8_t byte = Serial2.read();
        processIncomingByte(byte);
    }

    // Handle discovery mode
    if (currentState == STATE_DISCOVERY) {
        uint32_t interval = random(DISCOVERY_MIN_INTERVAL, DISCOVERY_MAX_INTERVAL);
        if (millis() - lastDiscoveryAnnounce > interval) {
            sendDiscoveryAnnounce();
            lastDiscoveryAnnounce = millis();
        }
    }

    // Cek fail-safe timeout
    if (currentState != STATE_DISCOVERY && currentState != STATE_FAILSAFE) {
        if (millis() - lastHeartbeatReceived > FAILSAFE_TIMEOUT) {
            Serial.println("⚠ FAIL-SAFE TIMEOUT!");
            enterFailSafeMode();
        }
    }

    // Auto speed berdasarkan suhu (hanya saat AUTO mode dan ada data sensor)
    if (currentState == STATE_AUTO && sensorDataReceived) {
        static uint32_t lastTempCheck = 0;
        if (millis() - lastTempCheck > 5000) {  // Cek tiap 5 detik
            FanSpeed targetSpeed = calcAutoSpeedFromTemp(latestTemperature);
            FanSpeed currentSpeed = getCurrentFanSpeed();
            if (targetSpeed != currentSpeed) {
                Serial.printf("[AUTO] Suhu=%.1f°C → speed: %s\n",
                              latestTemperature,
                              SmartMosqueUtils::fanSpeedToString(targetSpeed));
                applyFanSpeed(targetSpeed);
            }
            lastTempCheck = millis();
        }
    }

    // Periodic status report (tiap 60 detik)
    if (currentState == STATE_OPERATIONAL ||
        currentState == STATE_AUTO ||
        currentState == STATE_MANUAL) {
        if (millis() - lastStatusReport > 60000) {
            sendStatusReport();
            lastStatusReport = millis();
        }
    }

    // Update LED indicator
    updateLED();

    // Debug output (tiap 10 detik)
    static uint32_t lastDebug = 0;
    if (millis() - lastDebug > 10000) {
        Serial.printf("[DEBUG] State=%d CS=%s Relays=0x%02X HB_age=%lus",
                     currentState,
                     SmartMosqueUtils::controlSourceToString(controlSource),
                     relayStates,
                     (millis() - lastHeartbeatReceived) / 1000);
        if (sensorDataReceived) {
            Serial.printf(" T=%.1f°C H=%.1f%%", latestTemperature, latestHumidity);
        }
        Serial.println();
        lastDebug = millis();
    }
}

// ══════════════════════════════════════════════════════════════
// CONFIGURATION MANAGEMENT
// ══════════════════════════════════════════════════════════════

bool loadConfigFromNVS() {
    preferences.begin("config", true);  // Read-only
    
    // Try to read config
    size_t size = preferences.getBytes("slave_cfg", &config, sizeof(SlaveConfig));
    preferences.end();
    
    if (size != sizeof(SlaveConfig)) {
        Serial.println("⚠ NVS: Config size mismatch");
        return false;
    }
    
    // Validate config
    if (!SmartMosqueUtils::validateConfig(&config)) {
        Serial.println("⚠ NVS: Config validation failed");
        return false;
    }
    
    // Check if MAC matches
    if (memcmp(config.mac, myMAC, 6) != 0) {
        Serial.println("⚠ NVS: MAC address mismatch");
        return false;
    }
    
    return true;
}

bool saveConfigToNVS() {
    // Calculate and set CRC
    config.crc = SmartMosqueUtils::calculateConfigCRC(&config);
    
    // Read old config for comparison
    SlaveConfig oldConfig;
    preferences.begin("config", true);
    size_t size = preferences.getBytes("slave_cfg", &oldConfig, sizeof(SlaveConfig));
    preferences.end();
    
    // Compare if old config exists
    if (size == sizeof(SlaveConfig)) {
        if (memcmp(&config, &oldConfig, sizeof(SlaveConfig)) == 0) {
            Serial.println("✓ NVS: Config unchanged, skip write");
            return true;
        }
    }
    
    // Write new config
    preferences.begin("config", false);  // Read-write
    size = preferences.putBytes("slave_cfg", &config, sizeof(SlaveConfig));
    preferences.end();
    
    if (size != sizeof(SlaveConfig)) {
        Serial.println("✗ NVS: Write failed");
        return false;
    }
    
    Serial.println("✓ NVS: Config saved successfully");
    return true;
}

void factoryReset() {
    Serial.println("⚠ FACTORY RESET: Clearing NVS...");
    
    preferences.begin("config", false);
    preferences.clear();
    preferences.end();
    
    Serial.println("✓ NVS cleared");
    delay(1000);
    
    Serial.println("→ Restarting in 3 seconds...");
    delay(3000);
    ESP.restart();
}

// Continuation marker - File akan dilanjutkan di bagian berikutnya
// Total LOC so far: ~350 lines
