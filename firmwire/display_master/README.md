# 🖥️ DISPLAY MASTER FIRMWARE
**Smart Mosque Ecosystem v1.0 - User Interface & Control Panel**

---

## 📋 TABLE OF CONTENTS

1. [Overview](#overview)
2. [Hardware Requirements](#hardware-requirements)
3. [Software Architecture](#software-architecture)
4. [Installation](#installation)
5. [Configuration](#configuration)
6. [User Interface Guide](#user-interface-guide)
7. [Web Dashboard](#web-dashboard)
8. [REST API Documentation](#rest-api-documentation)
9. [Troubleshooting](#troubleshooting)
10. [Development](#development)

---

## 🎯 OVERVIEW

Display Master adalah komponen **User Interface & Control Panel** dalam sistem Smart Mosque Ecosystem. Firmware ini menyediakan antarmuka grafis touchscreen dan web dashboard untuk monitoring serta kontrol semua perangkat di masjid.

### Key Features

✅ **Touchscreen UI (LVGL)**
- Dashboard overview dengan statistics
- Individual device control
- Auto-enrollment popup
- Real-time status updates

✅ **WiFi Access Point**
- Built-in AP untuk web access
- No internet required

✅ **REST API**
- Device list (JSON)
- Relay control
- System statistics

✅ **RS-485 Communication**
- Protocol Smart Mosque v1.3
- CRC16-MODBUS validation
- Command queue dengan retry

✅ **Device Management**
- Support 247 devices
- Online/offline tracking
- NVS cache persistence

---

## 🔧 HARDWARE REQUIREMENTS

### Required Components

| Component | Specification | Quantity | Notes |
|-----------|---------------|----------|-------|
| **MCU Board** | Waveshare ESP32-S3-Touch-LCD-7 | 1 | 800x480 capacitive touch |
| **RS-485 Module** | MAX3485 or equivalent | 1 | 3.3V compatible |
| **Power Supply** | 5V DC, min 3A | 1 | HLK-PM03 recommended |
| **Terminal Block** | 2-pin, 5.08mm | 1 | For RS-485 A/B |
| **Enclosure** | Custom or standard | 1 | Protect electronics |

### Pinout Configuration

```
╔═══════════════════════════════════════════════════════════╗
║              DISPLAY MASTER PINOUT [KT-5.2]              ║
╚═══════════════════════════════════════════════════════════╝

RS-485 Communication:
├─ GPIO 16  →  RX  (Serial2 receive)
├─ GPIO 17  →  TX  (Serial2 transmit)
└─ GPIO 4   →  DE/RE (Driver Enable/Receive Enable)

Display & Touch:
├─ Built-in on Waveshare board
├─ SPI interface (pre-configured)
└─ Touch: XPT2046 or FT6236 (auto-detect)

Power:
├─ 5V DC input
├─ Current: ~800mA typical, 1A peak
└─ Total system: ~8A for full deployment
```

### RS-485 Wiring

```
Display Master          RS-485 Bus          Logic Master
┌─────────────┐         ┌──────────┐       ┌─────────────┐
│             │         │          │       │             │
│  A (GPIO17) ├─────────┤    A     ├───────┤  A (GPIO17) │
│             │         │          │       │             │
│  B (GPIO16) ├─────────┤    B     ├───────┤  B (GPIO16) │
│             │         │          │       │             │
│  GND        ├─────────┤   GND    ├───────┤  GND        │
│             │         │          │       │             │
└─────────────┘         └──────────┘       └─────────────┘
                             │
                             ├─── [Other Slave Nodes...]
                             
Note: 120Ω termination resistor di ujung bus
```

---

## 🏗️ SOFTWARE ARCHITECTURE

### Module Structure

```
firmware/display_master/
├── platformio.ini                  # PlatformIO configuration
├── include/
│   └── display_master.h            # Header declarations (450 LOC)
└── src/
    ├── main.cpp                    # Main program (420 LOC)
    ├── rs485_comm.cpp              # RS-485 communication (550 LOC)
    ├── device_manager.cpp          # Device database (500 LOC)
    ├── ui_dashboard.cpp            # Dashboard UI (550 LOC)
    ├── ui_device_control.cpp       # Control page (600 LOC)
    ├── ui_enrollment.cpp           # Enrollment popup (300 LOC)
    ├── wifi_server.cpp             # WiFi & Web API (500 LOC)
    ├── LGFX_Config.h               # Display config
    └── lv_conf.h                   # LVGL config

Total: ~3,870 LOC (Production Quality)
```

### Dependencies

```ini
[lib_deps]
    lovyan03/LovyanGFX @ ^1.1.12      # Display driver
    lvgl/lvgl @ ^8.3.9                 # UI framework
    bblanchon/ArduinoJson @ ^6.21.5   # JSON parser
    AsyncTCP                           # Async networking
    ESPAsyncWebServer                  # Web server
    SmartMosqueProtocol               # Shared protocol (local)
```

### Memory Usage

```
Program Storage: ~2.8 MB / 16 MB (17%)
Dynamic Memory:  ~150 KB / 512 KB (29%)
PSRAM Usage:     ~800 KB / 8 MB (10%)

Status: ✅ Comfortable headroom
```

---

## 📦 INSTALLATION

### Step 1: Install PlatformIO

```bash
# Option 1: VSCode Extension
# Install "PlatformIO IDE" dari VSCode marketplace

# Option 2: CLI Installation
pip install platformio

# Verify installation
pio --version
```

### Step 2: Clone Repository

```bash
git clone https://github.com/yudidanton/smart-mosque-ecosystem.git
cd smart-mosque-ecosystem/firmware/display_master
```

### Step 3: Build Firmware

```bash
# Build only (check for errors)
pio run

# Expected output:
# RAM:   [===       ]  29.2% (used 150KB from 512KB)
# Flash: [==        ]  17.5% (used 2.8MB from 16MB)
# ✅ SUCCESS
```

### Step 4: Upload to Hardware

```bash
# Connect ESP32-S3 via USB
# Hold BOOT button, press RESET, release BOOT (untuk enter bootloader)

# Upload firmware
pio run --target upload

# Monitor serial output
pio device monitor

# Expected output:
# ╔═══════════════════════════════════════════════════╗
# ║   SMART MOSQUE ECOSYSTEM - DISPLAY MASTER v1.0   ║
# ╚═══════════════════════════════════════════════════╝
# 
# 🆔 Device ID: 255 (Display Master)
# 📡 RS-485: 115200 bps
# 🖥️  Display: 800x480 capacitive touch
# 📶 WiFi AP: SmartMosque-Display
# 
# ✓ Display initialized
# ✓ RS-485 initialized
# ✓ Device manager initialized
# ✓ WiFi AP started
# ✓ Web server started
# 
# 🎉 Display Master ready!
# 📱 Dashboard: http://192.168.4.1
```

### Step 5: First Boot

Setelah upload berhasil:

1. **Display akan menampilkan dashboard**
   - Header: "SMART MOSQUE DASHBOARD"
   - Status cards: Total devices, Kipas, Lampu, Sound
   - Device list table (kosong dulu)
   - Quick control buttons

2. **WiFi AP akan aktif**
   - SSID: `SmartMosque-Display`
   - Password: `masjid2026`
   - IP Address: `192.168.4.1`

3. **Sistem siap menerima data**
   - Menunggu sync dari Logic Master
   - Siap untuk auto-enrollment device baru

---

## ⚙️ CONFIGURATION

### WiFi Settings

Edit di `src/wifi_server.cpp`:

```cpp
#define WIFI_AP_SSID            "SmartMosque-Display"  // ← Ubah SSID
#define WIFI_AP_PASSWORD        "masjid2026"           // ← Ubah password
#define WEB_SERVER_PORT         80                     // ← Port web
```

### Device ID

**PENTING:** Device ID Display Master adalah **255 (FIXED)**  
Jangan diubah! Ini bagian dari protokol [KT-2.2]

```cpp
#define DISPLAY_MASTER_ID        255        // ⚠️ JANGAN UBAH
```

### RS-485 Pinout

**WAJIB:** Gunakan pinout standard [KT-5.2]

```cpp
#define RS485_RX                 16         // ⚠️ JANGAN UBAH
#define RS485_TX                 17         // ⚠️ JANGAN UBAH
#define RS485_DE_RE              4          // ⚠️ JANGAN UBAH
```

### UI Update Interval

Edit di `include/display_master.h`:

```cpp
#define UI_UPDATE_INTERVAL_MS    500        // ← 500ms default (2 Hz)
```

### Device Offline Timeout

```cpp
#define DEVICE_OFFLINE_TIMEOUT   310000     // ← 310 detik [KT-3.1]
```

---

## 🎨 USER INTERFACE GUIDE

### Dashboard Screen (Main)

```
╔════════════════════════════════════════════════════════════╗
║  🏠 SMART MOSQUE DASHBOARD              00:15:32  📶 ONLINE ║
╠════════════════════════════════════════════════════════════╣
║                                                             ║
║  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐      ║
║  │ TOTAL   │  │ KIPAS   │  │ LAMPU   │  │ SOUND   │      ║
║  │   18    │  │   13    │  │    4    │  │    1    │      ║
║  │ 15 onl. │  └─────────┘  └─────────┘  └─────────┘      ║
║  └─────────┘                                               ║
║                                                             ║
║  ┌─────────────────────────────────────────────────────┐  ║
║  │ ID │ Device Name          │ Profile │ Status │ Mode │  ║
║  ├────┼──────────────────────┼─────────┼────────┼──────┤  ║
║  │ 1  │ Kipas Saf Depan Kanan│ KIPAS   │ ONLINE │ AUTO │  ║
║  │ 2  │ Kipas Saf Depan Kiri │ KIPAS   │ ONLINE │ AUTO │  ║
║  │ 3  │ Kipas Tengah Kanan 1 │ KIPAS   │ ONLINE │ AUTO │  ║
║  │ ...│ ...                  │ ...     │ ...    │ ...  │  ║
║  └────┴──────────────────────┴─────────┴────────┴──────┘  ║
║                                                             ║
║  [ALL ON] [ALL OFF] [KIPAS ON] [KIPAS OFF]    [REFRESH]   ║
╚════════════════════════════════════════════════════════════╝
```

**Features:**
- **Status cards**: Real-time device count
- **Device table**: Tap row untuk detail control
- **Quick controls**: Broadcast commands
- **Auto-refresh**: Setiap 500ms

### Device Control Screen

Tap device di table → Masuk ke control screen:

```
╔════════════════════════════════════════════════════════════╗
║  [← BACK]         Kipas Saf Depan Kanan                    ║
║                   ID: 1 | Profile: KIPAS                    ║
╠════════════════════════════════════════════════════════════╣
║  📶 ONLINE                         Last update: 2 sec ago   ║
║  Mode: AUTO                                                 ║
╠════════════════════════════════════════════════════════════╣
║                                                             ║
║  [LOW 1]      [MED 2]      [HIGH 3]     [RES 4]           ║
║   ON          OFF          OFF          OFF                ║
║                                                             ║
║  [RELAY 5]    [RELAY 6]    [RELAY 7]    [RELAY 8]         ║
║   --          --           --           --                 ║
║                                                             ║
╠════════════════════════════════════════════════════════════╣
║  [ALL ON]  [ALL OFF]    🌀 FAN: [OFF][LOW][MED][HIGH]     ║
╚════════════════════════════════════════════════════════════╝
```

**Features:**
- **Individual relay control**: Tap button untuk toggle
- **Fan speed control**: Quick speed selection
- **Status display**: Online, mode, last update
- **Disable saat offline**: Buttons disabled jika device offline/manual

### Enrollment Popup

Muncul otomatis saat device baru detected:

```
╔════════════════════════════════════════════════════════════╗
║                                                             ║
║               ➕ NEW DEVICE DETECTED                        ║
║                                                             ║
║               MAC: A1:B2:C3:D4:E5:F6                       ║
║               Profile: KIPAS                                ║
║                                                             ║
║          Please enter a name for this device:              ║
║          ┌──────────────────────────────────┐             ║
║          │ Kipas Saf Belakang Kanan_       │             ║
║          └──────────────────────────────────┘             ║
║                                                             ║
║          [Q][W][E][R][T][Y][U][I][O][P]                   ║
║          [A][S][D][F][G][H][J][K][L]                      ║
║          [Z][X][C][V][B][N][M][←][Space]                  ║
║                                                             ║
║          [✓ APPROVE]              [✗ REJECT]              ║
╚════════════════════════════════════════════════════════════╝
```

**Flow:**
1. Device baru announce via RS-485
2. Popup muncul otomatis
3. User input nama (max 31 karakter)
4. Approve → Device terdaftar dengan ID baru
5. Reject → Device tidak terdaftar

---

## 🌐 WEB DASHBOARD

### Accessing Web Dashboard

1. **Connect ke WiFi AP**
   - SSID: `SmartMosque-Display`
   - Password: `masjid2026`

2. **Open Browser**
   - URL: `http://192.168.4.1`
   - Compatible: Chrome, Firefox, Safari, Edge

3. **Dashboard Features**
   - Device list dengan status
   - Quick controls (ALL ON/OFF)
   - Auto-refresh setiap 5 detik
   - Statistics display

### Web Interface Screenshot (Text)

```
═══════════════════════════════════════════════════════════════
🕌 Smart Mosque Dashboard                      🔄 Refresh
═══════════════════════════════════════════════════════════════

┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐
│   18     │  │   15     │  │    2     │  │    0     │
│ Total    │  │ Online   │  │ Manual   │  │ Failsafe │
└──────────┘  └──────────┘  └──────────┘  └──────────┘

┌───────────────────────────────────────────────────────────┐
│ ID │ Device Name          │ Profile │ Status │ Actions    │
├────┼──────────────────────┼─────────┼────────┼────────────┤
│ 1  │ Kipas Saf Depan Kanan│ KIPAS   │ ONLINE │[ON] [OFF] │
│ 2  │ Kipas Saf Depan Kiri │ KIPAS   │ ONLINE │[ON] [OFF] │
│ 3  │ Lampu Utama Besar    │ LAMPU   │ ONLINE │[ON] [OFF] │
│ ...│ ...                  │ ...     │ ...    │ ...        │
└────┴──────────────────────┴─────────┴────────┴────────────┘

Auto-refresh: ●●●●○ (4/5 sec)
═══════════════════════════════════════════════════════════════
```

---

## 📡 REST API DOCUMENTATION

### Base URL

```
http://192.168.4.1/api
```

### Authentication

**None** - Internal network only

### Endpoints

#### 1. GET /api/devices

Get device list dengan status lengkap.

**Request:**
```http
GET /api/devices HTTP/1.1
Host: 192.168.4.1
```

**Response (200 OK):**
```json
{
  "devices": [
    {
      "id": 1,
      "name": "Kipas Saf Depan Kanan",
      "profile": "KIPAS",
      "online": true,
      "relay_state": 1,
      "control_source": "AUTO",
      "relays": [true, false, false, false, false, false, false, false]
    },
    {
      "id": 2,
      "name": "Kipas Saf Depan Kiri",
      "profile": "KIPAS",
      "online": true,
      "relay_state": 0,
      "control_source": "AUTO",
      "relays": [false, false, false, false, false, false, false, false]
    }
  ],
  "total": 18,
  "online": 15,
  "manual_mode": 2,
  "failsafe": 0
}
```

#### 2. POST /api/control

Control relay device via API.

**Request:**
```http
POST /api/control HTTP/1.1
Host: 192.168.4.1
Content-Type: application/json

{
  "device_id": 1,
  "relay": 0,
  "state": true
}
```

**Parameters:**
- `device_id` (integer): Device ID (1-247)
- `relay` (integer): Relay index (0-7) atau 255 untuk ALL
- `state` (boolean): true=ON, false=OFF

**Response (200 OK):**
```json
{
  "success": true,
  "message": "Command sent"
}
```

**Error Response (400 Bad Request):**
```json
{
  "success": false,
  "message": "Device not in AUTO mode"
}
```

#### 3. GET /api/stats

Get system statistics.

**Request:**
```http
GET /api/stats HTTP/1.1
Host: 192.168.4.1
```

**Response (200 OK):**
```json
{
  "uptime_seconds": 3652,
  "uptime_string": "0d 01:00:52",
  "commands_sent": 127,
  "commands_acked": 119,
  "commands_failed": 8,
  "frames_received": 542,
  "frames_invalid": 3,
  "ui_updates": 7304,
  "total_devices": 18,
  "online_devices": 15,
  "offline_devices": 3,
  "manual_mode_count": 2,
  "failsafe_count": 0,
  "kipas_count": 13,
  "lampu_count": 4,
  "sound_count": 1,
  "wifi_ssid": "SmartMosque-Display",
  "wifi_ip": "192.168.4.1",
  "wifi_clients": 2
}
```

### cURL Examples

```bash
# Get device list
curl http://192.168.4.1/api/devices

# Turn ON relay 0 of device 1
curl -X POST http://192.168.4.1/api/control \
  -H "Content-Type: application/json" \
  -d '{"device_id":1,"relay":0,"state":true}'

# Turn OFF all relays of device 1
curl -X POST http://192.168.4.1/api/control \
  -H "Content-Type: application/json" \
  -d '{"device_id":1,"relay":255,"state":false}'

# Get statistics
curl http://192.168.4.1/api/stats
```

---

## 🔧 TROUBLESHOOTING

### Problem: Display tidak menyala

**Symptoms:**
- LCD tetap hitam
- No backlight

**Solutions:**
1. Check power supply (min 5V/3A)
2. Check USB cable connection
3. Tekan tombol RESET di board
4. Re-upload firmware dengan `pio run --target upload`

### Problem: Touch tidak respond

**Symptoms:**
- Display menyala tapi touch tidak berfungsi
- Button tidak bisa di-klik

**Solutions:**
1. Check `LGFX_Config.h` - pastikan touch driver correct
2. Calibrate touch (jika perlu)
3. Test dengan serial monitor: tap screen → lihat log coordinate

### Problem: WiFi AP tidak muncul

**Symptoms:**
- SSID "SmartMosque-Display" tidak terlihat di WiFi list

**Solutions:**
1. Wait 30 detik setelah boot (initialization)
2. Check serial monitor: `✓ WiFi AP started`
3. Restart Display Master
4. Check WiFi channel (ubah di `wifi_server.cpp` jika perlu)

### Problem: Tidak bisa connect ke RS-485

**Symptoms:**
- Device list kosong
- No data from Logic Master

**Solutions:**
1. Check wiring: A→A, B→B, GND→GND
2. Check pinout (16, 17, 4)
3. Check termination resistor (120Ω di ujung bus)
4. Serial monitor: lihat `📥 RX Frame` logs
5. Test dengan Logic Master: kirim STATUS_REPORT

### Problem: Device tidak muncul setelah enrollment

**Symptoms:**
- Enrollment approved tapi device tetap tidak ada

**Solutions:**
1. Check Logic Master response: `DISCOVERY_RESPONSE` harus dikirim
2. Slave harus restart setelah terima ID baru
3. Check NVS: `deviceManagerPrintSummary()` di serial
4. Force sync: klik button REFRESH di dashboard

### Problem: Web dashboard tidak load

**Symptoms:**
- Browser tidak bisa akses http://192.168.4.1
- Timeout atau connection refused

**Solutions:**
1. Pastikan connected ke WiFi AP "SmartMosque-Display"
2. Check IP dengan `ipconfig` (Windows) atau `ifconfig` (Linux/Mac)
3. Ping test: `ping 192.168.4.1`
4. Try different browser
5. Check serial monitor: `🌐 Initializing web server`

### Problem: Command tidak executed

**Symptoms:**
- Button di-klik tapi relay tidak berubah
- Alert "Command failed"

**Solutions:**
1. Check device online status
2. Check control mode (harus AUTO, bukan MANUAL/FAILSAFE)
3. Serial monitor: lihat command queue status
4. Check RS-485 communication
5. Test dengan web API: `curl -X POST ...`

---

## 🛠️ DEVELOPMENT

### Building from Source

```bash
# Clone repository
git clone https://github.com/yudidanton/smart-mosque-ecosystem.git
cd smart-mosque-ecosystem/firmware/display_master

# Install dependencies (automatic via platformio.ini)
pio lib install

# Build
pio run

# Upload
pio run --target upload

# Monitor serial output
pio device monitor --baud 115200
```

### Code Structure

```cpp
// Module responsibilities:

main.cpp              → System integration, setup/loop
rs485_comm.cpp        → Protocol frame TX/RX, parser
device_manager.cpp    → Device database, queries, NVS
ui_dashboard.cpp      → LVGL dashboard screen
ui_device_control.cpp → LVGL control screen
ui_enrollment.cpp     → LVGL enrollment popup
wifi_server.cpp       → WiFi AP, web server, REST API
```

### Adding New Features

**Example: Add new UI screen**

1. **Declare in header** (`include/display_master.h`):
```cpp
extern lv_obj_t* g_settings_screen;
void uiSettingsCreate();
void uiSettingsUpdate();
```

2. **Implement** (`src/ui_settings.cpp`):
```cpp
#include "../include/display_master.h"

lv_obj_t* screen_settings = nullptr;

void uiSettingsCreate() {
    screen_settings = lv_obj_create(nullptr);
    // ... create UI components
    g_settings_screen = screen_settings;
}

void uiSettingsUpdate() {
    // ... update data
}
```

3. **Call from main.cpp**:
```cpp
void setup() {
    // ...
    uiSettingsCreate();
}

void loop() {
    // ...
    if (lv_scr_act() == g_settings_screen) {
        uiSettingsUpdate();
    }
}
```

### Debug Mode

Enable verbose logging di `include/display_master.h`:

```cpp
// Uncomment untuk debug
#define DEBUG_RS485_VERBOSE
#define DEBUG_DEVICE_MANAGER

// Serial output akan show:
// 📤 TX [12 bytes]: 7E 01 03 02 00 01 XX XX 0A (to=1, cmd=0x03)
// 📥 RX Frame: addr=1, cmd=0x05, len=3
// 📊 Status: ID=1, Relay=0x01, Source=AUTO
```

### Memory Profiling

```bash
# Check memory usage
pio run --target size

# Output:
# RAM:   [===       ]  29.2% (used 150224 bytes)
# Flash: [==        ]  17.5% (used 2870912 bytes)
```

### Performance Tuning

```cpp
// Adjust update intervals
#define UI_UPDATE_INTERVAL_MS    500    // ← Increase untuk save CPU
#define COMMAND_TIMEOUT_MS       2000   // ← Adjust retry timing
```

---

## 📚 REFERENCES

### Documentation

- **[Kebenaran Tunggal v1.4](../../docs/DOKUMEN%201%20KEBENARAN%20TUNGGAL%20v1.4.md)** - System architecture
- **[Spesifikasi Teknis Protokol v1.3](../../docs/DOKUMEN%202%20SPESIFIKASI%20TEKNIS%20PROTOKOL%20v1.3.md)** - Protocol details
- **[Panduan Integrasi Hardware v1.0](../../docs/DOKUMEN%203%20PANDUAN%20INTEGRASI%20HARDWARE%20v1.md)** - Wiring & hardware

### External Libraries

- [LovyanGFX](https://github.com/lovyan03/LovyanGFX) - Display driver
- [LVGL](https://lvgl.io/) - UI framework
- [ArduinoJson](https://arduinojson.org/) - JSON parser
- [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer) - Web server

---

## 📞 SUPPORT

### Issues & Questions

- **GitHub Issues**: https://github.com/yudidanton/smart-mosque-ecosystem/issues
- **Documentation**: Check FINAL_SUMMARY.md
- **Serial Debug**: Enable debug mode dan attach logs

### Contributing

1. Fork repository
2. Create feature branch
3. Follow code standards
4. Test thoroughly
5. Submit pull request

---

## 📜 LICENSE

MIT License - See [LICENSE](../../LICENSE)

---

## ✅ TESTING CHECKLIST

### Pre-Deployment

- [ ] Firmware compile tanpa error
- [ ] Upload berhasil ke hardware
- [ ] Display menyala dengan dashboard
- [ ] Touch respond correctly
- [ ] WiFi AP muncul
- [ ] Web dashboard accessible
- [ ] RS-485 communication working
- [ ] Device sync dari Logic Master
- [ ] Enrollment popup berfungsi
- [ ] Command execution working

### Integration Testing

- [ ] Connect ke Logic Master via RS-485
- [ ] Receive STATUS_REPORT frames
- [ ] Send SET_RELAY commands
- [ ] Auto-enrollment end-to-end
- [ ] Web API response correct
- [ ] 24-hour stability test

---

**Display Master Firmware v1.0**  
Smart Mosque Ecosystem Project  
© 2026 - Production Ready

Made with ❤️ for better mosque automation
