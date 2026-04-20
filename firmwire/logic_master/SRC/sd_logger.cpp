/**
 * ═══════════════════════════════════════════════════════════════
 *  LOGIC MASTER - MICROSD LOGGER
 *
 *  MicroSD terhubung via SPI:
 *    SS   = GPIO10
 *    MOSI = GPIO11
 *    SCK  = GPIO12
 *    MISO = GPIO13
 *
 *  Menyimpan:
 *  - Data sensor suhu & kelembaban (CSV, interval 60 detik)
 *  - Event sistem (enrollment, failsafe, error)
 *  - Format file: /data/YYYY-MM-DD.csv
 *                 /events/YYYY-MM-DD.log
 *
 *  Library: SD.h (built-in ESP32 Arduino framework)
 * ═══════════════════════════════════════════════════════════════
 */

#include "../include/logic_master.h"
#include <SPI.h>
#include <SD.h>

// ══════════════════════════════════════════════════════════════
// STATE
// ══════════════════════════════════════════════════════════════

static bool sdReady = false;

// ══════════════════════════════════════════════════════════════
// HELPER: FORMAT PATH FILE BERDASARKAN TANGGAL
// ══════════════════════════════════════════════════════════════

static void getDateString(char* buf, uint8_t bufLen) {
    uint32_t epoch = getEpochTime();
    uint32_t t = epoch / 86400;  // Hari sejak epoch

    uint16_t year = 1970;
    while (true) {
        bool leap = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
        uint32_t diy = leap ? 366 : 365;
        if (t < diy) break;
        t -= diy;
        year++;
    }
    const uint8_t daysInMonth[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    bool leap = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
    uint8_t month = 1;
    while (month <= 12) {
        uint8_t dim = daysInMonth[month - 1] + (month == 2 && leap ? 1 : 0);
        if (t < dim) break;
        t -= dim;
        month++;
    }
    uint8_t day = (uint8_t)(t + 1);

    snprintf(buf, bufLen, "%04d-%02d-%02d", year, month, day);
}

static void getTimeString(char* buf, uint8_t bufLen) {
    uint32_t t = getEpochTime();
    uint8_t sec  = t % 60; t /= 60;
    uint8_t min  = t % 60; t /= 60;
    uint8_t hour = t % 24;
    snprintf(buf, bufLen, "%02d:%02d:%02d", hour, min, sec);
}

// ══════════════════════════════════════════════════════════════
// INISIALISASI
// ══════════════════════════════════════════════════════════════

void initSD() {
    // Gunakan SPI custom dengan pin yang sesuai board IoT Logger
    SPI.begin(PIN_SD_SCK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_SS);

    if (!SD.begin(PIN_SD_SS)) {
        Serial.println("✗ MicroSD: Tidak ditemukan atau gagal init");
        Serial.println("  → Pastikan kartu SD terpasang dan format FAT32");
        Serial.println("  → SS=GPIO10, MOSI=GPIO11, SCK=GPIO12, MISO=GPIO13");
        sdReady = false;
        return;
    }

    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("✓ MicroSD: Siap (%llu MB, type=%d)\n",
                  cardSize, SD.cardType());

    // Buat direktori jika belum ada
    if (!SD.exists("/data"))   SD.mkdir("/data");
    if (!SD.exists("/events")) SD.mkdir("/events");

    // Tulis header CSV jika file hari ini belum ada
    char dateStr[16];
    getDateString(dateStr, sizeof(dateStr));
    char csvPath[32];
    snprintf(csvPath, sizeof(csvPath), "/data/%s.csv", dateStr);

    if (!SD.exists(csvPath)) {
        File f = SD.open(csvPath, FILE_WRITE);
        if (f) {
            f.println("timestamp,epoch,temperature_c,humidity_pct,sensor_addr,status");
            f.close();
            Serial.printf("  → File CSV baru: %s\n", csvPath);
        }
    }

    sdReady = true;
    logEvent("SD Logger initialized");
}

// ══════════════════════════════════════════════════════════════
// STATUS
// ══════════════════════════════════════════════════════════════

bool isSDReady() { return sdReady; }

// ══════════════════════════════════════════════════════════════
// LOG SENSOR DATA
// ══════════════════════════════════════════════════════════════

/**
 * Simpan data sensor ke file CSV harian.
 * Format: timestamp,epoch,temperature_c,humidity_pct,sensor_addr,status
 */
void logSensorData(const SensorReading* data) {
    if (!sdReady || data == nullptr) return;

    char dateStr[16];
    char timeStr[12];
    getDateString(dateStr, sizeof(dateStr));
    getTimeString(timeStr, sizeof(timeStr));

    char csvPath[32];
    snprintf(csvPath, sizeof(csvPath), "/data/%s.csv", dateStr);

    File f = SD.open(csvPath, FILE_APPEND);
    if (!f) {
        Serial.printf("✗ SD: Gagal buka %s\n", csvPath);
        return;
    }

    char line[128];
    snprintf(line, sizeof(line),
             "%s %s,%lu,%.1f,%.1f,0x%02X,%d",
             dateStr, timeStr,
             getEpochTime(),
             data->temperature,
             data->humidity,
             data->modbus_addr,
             data->status);
    f.println(line);
    f.close();
}

// ══════════════════════════════════════════════════════════════
// LOG EVENT
// ══════════════════════════════════════════════════════════════

/**
 * Simpan event sistem ke file log harian.
 */
void logEvent(const char* event) {
    if (!sdReady || event == nullptr) return;

    char dateStr[16];
    char timeStr[12];
    getDateString(dateStr, sizeof(dateStr));
    getTimeString(timeStr, sizeof(timeStr));

    char logPath[36];
    snprintf(logPath, sizeof(logPath), "/events/%s.log", dateStr);

    File f = SD.open(logPath, FILE_APPEND);
    if (!f) return;

    char line[192];
    snprintf(line, sizeof(line), "[%s %s] %s", dateStr, timeStr, event);
    f.println(line);
    f.close();
}
