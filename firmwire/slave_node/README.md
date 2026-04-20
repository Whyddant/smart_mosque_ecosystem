# 🕋 Slave Node Firmware v2.0

**Complete firmware implementation for Smart Mosque Ecosystem slave nodes**

## 📋 Overview

Firmware ini mendukung **One Firmware untuk Semua** konsep dimana satu firmware yang sama dapat di-flash ke semua slave node (FAN_4CH, LIGHT_8CH, SOUND_8CH). Profile dan konfigurasi ditentukan saat auto-enrollment.

## ✅ Compliance

- ✓ **Kebenaran Tunggal v1.4** (KT) - Filosofi dan arsitektur sistem
- ✓ **Spesifikasi Teknis Protokol v1.3** (STP) - Detail protokol RS-485
- ✓ **Panduan Integrasi Hardware v1.0** (HIG) - Pinout dan wiring

## 🚀 Features

### Core Features
- ✅ **Auto-Enrollment** - MAC-based registration tanpa konfigurasi manual
- ✅ **One Firmware** - Satu firmware untuk semua profile (FAN/LIGHT/SOUND)
- ✅ **Fail-Safe** - Proteksi otomatis saat heartbeat hilang >300 detik
- ✅ **Manual Override** - Support DPDT mode switch untuk kipas
- ✅ **Relay Interlock** - Prevent multiple fan speeds simultaneously
- ✅ **NVS Management** - Compare-before-write untuk hemat flash cycles
- ✅ **CRC Validation** - CRC16-MODBUS untuk integritas data

### Protocol Features
- ✅ Complete RS-485 frame parser dengan state machine
- ✅ All command handlers (HEARTBEAT, SET_RELAY, DISCOVERY, etc.)
- ✅ JSON payload support untuk discovery dan status
- ✅ Error handling dengan NACK responses
- ✅ Status reporting (periodic dan event-triggered)

### Safety Features
- ✅ Dead-time (200ms) untuk fan speed switching
- ✅ Fail-safe per profile (LIGHT=ALL ON, FAN/SOUND=ALL OFF)
- ✅ Manual mode rejection untuk remote commands
- ✅ Buffer overflow protection
- ✅ CRC error detection dan recovery

## 📂 File Structure

```
firmware/slave-node/
├── platformio.ini           # PlatformIO configuration
├── README.md               # This file
└── src/
    ├── main.cpp            # Core structure & configuration (Part 1/3)
    ├── slave_functions.cpp # Hardware control & state machine (Part 2/3)
    └── frame_processing.cpp# Frame parser & command handlers (Part 3/3)
```

## 🔧 Hardware Requirements

### Minimum Components
- **ESP32 DevKit V1** (4MB flash, 520KB RAM)
- **MAX3485** RS-485 transceiver
- **SSR G3MB-202P** (4 atau 8 channel)
- **HLK-PM03** 5V/3A power supply
- **DPDT switch** (kipas only, untuk mode AUTO/MANUAL)

### Pinout (FIXED - DO NOT CHANGE)

```cpp
// RS-485 Communication
#define PIN_RS485_RX    16  // Hardware Serial2 RX
#define PIN_RS485_TX    17  // Hardware Serial2 TX
#define PIN_RS485_DE_RE  4  // Driver Enable/Receive Enable

// Relay Outputs (8 channels)
const uint8_t RELAY_PINS[8] = {12, 13, 14, 15, 25, 26, 27, 32};

// Mode Detection (Kipas only)
#define PIN_MODE_DETECT 35  // HIGH=AUTO, LOW=MANUAL

// Status LED
#define PIN_LED_STATUS   2  // Onboard LED
```

## 📦 Dependencies

Dalam `platformio.ini`:

```ini
lib_deps =
    bblanchon/ArduinoJson @ ^6.21.3

lib_extra_dirs = ../shared
```

## 🔨 Compilation Instructions

### Option A: Single File (Recommended)

1. Gabungkan semua 3 file menjadi satu:
   ```bash
   cat src/main.cpp src/slave_functions.cpp src/frame_processing.cpp > src/main_complete.cpp
   ```

2. Edit `platformio.ini` untuk build dari file gabungan

3. Build:
   ```bash
   pio run
   ```

### Option B: Multi-File

1. File structure sudah OK (3 files terpisah)

2. Build langsung:
   ```bash
   pio run
   ```

3. PlatformIO otomatis compile semua .cpp di src/

## 📤 Flashing

### Via USB Serial
```bash
pio run --target upload
```

### Manual Upload
```bash
pio run --target upload --upload-port /dev/ttyUSB0
```

### Monitor Serial
```bash
pio device monitor --baud 115200
```

## ⚙️ Configuration

### First Boot (No Config)
Saat pertama kali boot tanpa konfigurasi:
1. Device masuk **DISCOVERY mode**
2. LED blink biru (500ms interval)
3. Broadcast DISCOVERY_ANNOUNCE setiap 1-3 detik
4. Tunggu DISCOVERY_RESPONSE dari Master
5. Setelah terima ID, save ke NVS dan restart

### Normal Boot (With Config)
Saat boot dengan konfigurasi valid:
1. Load config dari NVS
2. Validate CRC dan MAC address
3. Masuk **OPERATIONAL mode**
4. Mulai monitor heartbeat

### Factory Reset
Untuk reset ke factory default:
- Master kirim command `FORCE_RESET` via RS-485
- Atau manual: erase NVS via serial monitor

## 🎯 Profile Behaviors

### FAN_4CH (4-Channel Fan)
- **Relays**:
  - Relay 0: LOW speed
  - Relay 1: MED speed
  - Relay 2: HIGH speed
  - Relay 3: Reserved
- **Interlock**: Only ONE speed ON at a time
- **Dead-time**: 200ms between speed changes
- **Mode Switch**: DPDT untuk AUTO/MANUAL
- **Fail-Safe**: ALL OFF (prevent overheating)

### LIGHT_8CH (8-Channel Lighting)
- **Relays**: 8 independent channels
- **No interlock** - all can be ON simultaneously
- **Parallel wiring** dengan saklar manual (OR logic)
- **Fail-Safe**: ALL ON (safety visibility)

### SOUND_8CH (8-Channel Audio)
- **Relays**: 8 audio zones
- **No interlock** - all can be ON simultaneously
- **Parallel wiring** dengan saklar manual
- **Fail-Safe**: ALL OFF (prevent noise)

## 🔍 Testing Checklist

### Basic Functionality
- [ ] Discovery announce broadcasts correctly
- [ ] MAC address extracted properly
- [ ] Config saved to NVS
- [ ] Config loaded on reboot
- [ ] Device ID assigned correctly

### Protocol Compliance
- [ ] Heartbeat updates timestamp
- [ ] Fail-safe triggers at exactly 300s
- [ ] CRC validation works (reject invalid frames)
- [ ] All commands parsed correctly
- [ ] ACK/NACK sent when appropriate

### Hardware Control
- [ ] All 8 relays can be controlled independently
- [ ] Fan interlock prevents multiple speeds
- [ ] Dead-time implemented (200ms)
- [ ] Manual mode rejects RS-485 commands
- [ ] LED indicators match state

### Edge Cases
- [ ] Buffer overflow protection works
- [ ] Parser recovers from corrupted frames
- [ ] NVS write fails gracefully
- [ ] Factory reset works
- [ ] Rapid command sequence handled

## 📊 Memory Usage

### Flash Memory
- **Code**: ~120 KB
- **NVS**: ~4 KB (configuration)
- **Total**: ~124 KB (3% of 4MB flash)

### RAM Usage
- **Global variables**: ~2 KB
- **Stack**: ~4 KB
- **Heap (JSON)**: ~2 KB
- **Total**: ~8 KB (1.5% of 520 KB RAM)

## 🐛 Troubleshooting

### Device Stuck in Discovery Mode
**Symptoms**: LED blink biru terus menerus

**Diagnosa**:
1. Check RS-485 wiring (A-A, B-B, GND-GND)
2. Verify Master is listening
3. Check MAC address tidak duplikat
4. Monitor serial untuk frame transmission

**Solution**: Verify wiring, restart Master

### Fail-Safe Triggering Randomly
**Symptoms**: LED merah, relays masuk fail-safe mode

**Diagnosa**:
1. Check Master heartbeat is sending (60s interval)
2. Verify broadcast address (0x00) used
3. Check bus overload (too many commands)

**Solution**: Fix Master heartbeat timer, reduce bus traffic

### Commands Rejected in AUTO Mode
**Symptoms**: NACK error 0x04 received

**Diagnosa**:
1. Check GPIO35 state (should be HIGH for AUTO)
2. Verify DPDT switch wiring
3. Test switch with multimeter

**Solution**: Fix switch wiring, add pull-up resistor

### CRC Errors Frequent
**Symptoms**: Logs show "CRC mismatch" repeatedly

**Diagnosa**:
1. Check RS-485 wiring quality
2. Verify termination (120Ω at both ends ONLY)
3. Measure bus voltage (2-5V differential)

**Solution**: Fix wiring, add/remove termination

## 📈 Performance Metrics

### Target Metrics
- **Command latency**: < 500 ms
- **Frame error rate**: < 0.1%
- **Discovery time**: < 10 seconds
- **Fail-safe accuracy**: ±1 second
- **Memory stability**: No leaks

### Monitoring
Enable debug output dengan menambahkan di `platformio.ini`:
```ini
build_flags = 
    -D DEBUG_PROTOCOL=1
```

## 🔮 Future Enhancements

### Planned
- [ ] Hardware watchdog timer
- [ ] Temperature monitoring (DHT22)
- [ ] Power monitoring (INA219)
- [ ] OTA firmware update
- [ ] SD card event logging

### Considered
- [ ] WiFi telemetry
- [ ] Web-based configuration
- [ ] Modbus RTU compatibility
- [ ] Multiple Master support

## 📝 Version History

### v2.0.0 (2026-01-02)
- ✅ Complete rewrite from v1.0
- ✅ Full KT v1.4 compliance
- ✅ Full STP v1.3 compliance
- ✅ NVS compare-before-write
- ✅ Complete frame parser
- ✅ All command handlers
- ✅ Profile-specific behaviors

### v1.0.0 (2025-12-26)
- Initial prototype
- Basic discovery mode
- Placeholder CRC

## 📄 License

MIT License (atau sesuai lisensi proyek)

## 👥 Contributors

- Smart Mosque Project Team
- Based on KT v1.4 and STP v1.3 specifications

## 📞 Support

Untuk issue dan pertanyaan:
- GitHub Issues: [Project Repository](https://github.com/yudidanton/smart-mosque-ecosystem)
- Documentation: `/docs` folder
- Email: [whyddant@gmail.com]

---

**Made with ❤️ for Smart Mosque Ecosystem**
