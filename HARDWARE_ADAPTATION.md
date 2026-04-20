# Hardware Adaptation — Smart Mosque Ecosystem v1.1

Dokumen ini merangkum seluruh perubahan kode yang dilakukan untuk
menyesuaikan firmware dengan modul hardware aktual yang digunakan.

---

## Peta Modul Hardware

| Peran | Modul | Board |
|---|---|---|
| **Logic Master** | ESP32-S3 N16R8 IoT Logger RS485 Plus | ESP32-S3 |
| **Slave Kipas** | HwThinker 4-relay ESP32 | ESP32 WROOM-32 |
| **Slave Lampu** | Modbus RTU RS485 Relay 8Ch (5V) | — (tidak ada ESP32) |
| **Slave Sound** | Modbus RTU RS485 Relay 8Ch (5V) | — (tidak ada ESP32) |
| **Sensor Suhu** | SHT20 XY-MD02 RS485 | — (tidak ada ESP32) |

---

## Perubahan Per Firmware

### 1. `firmware/shared/SmartMosqueProtocol/SmartMosqueProtocol.h`
**Versi:** v2.0.0 → v2.1.0

| Yang Berubah | Sebelum | Sesudah |
|---|---|---|
| Protocol version | `2.0.0` | `2.1.0` |
| Command opcode | Sampai `0x08` (CMD_NACK) | Tambah `CMD_SENSOR_READ (0x09)` dan `CMD_SENSOR_DATA (0x0A)` |
| Device profiles | 4 profil (termasuk UNDEFINED) | Tambah `PROFILE_SENSOR_TEMP = 4` untuk SHT20 XY-MD02 |
| Struct baru | — | `SensorData` (temperature_x10, humidity_x10, sensor_id, status, timestamp) |
| validateConfig | Batas `PROFILE_SOUND_8CH` | Batas `PROFILE_SENSOR_TEMP` |

---

### 2. `firmware/logic_master/`
**Board:** ESP32-S3 N16R8 IoT Logger RS485 Plus

#### `include/logic_master.h`
| Yang Berubah | Sebelum | Sesudah |
|---|---|---|
| RS485 TX | GPIO17 | **GPIO15** (TX2 sesuai board) |
| RS485 RX | GPIO16 | **GPIO16** (tetap, RX2 sesuai board) |
| RS485 DE/RE | GPIO4 | **-1** (chip onboard auto DE/RE) |
| Display ILI9488 | GPIO12–15 | **Dihapus** (tidak ada display di board ini) |
| SD Card SS | GPIO21 | **GPIO10** (SPI sesuai board) |
| SD MOSI/SCK/MISO | — | Tambah: **MOSI=GPIO11, SCK=GPIO12, MISO=GPIO13** |
| RTC | Tidak ada | Tambah **DS3231M I2C: SDA=GPIO8, SCL=GPIO9** |
| Modbus addresses | Tidak ada | Tambah: sensor=0x01, lampu=0x02, sound=0x03 |
| Modbus registers | Tidak ada | Tambah: reg humidity=0x0001, temperature=0x0002 |
| Struct baru | — | `SensorReading` (temperature, humidity, valid, error_count) |
| Stats baru | — | `modbus_reads_ok`, `modbus_reads_err`, `relay_commands_sent` |
| Global baru | — | `sensorData`, `lastSensorRead` |
| Deklarasi fungsi baru | — | `initModbus()`, `readSensorSHT20()`, `setRelay8ch()`, `setAllRelays8ch()`, `pollSensorIfDue()`, `broadcastSensorData()`, `initRTC()`, `getRTCEpoch()`, `setRTCTime()`, `initSD()`, `logSensorData()`, `logEvent()`, `setModbusRelay8ch()`, `setAllModbusRelays8ch()` |

#### `src/main.cpp`
- Update deskripsi board dan pinout
- Hapus inisialisasi display ILI9488
- Hapus `pinMode(PIN_RS485_DE_RE, OUTPUT)` (chip onboard auto)
- Tambah `initRTC()`, `initSD()`, `initModbus()` di `setup()`
- Tambah Task 3: `pollSensorIfDue()` (baca sensor tiap 10 detik)
- Tambah Task 8: `logSensorData()` ke MicroSD (tiap 60 detik)
- `getEpochTime()` sekarang memanggil `getRTCEpoch()` (RTC DS3231M)
- `printStats()` tampilkan data suhu & kelembaban terkini

#### `src/display_driver.cpp`
- Seluruh isi diganti dengan **stub kosong** (Logic Master tidak punya display)

#### `src/heartbeat.cpp`
- Hapus `digitalWrite(PIN_RS485_DE_RE, HIGH/LOW)` pada `sendFrame()`

#### `src/frame_processing.cpp`
- Hapus `digitalWrite(RS485_DE_RE_PIN, HIGH/LOW)` pada `sendFrame()`
- Tambah case `CMD_SENSOR_DATA` di dispatcher (log saja, jarang diterima di master)

#### `src/command_router.cpp`
- `setFanSpeed()`: update relay mask jadi `0x0F` (4 relay) dan index sesuai HwThinker
  - values[0]=swing, values[1]=LOW, values[2]=MED, values[3]=HIGH
- Tambah fungsi baru:
  - `setModbusRelay8ch()` — kontrol satu relay di modul 8ch via Modbus RTU
  - `setAllModbusRelays8ch()` — kontrol semua relay di modul 8ch via Modbus RTU

#### File Baru
| File | Fungsi |
|---|---|
| `src/modbus_handler.cpp` | Modbus RTU master: baca SHT20, kontrol relay 8ch, broadcast sensor data |
| `src/rtc_handler.cpp` | RTC DS3231M via I2C (SDA=8, SCL=9), getEpoch, setTime |
| `src/sd_logger.cpp` | MicroSD logger: CSV sensor harian + event log |

#### `platformio.ini`
- Env: `esp32-s3-devkit` → `esp32-s3-iot-logger`
- Hapus dependencies: Adafruit GFX, ILI9341, TFT_eSPI, LovyanGFX
- Tambah flag: `-DARDUINO_USB_CDC_ON_BOOT=1`

---

### 3. `firmware/slave-node/`
**Board:** HwThinker 4-relay ESP32 (hanya untuk node KIPAS)

> **Penting:** Node Lampu dan Sound **tidak menggunakan firmware ini**.
> Kedua node tersebut menggunakan modul Modbus RTU RS485 Relay 8Ch yang
> dikontrol langsung oleh Logic Master via perintah Modbus RTU (FC05/FC0F).

#### `src/main.cpp`
| Yang Berubah | Sebelum | Sesudah |
|---|---|---|
| RS485 TX | GPIO17 | **GPIO19** (ModBUS TX label pada HwThinker) |
| RS485 RX | GPIO16 | **GPIO18** (ModBUS RX label pada HwThinker) |
| RS485 DE/RE | GPIO4 | **-1** (chip onboard auto DE/RE) |
| Relay pins (8ch) | GPIO12,13,14,15,25,26,27,32 | **GPIO23(swing),5(low),4(med),13(high)** |
| Mode detect | GPIO35 | **GPIO27** (Input3 pada HwThinker) |
| LED | GPIO2 | **GPIO15** |
| Input pins | — | Tambah: GPIO25, GPIO26, GPIO27, GPIO33 |
| Global baru | — | `latestTemperature`, `latestHumidity`, `sensorDataReceived` |
| Auto threshold | — | Tambah: `TEMP_THRESHOLD_LOW/MED/HIGH` (27/30/33°C) |
| Loop tambahan | — | Auto speed dari suhu (tiap 5 detik saat AUTO mode) |

#### `src/slave_functions.cpp`
| Yang Berubah | Sebelum | Sesudah |
|---|---|---|
| `initRelays()` | 8 relay | **4 relay** (GPIO23,5,4,13) |
| `setRelay()` | Batas index 7 | **Batas index 3** |
| `setAllRelays()` | 8 bit mask | **4 bit mask** |
| `getCurrentFanSpeed()` | Index 0,1,2 = LOW,MED,HIGH | **Index 1,2,3 = LOW,MED,HIGH** (index 0 = swing) |
| `isAutoMode()` | GPIO35 | **GPIO27** |
| `applyFanSpeed()` | Relay 0,1,2 = LOW,MED,HIGH | Relay 1,2,3 = LOW,MED,HIGH; relay 0 (swing) ikut ON/OFF |
| `validateRelayCommand()` | Cek values[0,1,2] | **Cek values[1,2,3]** (bukan values[0] karena swing) |
| `applyRelayCommand()` | 8ch langsung atau fan speed | **Fan speed + swing** |
| `sendStatusReport()` | 8 relay status | **4 relay + swing field + sensor data** |
| `sendFrame()` | `digitalWrite(DE/RE)` | **Dihapus** (chip onboard auto) |
| `printBootInfo()` | Info generik | Info spesifik HwThinker + pin mapping |
| Fungsi baru | — | `handleSensorData()`, `calcAutoSpeedFromTemp()` |

#### `src/frame_processing.cpp`
- Tambah case `CMD_SENSOR_DATA` → panggil `handleSensorData()`

#### `platformio.ini`
- Env: `esp32dev` → `slave-kipas`
- Hapus ModbusMaster library (tidak dibutuhkan di slave)
- Tambah flag: `-DPROFILE_FAN_4CH_BUILD`
- Tambah komentar penjelasan node lampu/sound (Modbus RTU, tanpa ESP32)

---

### 4. `firmware/display_master/`

#### `include/display_master.h`
| Yang Berubah | Sebelum | Sesudah |
|---|---|---|
| RS485 RX | GPIO16 | **GPIO18** |
| RS485 TX | GPIO17 | **GPIO19** |
| RS485 DE/RE | GPIO4 | **-1** (auto) |
| Struct baru | — | `SensorDisplayData` (temperature, humidity, valid, last_update_ms) |
| Global baru | — | `g_sensor_data` |
| Profile name | 3 profil | Tambah `PROFILE_SENSOR_TEMP → "SENSOR"` |

#### `src/rs485_comm.cpp`
- Tambah case `CMD_SENSOR_DATA`: parse `SensorData` payload, update `g_sensor_data`

#### `src/ui_dashboard.cpp`
- Tambah variabel static: `card_sensor`, `label_temp`, `label_humid`
- Tambah kartu sensor di UI (setelah kartu Sound)
- `uiDashboardUpdate()`: update label suhu & kelembaban tiap 500ms

#### `src/main.cpp`
- Tambah definisi global: `SensorDisplayData g_sensor_data = {0, 0, false, 0}`

---

## Arsitektur Bus RS485

```
[Logic Master ESP32-S3]
   TX2=GPIO15 ──────────────────────────────────────────────── RS485 A+
   RX2=GPIO16 ──────────────────────────────────────────────── RS485 B-
                              │
          ┌────────────────┬──┴───────────────┬────────────────┐
          │                │                  │                │
   [HwThinker]     [Relay 8ch]        [Relay 8ch]      [SHT20 XY-MD02]
   Slave Kipas     Node Lampu         Node Sound        Sensor Suhu
   ID: 1-N         Modbus addr:0x02   Modbus addr:0x03  Modbus addr:0x01
   TX=GPIO19        (tanpa ESP32)      (tanpa ESP32)     (tanpa ESP32)
   RX=GPIO18
   Relay:
     GPIO23=swing
     GPIO5=low
     GPIO4=med
     GPIO13=high
```

**Protokol di bus yang sama:**
- Frame Smart Mosque (custom) → untuk node HwThinker (kipas) + Display Master
- Modbus RTU → untuk sensor SHT20, relay lampu, relay sound
- Logic Master mengelola keduanya: tidak kirim Modbus saat menunggu frame custom

---

## File Baru yang Ditambahkan

```
firmware/logic_master/src/
├── modbus_handler.cpp   ← BARU: Modbus RTU master (sensor + relay 8ch)
├── rtc_handler.cpp      ← BARU: RTC DS3231M via I2C
└── sd_logger.cpp        ← BARU: MicroSD logging (CSV + event log)

HARDWARE_ADAPTATION.md   ← BARU: Dokumen ini
```

---

## Wiring Cepat

### Logic Master (ESP32-S3 IoT Logger RS485 Plus)
| Pin Board | Fungsi |
|---|---|
| A+ (RS485) | Bus RS485+ ke semua node |
| B- (RS485) | Bus RS485- ke semua node |
| SDA (GPIO8) | DS3231M SDA |
| SCL (GPIO9) | DS3231M SCL |
| SS=10, MOSI=11, SCK=12, MISO=13 | MicroSD SPI |
| DC 9-24V input | Sumber daya board |
| 5V output (max 3A) | Bisa untuk supply sensor/relay |

### Slave Kipas (HwThinker 4-relay ESP32)
| Pin Board | Relay | Fungsi |
|---|---|---|
| GPIO23 | Relay 1 | Swing kipas |
| GPIO5 | Relay 2 | Speed LOW |
| GPIO4 | Relay 3 | Speed MED |
| GPIO13 | Relay 4 | Speed HIGH |
| GPIO27 (Input3) | — | Saklar AUTO/MANUAL (HIGH=AUTO) |

### Sensor SHT20 XY-MD02
| Terminal | Hubungkan ke |
|---|---|
| A+ | RS485 bus A+ |
| B- | RS485 bus B- |
| + (power) | 5V–30V |
| - (GND) | GND |
| Modbus addr | 0x01 (default factory) |

### Relay 8ch Lampu (Modbus RTU RS485)
| Terminal | Hubungkan ke |
|---|---|
| A+ | RS485 bus A+ |
| B- | RS485 bus B- |
| VCC | 5V |
| GND | GND |
| Modbus addr | Set ke **0x02** via DIP switch/config |

### Relay 8ch Sound (Modbus RTU RS485)
| Terminal | Hubungkan ke |
|---|---|
| A+ | RS485 bus A+ |
| B- | RS485 bus B- |
| VCC | 5V |
| GND | GND |
| Modbus addr | Set ke **0x03** via DIP switch/config |
