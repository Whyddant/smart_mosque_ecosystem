# 📘 DOKUMEN 1: KEBENARAN TUNGGAL v1.4

```
╔═══════════════════════════════════════════════════════════════╗
║                                                               ║
║     KEBENARAN TUNGGAL - SMART MOSQUE ECOSYSTEM v1.4          ║
║                                                               ║
║     Status: Final untuk Implementasi                          ║
║     Berdasarkan: Diskusi teknis 26-27 Desember 2025          ║
║     Prinsip: Auto-Enrollment, Fail-Safe, Dual Master,        ║
║              One Firmware, Manual Priority                    ║
║                                                               ║
╚═══════════════════════════════════════════════════════════════╝
```

---

## 🎯 BAGIAN 1: VISI & PRINSIP DASAR

### **1.1 Filosofi Sistem**

> "Sistem otomasi masjid yang plug-and-play, fail-safe, dan mudah dirawat oleh teknisi lokal dengan pengetahuan listrik dasar. **Manual control tetap prioritas**, automation sebagai kemudahan tambahan."

### **1.2 Prinsip Inti (Tidak Bisa Ditawar)**

1. **AUTO-ENROLLMENT**
   - Perangkat baru otomatis terdaftar tanpa konfigurasi manual
   - MAC address sebagai unique identifier
   - User hanya perlu beri nama via dashboard

2. **FAIL-SAFE OTOMATIS**
   - Jika komunikasi terputus >300 detik, sistem ambil keputusan safety otomatis
   - Lampu: FORCE ON (keamanan jamaah)
   - Kipas: FORCE OFF (cegah overheating)
   - Sound: FORCE OFF (cegah noise)

3. **ARSITEKTUR DUAL MASTER**
   - Logic Master (ID:254): Otak sistem, handle RS-485 bus
   - Display Master (ID:255): User interface, dashboard kontrol
   - Pisahkan concern untuk reliability dan maintainability

4. **ONE FIRMWARE**
   - Satu firmware untuk semua Slave
   - Fungsi ditentukan oleh profile di NVS (FAN_4CH, LIGHT_8CH, SOUND_8CH)
   - Mudah update dan maintain

5. **MANUAL PRIORITY (NEW)**
   - Sistem automation adalah **layer tambahan** di atas sistem manual existing
   - Manual control selalu berfungsi, tidak tergantung automation
   - Parallel wiring: Manual ATAU Auto bisa kontrol
   - Untuk kipas: DPDT changeover switch untuk cegah konflik

### **1.3 Analogi untuk Pemahaman**

```
SISTEM LAMA (Manual):
  Wall Switch → Load
  Simple, reliable, everyone understands

SISTEM BARU (Hybrid Manual + Auto):
  Wall Switch ─┐
               ├→ Load (Parallel OR logic)
  Smart Node ─┘
  
  Benefit: Fleksibilitas tanpa hilangkan kontrol manual
```

**Analogi komponen:**
- **Logic Master** = Otak sistem (seperti panel MCB utama)
- **Display Master** = Remote control panel (seperti CCTV monitor)
- **Slave Nodes** = MCB pintar per ruangan
- **RS-485 Bus** = Kabel kontrol (seperti kabel bell/intercom)
- **DPDT Mode Switch** = Selector manual/auto (seperti ATS genset)

---

## 🏗️ BAGIAN 2: ARSITEKTUR SISTEM

### **2.1 Diagram Arsitektur Lengkap**

```
┌─────────────────────────────────────────────────────────────┐
│                    SMART MOSQUE ECOSYSTEM                    │
│                         Architecture v1.4                     │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│                   RS-485 COMMUNICATION BUS                   │
│             (MAX3485, 115200bps, 8N1, Daisy-chain)          │
└─────────────────────────────────────────────────────────────┘
         │                    │                    │
         ▼                    ▼                    ▼
┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐
│  LOGIC MASTER   │  │ DISPLAY MASTER  │  │  SLAVE NODES    │
│   (ID: 254)     │  │   (ID: 255)     │  │   (ID: 1-247)   │
├─────────────────┤  ├─────────────────┤  ├─────────────────┤
│ ESP32-S3 DevKit │  │  Waveshare 7"   │  │ ESP32 DevKit V1 │
│ + ILI9488 4"    │  │  Touch LCD      │  │ + MAX3485       │
│ + MAX3485       │  │  + RS-485       │  │ + 8CH SSR       │
│ + SD Card       │  │  + WiFi         │  │ + Mode Switch   │
│                 │  │                 │  │   (kipas only)  │
├─────────────────┤  ├─────────────────┤  ├─────────────────┤
│ FUNGSI:         │  │ FUNGSI:         │  │ FUNGSI:         │
│ • Heartbeat     │  │ • User Control  │  │ • Execute relay │
│ • Enrollment    │  │ • Monitoring    │  │ • Report status │
│ • Database      │  │ • Web Dashboard │  │ • Fail-safe     │
│ • Fail-safe     │  │ • Visualization │  │ • Manual detect │
│ • Sync Display  │  │                 │  │                 │
└─────────────────┘  └─────────────────┘  └─────────────────┘
         │                    │                    │
         ▼                    ▼                    ▼
    [Monitoring]         [Control]          [Execution]

┌─────────────────────────────────────────────────────────────┐
│                    PHYSICAL DEVICES                          │
├─────────────────────────────────────────────────────────────┤
│  • 13 Kipas nodes (FAN_4CH with DPDT mode switch)          │
│  •  4 Lampu nodes (LIGHT_8CH, parallel with manual)        │
│  •  1 Sound node  (SOUND_8CH, parallel with manual)        │
│                                                              │
│  Total: 18 slave nodes, 92 relay control points            │
└─────────────────────────────────────────────────────────────┘
```

### **2.2 Tabel Komponen Utama**

| Komponen | Hardware | ID | Fungsi Utama | Lokasi Fisik |
|----------|----------|----|--------------|--------------| 
| **Logic Master** | ESP32-S3 DevKit<br/>+ ILI9488 4"<br/>+ MAX3485 | 254 | • Manage RS-485 bus<br/>• Auto-enrollment<br/>• Heartbeat sender<br/>• Device registry<br/>• Fail-safe executor | Panel listrik utama |
| **Display Master** | Waveshare ESP32-S3<br/>Touch LCD 7"<br/>+ RS-485 built-in | 255 | • LVGL touchscreen UI<br/>• User control panel<br/>• Web dashboard (WiFi)<br/>• Real-time visualization<br/>• Command sender | Ruang mihrab/<br/>kantor pengurus |
| **Slave Kipas** | ESP32 DevKit V1<br/>+ MAX3485<br/>+ 4CH SSR G3MB-202P<br/>+ DPDT mode switch | 1-13 | • Control motor 3-speed<br/>• Mode detection (AUTO/MANUAL)<br/>• Interlock safety<br/>• Manual override support | Per kipas<br/>di masjid |
| **Slave Lampu** | ESP32 DevKit V1<br/>+ MAX3485<br/>+ 8CH SSR G3MB-202P | 14-17 | • Control 8 channel lampu<br/>• Parallel with manual switch<br/>• Independent ON/OFF | Per zona<br/>pencahayaan |
| **Slave Sound** | ESP32 DevKit V1<br/>+ MAX3485<br/>+ 8CH SSR G3MB-202P | 18 | • Control 8 channel audio<br/>• Zone audio control<br/>• Parallel with manual | Sistem audio<br/>masjid |

### **2.3 Alur Data Pokok**

```
1. PENDAFTARAN PERANGKAT BARU (Auto-Enrollment)
   ┌────────────────────────────────────────────────────────┐
   │ Slave (ID=0) → Broadcast MAC + Profile                │
   │      ↓                                                  │
   │ Logic Master → Simpan "Pending Device"                 │
   │      ↓                                                  │
   │ Logic Master → Notify Display Master                   │
   │      ↓                                                  │
   │ Display Master → Show popup "Device Baru"              │
   │      ↓                                                  │
   │ User → Input nama device di touchscreen                │
   │      ↓                                                  │
   │ Display → Send (ID baru + Nama) ke Logic Master        │
   │      ↓                                                  │
   │ Logic Master → Send DISCOVERY_RESPONSE to Slave        │
   │      ↓                                                  │
   │ Slave → Save config to NVS, restart with new ID        │
   │      ↓                                                  │
   │ Slave → Send STATUS_REPORT with new ID                 │
   │      ↓                                                  │
   │ Logic Master → Update database, sync to Display        │
   └────────────────────────────────────────────────────────┘

2. KONTROL NORMAL (User Control)
   ┌────────────────────────────────────────────────────────┐
   │ User → Touch Display atau Web Dashboard                │
   │      ↓                                                  │
   │ Display Master → Send SET_RELAY via RS-485             │
   │      ↓                                                  │
   │ Slave → Check mode (AUTO/MANUAL/FAILSAFE)              │
   │      ├─ AUTO: Execute command                          │
   │      ├─ MANUAL: Reject, send NACK                      │
   │      └─ FAILSAFE: Ignore, maintain fail-safe state     │
   │      ↓                                                  │
   │ Slave → Apply relay with safety interlock              │
   │      ↓                                                  │
   │ Slave → Send STATUS_REPORT (confirm execution)         │
   │      ↓                                                  │
   │ Display Master → Update UI in real-time                │
   │      ↓                                                  │
   │ Logic Master → Update database                         │
   └────────────────────────────────────────────────────────┘

3. MANUAL OVERRIDE (Kipas)
   ┌────────────────────────────────────────────────────────┐
   │ User → Flip DPDT mode switch to MANUAL                 │
   │      ↓                                                  │
   │ DPDT → Physically disconnect SSR common (L+N)          │
   │      ↓                                                  │
   │ DPDT → Connect speed switch to motor                   │
   │      ↓                                                  │
   │ Slave → Detect mode change via GPIO35                  │
   │      ↓                                                  │
   │ Slave → Turn off all SSR (clean state)                 │
   │      ↓                                                  │
   │ Slave → Send STATUS_REPORT (mode=MANUAL)               │
   │      ↓                                                  │
   │ Display → Show "⚠ MANUAL MODE" indicator               │
   │      ↓                                                  │
   │ User → Control speed via physical switch               │
   │      ↓                                                  │
   │ Motor → Responds directly (no ESP32 in path)           │
   └────────────────────────────────────────────────────────┘

4. FAIL-SAFE TRIGGER
   ┌────────────────────────────────────────────────────────┐
   │ Logic Master → Stop sending heartbeat (crash/power)    │
   │      ↓                                                  │
   │ Slave → Timer counts: 60s, 120s, 180s, ...            │
   │      ↓                                                  │
   │ Slave → At 300s: FAIL-SAFE TIMEOUT!                   │
   │      ↓                                                  │
   │ Slave → Execute fail-safe action per profile:          │
   │      ├─ LIGHT_8CH: Force ALL ON                        │
   │      ├─ FAN_4CH: Force ALL OFF                         │
   │      └─ SOUND_8CH: Force ALL OFF                       │
   │      ↓                                                  │
   │ Slave → LED indicator: RED (fail-safe mode)            │
   │      ↓                                                  │
   │ Slave → Send STATUS_REPORT (source=FAILSAFE)           │
   │      ↓ (if Master recovers)                            │
   │ Display → Show "⚠ FAIL-SAFE ACTIVE" alert              │
   └────────────────────────────────────────────────────────┘
```

---

## 🚨 BAGIAN 3: MATRIKS FAIL-SAFE & MANUAL CONTROL

### **3.1 Fail-Safe Actions (Heartbeat hilang >300s)**

| Profile | Fail-Safe Action | LED | Alasan |
|---------|------------------|-----|--------|
| **LIGHT_8CH** | FORCE ALL ON | 🔴 Merah steady | Keamanan jamaah, visibilitas darurat |
| **FAN_4CH** | FORCE ALL OFF | 🔴 Merah steady | Cegah overheating motor tanpa monitoring |
| **SOUND_8CH** | FORCE ALL OFF | 🔴 Merah steady | Cegah noise/feedback tak terkontrol |

**Recovery dari Fail-Safe:**
```
Heartbeat restored → Slave keluar dari fail-safe
                   → LED kembali hijau (AUTO) atau kuning (MANUAL)
                   → Resume normal operation
                   → Send STATUS_REPORT (recovery notification)
Catatan UI: Untuk lampu/sound dengan parallel manual switch, status di dashboard merefleksikan SSR state. Jika saklar manual ON saat SSR OFF, beban tetap ON (by design).
```

### **3.2 Manual Control Strategy**

#### **3.2.1 Lampu & Sound (Parallel OR Logic)**

```
WIRING TOPOLOGY:
┌─────────────────────────────────────────────────────────────┐
│                                                              │
│         Manual Switch ──┐                                   │
│                         ├─→ Load                            │
│         SSR Output ─────┘                                   │
│                                                              │
│  Truth Table:                                                │
│  ┌─────────┬──────────┬────────┐                           │
│  │ Manual  │ SSR Auto │ Load   │                           │
│  ├─────────┼──────────┼────────┤                           │
│  │ OFF     │ OFF      │ OFF    │                           │
│  │ OFF     │ ON       │ ON     │                           │
│  │ ON      │ OFF      │ ON     │                           │
│  │ ON      │ ON       │ ON     │                           │
│  └─────────┴──────────┴────────┘                           │
│                                                              │
│  Behavior: Salah satu ON → Load ON                         │
│  No conflict, no mode switch needed                         │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

**Firmware Behavior:**
- ESP32 tidak perlu deteksi manual override
- Execute SET_RELAY command langsung
- Actual load state mungkin berbeda (tapi OK)
- STATUS_REPORT hanya report SSR state

#### **3.2.2 Kipas (DPDT Changeover)**

```
DPDT 6-PIN MODE SWITCH:
┌─────────────────────────────────────────────────────────────┐
│                                                              │
│   Pin Layout:                                                │
│   1 (AUTO L)    2 (COM L)    3 (MANUAL L)                  │
│      ○             ○             ○                           │
│      │             │             │                           │
│      │       ┌─────┴─────┐      │                           │
│      │       │  TOGGLE   │      │                           │
│      │       └─────┬─────┘      │                           │
│      │             │             │                           │
│      ○             ○             ○                           │
│   4 (AUTO N)    5 (COM N)    6 (MANUAL N)                  │
│                                                              │
│   Connections:                                               │
│   Pin 2,5: From 220VAC source (L+N)                        │
│   Pin 1,4: To SSR commons (L+N)                            │
│   Pin 3,6: To Speed Switch input (L+N)                     │
│                                                              │
│   Position AUTO:                                             │
│   • Pin 2→1, 5→4: SSR path ACTIVE                          │
│   • Pin 3,6: OPEN (speed switch isolated)                  │
│                                                              │
│   Position MANUAL:                                           │
│   • Pin 2→3, 5→6: Speed switch ACTIVE                      │
│   • Pin 1,4: OPEN (SSR isolated)                           │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

**Firmware Behavior:**
```cpp
// Mode detection via GPIO35
bool isAutoMode = digitalRead(MODE_DETECT_PIN);  // HIGH=AUTO, LOW=MANUAL

if (isAutoMode) {
  // Execute RS-485 commands
  processRemoteControl();
} else {
  // Reject RS-485 commands
  if (commandReceived) {
    sendNACK(ERROR_MANUAL_MODE_ACTIVE);
  }
  // Manual switch controls motor directly via DPDT
}
```

### **3.3 State Machine Slave**

```
┌─────────────────────────────────────────────────────────────┐
│                    SLAVE STATE MACHINE                       │
└─────────────────────────────────────────────────────────────┘

        [POWER ON]
            │
            ▼
     [READ NVS CONFIG]
            │
     ┌──────┴──────┐
     │             │
  ID=0          ID=1-247
     │             │
     ▼             ▼
[DISCOVERY]   [OPERATIONAL]
     │             │
     │         ┌───┴───┐
     │         │       │
     │    Profile   Profile
     │    Kipas   Lampu/Sound
     │         │       │
     │         ▼       ▼
     │    ┌────────┐  [STATE_AUTO]
     │    │ Mode?  │      │
     │    └────┬───┘      │
     │         │          │
     │    ┌────┴────┐     │
     │    │         │     │
     │  AUTO    MANUAL    │
     │    │         │     │
     │    ▼         ▼     ▼
     │ [AUTO]  [MANUAL] [AUTO]
     │    │         │     │
     │    └─────┬───┴─────┘
     │          │
     │          ▼
     │   [Check Heartbeat]
     │          │
     │    Timeout >300s?
     │          │
     │          ▼
     │    [FAIL-SAFE]
     │          │
     │   [Heartbeat OK?]
     │          │
     │          ▼
     │    [Back to AUTO]
     │
     └─[Receive ID]─→ [Save NVS] → [RESTART]
```

**State Descriptions:**

| State | LED | Trigger | Action |
|-------|-----|---------|--------|
| **DISCOVERY** | 🔵 Biru blink | ID=0 di NVS | Broadcast MAC setiap 1-3 detik random |
| **AUTO** | 🟢 Hijau steady | Normal operation | Execute RS-485 commands |
| **MANUAL** | 🟡 Kuning steady | DPDT switch MANUAL | Reject RS-485, report mode |
| **FAIL-SAFE** | 🔴 Merah steady | No heartbeat >300s | Execute fail-safe per profile |

---

## 💾 BAGIAN 4: STRUKTUR DATABASE & PERSISTENCE

### **4.1 Device Registry (Logic Master)**

```cpp
// Stored in NVS namespace: "smart_mosque"
struct DeviceRecord {
  uint8_t id;              // 1-247 (operational ID)
  uint8_t mac[6];          // MAC address (binary, 6 bytes)
  char name[32];           // User-friendly name
  uint8_t profile;         // 1=FAN_4CH, 2=LIGHT_8CH, 3=SOUND_8CH
  uint32_t last_seen;      // Epoch seconds of last heartbeat/report
  bool online;             // Derived from (now() - last_seen < 310)
  uint8_t relay_state;     // Bitmask 8 relay (bit 0-7)
  uint8_t control_source;  // 0=AUTO, 1=MANUAL, 2=FAILSAFE
} __attribute__((packed));

// Total size: 1+6+32+1+4+1+1+1 = 47 bytes per device
// For 247 devices: 11,609 bytes (~11.6KB)
```

**NVS Key Structure:**
```
Namespace: "smart_mosque"
├─ "dev_count": uint8_t (total registered devices)
├─ "dev_1": DeviceRecord (ID 1)
├─ "dev_2": DeviceRecord (ID 2)
├─ ...
└─ "dev_247": DeviceRecord (ID 247)
```

**Compare-Before-Write Implementation:**
```cpp
void updateDevice(uint8_t id, DeviceRecord& newRecord) {
  DeviceRecord oldRecord;
  
  // Read existing
  preferences.getBytes(String("dev_") + id, &oldRecord, sizeof(DeviceRecord));
  
  // Compare (note: last_seen berbasis epoch sering berubah; tulis hanya saat field stabil berubah)
  if (oldRecord.profile == newRecord.profile &&
      oldRecord.relay_state == newRecord.relay_state &&
      oldRecord.control_source == newRecord.control_source &&
      oldRecord.online == newRecord.online &&
      memcmp(oldRecord.mac, newRecord.mac, 6) == 0 &&
      strncmp(oldRecord.name, newRecord.name, sizeof(oldRecord.name)) == 0) {
    // Tidak ada perubahan signifikan, skip write (hemat flash cycle)
    return;
  }
  
  // Write only when stable fields changed
  preferences.putBytes(String("dev_") + id, &newRecord, sizeof(DeviceRecord));
}
```

### **4.2 Slave Configuration (NVS)**

```cpp
// Stored in NVS namespace: "config"
struct SlaveConfig {
  uint8_t device_id;       // 0=unassigned, 1-247=assigned
  char device_name[32];    // Nama dari enrollment
  uint8_t profile;         // Profile type
  uint8_t mac[6];          // MAC address sendiri
  uint16_t crc;            // Config checksum untuk validasi
} __attribute__((packed));

// Total: 1+32+1+6+2 = 42 bytes
```

**Boot Validation:**
```cpp
void validateConfigOnBoot() {
  SlaveConfig config;
  preferences.getBytes("config", &config, sizeof(config));
  
  // Validate ID
  if (config.device_id > 247) {
    handleCorruptNVS("Invalid ID");
    return;
  }
  
  // Validate CRC
  uint16_t calculatedCRC = calculateCRC((uint8_t*)&config, sizeof(config)-2);
  if (calculatedCRC != config.crc) {
    handleCorruptNVS("CRC mismatch");
    return;
  }
  
  // Config valid, proceed to operational mode
  myDeviceId = config.device_id;
  myProfile = config.profile;
}
```

### **4.3 Periodic Flush Strategy (Write-Only-On-Change)**

```cpp
class SmartPersistence {
private:
  DeviceRecord cache[247];
  bool dirty[247] = {false};
  uint32_t lastFlush = 0;
  const uint32_t FLUSH_INTERVAL = 300000;  // 5 menit
  
public:
  void updateDevice(uint8_t id, DeviceRecord& record) {
    // Update RAM cache
    cache[id] = record;
    dirty[id] = true;  // Mark dirty
  }
  
  void periodicFlush() {
    if (millis() - lastFlush < FLUSH_INTERVAL) return;
    
    int flushedCount = 0;
    preferences.begin("smart_mosque", false);
    
    for (int i = 1; i <= 247; i++) {
      if (dirty[i]) {
        preferences.putBytes(String("dev_") + i, &cache[i], sizeof(DeviceRecord));
        dirty[i] = false;
        flushedCount++;
      }
    }
    
    preferences.end();
    lastFlush = millis();
    
    if (flushedCount > 0) {
      Serial.printf("✓ Flushed %d devices to NVS\n", flushedCount);
    }
  }
  
  void forceFlush() {
    lastFlush = 0;
    periodicFlush();
  }
};
```

---

## 📊 BAGIAN 5: SPESIFIKASI HARDWARE SINGKAT

### **5.1 Device Specifications**

| Component | Model | Key Specs | Location | Price (est.) |
|-----------|-------|-----------|----------|--------------|
| **Master Logic** | ESP32-S3 DevKit | 16MB Flash, 8MB PSRAM | Panel utama | Rp 150k |
| Status Display | ILI9488 4" | 480x320 SPI | Attached to Logic | Rp 120k |
| **Display Master** | Waveshare ESP32-S3-Touch-LCD-7 | 800x480 capacitive touch | Mihrab/Kantor | Rp 800k |
| **Slave Node** | ESP32 DevKit V1 | 4MB Flash, 520KB RAM | Per device | Rp 120k |
| RS-485 IC | MAX3485 | 115200bps, 3.3V/5V | All nodes | Rp 15k |
| SSR | G3MB-202P | 5V ctrl, 2A load, zero-cross | Slave nodes | Rp 150k/8CH |
| Mode Switch | DPDT 6-pin | 10A/250VAC changeover | Kipas only | Rp 25k |
| PSU | HLK-PM03 | 220VAC to 5VDC/3A | Per node | Rp 75k |

### **5.2 Pinout Standard (WAJIB - Tidak Boleh Diubah)**

```cpp
// RS-485 Communication (All nodes)
#define RS485_RX       16    // Hardware Serial2 RX
#define RS485_TX       17    // Hardware Serial2 TX
#define RS485_DE_RE     4    // Driver Enable/Receive Enable (HIGH=TX)

// Relay Outputs (Slave nodes, sequence 1-8)
#define RELAY_1        12    // Kipas: LOW / Lampu: CH1
#define RELAY_2        13    // Kipas: MED / Lampu: CH2
#define RELAY_3        14    // Kipas: HIGH / Lampu: CH3
#define RELAY_4        15    // Kipas: Reserve / Lampu: CH4
#define RELAY_5        25    // Lampu: CH5
#define RELAY_6        26    // Lampu: CH6
#define RELAY_7        27    // Lampu: CH7
#define RELAY_8        32    // Lampu: CH8

// Mode Detection (Kipas only)
#define MODE_DETECT_PIN 35   // HIGH=AUTO, LOW=MANUAL

// LED Indicator
#define LED_STATUS      2    // Onboard LED (mode indicator)
```

### **5.3 Power Budget**

```
SYSTEM POWER CONSUMPTION (5VDC):

Master Logic:
  ESP32-S3 (120mA) + MAX3485 (10mA) + ILI9488 4" (180mA) = 310mA

Display Master:
  Waveshare board (800mA full brightness) = 800mA

Per Kipas Node (4CH SSR):
  ESP32 (120mA) + MAX3485 (10mA) + 4×SSR (80mA) = 210mA

Per Lampu/Sound Node (8CH SSR):
  ESP32 (120mA) + MAX3485 (10mA) + 8×SSR (160mA) = 290mA

TOTAL FOR 18 NODES:
  Master Logic: 310mA
  Display Master: 800mA
  13 Kipas: 2,730mA
  4 Lampu: 1,160mA
  1 Sound: 290mA
  ────────────────
  TOTAL: 5,290mA = 5.3A @ 5VDC

Recommended PSU: 5V/8A (with margin)

Note: Ini hanya control circuit. AC load (motor/lampu aktual) terpisah.
```

---

## 📋 BAGIAN 6: ADDRESSING & NAMING CONVENTION

### **6.1 Device ID Allocation**

```
┌────────────────────────────────────────────────────────────┐
│ ID Range │ Device Type       │ Quantity │ Notes          │
├────────────────────────────────────────────────────────────┤
│ 0        │ Unassigned        │ -        │ Discovery mode │
│ 1-13     │ FAN_4CH          │ 13       │ Kipas masjid   │
│ 14-17    │ LIGHT_8CH        │ 4        │ Lampu zona     │
│ 18       │ SOUND_8CH        │ 1        │ Audio system   │
│ 19-247   │ Reserved (future)│ -        │ Expansion      │
│ 254      │ Logic Master     │ 1        │ Fixed ID       │
│ 255      │ Display Master   │ 1        │ Fixed ID       │
└────────────────────────────────────────────────────────────┘
```

### **6.2 Naming Convention Examples**

**Kipas (FAN_4CH):**
```
ID 1  → "Kipas Saf Depan Kanan"
ID 2  → "Kipas Saf Depan Kiri"
ID 3  → "Kipas Saf Tengah Kanan 1"
ID 4  → "Kipas Saf Tengah kiri 1"
ID 5  → "Kipas Saf Tengah Kanan 2"
ID 6  → "Kipas Saf Tengah Kiri 2"
ID 7  → "Kipas Saf Tengah Besar Kanan"
ID 8  → "Kipas Saf Tengah Besar Kiri"
ID 9  → "Kipas Saf Belakang kanan"
ID 10 → "Kipas Saf Belakang kiri"
ID 11 → "Kipas Saf Wanita Kanan"
ID 12 → "Kipas Saf Wanita Kiri"
ID 13 → "Kipas Imam"
```

**Lampu (LIGHT_8CH):**
```
ID 14 → "Lampu Utama"
        • CH1: Lampu Utama Besar
        • CH2: Lampu Utama Kecil
        • CH3: Lampu Gantung
        • CH4: Lampu Utama Kanan
        • CH5: Lampu Utama Kiri
        • CH6: Lampu Utama Belakang
        • CH7: Lampu Mihrab
        • CH8: Lampu Ruang Sound

ID 15 → "Lampu Luar"
        • CH1: Lampu serambi berbagai zona
        • CH2: Lampu Tangga Wudhu
        • CH3: Lampu Dapur
        • CH4: Lampu Luar Kiri
        • CH5: Lampu Luar Kanan
        • CH6: Lampu Luar Pintu Kanan
        • CH7: Lampu Luar Depan
        • CH8: Lampu Parkir dan Taman

ID 16 → "Lampu Kantor"
        • CH1-4: Lampu Sekretariat berbagai zona
        • CH5: Lampu Ruang Komputer
        • CH6: Lampu Menara
        • CH7: Lampu Plang Nama
        • CH8: Lampu Kubah & Bulan Bintang
        -8: Reserve/future

ID 17 → "Lampu Aula & Wudhu"
        • CH1-3: Lampu Aula Berbagai Zona
        • CH4: Lampu Ruang Peralatan
        • CH5-8: Lampu area wudhu Berbagai Zona
```

**Sound (SOUND_8CH):**
```
ID 18 → "Audio Mimbar"
        • CH1: Power Master
        • CH2: Speaker Dalam
        • CH3: Speaker Luar
        • CH4: toa Menara
        • CH5: Mic Imam
        • CH6: Mic Khotib
        • CH7: Mic Adzan
        • CH8: Mic Bilal
```

### **6.3 Profile Type Mapping**

```cpp
enum DeviceProfile {
  PROFILE_UNDEFINED = 0,
  PROFILE_FAN_4CH = 1,      // 4-channel kipas dengan mode switch
  PROFILE_LIGHT_8CH = 2,    // 8-channel lampu
  PROFILE_SOUND_8CH = 3     // 8-channel audio
};

// String mapping untuk display
const char* profileToString(uint8_t profile) {
  switch(profile) {
    case PROFILE_FAN_4CH:   return "KIPAS";
    case PROFILE_LIGHT_8CH: return "LAMPU";
    case PROFILE_SOUND_8CH: return "SOUND";
    default:                return "UNKNOWN";
  }
}
```

---

## 🔄 BAGIAN 7: PROTOKOL KOMUNIKASI RINGKAS

### **7.1 Frame Format**

```
BYTE STRUKTUR:
┌─────┬─────┬─────┬─────┬──────────┬──────┬──────┬─────┐
│0x7E │ADDR │CMD  │LEN  │ PAYLOAD  │CRC_L │CRC_H │0x0A │
└─────┴─────┴─────┴─────┴──────────┴──────┴──────┴─────┘
  ^     ^     ^     ^        ^        ^      ^      ^
START  TO   OPCODE SIZE   DATA    CHECKSUM    END
 (1)   (1)   (1)   (1)   (0-255)    (2)       (1)

Total frame size: 7 + payload_length bytes
Min: 7 bytes (no payload)
Max: 262 bytes (255 payload + 7 header/footer)
```

### **7.2 Command Opcodes Summary**

| Opcode | Nama | Direction | Fungsi Singkat |
|--------|------|-----------|----------------|
| `0x01` | DISCOVERY_ANNOUNCE | Slave→Master | "Saya device baru, daftarkan!" |
| `0x02` | DISCOVERY_RESPONSE | Master→Slave | "Kamu sekarang ID X, nama Y" |
| `0x03` | SET_RELAY | Master→Slave | "Nyala/matikan relay Z" |
| `0x04` | HEARTBEAT | Master→Broadcast | "Saya masih hidup" (60s interval) |
| `0x05` | STATUS_REPORT | Slave→Master | "Status saya: ..." |
| `0x06` | FORCE_RESET | Master→Slave | "Reset ke factory" (optional) |
| `0x07` | ACK | Bidirectional | "Pesan diterima OK" |
| `0x08` | NACK | Bidirectional | "Ada error" |

### **7.3 Heartbeat Mechanism**

```
HEARTBEAT TIMING:
┌─────────────────────────────────────────────────────────────┐
│                                                              │
│  Logic Master → Broadcast 0x04 setiap 60 detik              │
│                                                              │
│  Slave behavior:                                             │
│    • Receive heartbeat → Reset timer                         │
│    • Timer > 300 detik → FAIL-SAFE TRIGGER                  │
│                                                              │
│  Tolerance: ±5 detik (55-65 detik interval OK)              │
│                                                              │
│  Payload: 1 byte (0xAA = alive signal)                      │
│                                                              │
└─────────────────────────────────────────────────────────────┘

IMPLEMENTATION:
┌─────────────────────────────────────────────────────────────┐
│ // Di Master Logic                                           │
│ void sendHeartbeat() {                                       │
│   static uint32_t lastHB = 0;                               │
│   if (millis() - lastHB > 60000) {                          │
│     uint8_t payload = 0xAA;                                 │
│     sendRS485Frame(0x00, 0x04, &payload, 1);  // Broadcast │
│     lastHB = millis();                                      │
│   }                                                          │
│ }                                                            │
│                                                              │
│ // Di Slave                                                  │
│ void checkFailSafe() {                                       │
│   if (millis() - lastHeartbeat > 300000) {                 │
│     enterFailSafeMode();                                    │
│   }                                                          │
│ }                                                            │
└─────────────────────────────────────────────────────────────┘
```

### **7.4 Error Codes (NACK Payload)**

```cpp
enum ErrorCode {
  ERROR_OK = 0x00,                    // No error
  ERROR_INVALID_CRC = 0x01,           // CRC mismatch
  ERROR_PAYLOAD_TOO_LONG = 0x02,      // Length > 255
  ERROR_INVALID_COMMAND = 0x03,       // Unknown opcode
  ERROR_MANUAL_MODE_ACTIVE = 0x04,    // Kipas in MANUAL mode
  ERROR_INTERLOCK_VIOLATION = 0x05,   // Multiple speed relay ON
  ERROR_NVS_WRITE_ERROR = 0x06,       // Flash write failed
  ERROR_PROFILE_MISMATCH = 0x07,      // Command tidak sesuai profile
  ERROR_DEVICE_NOT_FOUND = 0x08       // ID tidak terdaftar
};
```

---

## 🎯 BAGIAN 8: REFERENSI DOKUMEN TERKAIT

### **8.1 Hierarki Dokumen**

```
1. 📘 KEBENARAN TUNGGAL v1.4 (Dokumen ini)
   └─ Filosofi, arsitektur, dan prinsip sistem

2. ⚙️ SPESIFIKASI TEKNIS PROTOKOL v1.3 (Update dari v1.2)
   └─ Detail byte-level protocol RS-485

3. 🔌 PANDUAN INTEGRASI HARDWARE v1.1 (Update dari v1.0)
   └─ Wiring diagram, BOM, instalasi fisik

4. 🛡️ SOLUSI MASALAH & ENHANCEMENT v1.0 (NEW)
   └─ Security, reliability, monitoring

5. 🌐 WEB DASHBOARD SPECIFICATION v1.0 (NEW)
   └─ Multi-display architecture & UI/UX

6. 📝 CHANGE LOG & MIGRATION v1.0 (NEW)
   └─ Implementation roadmap & migration guide

7. 🧠 AI COLLABORATION PLAYBOOK v1.0
   └─ Standar kolaborasi dengan AI assistant
```

### **8.2 Cara Merujuk Dokumen**

**Format referensi:**
```
[KT-3.2]   = Kebenaran Tunggal bagian 3.2
[STP-4.3]  = Spesifikasi Teknis Protokol bagian 4.3
[HIG-2.1]  = Hardware Integration Guide bagian 2.1
[SME-5.2]  = Solusi Masalah & Enhancement bagian 5.2
[WDS-3.1]  = Web Dashboard Specification bagian 3.1
[CLM-2.4]  = Change Log & Migration bagian 2.4
```

**Contoh penggunaan dalam kode:**
```cpp
// KT_REF: 3.1 - Fail-safe matrix untuk LIGHT_8CH
void enterFailSafe() {
  if (myProfile == PROFILE_LIGHT_8CH) {
    forceAllRelaysON();  // Safety: visibility untuk jamaah
  }
}
```

### **8.3 Prinsip Yang Tidak Berubah (Immutable)**

```
┌─────────────────────────────────────────────────────────────┐
│              PRINSIP FUNDAMENTAL (LOCKED)                    │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  1. Auto-enrollment protocol WAJIB                          │
│     → Setiap slave baru harus bisa self-register            │
│                                                              │
│  2. Fail-safe timeout 300 detik TETAP                       │
│     → Tidak boleh lebih cepat atau lebih lambat             │
│                                                              │
│  3. Dual Master architecture TIDAK BERUBAH                  │
│     → Logic Master (254) & Display Master (255)             │
│                                                              │
│  4. Pinout standard GPIO LOCKED                             │
│     → RS-485: 16,17,4 | Relay: 12-15,25-27,32              │
│                                                              │
│  5. Manual control PRIORITY                                  │
│     → Manual selalu bisa override automation                │
│                                                              │
│  6. RS-485 bus speed: 115200 bps FIXED                      │
│     → Jangan ubah untuk kompatibilitas                      │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

---

## 📝 BAGIAN 9: CHANGE LOG

### **v1.4 (27 Desember 2025) - CURRENT**

**Major Changes:**
- ✅ **Manual Priority Philosophy**
  - Manual control sebagai primary, auto sebagai secondary
  - Parallel wiring untuk lampu/sound (OR logic)
  - DPDT changeover untuk kipas (physical isolation)

- ✅ **DPDT Mode Switch Detail**
  - 6-pin changeover specification
  - L+N switching untuk safety
  - Mode detection via GPIO35

- ✅ **Hardware Specifications**
  - G3MB-202P SSR detail (zero-cross, 2A, 5V control)
  - HLK-PM03 PSU (5V/3A per node)
  - Complete BOM with actual components

- ✅ **Scale Definition**
  - 13 Kipas nodes (FAN_4CH)
  - 4 Lampu nodes (LIGHT_8CH)
  - 1 Sound node (SOUND_8CH)
  - Total: 18 slave nodes, 92 relay points

**Minor Updates:**
- Updated power calculations with actual SSR consumption
- Added detailed naming conventions
- Clarified manual override behavior per profile
- Enhanced state machine with MANUAL state (kipas only)

**Referensi:**
- Integrated feedback dari diskusi detail 27 Des 2025
- Based on real hardware: DPDT 6-pin, G3MB-202P, HLK-PM03

---

### **v1.3 (26 Desember 2025)**

**Changes:**
- Added Dual Master architecture
- Defined auto-enrollment protocol
- Created fail-safe matrix
- Established database structure

---

### **v1.2 (25 Desember 2025)**

**Changes:**
- Initial concept: One Firmware for All
- Basic RS-485 protocol outline
- Profile definitions (FAN/LIGHT/SOUND)

---

## 🎯 BAGIAN 10: QUICK REFERENCE

### **10.1 System at a Glance**

```
╔═══════════════════════════════════════════════════════════════╗
║            SMART MOSQUE ECOSYSTEM - QUICK FACTS              ║
╠═══════════════════════════════════════════════════════════════╣
║                                                               ║
║  Total Devices:     20 (2 Master + 18 Slave)                ║
║  Control Points:    92 relay channels                        ║
║  Communication:     RS-485, 115200bps, daisy-chain          ║
║  Protocol:          Custom frame with CRC16-MODBUS           ║
║  Power:             Distributed 5VDC PSU per node            ║
║  Manual Override:   Parallel (Lampu/Sound), DPDT (Kipas)    ║
║  Fail-Safe:         300s timeout, per-profile action         ║
║  Auto-Enrollment:   MAC-based, user assigns name            ║
║                                                               ║
╚═══════════════════════════════════════════════════════════════╝
```

### **10.2 Common Troubleshooting Quick Guide**

```
┌─────────────────────────────────────────────────────────────┐
│ PROBLEM: Slave tidak terdeteksi                             │
├─────────────────────────────────────────────────────────────┤
│ CHECK:                                                       │
│  1. Power 5V ada? (LED ESP32 nyala?)                       │
│  2. RS-485 wiring benar? (A-A, B-B, GND-GND)              │
│  3. Termination 120Ω dipasang? (ujung bus saja)           │
│  4. ID=0 untuk device baru? (discovery mode)               │
│  5. Serial monitor: Ada broadcast DISCOVERY_ANNOUNCE?      │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│ PROBLEM: Kipas tidak respon remote command                  │
├─────────────────────────────────────────────────────────────┤
│ CHECK:                                                       │
│  1. Mode switch posisi AUTO? (bukan MANUAL)               │
│  2. LED warna apa? (Hijau=AUTO, Kuning=MANUAL)            │
│  3. Dashboard show "MANUAL MODE" warning?                  │
│  4. GPIO35 state: HIGH=AUTO, LOW=MANUAL                   │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│ PROBLEM: Lampu nyala sendiri padahal dashboard OFF          │
├─────────────────────────────────────────────────────────────┤
│ CHECK:                                                       │
│  1. Manual wall switch posisi ON? (parallel wiring)        │
│  2. Ini bukan error, ini by design (manual override)       │
│  3. SSR state di dashboard hanya show command, bukan       │
│     actual load state                                       │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│ PROBLEM: LED merah, kipas/lampu behavior aneh               │
├─────────────────────────────────────────────────────────────┤
│ CHECK:                                                       │
│  1. LED merah = FAIL-SAFE mode                             │
│  2. Cause: No heartbeat from Master Logic >300s            │
│  3. Check Master Logic status (crash? power loss?)         │
│  4. Fix Master, slave akan auto-recovery                   │
└─────────────────────────────────────────────────────────────┘
```

### **10.3 Emergency Contact & Escalation**

```
┌─────────────────────────────────────────────────────────────┐
│             EMERGENCY PROCEDURES                             │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│ LEVEL 1: Device Malfunction                                 │
│  Action: Flip mode switch to MANUAL (kipas)                │
│          Use manual wall switch (lampu/sound)               │
│  Result: Immediate bypass automation                        │
│                                                              │
│ LEVEL 2: Master Logic Down                                  │
│  Action: All devices enter fail-safe after 300s            │
│  Result: Lampu ON, Kipas OFF, Sound OFF                    │
│                                                              │
│ LEVEL 3: Complete System Failure                            │
│  Action: MCB off, use pure manual system                   │
│  Result: Back to traditional manual control                 │
│                                                              │
│ TRAINING: All staff must know:                             │
│  1. Location of mode switches (kipas)                      │
│  2. Location of manual wall switches                        │
│  3. Location of main MCB                                    │
│  4. Emergency contact technician                            │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

---

## 🏁 BAGIAN 11: COMPLIANCE & VALIDATION

### **11.1 Design Compliance**

Sistem ini comply dengan:

✅ **Electrical Safety Standards:**
- PUIL 2020 (Persyaratan Umum Instalasi Listrik)
- Proper L+N switching via DPDT
- Isolated PSU (HLK-PM03)
- Fuse protection per node

✅ **Communication Standards:**
- RS-485 hardware layer: TIA/EIA-485
- CRC error detection: CRC16-MODBUS
- Collision avoidance: Random backoff for discovery

✅ **Software Best Practices:**
- Compare-before-write (NVS wear leveling)
- Hardware watchdog (ESP32 task_wdt)
- Fail-safe by design
- State machine validation

✅ **User Safety:**
- Manual override always available
- Visual indicators (LED)
- Fail-safe actions protect equipment & people
- No single point of failure (manual fallback)

### **11.2 Validation Checklist**

```
┌─────────────────────────────────────────────────────────────┐
│          PRE-DEPLOYMENT VALIDATION                           │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│ HARDWARE:                                                    │
│  [ ] All wiring follow diagram [HIG-3.x]                   │
│  [ ] Mode switch tested (AUTO/MANUAL)                       │
│  [ ] Manual override confirmed working                      │
│  [ ] RS-485 continuity & termination OK                     │
│  [ ] Power supply voltage 5V ±5%                           │
│  [ ] SSR heatsink properly attached                         │
│                                                              │
│ FIRMWARE:                                                    │
│  [ ] All slaves auto-enroll successfully                    │
│  [ ] Heartbeat mechanism tested                             │
│  [ ] Fail-safe triggered at 300s                           │
│  [ ] Manual mode detection works                            │
│  [ ] Profile-specific behavior correct                      │
│                                                              │
│ INTEGRATION:                                                 │
│  [ ] Display Master shows all devices                       │
│  [ ] Web dashboard accessible                               │
│  [ ] Command execution < 2s response                        │
│  [ ] Mode indicators accurate                               │
│                                                              │
│ SAFETY:                                                      │
│  [ ] Emergency procedures documented                        │
│  [ ] Staff trained on manual override                       │
│  [ ] Fail-safe behavior verified                            │
│  [ ] Electrical isolation confirmed                         │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

### **11.3 Acceptance Criteria**

Sistem dinyatakan **READY FOR PRODUCTION** jika:

1. ✅ **Functional Requirements Met:**
   - 18 slave nodes terdaftar dan online
   - Response time command < 2 detik
   - Manual override berfungsi 100%
   - Fail-safe tested dan working

2. ✅ **Reliability Requirements Met:**
   - 24 jam continuous operation tanpa crash
   - Memory tidak leak (free heap stabil)
   - NVS data persist setelah power cycle
   - RS-485 error rate < 1%

3. ✅ **Safety Requirements Met:**
   - Manual control independent dari automation
   - Fail-safe protection active
   - Visual indicators accurate
   - Emergency procedures documented

4. ✅ **Usability Requirements Met:**
   - Dashboard intuitive (user testing)
   - Mode switching clear & easy
   - Status information complete
   - Error messages helpful

---

## 📚 BAGIAN 12: APPENDIX

### **12.1 Glossary**

| Term | Definition |
|------|------------|
| **Auto-Enrollment** | Proses pendaftaran device baru secara otomatis tanpa konfigurasi manual |
| **Changeover Switch** | DPDT switch yang memindahkan sumber power dari satu jalur ke jalur lain |
| **CRC16-MODBUS** | Cyclic Redundancy Check 16-bit untuk deteksi error transmisi |
| **Daisy-chain** | Topologi wiring linear dimana device tersambung berurutan |
| **Fail-Safe** | Mode darurat saat komunikasi terputus >300 detik |
| **Interlock** | Safety mechanism yang cegah multiple relay aktif bersamaan (kipas) |
| **NVS** | Non-Volatile Storage, flash memory untuk simpan config permanent |
| **Profile** | Template behavior device (FAN_4CH, LIGHT_8CH, SOUND_8CH) |
| **SSR** | Solid State Relay, relay elektronik tanpa moving parts |
| **Zero-Cross** | Switching saat AC voltage = 0V untuk minimize electrical noise |

### **12.2 Acronyms**

| Acronym | Full Form |
|---------|-----------|
| **BOM** | Bill of Materials |
| **DPDT** | Double Pole Double Throw |
| **ESP32** | Espressif Systems 32-bit microcontroller |
| **GPIO** | General Purpose Input/Output |
| **HIG** | Hardware Integration Guide |
| **KT** | Kebenaran Tunggal |
| **LVGL** | Light and Versatile Graphics Library |
| **MAC** | Media Access Control (address) |
| **MCB** | Miniature Circuit Breaker |
| **PSU** | Power Supply Unit |
| **RS-485** | Recommended Standard 485 (serial communication) |
| **SPI** | Serial Peripheral Interface |
| **SSR** | Solid State Relay |
| **STP** | Spesifikasi Teknis Protokol |
| **UART** | Universal Asynchronous Receiver-Transmitter |

### **12.3 Additional Resources**

**Datasheets:**
- ESP32-S3: https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf
- MAX3485: https://datasheets.maximintegrated.com/en/ds/MAX3483-MAX3491.pdf
- G3MB-202P: https://omronfs.omron.com/en_US/ecb/products/pdf/en-g3mb.pdf
- HLK-PM03: http://www.hlktech.net/product_detail.php?ProId=59

**Development Tools:**
- PlatformIO: https://platformio.org/
- LVGL Designer: https://lvgl.io/tools/designer
- RS-485 Analyzer: Logic analyzer dengan protocol decode

**Community & Support:**
- ESP32 Forum: https://esp32.com/
- Arduino Forum RS-485: https://forum.arduino.cc/
- LVGL Forum: https://forum.lvgl.io/

---

```
╔═══════════════════════════════════════════════════════════════╗
║                                                               ║
║                   END OF DOCUMENT                            ║
║           KEBENARAN TUNGGAL v1.4 - COMPLETE                  ║
║                                                               ║
║  Dokumen ini adalah SINGLE SOURCE OF TRUTH untuk sistem.     ║
║  Semua implementasi HARUS mengikuti spesifikasi di sini.     ║
║                                                               ║
║  Next: SPESIFIKASI TEKNIS PROTOKOL v1.3                      ║
║                                                               ║
╚═══════════════════════════════════════════════════════════════╝
```

---

**DOKUMEN 1 SELESAI**