/**
 * ═══════════════════════════════════════════════════════════════
 *  LOGIC MASTER - RTC DS3231M HANDLER
 *
 *  RTC DS3231M terhubung via I2C:
 *    SDA = GPIO8
 *    SCL = GPIO9
 *    I2C Address = 0x68
 *
 *  Menyediakan timestamp akurat untuk:
 *  - Device registry (last_seen, first_seen)
 *  - Log MicroSD dengan waktu nyata
 *  - Heartbeat timestamp
 *
 *  Library: Wire.h (built-in Arduino/ESP32)
 *  Tidak memerlukan library eksternal untuk operasi dasar.
 * ═══════════════════════════════════════════════════════════════
 */

#include "../include/logic_master.h"
#include <Wire.h>

// ══════════════════════════════════════════════════════════════
// REGISTER DS3231M
// ══════════════════════════════════════════════════════════════

#define DS3231_REG_SECONDS  0x00
#define DS3231_REG_MINUTES  0x01
#define DS3231_REG_HOURS    0x02
#define DS3231_REG_DAY      0x03
#define DS3231_REG_DATE     0x04
#define DS3231_REG_MONTH    0x05
#define DS3231_REG_YEAR     0x06
#define DS3231_REG_CONTROL  0x0E
#define DS3231_REG_STATUS   0x0F
#define DS3231_REG_TEMP_MSB 0x11
#define DS3231_REG_TEMP_LSB 0x12

// ══════════════════════════════════════════════════════════════
// HELPER BCD
// ══════════════════════════════════════════════════════════════

static uint8_t bcdToDec(uint8_t bcd) { return (bcd / 16 * 10) + (bcd % 16); }
static uint8_t decToBcd(uint8_t dec) { return (dec / 10 * 16) + (dec % 10); }

// ══════════════════════════════════════════════════════════════
// BACA / TULIS REGISTER
// ══════════════════════════════════════════════════════════════

static uint8_t rtcReadReg(uint8_t reg) {
    Wire.beginTransmission(RTC_I2C_ADDRESS);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom((uint8_t)RTC_I2C_ADDRESS, (uint8_t)1);
    return Wire.available() ? Wire.read() : 0;
}

static void rtcWriteReg(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(RTC_I2C_ADDRESS);
    Wire.write(reg);
    Wire.write(val);
    Wire.endTransmission();
}

// ══════════════════════════════════════════════════════════════
// INISIALISASI
// ══════════════════════════════════════════════════════════════

void initRTC() {
    Wire.begin(PIN_RTC_SDA, PIN_RTC_SCL);
    Wire.setClock(400000);  // 400kHz Fast Mode

    // Cek apakah DS3231M merespons
    Wire.beginTransmission(RTC_I2C_ADDRESS);
    uint8_t err = Wire.endTransmission();

    if (err != 0) {
        Serial.printf("✗ RTC DS3231M tidak ditemukan di I2C 0x%02X (err=%d)\n",
                      RTC_I2C_ADDRESS, err);
        Serial.println("  → Pastikan SDA=GPIO8 dan SCL=GPIO9 terhubung benar");
        return;
    }

    // Clear OSF (Oscillator Stop Flag) jika ada
    uint8_t status = rtcReadReg(DS3231_REG_STATUS);
    if (status & 0x80) {
        Serial.println("⚠ RTC: Oscillator sempat berhenti (baterai habis?)");
        rtcWriteReg(DS3231_REG_STATUS, status & ~0x80);
    }

    // Nonaktifkan square wave output, aktifkan 32kHz output = false
    rtcWriteReg(DS3231_REG_CONTROL, 0x00);

    Serial.printf("✓ RTC DS3231M OK (I2C addr=0x%02X, SDA=8, SCL=9)\n",
                  RTC_I2C_ADDRESS);
    printRTCTime();
}

// ══════════════════════════════════════════════════════════════
// GET EPOCH TIME
// ══════════════════════════════════════════════════════════════

/**
 * Baca waktu dari DS3231M dan konversi ke Unix epoch (detik sejak 1 Jan 1970).
 * Menggunakan tahun dasar 2000 (DS3231 menyimpan 2-digit tahun).
 */
uint32_t getRTCEpoch() {
    Wire.beginTransmission(RTC_I2C_ADDRESS);
    Wire.write(DS3231_REG_SECONDS);
    Wire.endTransmission(false);
    Wire.requestFrom((uint8_t)RTC_I2C_ADDRESS, (uint8_t)7);

    if (Wire.available() < 7) {
        // Fallback: gunakan millis() offset
        static uint32_t fallbackEpoch = 1704067200UL;  // 1 Jan 2024 00:00:00 UTC
        return fallbackEpoch + (millis() / 1000);
    }

    uint8_t sec   = bcdToDec(Wire.read() & 0x7F);
    uint8_t min   = bcdToDec(Wire.read() & 0x7F);
    uint8_t hour  = bcdToDec(Wire.read() & 0x3F);
    Wire.read();  // day of week (abaikan)
    uint8_t date  = bcdToDec(Wire.read() & 0x3F);
    uint8_t month = bcdToDec(Wire.read() & 0x1F);
    uint8_t year  = bcdToDec(Wire.read());  // 0-99 (relatif ke 2000)

    // Konversi ke epoch Unix (algoritma sederhana, akurat 2001-2099)
    uint16_t y = 2000 + year;
    uint8_t  m = month;
    uint8_t  d = date;

    // Days in months
    const uint8_t daysInMonth[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    uint32_t days = 0;

    // Hitung hari dari 1970 ke awal tahun y
    for (uint16_t yr = 1970; yr < y; yr++) {
        days += (yr % 4 == 0 && (yr % 100 != 0 || yr % 400 == 0)) ? 366 : 365;
    }
    // Tambah hari dari awal tahun ke bulan m
    bool leap = (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0));
    for (uint8_t mo = 1; mo < m; mo++) {
        days += daysInMonth[mo - 1];
        if (mo == 2 && leap) days++;
    }
    days += d - 1;

    return days * 86400UL + hour * 3600UL + min * 60UL + sec;
}

// ══════════════════════════════════════════════════════════════
// SET WAKTU
// ══════════════════════════════════════════════════════════════

/**
 * Set waktu RTC dari Unix epoch.
 * Digunakan saat sinkronisasi NTP via WiFi (jika tersedia).
 */
bool setRTCTime(uint32_t epoch) {
    // Konversi epoch ke komponen waktu
    uint32_t t = epoch;
    uint8_t sec  = t % 60; t /= 60;
    uint8_t min  = t % 60; t /= 60;
    uint8_t hour = t % 24; t /= 24;

    // Hitung tanggal dari jumlah hari
    uint16_t year = 1970;
    while (true) {
        bool leap = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
        uint16_t diy = leap ? 366 : 365;
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

    // Tulis ke DS3231M
    Wire.beginTransmission(RTC_I2C_ADDRESS);
    Wire.write(DS3231_REG_SECONDS);
    Wire.write(decToBcd(sec));
    Wire.write(decToBcd(min));
    Wire.write(decToBcd(hour));
    Wire.write(0x01);                          // day of week (abaikan)
    Wire.write(decToBcd(day));
    Wire.write(decToBcd(month));
    Wire.write(decToBcd((uint8_t)(year - 2000)));
    uint8_t err = Wire.endTransmission();

    if (err == 0) {
        Serial.printf("✓ RTC set: %04d-%02d-%02d %02d:%02d:%02d\n",
                      year, month, day, hour, min, sec);
        return true;
    }
    Serial.printf("✗ RTC set failed (err=%d)\n", err);
    return false;
}

// ══════════════════════════════════════════════════════════════
// PRINT WAKTU
// ══════════════════════════════════════════════════════════════

void printRTCTime() {
    uint32_t epoch = getRTCEpoch();
    uint32_t t = epoch;
    uint8_t sec  = t % 60; t /= 60;
    uint8_t min  = t % 60; t /= 60;
    uint8_t hour = t % 24; t /= 24;

    uint16_t year = 1970;
    while (true) {
        bool leap = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
        uint16_t diy = leap ? 366 : 365;
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

    Serial.printf("  RTC: %04d-%02d-%02d %02d:%02d:%02d (epoch=%lu)\n",
                  year, month, day, hour, min, sec, epoch);
}
