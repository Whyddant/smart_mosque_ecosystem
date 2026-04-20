/**
 * ═══════════════════════════════════════════════════════════════
 *  LOGIC MASTER - MODBUS RTU HANDLER v1.2
 *
 *  Perubahan v1.2 (integrasi ESP32 X8 Relay 303e32dc812):
 *  ─────────────────────────────────────────────────────────────
 *  - Modbus RTU sekarang berjalan di BUS B (Serial1) terpisah
 *    dari Smart Mosque Protocol (Bus A / Serial2)
 *  - Tambah fungsi modbusX8WriteRelays() dan modbusX8ReadStatus()
 *    untuk protokol khusus ESP32 X8 Relay (FC15/FC02 custom)
 *  - Node lampu & sound sekarang ESP32 X8 Relay bukan modul
 *    hardware murni tanpa ESP32
 *
 *  BUS B (Serial1) — PIN_MODBUS_TX=GPIO1, PIN_MODBUS_RX=GPIO2:
 *    addr 0x01 → SHT20 XY-MD02         (Modbus standar FC03)
 *    addr 0x02 → ESP32 X8 Relay Lampu  (FC15 write / FC02 read)
 *    addr 0x03 → ESP32 X8 Relay Sound  (FC15 write / FC02 read)
 *
 *  Protokol ESP32 X8 Relay (303e32dc812):
 *    FC15 Write Multiple Coils:
 *      TX: [addr][0x0F][0x00][0x00][0x00][0x08][0x01][MASK][CRC_L][CRC_H]
 *      RX: [addr][0x0F][0x00][0x00][0x00][0x08][CRC_L][CRC_H]
 *      MASK = bitmask 8 relay (bit0=relay1 ... bit7=relay8)
 *    FC02 Read Discrete Inputs:
 *      TX: [addr][0x02][0x00][MEM][0x00][0x01][CRC_L][CRC_H]
 *      RX: [addr][0x02][0x01][VAL][CRC_L][CRC_H]
 *      MEM=0x00 → val_in (optocoupler input)
 *      MEM=0x01 → val_out (relay state saat ini)
 * ═══════════════════════════════════════════════════════════════
 */

#include "../include/logic_master.h"

// ══════════════════════════════════════════════════════════════
// FUNCTION CODES
// ══════════════════════════════════════════════════════════════

#define FC_READ_DISCRETE_INPUTS  0x02
#define FC_READ_HOLDING_REGS     0x03
#define FC_WRITE_SINGLE_COIL     0x05
#define FC_WRITE_MULTIPLE_COILS  0x0F

#define COIL_ON   0xFF00
#define COIL_OFF  0x0000

#define MODBUS_BUF_SIZE 32

// ══════════════════════════════════════════════════════════════
// BUFFER INTERNAL
// ══════════════════════════════════════════════════════════════

static uint8_t  modbusTxBuf[MODBUS_BUF_SIZE];
static uint8_t  modbusRxBuf[MODBUS_BUF_SIZE];
static uint32_t lastModbusActivity = 0;

// ══════════════════════════════════════════════════════════════
// CRC16 MODBUS
// ══════════════════════════════════════════════════════════════

static uint16_t modbusCRC16(const uint8_t* data, uint8_t len) {
    uint16_t crc = 0xFFFF;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x0001) { crc >>= 1; crc ^= 0xA001; }
            else               { crc >>= 1; }
        }
    }
    return crc;
}

// ══════════════════════════════════════════════════════════════
// LOW-LEVEL TRANSCEIVER — BUS B (Serial1)
// ══════════════════════════════════════════════════════════════

/**
 * Kirim frame Modbus RTU via Serial1 (Bus B) dan tunggu respons.
 *
 * @param txLen      Panjang data tanpa CRC
 * @param rxExpected Jumlah byte respons yang diharapkan
 * @param timeoutMs  Batas waktu tunggu
 * @return true jika respons valid
 */
static bool modbusTransaction(uint8_t txLen, uint8_t rxExpected, uint32_t timeoutMs) {
    // Tambahkan CRC
    uint16_t crc = modbusCRC16(modbusTxBuf, txLen);
    modbusTxBuf[txLen]     = (uint8_t)(crc & 0xFF);
    modbusTxBuf[txLen + 1] = (uint8_t)((crc >> 8) & 0xFF);
    uint8_t totalTx = txLen + 2;

    // Inter-frame gap
    delay(MODBUS_INTER_FRAME_GAP_MS);

    // Aktifkan TX (DE/RE HIGH)
    if (PIN_MODBUS_DE_RE >= 0) {
        digitalWrite(PIN_MODBUS_DE_RE, HIGH);
        delayMicroseconds(100);
    }

    // Flush RX sebelum kirim
    while (Serial1.available()) Serial1.read();

    // Kirim via Serial1 (Bus B)
    Serial1.write(modbusTxBuf, totalTx);
    Serial1.flush();
    lastModbusActivity = millis();

    // Kembali ke mode RX
    if (PIN_MODBUS_DE_RE >= 0) {
        delayMicroseconds(100);
        digitalWrite(PIN_MODBUS_DE_RE, LOW);
    }

    // Tunggu respons
    uint8_t  rxLen   = 0;
    uint32_t deadline = millis() + timeoutMs;
    while (millis() < deadline && rxLen < rxExpected) {
        if (Serial1.available()) {
            modbusRxBuf[rxLen++] = Serial1.read();
        }
    }

    if (rxLen < rxExpected) {
        Serial.printf("[ModbusB] Timeout: rcvd %d/%d bytes\n", rxLen, rxExpected);
        return false;
    }

    // Validasi CRC respons
    uint16_t calcCRC = modbusCRC16(modbusRxBuf, rxLen - 2);
    uint16_t rcvdCRC = modbusRxBuf[rxLen - 2] | ((uint16_t)modbusRxBuf[rxLen - 1] << 8);
    if (calcCRC != rcvdCRC) {
        Serial.printf("[ModbusB] CRC error: calc=0x%04X rcvd=0x%04X\n", calcCRC, rcvdCRC);
        return false;
    }

    // Cek exception response
    if (modbusRxBuf[1] & 0x80) {
        Serial.printf("[ModbusB] Exception FC=0x%02X code=0x%02X\n",
                      modbusRxBuf[1] & 0x7F, modbusRxBuf[2]);
        return false;
    }

    return true;
}

// ══════════════════════════════════════════════════════════════
// INISIALISASI
// ══════════════════════════════════════════════════════════════

void initModbus() {
    // Inisialisasi Serial1 untuk Bus B (Modbus RTU)
    Serial1.begin(MODBUS_BAUD_RATE, SERIAL_8N1, PIN_MODBUS_RX, PIN_MODBUS_TX);

    // Setup pin DE/RE jika digunakan
    if (PIN_MODBUS_DE_RE >= 0) {
        pinMode(PIN_MODBUS_DE_RE, OUTPUT);
        digitalWrite(PIN_MODBUS_DE_RE, LOW);  // Default: receive mode
    }

    lastModbusActivity = millis();

    Serial.println("✓ Modbus RTU handler ready");
    Serial.printf("  Bus B: Serial1 TX=GPIO%d RX=GPIO%d DE/RE=GPIO%d @ %d bps\n",
                  PIN_MODBUS_TX, PIN_MODBUS_RX,
                  PIN_MODBUS_DE_RE, MODBUS_BAUD_RATE);
    Serial.printf("  SHT20 Sensor: addr=0x%02X (FC03)\n",     MODBUS_ADDR_SENSOR_DEFAULT);
    Serial.printf("  Relay Lampu:  addr=0x%02X (FC15/FC02)\n", MODBUS_ADDR_RELAY_LIGHT_DEFAULT);
    Serial.printf("  Relay Sound:  addr=0x%02X (FC15/FC02)\n", MODBUS_ADDR_RELAY_SOUND_DEFAULT);
}

// ══════════════════════════════════════════════════════════════
// SENSOR SHT20 XY-MD02 — FC03 Read Holding Registers
// ══════════════════════════════════════════════════════════════

bool readSensorSHT20(uint8_t modbusAddr, SensorReading* out) {
    // FC03: baca 2 register mulai 0x0001 (humidity, temperature)
    modbusTxBuf[0] = modbusAddr;
    modbusTxBuf[1] = FC_READ_HOLDING_REGS;
    modbusTxBuf[2] = 0x00;
    modbusTxBuf[3] = 0x01;  // Start reg: 0x0001 (humidity)
    modbusTxBuf[4] = 0x00;
    modbusTxBuf[5] = 0x02;  // Qty: 2 register

    // Respons: [addr][0x03][0x04][H_H][H_L][T_H][T_L][CRC_L][CRC_H] = 9 byte
    bool ok = modbusTransaction(6, 9, MODBUS_TIMEOUT_MS);

    if (!ok) {
        out->error_count++;
        out->status = 2;
        stats.modbus_reads_err++;
        Serial.printf("[SHT20] Read failed addr=0x%02X err=%d\n",
                      modbusAddr, out->error_count);
        return false;
    }

    uint16_t rawHumidity    = ((uint16_t)modbusRxBuf[3] << 8) | modbusRxBuf[4];
    int16_t  rawTemperature = (int16_t)(((uint16_t)modbusRxBuf[5] << 8) | modbusRxBuf[6]);

    float temperature = rawTemperature / 10.0f;
    float humidity    = rawHumidity    / 10.0f;

    if (temperature < -40.0f || temperature > 125.0f ||
        humidity < 0.0f || humidity > 100.0f) {
        Serial.printf("[SHT20] Out of range T=%.1f H=%.1f\n", temperature, humidity);
        out->status = 3;
        out->error_count++;
        stats.modbus_reads_err++;
        return false;
    }

    out->temperature  = temperature;
    out->humidity     = humidity;
    out->modbus_addr  = modbusAddr;
    out->last_read_ms = millis();
    out->valid        = true;
    out->status       = 0;
    out->error_count  = 0;

    stats.modbus_reads_ok++;
    Serial.printf("[SHT20] T=%.1f°C H=%.1f%%RH addr=0x%02X\n",
                  temperature, humidity, modbusAddr);
    return true;
}

// ══════════════════════════════════════════════════════════════
// ESP32 X8 RELAY (303e32dc812) — FUNGSI UTAMA
// ══════════════════════════════════════════════════════════════

/**
 * Tulis state 8 relay sekaligus ke ESP32 X8 Relay via FC15.
 *
 * Frame TX: [addr][0x0F][0x00][0x00][0x00][0x08][0x01][MASK][CRC_L][CRC_H]
 * Frame RX: [addr][0x0F][0x00][0x00][0x00][0x08][CRC_L][CRC_H] = 8 byte
 *
 * @param modbusAddr  0x02 = lampu, 0x03 = sound
 * @param mask        Bitmask 8 relay: bit0=relay1 ... bit7=relay8
 *                    Contoh: 0b00000001 = relay1 ON, lainnya OFF
 *                            0xFF       = semua ON
 *                            0x00       = semua OFF
 * @return true jika berhasil
 */
bool modbusX8WriteRelays(uint8_t modbusAddr, uint8_t mask) {
    modbusTxBuf[0] = modbusAddr;
    modbusTxBuf[1] = FC_WRITE_MULTIPLE_COILS;  // 0x0F
    modbusTxBuf[2] = 0x00;  // Start coil HIGH
    modbusTxBuf[3] = 0x00;  // Start coil LOW
    modbusTxBuf[4] = 0x00;  // Qty HIGH
    modbusTxBuf[5] = 0x08;  // Qty LOW (8 coil)
    modbusTxBuf[6] = 0x01;  // Byte count
    modbusTxBuf[7] = mask;  // Bitmask relay

    // Respons: 8 byte
    bool ok = modbusTransaction(8, 8, MODBUS_TIMEOUT_MS);

    if (ok) {
        stats.relay_commands_sent++;
        const char* nodeName = (modbusAddr == MODBUS_ADDR_RELAY_LIGHT_DEFAULT)
                               ? "LAMPU" : "SOUND";
        Serial.printf("[X8-%s] FC15 Write mask=0b%08d (0x%02X)\n",
                      nodeName,
                      // Print biner manual
                      ((mask>>7)&1)*10000000 + ((mask>>6)&1)*1000000 +
                      ((mask>>5)&1)*100000   + ((mask>>4)&1)*10000   +
                      ((mask>>3)&1)*1000     + ((mask>>2)&1)*100     +
                      ((mask>>1)&1)*10       + ((mask>>0)&1),
                      mask);
    } else {
        Serial.printf("[X8] FC15 GAGAL addr=0x%02X mask=0x%02X\n",
                      modbusAddr, mask);
    }
    return ok;
}

/**
 * Baca status dari ESP32 X8 Relay via FC02.
 *
 * Frame TX: [addr][0x02][0x00][MEM][0x00][0x01][CRC_L][CRC_H]
 * Frame RX: [addr][0x02][0x01][VAL][CRC_L][CRC_H] = 6 byte
 *
 * @param modbusAddr  0x02 = lampu, 0x03 = sound
 * @param memaddr     0x00 = input optocoupler, 0x01 = relay state
 * @param outVal      Pointer ke byte output
 * @return true jika berhasil
 */
bool modbusX8ReadStatus(uint8_t modbusAddr, uint8_t memaddr, uint8_t* outVal) {
    modbusTxBuf[0] = modbusAddr;
    modbusTxBuf[1] = FC_READ_DISCRETE_INPUTS;  // 0x02
    modbusTxBuf[2] = 0x00;
    modbusTxBuf[3] = memaddr;  // 0x00=input, 0x01=output
    modbusTxBuf[4] = 0x00;
    modbusTxBuf[5] = 0x01;

    // Respons: 6 byte
    bool ok = modbusTransaction(6, 6, MODBUS_TIMEOUT_MS);

    if (ok && outVal) {
        *outVal = modbusRxBuf[3];
        Serial.printf("[X8] FC02 Read addr=0x%02X mem=0x%02X val=0x%02X\n",
                      modbusAddr, memaddr, *outVal);
    } else if (!ok) {
        Serial.printf("[X8] FC02 GAGAL addr=0x%02X\n", modbusAddr);
    }
    return ok;
}

// ══════════════════════════════════════════════════════════════
// LEGACY RELAY 8CH — FC05 (kompatibilitas modul lain)
// ══════════════════════════════════════════════════════════════

bool setRelay8ch(uint8_t modbusAddr, uint8_t relayIndex, bool state) {
    if (relayIndex > 7) return false;

    uint16_t coilAddr = MODBUS_COIL_RELAY_BASE + relayIndex;
    uint16_t coilVal  = state ? COIL_ON : COIL_OFF;

    modbusTxBuf[0] = modbusAddr;
    modbusTxBuf[1] = FC_WRITE_SINGLE_COIL;
    modbusTxBuf[2] = (uint8_t)(coilAddr >> 8);
    modbusTxBuf[3] = (uint8_t)(coilAddr & 0xFF);
    modbusTxBuf[4] = (uint8_t)(coilVal >> 8);
    modbusTxBuf[5] = (uint8_t)(coilVal & 0xFF);

    bool ok = modbusTransaction(6, 8, MODBUS_TIMEOUT_MS);
    if (ok) stats.relay_commands_sent++;
    return ok;
}

bool setAllRelays8ch(uint8_t modbusAddr, uint8_t mask) {
    // Untuk X8 Relay 303e32dc812, gunakan modbusX8WriteRelays
    // Fungsi ini tetap ada untuk kompatibilitas modul lama (FC0F standar)
    return modbusX8WriteRelays(modbusAddr, mask);
}

// ══════════════════════════════════════════════════════════════
// POLLING SENSOR
// ══════════════════════════════════════════════════════════════

void pollSensorIfDue() {
    if (millis() - lastSensorRead < MODBUS_SENSOR_INTERVAL_MS) return;
    lastSensorRead = millis();

    bool ok = readSensorSHT20(MODBUS_ADDR_SENSOR_DEFAULT, &sensorData);
    if (ok) {
        broadcastSensorData();
    }
}

void broadcastSensorData() {
    if (!sensorData.valid) return;

    SensorData payload;
    payload.temperature_x10 = (int16_t)(sensorData.temperature * 10.0f);
    payload.humidity_x10    = (uint16_t)(sensorData.humidity * 10.0f);
    payload.sensor_id       = sensorData.modbus_addr;
    payload.status          = 0;
    payload.timestamp       = millis();

    // Broadcast ke semua slave Bus A (Smart Mosque Protocol)
    sendFrame(ADDR_BROADCAST, CMD_SENSOR_DATA,
              (uint8_t*)&payload, sizeof(SensorData));

    Serial.printf("[SensorBcast] T=%.1f°C H=%.1f%%RH → broadcast Bus A\n",
                  sensorData.temperature, sensorData.humidity);
}
