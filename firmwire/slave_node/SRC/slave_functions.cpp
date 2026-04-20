/**
 * ═══════════════════════════════════════════════════════════════
 *  SLAVE NODE - FUNCTION IMPLEMENTATIONS (Part 2/3)
 *
 *  Board  : HwThinker 4-relay ESP32
 *  Profile: PROFILE_FAN_4CH
 *    Relay 1 = GPIO23 → SWING
 *    Relay 2 = GPIO5  → Speed LOW
 *    Relay 3 = GPIO4  → Speed MED
 *    Relay 4 = GPIO13 → Speed HIGH
 *
 *  RS485  : TX=GPIO19, RX=GPIO18
 *  LED    : GPIO15
 *  Mode   : GPIO27 (Input3) HIGH=AUTO, LOW=MANUAL
 * ═══════════════════════════════════════════════════════════════
 */

// NOTE: This file should be appended to main.cpp or included as implementation

// ══════════════════════════════════════════════════════════════
// HARDWARE CONTROL
// ══════════════════════════════════════════════════════════════

void initHardware() {
    // Initialize LED
    pinMode(PIN_LED_STATUS, OUTPUT);
    digitalWrite(PIN_LED_STATUS, LOW);

    // Initialize relays (all OFF)
    initRelays();

    // Initialize mode detect (GPIO27/Input3) dengan pull-up internal
    // HIGH = AUTO mode, LOW = MANUAL mode
    pinMode(PIN_MODE_DETECT, INPUT_PULLUP);

    // Initialize input pins lainnya
    pinMode(PIN_INPUT_1, INPUT_PULLUP);
    pinMode(PIN_INPUT_2, INPUT_PULLUP);
    // PIN_INPUT_3 = PIN_MODE_DETECT sudah di atas
    pinMode(PIN_INPUT_4, INPUT_PULLUP);

    Serial.println("✓ GPIO configured (HwThinker 4-relay ESP32)");
    Serial.println("  Relay1=GPIO23(swing) Relay2=GPIO5(low) Relay3=GPIO4(med) Relay4=GPIO13(high)");
    Serial.println("  LED=GPIO15  Mode=GPIO27(Input3)  RS485 TX=GPIO19 RX=GPIO18");
}

void initRelays() {
    // 4 relay pada HwThinker: GPIO23, GPIO5, GPIO4, GPIO13
    for (int i = 0; i < 4; i++) {
        pinMode(RELAY_PINS[i], OUTPUT);
        digitalWrite(RELAY_PINS[i], LOW);  // Relay OFF (active HIGH)
    }
    relayStates = 0x00;
    Serial.println("✓ All 4 relays initialized OFF");
}

void setRelay(uint8_t index, bool state) {
    if (index >= 4) return;  // HwThinker hanya 4 relay

    digitalWrite(RELAY_PINS[index], state ? HIGH : LOW);

    if (state) {
        relayStates |= (1 << index);
    } else {
        relayStates &= ~(1 << index);
    }
}

void setAllRelays(uint8_t mask) {
    for (int i = 0; i < 4; i++) {
        bool state = (mask & (1 << i)) != 0;
        digitalWrite(RELAY_PINS[i], state ? HIGH : LOW);
    }
    relayStates = mask & 0x0F;  // Hanya 4 bit bawah
}

FanSpeed getCurrentFanSpeed() {
    // Relay index: 0=swing, 1=LOW, 2=MED, 3=HIGH
    bool lowOn  = (relayStates & (1 << 1)) != 0;
    bool medOn  = (relayStates & (1 << 2)) != 0;
    bool highOn = (relayStates & (1 << 3)) != 0;

    if (highOn) return FAN_HIGH;
    if (medOn)  return FAN_MED;
    if (lowOn)  return FAN_LOW;
    return FAN_OFF;
}

bool isAutoMode() {
    // GPIO27 (Input3): HIGH=AUTO, LOW=MANUAL
    return digitalRead(PIN_MODE_DETECT) == HIGH;
}

// ══════════════════════════════════════════════════════════════
// STATE MACHINE
// Referensi: [STP-5.1] Slave State Machine
// ══════════════════════════════════════════════════════════════

void updateState() {
    SlaveState previousState = currentState;
    
    switch (currentState) {
        case STATE_INIT:
            // Should transition immediately in setup()
            break;
            
        case STATE_DISCOVERY:
            // Stay in discovery until DISCOVERY_RESPONSE received
            break;
            
        case STATE_OPERATIONAL:
            // Check profile and mode
            if (config.profile == PROFILE_FAN_4CH) {
                if (isAutoMode()) {
                    currentState = STATE_AUTO;
                    controlSource = CONTROL_AUTO;
                } else {
                    currentState = STATE_MANUAL;
                    controlSource = CONTROL_MANUAL;
                }
            } else {
                // LIGHT/SOUND always AUTO
                currentState = STATE_AUTO;
                controlSource = CONTROL_AUTO;
            }
            break;
            
        case STATE_AUTO:
        case STATE_MANUAL:
            // Check heartbeat timeout
            if (millis() - lastHeartbeatReceived > FAILSAFE_TIMEOUT) {
                enterFailSafeMode();
            }
            
            // For FAN_4CH, check mode switch changes
            if (config.profile == PROFILE_FAN_4CH) {
                if (currentState == STATE_AUTO && !isAutoMode()) {
                    currentState = STATE_MANUAL;
                    controlSource = CONTROL_MANUAL;
                    Serial.println("→ Switched to MANUAL mode");
                    sendStatusReport();
                } else if (currentState == STATE_MANUAL && isAutoMode()) {
                    currentState = STATE_AUTO;
                    controlSource = CONTROL_AUTO;
                    Serial.println("→ Switched to AUTO mode");
                    sendStatusReport();
                }
            }
            break;
            
        case STATE_FAILSAFE:
            // Check if heartbeat restored
            if (millis() - lastHeartbeatReceived < FAILSAFE_TIMEOUT) {
                exitFailSafeMode();
            }
            break;
    }
    
    // Log state changes
    if (currentState != previousState) {
        Serial.printf("STATE CHANGE: %d → %d\n", previousState, currentState);
    }
}

void enterDiscoveryMode() {
    currentState = STATE_DISCOVERY;
    Serial.println("→ Entering DISCOVERY mode");
}

void enterOperationalMode() {
    currentState = STATE_OPERATIONAL;
    Serial.println("→ Entering OPERATIONAL mode");
}

void enterFailSafeMode() {
    currentState = STATE_FAILSAFE;
    controlSource = CONTROL_FAILSAFE;
    
    Serial.println("⚠⚠⚠ ENTERING FAIL-SAFE MODE ⚠⚠⚠");
    executeFailSafe();
    
    // Send status report if possible
    sendStatusReport();
}

void exitFailSafeMode() {
    Serial.println("✓ Heartbeat restored, exiting FAIL-SAFE");
    
    currentState = STATE_OPERATIONAL;
    
    // Will be updated to AUTO/MANUAL in next updateState()
    
    // Send recovery notification
    sendStatusReport();
}

void executeFailSafe() {
    // Referensi: [KT-3.1] Fail-Safe Actions Matrix
    
    switch (config.profile) {
        case PROFILE_LIGHT_8CH:
            // FORCE ALL ON (safety: visibility)
            Serial.println("FAIL-SAFE: LIGHT → ALL ON");
            setAllRelays(0xFF);  // All 8 relays ON
            break;
            
        case PROFILE_FAN_4CH:
            // FORCE ALL OFF (safety: prevent overheating)
            Serial.println("FAIL-SAFE: FAN → ALL OFF");
            setAllRelays(0x00);  // All relays OFF
            break;
            
        case PROFILE_SOUND_8CH:
            // FORCE ALL OFF (safety: prevent noise)
            Serial.println("FAIL-SAFE: SOUND → ALL OFF");
            setAllRelays(0x00);  // All relays OFF
            break;
            
        default:
            Serial.println("FAIL-SAFE: Unknown profile, ALL OFF");
            setAllRelays(0x00);
            break;
    }
}

// ══════════════════════════════════════════════════════════════
// LED INDICATORS
// Referensi: [STP-5.1] LED Indicators table
// ══════════════════════════════════════════════════════════════

void updateLED() {
    static uint32_t lastBlink = 0;
    static bool ledState = false;
    
    switch (currentState) {
        case STATE_INIT:
            // Fast blink (100ms)
            blinkLED(100);
            break;
            
        case STATE_DISCOVERY:
            // Medium blink (500ms) - Blue LED equivalent
            blinkLED(500);
            break;
            
        case STATE_AUTO:
            // Steady ON (or slow blink 2s) - Green equivalent
            digitalWrite(PIN_LED_STATUS, HIGH);
            break;
            
        case STATE_MANUAL:
            // Steady ON different pattern - Yellow equivalent
            // Since we only have 1 LED, use slower blink to differentiate
            blinkLED(1000);
            break;
            
        case STATE_FAILSAFE:
            // Steady ON - Red equivalent (critical)
            digitalWrite(PIN_LED_STATUS, HIGH);
            break;
            
        default:
            digitalWrite(PIN_LED_STATUS, LOW);
            break;
    }
}

void blinkLED(uint32_t interval) {
    static uint32_t lastToggle = 0;
    static bool state = false;
    
    if (millis() - lastToggle >= interval) {
        state = !state;
        digitalWrite(PIN_LED_STATUS, state ? HIGH : LOW);
        lastToggle = millis();
    }
}

// ══════════════════════════════════════════════════════════════
// RS-485 FRAME TRANSMISSION
// Referensi: [STP-11.1] Complete Frame Encoder
// ══════════════════════════════════════════════════════════════

void sendFrame(uint8_t address, uint8_t command, const uint8_t* payload, uint8_t length) {
    if (length > MAX_PAYLOAD_SIZE) {
        Serial.println("✗ Payload too large");
        return;
    }
    
    // Build frame in buffer
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
    
    // Transmit
    // Board HwThinker memiliki chip RS485 onboard dengan DE/RE otomatis
    // Tidak perlu mengontrol pin DE/RE secara manual
    Serial2.write(frame, idx);
    Serial2.flush();  // Tunggu transmisi selesai
    
    stats.framesSent++;
    
    // Debug output
    Serial.printf("→ TX[%d bytes]: ADDR=0x%02X CMD=0x%02X LEN=%d\n",
                 idx, address, command, length);
}

void sendDiscoveryAnnounce() {
    // Referensi: [STP-4.2.1] DISCOVERY_ANNOUNCE
    
    Serial.println("\n→ Sending DISCOVERY_ANNOUNCE...");
    
    // Build JSON payload
    StaticJsonDocument<200> doc;
    
    // MAC address
    char macStr[18];
    SmartMosqueUtils::macToString(myMAC, macStr);
    doc["mac"] = macStr;
    
    // Profile (default to FAN_4CH if not set)
    uint8_t profile = config.profile;
    if (profile == 0 || profile > PROFILE_SOUND_8CH) {
        profile = PROFILE_FAN_4CH;  // Default
    }
    doc["profile"] = SmartMosqueUtils::profileToString(profile);
    
    // Firmware version
    doc["ver"] = FIRMWARE_VERSION;
    
    // Serialize to string
    String jsonStr;
    serializeJson(doc, jsonStr);
    
    Serial.println("  Payload: " + jsonStr);
    
    // Send frame (broadcast to Master)
    sendFrame(ADDR_BROADCAST, CMD_DISCOVERY_ANNOUNCE, 
             (uint8_t*)jsonStr.c_str(), jsonStr.length());
}

void sendStatusReport() {
    StaticJsonDocument<256> doc;

    doc["device_id"]      = config.device_id;
    doc["control_source"] = SmartMosqueUtils::controlSourceToString(controlSource);

    // Status 4 relay HwThinker
    JsonArray relayArray = doc.createNestedArray("relay_states");
    for (int i = 0; i < 4; i++) {
        relayArray.add((relayStates & (1 << i)) ? 1 : 0);
    }

    // Field khusus FAN_4CH
    FanSpeed speed = getCurrentFanSpeed();
    doc["fan_speed"]   = SmartMosqueUtils::fanSpeedToString(speed);
    doc["mode_switch"] = isAutoMode() ? "AUTO" : "MANUAL";
    doc["relay_swing"] = (relayStates & (1 << 0)) ? 1 : 0;

    // Data suhu jika ada
    if (sensorDataReceived) {
        doc["temperature"] = latestTemperature;
        doc["humidity"]    = latestHumidity;
    }

    doc["uptime"] = (millis() - bootTime) / 1000;

    String jsonStr;
    serializeJson(doc, jsonStr);

    sendFrame(ADDR_LOGIC_MASTER, CMD_STATUS_REPORT,
             (uint8_t*)jsonStr.c_str(), jsonStr.length());

    Serial.println("→ Status report sent");
}

void sendACK() {
    sendFrame(ADDR_LOGIC_MASTER, CMD_ACK, nullptr, 0);
    Serial.println("→ ACK sent");
}

void sendNACK(ErrorCode errorCode, const char* message) {
    StaticJsonDocument<128> doc;
    
    doc["code"] = errorCode;
    
    if (message != nullptr) {
        doc["msg"] = message;
    } else {
        // Default messages
        switch (errorCode) {
            case ERROR_INVALID_CRC:       doc["msg"] = "Invalid CRC"; break;
            case ERROR_INVALID_COMMAND:   doc["msg"] = "Unknown command"; break;
            case ERROR_MANUAL_MODE_ACTIVE: doc["msg"] = "Manual mode active"; break;
            case ERROR_INTERLOCK_VIOLATION: doc["msg"] = "Interlock violation"; break;
            default: doc["msg"] = "Error"; break;
        }
    }
    
    String jsonStr;
    serializeJson(doc, jsonStr);
    
    sendFrame(ADDR_LOGIC_MASTER, CMD_NACK,
             (uint8_t*)jsonStr.c_str(), jsonStr.length());
    
    Serial.printf("→ NACK sent: code=0x%02X\n", errorCode);
}

// ══════════════════════════════════════════════════════════════
// RELAY CONTROL (PROFILE-SPECIFIC)
// ══════════════════════════════════════════════════════════════

bool validateRelayCommand(const uint8_t* values) {
    // Hanya untuk PROFILE_FAN_4CH
    // Relay layout HwThinker:
    //   values[0] = swing (boleh ON bersamaan dengan speed)
    //   values[1] = LOW, values[2] = MED, values[3] = HIGH
    // Interlock: hanya SATU relay speed (1/2/3) yang boleh ON sekaligus

    int speedCount = 0;
    if (values[1]) speedCount++;  // LOW
    if (values[2]) speedCount++;  // MED
    if (values[3]) speedCount++;  // HIGH

    if (speedCount > 1) {
        Serial.println("✗ Interlock violation: Multiple speeds ON (relay 2/3/4)");
        return false;
    }

    return true;
}

void applyRelayCommand(const uint8_t* values) {
    // Terapkan perintah relay sesuai layout HwThinker
    // values[0]=swing, values[1]=low, values[2]=med, values[3]=high

    FanSpeed newSpeed = FAN_OFF;
    if (values[3]) newSpeed = FAN_HIGH;
    else if (values[2]) newSpeed = FAN_MED;
    else if (values[1]) newSpeed = FAN_LOW;

    applyFanSpeed(newSpeed);

    // Override swing jika perintah eksplisit
    // (applyFanSpeed sudah atur swing, tapi hormati perintah eksplisit)
    if (newSpeed != FAN_OFF && values[0] == 0) {
        // Perintah: speed ON tapi swing OFF → nonaktifkan swing
        setRelay(0, false);
    }

    Serial.printf("✓ Relay command applied: swing=%d speed=%s relays=0x%02X\n",
                 values[0], SmartMosqueUtils::fanSpeedToString(newSpeed), relayStates);
}

void applyFanSpeed(FanSpeed speed) {
    // Dead-time implementation untuk keamanan relay
    // Relay index pada HwThinker:
    //   0 = GPIO23 → SWING (tidak dimatikan saat ganti speed)
    //   1 = GPIO5  → Speed LOW
    //   2 = GPIO4  → Speed MED
    //   3 = GPIO13 → Speed HIGH

    Serial.printf("→ Changing fan speed to: %s\n",
                 SmartMosqueUtils::fanSpeedToString(speed));

    // Step 1: Matikan semua relay speed (bukan swing)
    setRelay(1, false);  // LOW off
    setRelay(2, false);  // MED off
    setRelay(3, false);  // HIGH off

    // Step 2: Dead-time 200ms
    delay(FAN_DEAD_TIME);

    // Step 3: Aktifkan speed baru
    switch (speed) {
        case FAN_LOW:
            setRelay(1, true);
            // Relay swing (index 0) ikut aktif saat kipas menyala
            setRelay(0, true);
            break;
        case FAN_MED:
            setRelay(2, true);
            setRelay(0, true);
            break;
        case FAN_HIGH:
            setRelay(3, true);
            setRelay(0, true);
            break;
        case FAN_OFF:
        default:
            // Semua sudah off, termasuk swing
            setRelay(0, false);
            break;
    }

    Serial.printf("✓ Fan speed applied: %s  relays=0x%02X\n",
                 SmartMosqueUtils::fanSpeedToString(speed), relayStates);
}

// ══════════════════════════════════════════════════════════════
// UTILITY FUNCTIONS
// ══════════════════════════════════════════════════════════════

void getMACAddress() {
    esp_read_mac(myMAC, ESP_MAC_WIFI_STA);
}

void printBootInfo() {
    Serial.println("\n╔══════════════════════════════════════════════╗");
    Serial.println("║           BOOT INFORMATION                   ║");
    Serial.println("╠══════════════════════════════════════════════╣");
    Serial.printf( "║ Firmware:    v%-29s ║\n", FIRMWARE_VERSION);
    Serial.printf( "║ Build Date:  %-29s ║\n", BUILD_DATE);

    char macStr[18];
    SmartMosqueUtils::macToString(myMAC, macStr);
    Serial.printf( "║ MAC:         %-29s ║\n", macStr);
    Serial.printf( "║ Device ID:   %-29d ║\n", config.device_id);
    Serial.printf( "║ Profile:     %-29s ║\n",
                 SmartMosqueUtils::profileToString(config.profile));
    Serial.printf( "║ Name:        %-29s ║\n", config.device_name);
    Serial.println("╠══════════════════════════════════════════════╣");
    Serial.println("║ Board: HwThinker 4-relay ESP32               ║");
    Serial.println("║ RS485: TX=GPIO19, RX=GPIO18 (auto DE/RE)     ║");
    Serial.println("║ Relay: GPIO23(swing) 5(low) 4(med) 13(high) ║");
    Serial.println("║ LED: GPIO15  Mode: GPIO27(Input3)            ║");
    Serial.println("╚══════════════════════════════════════════════╝");
}

void printStats() {
    Serial.println("\n╔══════════════════════════════════════════════╗");
    Serial.println("║             STATISTICS                       ║");
    Serial.println("╠══════════════════════════════════════════════╣");
    Serial.printf( "║ Frames RX:   %8lu                      ║\n", stats.framesReceived);
    Serial.printf( "║ Frames TX:   %8lu                      ║\n", stats.framesSent);
    Serial.printf( "║ CRC Errors:  %8lu                      ║\n", stats.crcErrors);
    Serial.printf( "║ Cmds Exec:   %8lu                      ║\n", stats.commandsExecuted);
    Serial.printf( "║ Cmds Reject: %8lu                      ║\n", stats.commandsRejected);
    Serial.printf( "║ Uptime:      %8lu s                    ║\n", (millis() - bootTime) / 1000);
    if (sensorDataReceived) {
        Serial.printf("║ Suhu:        %6.1f °C                    ║\n", latestTemperature);
        Serial.printf("║ Kelembaban:  %6.1f %%RH                  ║\n", latestHumidity);
    }
    Serial.println("╚══════════════════════════════════════════════╝\n");
}

// ══════════════════════════════════════════════════════════════
// SENSOR DATA HANDLER
// Menerima broadcast CMD_SENSOR_DATA dari Logic Master
// ══════════════════════════════════════════════════════════════

void handleSensorData(const uint8_t* payload, uint8_t length) {
    if (length < sizeof(SensorData)) return;

    const SensorData* data = (const SensorData*)payload;

    if (data->status != 0) {
        Serial.printf("[SensorData] Status error: %d\n", data->status);
        return;
    }

    latestTemperature = data->temperature_x10 / 10.0f;
    latestHumidity    = data->humidity_x10    / 10.0f;
    sensorDataReceived = true;

    Serial.printf("[SensorData] T=%.1f°C H=%.1f%%RH\n",
                  latestTemperature, latestHumidity);
}

/**
 * Hitung kecepatan kipas yang sesuai berdasarkan suhu.
 * Threshold bisa disesuaikan sesuai kondisi masjid.
 */
FanSpeed calcAutoSpeedFromTemp(float temp) {
    if (temp >= TEMP_THRESHOLD_HIGH) return FAN_HIGH;
    if (temp >= TEMP_THRESHOLD_MED)  return FAN_MED;
    if (temp >= TEMP_THRESHOLD_LOW)  return FAN_LOW;
    return FAN_OFF;
}
