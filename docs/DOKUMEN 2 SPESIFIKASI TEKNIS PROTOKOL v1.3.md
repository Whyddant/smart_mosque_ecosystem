# ⚙️ DOKUMEN 2: SPESIFIKASI TEKNIS PROTOKOL v1.3

```
╔═══════════════════════════════════════════════════════════════╗
║                                                               ║
║   SPESIFIKASI TEKNIS PROTOKOL RS-485 v1.3                    ║
║   SMART MOSQUE ECOSYSTEM                                      ║
║                                                               ║
║   Status: Stabil - Untuk Implementasi Firmware               ║
║   Berdasarkan: Kebenaran Tunggal v1.4                        ║
║   Terakhir Diupdate: 27 Desember 2025                        ║
║                                                               ║
╚═══════════════════════════════════════════════════════════════╝
```

---

## 🎯 BAGIAN 1: TUJUAN DAN RUANG LINGKUP

### **1.1 Tujuan Dokumen**

Dokumen ini mendefinisikan protokol komunikasi lengkap antara Master dan Slave dalam sistem Smart Mosque. Setiap implementasi firmware **HARUS** mengikuti spesifikasi ini secara ketat untuk menjamin interoperabilitas.

**Dokumen ini mencakup:**
- Physical layer specification (RS-485 hardware)
- Data link layer (frame format, addressing)
- Application layer (command set, payload structure)
- Error handling & recovery mechanisms
- Timing requirements & tolerances

### **1.2 Target Audience**

- **Firmware Developer**: Yang implementasi protocol stack
- **Integration Engineer**: Yang integrate dengan sistem lain
- **Tester**: Yang validasi protocol compliance
- **Anda sendiri**: Sebagai system architect & implementer

### **1.3 Referensi Wajib**

| Dokumen | Bagian | Topik |
|---------|--------|-------|
| [KT-2.1] | Arsitektur | Diagram sistem & addressing |
| [KT-3.1] | Fail-Safe | Timeout & emergency actions |
| [KT-6.1] | Hardware | Pinout standard GPIO |
| [HIG-3.1] | Wiring | RS-485 bus physical connection |

---

## 🔌 BAGIAN 2: PHYSICAL & DATA LINK LAYER

### **2.1 Spesifikasi Physical Layer**

```
┌─────────────────────────────────────────────────────────────┐
│             RS-485 PHYSICAL SPECIFICATIONS                   │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  Media:           UTP Cat5/6 (solid copper)                 │
│  Topology:        Bus linear (daisy-chain)                  │
│  Max Length:      200 meters @ 115200bps                    │
│  Max Nodes:       32 (theoretical), 20 (practical)          │
│  Termination:     120Ω resistor at both ends                │
│  Driver IC:       MAX3485 or compatible                     │
│  Voltage Levels:  Differential ±(1.5V to 5V)               │
│                                                              │
│  CRITICAL:                                                   │
│  • NO star/branch topology                                  │
│  • Termination ONLY at first & last node                    │
│  • Common ground MANDATORY (pair coklat UTP)                │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

#### **2.1.1 Cable Specification**

```
UTP Cat5/6 Pin Assignment (T568B Standard):
┌─────────────────────────────────────────────────────────────┐
│                                                              │
│  Pair 1 (Biru):                                             │
│    • Pin 5: Blue-White  → RS-485 A+ (DATA+)                │
│    • Pin 4: Blue        → RS-485 A- (not used)             │
│                                                              │
│  Pair 2 (Hijau):                                            │
│    • Pin 1: Green       → RS-485 B+ (not used)             │
│    • Pin 2: Green-White → RS-485 B- (DATA-)                │
│                                                              │
│  Pair 3 (Coklat):                                           │
│    • Pin 7: Brown-White → Common GND                        │
│    • Pin 8: Brown       → Common GND (parallel)            │
│                                                              │
│  Pair 4 (Orange): NOT USED (reserve/future)                │
│                                                              │
│  CONVENTION:                                                 │
│  • A+ (Blue-White)  = Positive data line                    │
│  • B- (Green-White) = Negative data line                    │
│  • GND (Brown pair) = Reference ground                      │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

#### **2.1.2 Transceiver Connection**

```
MAX3485 PIN MAPPING:
┌─────────────────────────────────────────────────────────────┐
│                                                              │
│   MAX3485         ESP32          RS-485 Bus                 │
│   ───────         ─────          ───────────                │
│                                                              │
│   RO (1) ────────► GPIO16 (RX)                              │
│   RE (2) ────┐                                              │
│   DE (3) ────┴───► GPIO4  (DE/RE)                           │
│   DI (4) ────────► GPIO17 (TX)                              │
│                                                              │
│   GND (5) ───────────────────────► GND (Brown pair)        │
│   A  (6) ────────────────────────► A+ (Blue-White)         │
│   B  (7) ────────────────────────► B- (Green-White)        │
│   VCC (8) ──────► 3.3V                                      │
│                                                              │
│  CONTROL LOGIC:                                             │
│    DE/RE = LOW  → Receive mode (listen to bus)             │
│    DE/RE = HIGH → Transmit mode (drive bus)                │
│                                                              │
│  TIMING:                                                     │
│    Before TX: Set DE/RE HIGH, wait 1ms                      │
│    After TX:  Wait for last byte sent, set DE/RE LOW        │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

### **2.2 Data Link Layer Specification**

```
┌─────────────────────────────────────────────────────────────┐
│              SERIAL COMMUNICATION PARAMETERS                 │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  Baud Rate:       115200 bps (FIXED, do not change)        │
│  Data Bits:       8                                          │
│  Parity:          None                                       │
│  Stop Bits:       1                                          │
│  Flow Control:    None (hardware control via DE/RE)         │
│                                                              │
│  Format:          8N1 (8 data, no parity, 1 stop)          │
│                                                              │
│  Bit Time:        ~8.68 microseconds (1/115200)             │
│  Byte Time:       ~86.8 microseconds (10 bits total)        │
│  Max Throughput:  ~11,520 bytes/second (theoretical)        │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

#### **2.2.1 Timing Requirements**

| Parameter | Value | Description |
|-----------|-------|-------------|
| **Inter-frame gap** | ≥ 3.5 char times | Minimum 303μs between frames |
| **Turnaround delay** | ≥ 1 ms | Slave response delay minimum |
| **DE/RE setup** | ≥ 1 ms | Before transmit, enable driver |
| **DE/RE hold** | ≥ 100 μs | After last byte, before disable |
| **Heartbeat interval** | 60 ± 5 seconds | Master broadcast heartbeat |
| **Discovery interval** | 1-3 seconds (random) | Slave announce interval |
| **Command timeout** | 3 seconds | Max wait for response |
| **Fail-safe timeout** | 300 seconds | No heartbeat → fail-safe |

#### **2.2.2 Bus Access Control**

```
COLLISION AVOIDANCE STRATEGY:
┌─────────────────────────────────────────────────────────────┐
│                                                              │
│  MASTER:                                                     │
│    • Can transmit anytime (bus master privilege)           │
│    • Should avoid flooding (max 10 commands/second)         │
│                                                              │
│  SLAVE:                                                      │
│    • Only transmit when:                                    │
│      a) Response to Master command (requested)              │
│      b) Unsolicited status report (infrequent)              │
│      c) Discovery announce (ID=0 only)                      │
│                                                              │
│    • Before transmit:                                       │
│      1. Check bus idle (no activity 3.5 char times)        │
│      2. Random backoff if collision suspected               │
│                                                              │
│  DISCOVERY MODE (ID=0):                                     │
│    • Random interval: 1000-3000 ms                          │
│    • Exponential backoff if no response                     │
│    • Max attempts: 100 (then restart)                       │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

---

## 📦 BAGIAN 3: FRAME FORMAT & STRUCTURE

### **3.1 Frame Structure Byte-by-Byte**

```
COMPLETE FRAME LAYOUT:
┌───┬───┬───┬───┬───────────────┬────┬────┬───┐
│ 0 │ 1 │ 2 │ 3 │   4 ... N+3   │N+4 │N+5 │N+6│
├───┼───┼───┼───┼───────────────┼────┼────┼───┤
│STX│ADR│CMD│LEN│   PAYLOAD     │CRC │CRC │ETX│
│   │   │   │   │               │ _L │ _H │   │
└───┴───┴───┴───┴───────────────┴────┴────┴───┘
 (1) (1) (1) (1)    (0-255)       (1)  (1)  (1)

MINIMUM FRAME: 7 bytes (no payload)
MAXIMUM FRAME: 262 bytes (255 payload + 7 overhead)
```

### **3.2 Field Specifications**

#### **3.2.1 START_DELIM (Byte 0)**

```
Value:      0x7E (126 decimal, '~' ASCII)
Purpose:    Frame synchronization
Mandatory:  YES

Receiver behavior:
  • Scan for 0x7E to detect frame start
  • Discard any data before 0x7E
  • Reset frame parser state machine
```

#### **3.2.2 ADDRESS (Byte 1)**

```
Value Range:  0x00 - 0xFF (0-255)
Purpose:      Target or source device ID

ADDRESS ALLOCATION:
┌──────────────────────────────────────────────────────────┐
│ Value    │ Meaning                │ Usage                │
├──────────────────────────────────────────────────────────┤
│ 0x00     │ Broadcast              │ All slaves process   │
│ 0x01-F7  │ Slave IDs (1-247)      │ Individual slave     │
│ 0xF8-FD  │ Reserved (future)      │ Not used yet         │
│ 0xFE     │ Logic Master           │ Fixed ID             │
│ 0xFF     │ Display Master         │ Fixed ID             │
└──────────────────────────────────────────────────────────┘

DIRECTION RULES:
  Master → Slave:  ADDRESS = target slave ID or 0x00
  Slave → Master:  ADDRESS = slave's own ID (source)
  
BROADCAST COMMANDS (0x00):
  • HEARTBEAT (0x04)
  • DISCOVERY_RESPONSE (0x02) - optional
  
Slave MUST process if:
  • ADDRESS == own ID, OR
  • ADDRESS == 0x00 (broadcast)
```

#### **3.2.3 COMMAND (Byte 2)**

```
Value Range:  0x01 - 0xFF
Purpose:      Command opcode

DEFINED OPCODES:
┌──────────────────────────────────────────────────────────┐
│ Code │ Name                │ Direction      │ Priority   │
├──────────────────────────────────────────────────────────┤
│ 0x01 │ DISCOVERY_ANNOUNCE  │ Slave→Master   │ High       │
│ 0x02 │ DISCOVERY_RESPONSE  │ Master→Slave   │ High       │
│ 0x03 │ SET_RELAY           │ Master→Slave   │ Normal     │
│ 0x04 │ HEARTBEAT           │ Master→All     │ Critical   │
│ 0x05 │ STATUS_REPORT       │ Slave→Master   │ Normal     │
│ 0x06 │ FORCE_RESET         │ Master→Slave   │ High       │
│ 0x07 │ ACK                 │ Bidirectional  │ Normal     │
│ 0x08 │ NACK                │ Bidirectional  │ Normal     │
│ 0x09-FF │ Reserved (future) │ -              │ -          │
└──────────────────────────────────────────────────────────┘

Unknown opcodes MUST return NACK (0x08) with error code 0x03.
```

#### **3.2.4 LENGTH (Byte 3)**

```
Value Range:  0x00 - 0xFF (0-255)
Purpose:      Payload length in bytes

Rules:
  • LENGTH = 0: No payload (valid for HEARTBEAT, ACK, etc.)
  • LENGTH > 0: Payload follows immediately after this byte
  • Receiver MUST validate: actual bytes received == LENGTH
  • If mismatch: Discard frame, send NACK (error 0x02)
```

#### **3.2.5 PAYLOAD (Bytes 4 to N+3)**

```
Length:     0 to 255 bytes (variable)
Format:     Command-specific (JSON or binary)
Purpose:    Command parameters or response data

PAYLOAD TYPES:
┌──────────────────────────────────────────────────────────┐
│ Format   │ Commands                    │ Example         │
├──────────────────────────────────────────────────────────┤
│ JSON     │ DISCOVERY_ANNOUNCE          │ {"mac":"..."}   │
│          │ DISCOVERY_RESPONSE          │                 │
│          │ STATUS_REPORT               │                 │
│          │                              │                 │
│ Binary   │ SET_RELAY                   │ 0x03 0x01 ...   │
│          │ HEARTBEAT                   │ 0xAA            │
│          │                              │                 │
│ Mixed    │ NACK                        │ JSON with code  │
└──────────────────────────────────────────────────────────┘

JSON RULES:
  • UTF-8 encoding
  • No newline/whitespace padding
  • Compact format (no pretty-print)
  • Max nesting: 3 levels
```

#### **3.2.6 CRC16 (Bytes N+4 and N+5)**

```
Algorithm:  CRC16-MODBUS
Polynomial: 0x8005 (x^16 + x^15 + x^2 + 1)
Init Value: 0xFFFF
Final XOR:  0x0000
Byte Order: Little-endian (CRC_L first, then CRC_H)

DATA RANGE FOR CRC:
  Start: Byte 1 (ADDRESS)
  End:   Byte N+3 (last byte of PAYLOAD)
  Total: 3 + LENGTH bytes

CALCULATION:
  CRC = CRC16_MODBUS(frame[1 .. N+3])
  frame[N+4] = CRC & 0xFF        // Low byte
  frame[N+5] = (CRC >> 8) & 0xFF // High byte

VALIDATION:
  received_crc = frame[N+4] | (frame[N+5] << 8)
  calculated_crc = CRC16_MODBUS(frame[1 .. N+3])
  
  if (received_crc != calculated_crc) {
    // Frame corrupted, send NACK (error 0x01)
  }
```

#### **3.2.7 END_DELIM (Byte N+6)**

```
Value:      0x0A (10 decimal, '\n' newline)
Purpose:    Frame termination marker
Mandatory:  YES

Receiver behavior:
  • Expect 0x0A after CRC
  • If mismatch: Frame invalid, discard
  • After 0x0A: Frame complete, process or wait next
```

---

## 📋 BAGIAN 4: COMMAND SPECIFICATIONS

### **4.1 Command Summary Table**

| Opcode | Name | Direction | Length | Response | Timeout |
|--------|------|-----------|--------|----------|---------|
| 0x01 | DISCOVERY_ANNOUNCE | S→M | ~60 | 0x02 or ignore | 5 min |
| 0x02 | DISCOVERY_RESPONSE | M→S | ~80 | 0x07 | 3 sec |
| 0x03 | SET_RELAY | M→S | 9 or ~50 | 0x05+0x07 | 3 sec |
| 0x04 | HEARTBEAT | M→All | 1 | None | - |
| 0x05 | STATUS_REPORT | S→M | ~100 | 0x07 | - |
| 0x06 | FORCE_RESET | M→S | 0 | 0x07 | 3 sec |
| 0x07 | ACK | Both | 0-20 | None | - |
| 0x08 | NACK | Both | ~30 | None | - |

Legend: M=Master, S=Slave

---

### **4.2 Command Details**

#### **4.2.1 DISCOVERY_ANNOUNCE (0x01)**

**Purpose:** Slave dengan ID=0 (unregistered) announce keberadaannya ke Master untuk enrollment.

**Direction:** Slave → Master (broadcast ke 0x00 atau unicast ke 0xFE)

**Trigger:** 
- Boot dengan ID=0 di NVS
- Retry setiap 1-3 detik (random) jika belum dapat response

**Payload Format (JSON):**
```json
{
  "mac": "A0:B1:C2:D3:E4:F5",    // MAC address ESP32 (string, 17 chars)
  "profile": "FAN_4CH",          // Profile type (string)
  "ver": "1.0.0",                // Firmware version (string)
  "rssi": -65                    // WiFi RSSI (integer, optional)
}
```

**Profile Values:**
- `"FAN_4CH"` = Kipas 4-channel dengan mode switch
- `"LIGHT_8CH"` = Lampu 8-channel
- `"SOUND_8CH"` = Audio 8-channel

**Example Frame (hex):**
```
7E 00 01 3C 7B226D6163223A2241303A42313A43323A44333A45343A4635222C2270726F66696C65223A2246414E5F344348222C22766572223A22312E302E30227D XX XX 0A
│  │  │  │  └─ JSON payload (60 bytes)                                                                    └─CRC─┘ │
│  │  │  └─ Length: 0x3C = 60 bytes
│  │  └─ Command: 0x01 (DISCOVERY_ANNOUNCE)
│  └─ Address: 0x00 (broadcast)
└─ Start: 0x7E
```

**Master Behavior:**
1. Receive DISCOVERY_ANNOUNCE
2. Parse MAC address
3. Check if MAC already registered:
   - YES: Send existing ID via DISCOVERY_RESPONSE
   - NO: Store in "Pending Devices" list
4. Notify Display Master (show popup)
5. Wait for user input (name assignment)
6. Send DISCOVERY_RESPONSE with new ID

**Slave Behavior:**
1. Send DISCOVERY_ANNOUNCE
2. Wait up to 5 seconds for DISCOVERY_RESPONSE
3. If timeout: Wait random 1-3s, retry
4. Max 100 attempts, then restart ESP32

---

#### **4.2.2 DISCOVERY_RESPONSE (0x02)**

**Purpose:** Master assign ID dan nama ke slave yang baru enroll.

**Direction:** Master → Slave (broadcast 0x00 atau unicast ke MAC)

**Trigger:** User confirm enrollment dari Display Master

**Payload Format (JSON):**
```json
{
  "mac": "A0:B1:C2:D3:E4:F5",    // Target MAC (verify recipient)
  "assigned_id": 5,               // New ID (1-247)
  "device_name": "Kipas Saf 1",   // User-assigned name
  "profile": "FAN_4CH",           // Confirm profile
  "timestamp": 1735300800         // Unix timestamp (optional)
}
```

**Example Frame (hex):**
```
7E 00 02 50 7B226D6163223A2241303A...227D XX XX 0A
│  │  │  │  └─ JSON payload (~80 bytes)
│  │  │  └─ Length: 0x50 = 80 bytes
│  │  └─ Command: 0x02
│  └─ Address: 0x00 (broadcast) or specific ID
└─ Start: 0x7E
```

**Slave Behavior:**
1. Receive DISCOVERY_RESPONSE
2. Validate MAC address (must match own MAC)
3. If match:
   a. Extract assigned_id and device_name
   b. Validate assigned_id (1-247)
   c. **Compare with existing NVS** (compare-before-write):
      ```cpp
      uint8_t oldID = preferences.getUChar("device_id", 0);
      if (oldID == assigned_id) {
        // Sama, skip write
        Serial.println("ID sama, skip NVS write");
      } else {
        // Beda, write baru
        preferences.putUChar("device_id", assigned_id);
      }
      ```
   d. Save to NVS (namespace "config")
   e. Send ACK to Master
   f. Restart ESP32
4. If MAC mismatch: Ignore frame

**NVS Keys Written:**
```cpp
namespace "config" {
  "device_id": uint8_t          // 1-247
  "device_name": string         // Max 32 chars
  "profile": uint8_t            // 1,2,3
  "config_crc": uint16_t        // Checksum untuk validasi
}
```

---

#### **4.2.3 SET_RELAY (0x03)**

**Purpose:** Master kontrol relay di Slave.

**Direction:** Master → Slave (unicast ke specific ID)

**Payload Format (2 options):**

**OPTION A: Binary (Efficient, recommended)**
```
Byte 0:     Relay mask (bitmask which relays to change)
Byte 1-8:   Values for relay 1-8 (0x00=OFF, 0x01=ON)

Total: 9 bytes

Example: Nyalakan relay 2 dan 7
  Mask: 0x42 (binary 01000010 = bit 1 and 6 set)
  Values: [0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00]
```

**OPTION B: JSON (Debugging, verbose)**
```json
{
  "relay_mask": 66,              // Decimal of 0x42
  "values": [0,1,0,0,0,0,1,0]   // Array of 8 integers
}
```

**Example Frame (Binary, hex):**
```
7E 05 03 09 42 00 01 00 00 00 00 01 00 XX XX 0A
│  │  │  │  │  └─ Values: relay 1-8
│  │  │  │  └─ Mask: 0x42 (relay 2 & 7)
│  │  │  └─ Length: 0x09 = 9 bytes
│  │  └─ Command: 0x03 (SET_RELAY)
│  └─ Address: 0x05 (Slave ID 5)
└─ Start: 0x7E
```

**Special Rules for FAN_4CH:**

```cpp
// INTERLOCK: Hanya satu dari relay 1-3 (LOW/MED/HIGH) boleh ON
void validateFanCommand(uint8_t* values) {
  int speedRelayCount = 0;
  
  if (values[0]) speedRelayCount++; // LOW
  if (values[1]) speedRelayCount++; // MED
  if (values[2]) speedRelayCount++; // HIGH
  
  if (speedRelayCount > 1) {
    sendNACK(ERROR_INTERLOCK_VIOLATION);
    return;
  }
  
  // Valid, proceed with dead-time
  setFanSpeed(values);
}

void setFanSpeed(uint8_t* values) {
  // Turn off current speed
  turnOffAllSpeeds();
  
  // Dead-time: 200ms
  delay(200);
  
  // Turn on new speed
  if (values[0]) digitalWrite(SSR_LOW_PIN, HIGH);
  if (values[1]) digitalWrite(SSR_MED_PIN, HIGH);
  if (values[2]) digitalWrite(SSR_HIGH_PIN, HIGH);
}
```

**Slave Behavior:**
1. Receive SET_RELAY
2. Check current mode:
   - **AUTO**: Proceed to step 3
   - **MANUAL**: Send NACK (error 0x04), abort
   - **FAILSAFE**: Ignore command (maintain fail-safe state)
3. Validate command:
   - For FAN: Check interlock violation
   - For LIGHT/SOUND: Direct execution OK
4. Execute relay change
5. Send STATUS_REPORT (confirm new state)
6. Send ACK to Master

**Master Behavior:**
1. Send SET_RELAY
2. Wait up to 3 seconds for ACK + STATUS_REPORT
3. If timeout: Retry (max 3 attempts with exponential backoff)
4. If NACK received: Check error code, handle accordingly
5. Update dashboard UI with new state

---

#### **4.2.4 HEARTBEAT (0x04)**

**Purpose:** Master broadcast "alive signal" ke semua Slave untuk prevent fail-safe.

**Direction:** Master → All (broadcast 0x00)

**Interval:** 60 seconds ± 5% (57-63 seconds)

**Payload:** 1 byte, arbitrary value (typically 0xAA)

**Example Frame (hex):**
```
7E 00 04 01 AA XX XX 0A
│  │  │  │  │  └─CRC─┘│
│  │  │  │  └─ Payload: 0xAA (alive signal)
│  │  │  └─ Length: 0x01 = 1 byte
│  │  └─ Command: 0x04 (HEARTBEAT)
│  └─ Address: 0x00 (broadcast)
└─ Start: 0x7E
```

**Master Implementation:**
```cpp
void sendHeartbeatTask() {
  static uint32_t lastHeartbeat = 0;
  const uint32_t INTERVAL = 60000; // 60 seconds
  
  if (millis() - lastHeartbeat >= INTERVAL) {
    uint8_t payload = 0xAA;
    sendRS485Frame(0x00, 0x04, &payload, 1);
    
    lastHeartbeat = millis();
    Serial.println("→ Heartbeat sent");
  }
}
```

**Slave Implementation:**
```cpp
volatile uint32_t lastHeartbeatReceived = 0;

void handleHeartbeat() {
  // Update timestamp (called from ISR or main loop)
  lastHeartbeatReceived = millis();
  
  // If was in FAILSAFE, recover
  if (currentState == STATE_FAILSAFE) {
    Serial.println("✓ Heartbeat restored, exiting fail-safe");
    exitFailSafeMode();
  }
}

void checkFailSafeTimeout() {
  uint32_t elapsed = millis() - lastHeartbeatReceived;
  
  if (elapsed > 300000) { // 300 seconds = 5 minutes
    if (currentState != STATE_FAILSAFE) {
      Serial.println("⚠ FAIL-SAFE TIMEOUT!");
      enterFailSafeMode();
    }
  }
}
```

**CRITICAL NOTES:**
- Heartbeat TIDAK disimpan ke NVS (volatile data)
- Setelah restart, Slave menunggu first heartbeat (timeout 300s dari boot)
- Master crash/restart → All slaves fail-safe after 300s
- Master SHOULD maintain real-time clock (RTC/NTP) to store epoch seconds for last_seen; jika tidak tersedia, gunakan offset dari millis() dan lakukan normalisasi saat sinkronisasi waktu.
- Manual override (kipas) tidak affected oleh fail-safe

---

#### **4.2.5 STATUS_REPORT (0x05)**

**Purpose:** Slave report status terkini ke Master.

**Direction:** Slave → Master (unicast ke 0xFE)

**Trigger:**
- Response to SET_RELAY (confirm execution)
- Mode change (AUTO↔MANUAL↔FAILSAFE)
- Periodic (every 60 seconds, optional)
- On request (future command)

**Payload Format (JSON):**
```json
{
  "device_id": 5,
  "control_source": "AUTO",         // "AUTO", "MANUAL", "FAILSAFE"
  "relay_states": [0,1,0,0,0,0,0,0], // Array 8 int (0/1)
  "fan_speed": "MED",               // FAN_4CH only: "OFF","LOW","MED","HIGH"
  "mode_switch": "AUTO",            // FAN_4CH only: "AUTO","MANUAL"
  "temp": 28.5,                     // Optional: temperature (float)
  "rssi": -68,                      // Optional: WiFi RSSI (int)
  "uptime": 86400                   // Optional: uptime seconds (uint32)
}
```

**Field Descriptions:**

| Field | Type | Values | Mandatory | Notes |
|-------|------|--------|-----------|-------|
| device_id | uint8_t | 1-247 | YES | Slave's own ID |
| control_source | string | AUTO/MANUAL/FAILSAFE | YES | Current control mode |
| relay_states | array[8] | 0 or 1 | YES | Actual SSR output states |
| fan_speed | string | OFF/LOW/MED/HIGH | FAN only | Derived from relay states |
| mode_switch | string | AUTO/MANUAL | FAN only | Physical DPDT position |
| temp | float | -40 to 125 | NO | From DHT22 if available |
| rssi | int | -120 to 0 | NO | WiFi signal strength |
| uptime | uint32 | 0 to 4294967295 | NO | Seconds since boot |
```

**Example Frames:**

**FAN_4CH in AUTO mode, speed MEDIUM:**
```json
{
  "device_id": 5,
  "control_source": "AUTO",
  "relay_states": [0,1,0,0,0,0,0,0],
  "fan_speed": "MED",
  "mode_switch": "AUTO",
  "temp": 28.5,
  "uptime": 3600
}
```

**FAN_4CH in MANUAL mode:**
```json
{
  "device_id": 5,
  "control_source": "MANUAL",
  "relay_states": [0,0,0,0,0,0,0,0],
  "fan_speed": "UNKNOWN",
  "mode_switch": "MANUAL",
  "note": "SSR path isolated by DPDT switch"
}
```

**LIGHT_8CH in FAILSAFE (all ON):**
```json
{
  "device_id": 14,
  "control_source": "FAILSAFE",
  "relay_states": [1,1,1,1,1,1,1,1],
  "uptime": 450
}
```

**Example Frame (hex, truncated for readability):**
```
7E FE 05 64 7B22646576696365...227D XX XX 0A
│  │  │  │  └─ JSON payload (~100 bytes)
│  │  │  └─ Length: 0x64 = 100 bytes
│  │  └─ Command: 0x05 (STATUS_REPORT)
│  └─ Address: 0xFE (Logic Master)
└─ Start: 0x7E
```

**Master Behavior:**
1. Receive STATUS_REPORT
2. Parse JSON payload
3. Update device registry in database:
   ```cpp
   void processStatusReport(uint8_t slaveId, StatusReport& report) {
     DeviceRecord& record = deviceRegistry[slaveId];
     
     // Compare before write (NVS optimization)
     bool changed = false;
     
     if (record.relay_state != report.relay_bitmask) {
       record.relay_state = report.relay_bitmask;
       changed = true;
     }
     
     if (record.control_source != report.control_source) {
       record.control_source = report.control_source;
       changed = true;
     }
     
     // Always update last_seen as epoch seconds (do not trigger NVS write by itself)
     record.last_seen = now(); // epoch seconds from RTC/NTP
     record.online = (now() - record.last_seen) < 310; // online if <= ~5m10s
     
     if (changed) {
       markDirty(slaveId); // Schedule NVS flush
     }
     
     // Sync to Display Master
     notifyDisplayUpdate(slaveId, report);
   }
   ```
4. Send ACK to Slave (optional, for confirmation)

**Slave Behavior:**
1. Construct JSON payload
2. Send to Logic Master (0xFE)
3. Optional: Wait for ACK (timeout 1s)
4. Don't retry if no ACK (unsolicited report)

---

#### **4.2.6 FORCE_RESET (0x06) - OPTIONAL**

**Purpose:** Master force Slave reset ke factory default (clear NVS).

**Direction:** Master → Slave (unicast)

**Payload:** Empty (0 bytes) or confirmation code

**WARNING:** Ini destructive action! Slave akan lose ID dan perlu re-enrollment.

**Use Cases:**
- Troubleshooting NVS corruption
- Re-assign device to different location
- Factory reset for replacement/repair

**Example Frame (hex):**
```
7E 05 06 00 XX XX 0A
│  │  │  │  └─CRC─┘│
│  │  │  └─ Length: 0x00 (no payload)
│  │  └─ Command: 0x06 (FORCE_RESET)
│  └─ Address: 0x05 (target slave)
└─ Start: 0x7E
```

**Slave Behavior:**
```cpp
void handleForceReset() {
  Serial.println("⚠ FORCE_RESET received!");
  Serial.println("  Clearing NVS configuration...");
  
  // Backup MAC for logging
  uint8_t mac[6];
  getMacAddress(mac);
  
  // Clear all config
  preferences.begin("config", false);
  preferences.clear();
  preferences.end();
  
  // Send ACK before restart
  sendACK();
  delay(100); // Ensure ACK transmitted
  
  // Log to serial
  Serial.printf("  Device %02X:%02X:%02X:%02X:%02X:%02X reset\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.println("  Restarting in discovery mode...");
  
  delay(1000);
  ESP.restart();
}
```

**Master Behavior:**
1. Show confirmation dialog to user:
   ```
   ⚠ WARNING: Reset device "Kipas Saf 1" (ID 5)?
   This will erase configuration and require re-enrollment.
   
   [Cancel] [Confirm]
   ```
2. If confirmed:
   a. Send FORCE_RESET
   b. Wait for ACK (3 seconds)
   c. Remove device from registry
   d. Notify Display Master
3. Device will restart and broadcast DISCOVERY_ANNOUNCE

**Security Consideration:**
```cpp
// Optional: Require confirmation code
struct ForceResetPayload {
  uint32_t magic;     // Must be 0xDEADBEEF
  uint16_t device_id; // Must match target ID
};

// Slave validates:
if (payload.magic != 0xDEADBEEF || 
    payload.device_id != myDeviceId) {
  sendNACK(ERROR_INVALID_COMMAND);
  return;
}
```

---

#### **4.2.7 ACK (0x07)**

**Purpose:** Acknowledge successful reception and processing.

**Direction:** Bidirectional

**Payload:** Empty or JSON with additional info

**Example Frame (Simple ACK, no payload):**
```
7E 05 07 00 XX XX 0A
│  │  │  │  └─CRC─┘│
│  │  │  └─ Length: 0x00
│  │  └─ Command: 0x07 (ACK)
│  └─ Address: 0x05 (from slave)
└─ Start: 0x7E
```

**Example Frame (ACK with info):**
```json
{
  "code": 0,
  "msg": "OK",
  "executed": true
}
```

**When to Send ACK:**
- After successful command execution
- After receiving critical configuration
- As confirmation in request-response pattern

**When NOT to Send ACK:**
- After HEARTBEAT (no response needed)
- After unsolicited STATUS_REPORT
- If already sending STATUS_REPORT as response

---

#### **4.2.8 NACK (0x08)**

**Purpose:** Negative acknowledgment - command failed or error detected.

**Direction:** Bidirectional

**Payload:** JSON with error code and message

**Payload Format:**
```json
{
  "code": 4,
  "msg": "Device in MANUAL mode",
  "detail": "Flip mode switch to AUTO"
}
```

**Error Code Table:**

| Code | Message | Cause | Resolution |
|------|---------|-------|------------|
| 0x00 | OK | No error (shouldn't be NACK) | - |
| 0x01 | Invalid CRC | Frame corruption | Retransmit |
| 0x02 | Payload too long | LENGTH > actual | Check frame format |
| 0x03 | Invalid command | Unknown opcode | Update firmware |
| 0x04 | Manual mode active | DPDT in MANUAL | Switch to AUTO |
| 0x05 | Interlock violation | Multiple speed ON | Fix command |
| 0x06 | NVS write error | Flash failure | Check hardware |
| 0x07 | Profile mismatch | Wrong command for profile | Check device type |
| 0x08 | Device not found | ID not registered | Check enrollment |
| 0x09 | Timeout | Response too slow | Check bus load |
| 0x0A | Buffer overflow | Too many commands | Slow down |

**Example Frame (hex):**
```
7E FE 08 28 7B22636F6465223A342C226D7367223A224D616E75616C206D6F6465227D XX XX 0A
│  │  │  │  └─ JSON: {"code":4,"msg":"Manual mode"}
│  │  │  └─ Length: 0x28 = 40 bytes
│  │  └─ Command: 0x08 (NACK)
│  └─ Address: 0xFE (to Master)
└─ Start: 0x7E
```

**Sender Behavior:**
```cpp
void sendNACK(uint8_t errorCode, const char* detail = nullptr) {
  StaticJsonDocument<128> doc;
  doc["code"] = errorCode;
  doc["msg"] = getErrorMessage(errorCode);
  
  if (detail) {
    doc["detail"] = detail;
  }
  
  String payload;
  serializeJson(doc, payload);
  
  sendRS485Frame(MASTER_ADDR, 0x08, 
                 (uint8_t*)payload.c_str(), payload.length());
}
```

**Receiver Behavior:**
```cpp
void handleNACK(uint8_t fromAddr, uint8_t* payload, uint8_t len) {
  StaticJsonDocument<128> doc;
  deserializeJson(doc, payload, len);
  
  uint8_t errorCode = doc["code"];
  const char* message = doc["msg"];
  
  Serial.printf("✗ NACK from device %d: [0x%02X] %s\n", 
                fromAddr, errorCode, message);
  
  // Handle specific errors
  switch(errorCode) {
    case 0x04: // Manual mode
      // Update UI: Show manual override indicator
      notifyManualOverride(fromAddr);
      break;
      
    case 0x05: // Interlock
      // Log error, don't retry
      logInterlockViolation(fromAddr);
      break;
      
    case 0x01: // CRC error
    case 0x09: // Timeout
      // Retry with exponential backoff
      scheduleRetry(fromAddr);
      break;
      
    default:
      // Generic error handling
      logError(fromAddr, errorCode, message);
  }
}
```

---

## 🔄 BAGIAN 5: PROTOCOL STATE MACHINES

### **5.1 Slave State Machine (Complete)**

```
┌─────────────────────────────────────────────────────────────┐
│                   SLAVE STATE MACHINE                        │
└─────────────────────────────────────────────────────────────┘

                        [POWER ON]
                            │
                            ▼
                    [INIT HARDWARE]
                            │
                            ▼
                    [READ NVS CONFIG]
                            │
                    ┌───────┴────────┐
                    │                │
                 ID = 0           ID = 1-247
                    │                │
                    ▼                ▼
            ┌───────────────┐  [VALIDATE CONFIG]
            │  DISCOVERY    │        │
            │  MODE         │        │
            └───────┬───────┘        ▼
                    │         [OPERATIONAL MODE]
                    │                │
                    │         ┌──────┴──────┐
                    │         │             │
                    │    Profile?       Profile?
                    │    KIPAS          LAMPU/SOUND
                    │         │             │
                    │         ▼             ▼
                    │    [Check Mode]  [STATE_AUTO]
                    │         │             │
                    │    ┌────┴────┐        │
                    │    │         │        │
                    │  DPDT      DPDT       │
                    │   AUTO    MANUAL      │
                    │    │         │        │
                    │    ▼         ▼        │
                    │ [AUTO]  [MANUAL]      │
                    │    │         │        │
                    │    └────┬────┴────────┘
                    │         │
                    │         ▼
                    │   [Monitor Heartbeat]
                    │         │
                    │    Timeout >300s?
                    │         │
                    │         ▼
                    │   [FAIL-SAFE MODE]
                    │         │
                    │    Heartbeat OK?
                    │         │
                    │         ▼
                    │   [Return to AUTO/MANUAL]
                    │
                    └─[DISCOVERY_RESPONSE]─→ [Save Config]
                                                    │
                                                    ▼
                                               [RESTART]
```

**State Descriptions:**

```cpp
enum SlaveState {
  STATE_INIT,           // Initializing hardware
  STATE_DISCOVERY,      // ID=0, waiting for enrollment
  STATE_OPERATIONAL,    // ID assigned, normal operation
  STATE_AUTO,           // Operational + AUTO mode
  STATE_MANUAL,         // Operational + MANUAL mode (kipas only)
  STATE_FAILSAFE        // No heartbeat >300s
};

// State transition rules
void updateState() {
  switch(currentState) {
    case STATE_INIT:
      if (nvs_valid && device_id > 0) {
        currentState = STATE_OPERATIONAL;
      } else {
        currentState = STATE_DISCOVERY;
      }
      break;
      
    case STATE_DISCOVERY:
      if (received_discovery_response) {
        saveConfigToNVS();
        ESP.restart();
      }
      break;
      
    case STATE_OPERATIONAL:
      if (profile == PROFILE_FAN_4CH) {
        // Check mode switch
        if (isManualMode()) {
          currentState = STATE_MANUAL;
        } else {
          currentState = STATE_AUTO;
        }
      } else {
        // Lampu/Sound always AUTO
        currentState = STATE_AUTO;
      }
      break;
      
    case STATE_AUTO:
    case STATE_MANUAL:
      // Check heartbeat timeout
      if (millis() - lastHeartbeat > 300000) {
        currentState = STATE_FAILSAFE;
        executeFailSafe();
      }
      break;
      
    case STATE_FAILSAFE:
      // Check heartbeat recovery
      if (millis() - lastHeartbeat < 300000) {
        Serial.println("✓ Heartbeat restored");
        currentState = STATE_OPERATIONAL;
      }
      break;
  }
}
```

**LED Indicators:**

| State | LED Color | Blink Pattern | Meaning |
|-------|-----------|---------------|---------|
| INIT | White | Fast blink (100ms) | Booting |
| DISCOVERY | Blue | Medium blink (500ms) | Waiting enrollment |
| AUTO | Green | Steady or slow blink | Normal operation |
| MANUAL | Yellow | Steady | Manual override active |
| FAILSAFE | Red | Steady | Emergency mode |

---

### **5.2 Master State Machine (Simplified)**

```
┌─────────────────────────────────────────────────────────────┐
│                  MASTER STATE MACHINE                        │
└─────────────────────────────────────────────────────────────┘

                        [POWER ON]
                            │
                            ▼
                    [INIT HARDWARE]
                            │
                            ▼
                    [LOAD DATABASE FROM NVS]
                            │
                            ▼
                    [START HEARTBEAT TIMER]
                            │
                            ▼
                ┌───────[OPERATIONAL]───────┐
                │                            │
                ▼                            ▼
        [Listen Commands]           [Send Heartbeat]
        from Display Master         every 60 seconds
                │                            │
                ▼                            │
        [Relay to Slaves]                    │
                │                            │
                ▼                            │
        [Collect Status]                     │
                │                            │
                └────────────┬───────────────┘
                             │
                             ▼
                    [Update Database]
                             │
                             ▼
                    [Sync to Display]
```

**Key Tasks:**

```cpp
void masterMainLoop() {
  // Task 1: Process incoming frames
  if (Serial2.available()) {
    processIncomingFrame();
  }
  
  // Task 2: Send periodic heartbeat
  sendHeartbeatIfDue();
  
  // Task 3: Process pending enrollments
  processPendingEnrollments();
  
  // Task 4: Update device online status
  updateDeviceOnlineStatus();
  
  // Task 5: Flush dirty data to NVS
  flushDatabaseIfDue();
  
  // Task 6: Sync with Display Master
  syncToDisplayMaster();
  
  // Task 7: Handle watchdog
  feedWatchdog();
}
```

---

## 🧮 BAGIAN 6: CRC16 IMPLEMENTATION

### **6.1 Algorithm Specification**

```
┌─────────────────────────────────────────────────────────────┐
│                 CRC16-MODBUS SPECIFICATION                   │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  Name:        CRC-16/MODBUS (also known as CRC-16/IBM)     │
│  Width:       16 bits                                        │
│  Polynomial:  0x8005 (x^16 + x^15 + x^2 + 1)               │
│  Init:        0xFFFF                                         │
│  RefIn:       True (LSB first)                              │
│  RefOut:      True (LSB first)                              │
│  XorOut:      0x0000                                         │
│  Check:       0x4B37 (for "123456789")                      │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

### **6.2 Reference Implementation (C/C++)**

```cpp
/**
 * CRC16-MODBUS calculation
 * 
 * @param data Pointer to data buffer
 * @param length Number of bytes to process
 * @return 16-bit CRC value (little-endian)
 */
uint16_t calculateCRC16(const uint8_t *data, uint16_t length) {
  uint16_t crc = 0xFFFF;  // Initial value
  
  for (uint16_t i = 0; i < length; i++) {
    crc ^= (uint16_t)data[i];  // XOR byte into CRC
    
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 0x0001) {
        crc >>= 1;
        crc ^= 0xA001;  // Polynomial (0x8005 reversed)
      } else {
        crc >>= 1;
      }
    }
  }
  
  return crc;  // No final XOR (XorOut = 0x0000)
}
```

### **6.3 Usage Examples**

**Example 1: Calculate CRC for frame**
```cpp
// Frame: 7E 05 03 09 42 00 01 00 00 00 00 01 00 ?? ?? 0A
//        │  └──────────────────────────────────┘
//        │          Data for CRC (12 bytes)
//        └─ START_DELIM (not included in CRC)

uint8_t frame[] = {
  0x7E,  // START (skip)
  0x05,  // ADDRESS (include)
  0x03,  // COMMAND (include)
  0x09,  // LENGTH (include)
  0x42, 0x00, 0x01, 0x00,  // PAYLOAD
  0x00, 0x00, 0x00, 0x01, 0x00
};

// Calculate CRC from byte 1 to byte 12
uint16_t crc = calculateCRC16(&frame[1], 12);

// Result: crc = 0x1A0B (example, actual will vary)
// Low byte: 0x0B
// High byte: 0x1A

// Complete frame:
// 7E 05 03 09 42 00 01 00 00 00 00 01 00 0B 1A 0A
//                                         └──┘
//                                      CRC_L CRC_H
```

**Example 2: Validate received frame**
```cpp
bool validateFrame(uint8_t* frame, uint16_t totalLength) {
  // Extract length field
  uint8_t payloadLength = frame[3];
  
  // Calculate expected CRC position
  uint16_t crcPos = 4 + payloadLength;
  
  // Extract received CRC (little-endian)
  uint16_t receivedCRC = frame[crcPos] | (frame[crcPos + 1] << 8);
  
  // Calculate CRC for frame data (ADDRESS to end of PAYLOAD)
  uint16_t calculatedCRC = calculateCRC16(&frame[1], 3 + payloadLength);
  
  if (receivedCRC != calculatedCRC) {
    Serial.printf("✗ CRC mismatch: RX=0x%04X, CALC=0x%04X\n",
                  receivedCRC, calculatedCRC);
    return false;
  }
  
  return true;
}
```

### **6.4 Test Vectors**

```cpp
// Standard test: "123456789"
uint8_t test1[] = {'1','2','3','4','5','6','7','8','9'};
uint16_t crc1 = calculateCRC16(test1, 9);
// Expected: 0x4B37

// Frame test: Simple HEARTBEAT
uint8_t test2[] = {0x00, 0x04, 0x01, 0xAA};  // ADDR, CMD, LEN, PAYLOAD
uint16_t crc2 = calculateCRC16(test2, 4);
// Expected: (calculate and verify)

// Empty payload test
uint8_t test3[] = {0x05, 0x07, 0x00};  // ACK with no payload
uint16_t crc3 = calculateCRC16(test3, 3);
// Expected: (calculate and verify)
```

---

## 📊 BAGIAN 7: TIMING & PERFORMANCE

### **7.1 Timing Requirements Summary**

| Event | Min Time | Typical | Max Time | Notes |
|-------|----------|---------|----------|-------|
| **Bit time** | 8.68 μs | 8.68 μs | 8.68 μs | Fixed (115200bps) |
| **Byte time** | 86.8 μs | 86.8 μs | 86.8 μs | 10 bits per byte |
| **Inter-frame gap** | 303 μs | 500 μs | 1 ms | 3.5 char minimum |
| **DE/RE setup** | 1 ms | 1 ms | 2 ms | Before transmit |
| **Turnaround** | 1 ms | 5 ms | 10 ms | Slave response delay |
| **Command timeout** | - | 1 s | 3 s | Wait for ACK/response |
| **Heartbeat interval** | 57 s | 60 s | 63 s | ±5% tolerance |
| **Discovery interval** | 1 s | 2 s | 3 s | Random per attempt |
| **Fail-safe timeout** | 300 s | 300 s | 300 s | FIXED, not configurable |

### **7.2 Throughput Calculations**

```
THEORETICAL MAXIMUM:
  Baud rate: 115200 bps
  Bytes/second: 115200 / 10 = 11,520 bytes/s
  Frames/second: 11,520 / 7 = 1,645 (min frame size)

PRACTICAL THROUGHPUT (with overhead):
  Inter-frame gap: 0.5 ms
  Turnaround: 5 ms
  Processing: 2 ms
  Total overhead per frame: 7.5 ms
  
  Effective frames/second: 1000 / 7.5 = 133 frames/s
  
FOR 18 SLAVES:
  Polling all slaves: 18 × 7.5 ms = 135 ms
  Max polling rate: 7.4 times/second
  
  With heartbeat (60s): 18 / 60 = 0.3 slaves/second
  Command budget: 133 - 0.3 = ~132 commands/second available
```

### **7.3 Bus Load Estimation**

```
TYPICAL OPERATION (18 slaves):
┌──────────────────────────────────────────────────────────┐
│ Activity            │ Frequency      │ Bytes  │ Load %   │
├──────────────────────────────────────────────────────────┤
│ Heartbeat           │ 1/60s          │ 8      │ 0.01%    │
│ SET_RELAY (avg)     │ 10/minute      │ 16     │ 0.23%    │
│ STATUS_REPORT (avg) │ 20/minute      │ 107    │ 3.1%     │
│ Discovery (startup) │ Rare           │ 67     │ Negligible│
│                                                           │
│ TOTAL AVERAGE LOAD:                          │ 3.34%    │
└──────────────────────────────────────────────────────────┘

PEAK LOAD (simultaneous control):
  18 × SET_RELAY: 18 × 16 bytes = 288 bytes
  Time: 288 / 11520 = 25 ms
  Peak burst: ~4% of 1 second

CONCLUSION: Bus highly underutilized, plenty of headroom
```

---

## ⚠️ BAGIAN 8: ERROR HANDLING & RECOVERY

### **8.1 Error Detection Mechanisms**

```
LAYER 1: Physical (Hardware)
  • RS-485 differential signaling (noise immunity)
  • Common ground (reduce ground loops)
  • Termination resistors (prevent reflections)

LAYER 2: Data Link (CRC)
  • CRC16-MODBUS checksum
  • Detection capability: ~99.998%
  • Corrupt frames discarded

LAYER 3: Application (Protocol)
  • Command validation
  • Sequence number (future)
  • Timeout detection
```

### **8.2 Error Handling Strategies**

**Strategy 1: Immediate Retry**
```cpp
void sendWithRetry(uint8_t addr, uint8_t cmd, 
                   uint8_t* payload, uint8_t len) {
  const int MAX_RETRIES = 3;
  
  for (int attempt = 0; attempt < MAX_RETRIES; attempt++) {
    // Send frame
    sendRS485Frame(addr, cmd, payload, len);
    
    // Wait for response
    if (waitForResponse(3000)) {
      if (responseIsACK()) {
        return;  // Success
      }
    }
    
    // Retry with exponential backoff
    uint32_t backoff = (1 << attempt) * 100;  // 100, 200, 400 ms
    delay(backoff);
    
    Serial.printf("Retry %d/%d for device %d\n", 
                  attempt+1, MAX_RETRIES, addr);
  }
  
  // All retries failed
  markDeviceOffline(addr);
  logError("Device %d not responding", addr);
}
```

**Strategy 2: Exponential Backoff (Discovery)**
```cpp
void discoveryWithBackoff() {
  int attempt = 0;
  uint32_t backoff = 1000;  // Start with 1 second
  
  while (attempt < 100) {
    sendDiscoveryAnnounce();
    
    if (waitForResponse(5000)) {
      // Got DISCOVERY_RESPONSE
      return;
    }
    
    // No response, increase backoff
    backoff = min(backoff * 2, 30000);  // Max 30 seconds
    backoff += random(0, 1000);  // Add jitter
    
    delay(backoff);
    attempt++;
  }
  
  // Give up, restart
  ESP.restart();
}
```

**Strategy 3: Circuit Breaker**
```cpp
class CircuitBreaker {
private:
  int failureCount = 0;
  uint32_t lastFailure = 0;
  bool isOpen = false;
  
public:
  bool canAttempt() {
    if (!isOpen) return true;
    
    // Check if enough time passed to retry
    if (millis() - lastFailure > 60000) {  // 1 minute
      Serial.println("Circuit breaker: Attempting reset");
      isOpen = false;
      failureCount = 0;
      return true;
    }
    
    return false;
  }
  
  void recordFailure() {
    failureCount++;
    lastFailure = millis();
    
    if (failureCount >= 5) {
      Serial.println("Circuit breaker: OPEN (too many failures)");
      isOpen = true;
    }
  }
  
  void recordSuccess() {
    failureCount = 0;
    isOpen = false;
  }
};
```

### **8.3 Timeout Values**

| Scenario | Timeout | Action on Timeout |
|----------|---------|-------------------|
| **Command ACK** | 3 seconds | Retry (max 3 times) |
| **Discovery response** | 5 seconds | Increase backoff, retry |
| **Status report** | Not applicable | Unsolicited, no timeout |
| **Heartbeat (slave)** | 300 seconds | Enter FAIL-SAFE mode |
| **Device online** | 310 seconds | Mark offline in database |

---

## 📝 BAGIAN 9: IMPLEMENTATION CHECKLIST

### **9.1 Firmware Implementation Checklist**

```
MANDATORY FEATURES:
[ ] CRC16-MODBUS calculation & validation
[ ] Frame parser (START to END detection)
[ ] Address filtering (own ID + broadcast)
[ ] All 8 command opcodes supported
[ ] NVS config management with compare-before-write
[ ] Heartbeat monitoring (300s timeout)
[ ] Fail-safe execution per profile
[ ] LED status indicators
[ ] Serial debug logging

SLAVE-SPECIFIC:
[ ] Discovery announce (if ID=0)
[ ] Profile-specific behavior (FAN/LIGHT/SOUND)
[ ] Mode detection (for FAN_4CH: AUTO/MANUAL)
[ ] Relay interlock (FAN_4CH speed safety)
[ ] Dead-time implementation (200ms for FAN)
[ ] Manual override rejection in MANUAL mode
[ ] STATUS_REPORT generation
[ ] Config validation on boot

MASTER-SPECIFIC:
[ ] Device registry database (RAM + NVS)
[ ] Periodic heartbeat sender (60s interval)
[ ] Discovery response handling
[ ] Command routing to slaves
[ ] Status aggregation from slaves
[ ] Database sync to Display Master
[ ] Offline device detection (>310s)
[ ] NVS periodic flush (5 min interval)

OPTIONAL BUT RECOMMENDED:
[ ] Watchdog timer (hardware + software)
[ ] Retry logic with exponential backoff
[ ] Circuit breaker for problematic devices
[ ] Event logging to SD card
[ ] Performance metrics (latency, error rate)
[ ] Remote firmware update (OTA)
```

### **9.2 Testing Checklist**

```
UNIT TESTS:
[ ] CRC16 calculation (known test vectors)
[ ] Frame encoding/decoding (round-trip)
[ ] Command payload parsing (JSON + binary)
[ ] NVS compare-before-write logic
[ ] Interlock validation (FAN multiple speeds)

INTEGRATION TESTS:
[ ] Master-Slave communication (unicast)
[ ] Broadcast commands (heartbeat)
[ ] Discovery enrollment (new device)
[ ] Manual override detection (DPDT switch)
[ ] Fail-safe trigger (300s timeout)
[ ] Fail-safe recovery (heartbeat restored)
[ ] Mode switching (AUTO ↔ MANUAL)
[ ] Parallel control (manual + auto)

SYSTEM TESTS:
[ ] 18 slaves simultaneously
[ ] Continuous operation 24 hours
[ ] Power cycle all devices
[ ] RS-485 noise immunity (EMI test)
[ ] Long cable test (100m+)
[ ] Multiple concurrent commands
[ ] Database persistence after restart
[ ] Web dashboard real-time sync

STRESS TESTS:
[ ] Rapid command sequence (100 commands/min)
[ ] All slaves control simultaneously
[ ] Discovery storm (10 devices at once)
[ ] Network saturation (flood test)
[ ] Memory leak test (48 hour run)
```

### **9.3 Validation Checklist**

```
PROTOCOL COMPLIANCE:
[ ] All frames start with 0x7E
[ ] All frames end with 0x0A
[ ] CRC calculated correctly
[ ] Address space 0x00-0xFF used properly
[ ] Command opcodes 0x01-0x08 implemented
[ ] Payload length matches LENGTH field
[ ] Inter-frame gap ≥ 3.5 char times

TIMING COMPLIANCE:
[ ] Heartbeat interval 60 ± 3 seconds
[ ] Fail-safe timeout exactly 300 seconds
[ ] Command response < 3 seconds
[ ] DE/RE switching ≥ 1ms before/after TX
[ ] Discovery interval 1-3 seconds random

SAFETY COMPLIANCE:
[ ] Manual control always works (independent)
[ ] Fail-safe actions correct per profile
[ ] Interlock prevents multiple FAN speeds
[ ] Dead-time prevents motor stress
[ ] DPDT physically isolates AUTO/MANUAL paths
```

---

## 🔍 BAGIAN 10: TROUBLESHOOTING GUIDE

### **10.1 Common Protocol Issues**

**ISSUE 1: CRC Errors Frequent**

```
SYMPTOMS:
  • Logs show "CRC mismatch" repeatedly
  • Communication intermittent
  • Some commands work, some don't

DIAGNOSTIC:
  1. Check RS-485 wiring (A-A, B-B correct?)
  2. Verify termination (120Ω at both ends ONLY)
  3. Measure bus voltage (should be differential 2-5V)
  4. Check for ground loops (voltage between node grounds)
  5. Test cable quality (continuity, no breaks)

SOLUTIONS:
  • Fix wiring polarity (swap A/B if reversed)
  • Add/remove termination resistors correctly
  • Improve grounding (single point ground)
  • Use shielded cable if EMI high
  • Reduce cable length or lower baud rate
```

**ISSUE 2: Discovery Not Working**

```
SYMPTOMS:
  • New device LED blinking blue (discovery mode)
  • Master doesn't receive DISCOVERY_ANNOUNCE
  • Device never gets ID assigned

DIAGNOSTIC:
  1. Check slave ID in NVS (should be 0)
  2. Verify RS-485 connection (slave can transmit?)
  3. Monitor serial output (is slave broadcasting?)
  4. Check Master is listening on 0x00 (broadcast)
  5. Verify MAC address not duplicate

SOLUTIONS:
  • Clear NVS on slave: preferences.clear()
  • Test slave TX with oscilloscope/logic analyzer
  • Ensure Master processes broadcast frames
  • Check MAC uniqueness in database
  • Increase discovery timeout (>5 seconds)
```

**ISSUE 3: Heartbeat Timeout False Positives**

```
SYMPTOMS:
  • Slaves enter fail-safe randomly
  • LED turns red unexpectedly
  • Master is running fine

DIAGNOSTIC:
  1. Check Master heartbeat is actually sending (log)
  2. Verify broadcast address (0x00) used
  3. Measure actual interval (should be 60s ± 3s)
  4. Check for bus overload (too many commands)
  5. Verify slave timer accuracy (millis() rollover?)

SOLUTIONS:
  • Fix Master heartbeat timer logic
  • Ensure broadcast frames processed by all slaves
  • Adjust heartbeat interval if needed (57-63s OK)
  • Implement overflow-safe timer comparison:
    if ((millis() - lastHB) > 300000) { ... }
```

**ISSUE 4: Commands Rejected in AUTO Mode**

```
SYMPTOMS:
  • Kipas receiving NACK error 0x04 (Manual mode)
  • But mode switch is in AUTO position
  • Dashboard shows device in AUTO

DIAGNOSTIC:
  1. Check GPIO35 state (HIGH = AUTO, LOW = MANUAL)
  2. Verify DPDT switch wiring (micro switch connection)
  3. Test switch with multimeter (continuity)
  4. Check for loose connections
  5. Verify pull-up resistor on GPIO35

SOLUTIONS:
  • Fix switch wiring (check continuity)
  • Add external pull-up if needed (10kΩ to 3.3V)
  • Clean switch contacts (corrosion?)
  • Replace faulty switch
  • Add debounce logic in firmware (50ms)
```

### **10.2 Diagnostic Tools**

**Tool 1: Protocol Analyzer (Software)**

```cpp
void analyzeFrame(uint8_t* frame, uint16_t len) {
  Serial.println("\n═══ FRAME ANALYSIS ═══");
  
  // Hex dump
  Serial.print("HEX: ");
  for (int i = 0; i < len; i++) {
    Serial.printf("%02X ", frame[i]);
  }
  Serial.println();
  
  // Parse fields
  if (len >= 7) {
    Serial.printf("START:   0x%02X %s\n", 
                  frame[0], frame[0] == 0x7E ? "✓" : "✗");
    Serial.printf("ADDR:    0x%02X (%d)\n", frame[1], frame[1]);
    Serial.printf("CMD:     0x%02X (%s)\n", 
                  frame[2], getCommandName(frame[2]));
    Serial.printf("LENGTH:  0x%02X (%d bytes)\n", frame[3], frame[3]);
    
    // Payload
    if (frame[3] > 0) {
      Serial.print("PAYLOAD: ");
      for (int i = 0; i < frame[3]; i++) {
        Serial.printf("%02X ", frame[4 + i]);
      }
      Serial.println();
    }
    
    // CRC
    uint16_t rxCRC = frame[4 + frame[3]] | (frame[5 + frame[3]] << 8);
    uint16_t calcCRC = calculateCRC16(&frame[1], 3 + frame[3]);
    Serial.printf("CRC:     0x%04X (calc: 0x%04X) %s\n", 
                  rxCRC, calcCRC, rxCRC == calcCRC ? "✓" : "✗");
    
    Serial.printf("END:     0x%02X %s\n", 
                  frame[6 + frame[3]], 
                  frame[6 + frame[3]] == 0x0A ? "✓" : "✗");
  }
  
  Serial.println("═════════════════════\n");
}
```

**Tool 2: Bus Monitor (Passive)**

```cpp
class BusMonitor {
private:
  struct BusStats {
    uint32_t totalFrames;
    uint32_t validFrames;
    uint32_t crcErrors;
    uint32_t malformedFrames;
    uint32_t lastActivity;
  } stats;
  
public:
  void monitorFrame(uint8_t* frame, uint16_t len, bool valid) {
    stats.totalFrames++;
    stats.lastActivity = millis();
    
    if (valid) {
      stats.validFrames++;
    } else {
      if (validateCRC(frame, len)) {
        stats.malformedFrames++;
      } else {
        stats.crcErrors++;
      }
    }
  }
  
  void printStats() {
    Serial.println("\n╔════════════════════════════════╗");
    Serial.println("║     BUS STATISTICS             ║");
    Serial.println("╠════════════════════════════════╣");
    Serial.printf("║ Total Frames:    %6d       ║\n", stats.totalFrames);
    Serial.printf("║ Valid Frames:    %6d       ║\n", stats.validFrames);
    Serial.printf("║ CRC Errors:      %6d       ║\n", stats.crcErrors);
    Serial.printf("║ Malformed:       %6d       ║\n", stats.malformedFrames);
    
    float errorRate = (float)(stats.crcErrors + stats.malformedFrames) 
                      / stats.totalFrames * 100;
    Serial.printf("║ Error Rate:      %5.2f%%     ║\n", errorRate);
    
    uint32_t idleTime = millis() - stats.lastActivity;
    Serial.printf("║ Last Activity:   %6ds ago  ║\n", idleTime / 1000);
    Serial.println("╚════════════════════════════════╝\n");
  }
};
```

**Tool 3: Traffic Generator (Testing)**

```cpp
void generateTestTraffic() {
  // Test 1: Flood test (max throughput)
  Serial.println("Test 1: Flood test (100 commands)");
  for (int i = 0; i < 100; i++) {
    sendSetRelay(1, 0x01, 0x01);  // Toggle relay 1
    delay(10);  // 100 commands/second
  }
  
  // Test 2: Simultaneous control
  Serial.println("Test 2: Simultaneous control (18 slaves)");
  for (int id = 1; id <= 18; id++) {
    sendSetRelay(id, 0xFF, 0x01);  // All relays ON
  }
  
  // Test 3: Discovery storm
  Serial.println("Test 3: Discovery storm (10 fake devices)");
  for (int i = 0; i < 10; i++) {
    sendFakeDiscovery(generateRandomMAC());
    delay(random(100, 500));
  }
  
  // Test 4: Long payload
  Serial.println("Test 4: Long payload (255 bytes)");
  uint8_t longPayload[255];
  memset(longPayload, 0xAA, 255);
  sendCustomFrame(1, 0x03, longPayload, 255);
}
```

---

## 📚 BAGIAN 11: EXAMPLE IMPLEMENTATIONS

### **11.1 Complete Frame Encoder**

```cpp
class RS485FrameEncoder {
private:
  uint8_t buffer[262];  // Max frame size
  
public:
  bool encode(uint8_t address, uint8_t command,
              const uint8_t* payload, uint8_t payloadLen,
              uint8_t* output, uint16_t* outputLen) {
    
    // Validate inputs
    if (payloadLen > 255) {
      Serial.println("✗ Payload too long");
      return false;
    }
    
    // Build frame
    int idx = 0;
    buffer[idx++] = 0x7E;              // START_DELIM
    buffer[idx++] = address;           // ADDRESS
    buffer[idx++] = command;           // COMMAND
    buffer[idx++] = payloadLen;        // LENGTH
    
    // Copy payload
    if (payloadLen > 0) {
      memcpy(&buffer[idx], payload, payloadLen);
      idx += payloadLen;
    }
    
    // Calculate CRC (from ADDRESS to end of PAYLOAD)
    uint16_t crc = calculateCRC16(&buffer[1], 3 + payloadLen);
    buffer[idx++] = crc & 0xFF;        // CRC_LOW
    buffer[idx++] = (crc >> 8) & 0xFF; // CRC_HIGH
    
    buffer[idx++] = 0x0A;              // END_DELIM
    
    // Copy to output
    memcpy(output, buffer, idx);
    *outputLen = idx;
    
    return true;
  }
  
  void encodeJSON(uint8_t address, uint8_t command,
                  const JsonDocument& doc,
                  uint8_t* output, uint16_t* outputLen) {
    String jsonStr;
    serializeJson(doc, jsonStr);
    
    encode(address, command, 
           (uint8_t*)jsonStr.c_str(), jsonStr.length(),
           output, outputLen);
  }
};
```

### **11.2 Complete Frame Decoder**

```cpp
class RS485FrameDecoder {
private:
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
  
  ParserState state = WAIT_START;
  uint8_t buffer[262];
  uint16_t index = 0;
  uint8_t expectedLength = 0;
  
public:
  bool processByte(uint8_t byte, RS485Frame* frame) {
    switch (state) {
      case WAIT_START:
        if (byte == 0x7E) {
          buffer[0] = byte;
          index = 1;
          state = READ_ADDRESS;
        }
        break;
        
      case READ_ADDRESS:
        buffer[index++] = byte;
        state = READ_COMMAND;
        break;
        
      case READ_COMMAND:
        buffer[index++] = byte;
        state = READ_LENGTH;
        break;
        
      case READ_LENGTH:
        buffer[index++] = byte;
        expectedLength = byte;
        
        if (expectedLength == 0) {
          state = READ_CRC_LOW;  // No payload
        } else {
          state = READ_PAYLOAD;
        }
        break;
        
      case READ_PAYLOAD:
        buffer[index++] = byte;
        if (index >= (4 + expectedLength)) {
          state = READ_CRC_LOW;
        }
        break;
        
      case READ_CRC_LOW:
        buffer[index++] = byte;
        state = READ_CRC_HIGH;
        break;
        
      case READ_CRC_HIGH:
        buffer[index++] = byte;
        state = READ_END;
        break;
        
      case READ_END:
        buffer[index++] = byte;
        
        if (byte == 0x0A) {
          // Frame complete, validate
          if (validateAndExtract(frame)) {
            state = WAIT_START;
            return true;  // Valid frame received
          }
        }
        
        // Invalid frame, reset
        Serial.println("✗ Invalid frame, resetting parser");
        state = WAIT_START;
        break;
    }
    
    return false;  // Frame not complete yet
  }
  
private:
  bool validateAndExtract(RS485Frame* frame) {
    // Extract fields
    frame->address = buffer[1];
    frame->command = buffer[2];
    frame->length = buffer[3];
    
    // Copy payload
    if (frame->length > 0) {
      memcpy(frame->payload, &buffer[4], frame->length);
    }
    
    // Extract CRC
    uint16_t receivedCRC = buffer[4 + frame->length] | 
                          (buffer[5 + frame->length] << 8);
    
    // Calculate CRC
    uint16_t calculatedCRC = calculateCRC16(&buffer[1], 3 + frame->length);
    
    // Validate
    if (receivedCRC != calculatedCRC) {
      Serial.printf("✗ CRC error: RX=0x%04X, CALC=0x%04X\n",
                    receivedCRC, calculatedCRC);
      return false;
    }
    
    return true;
  }
};

// Usage
RS485FrameDecoder decoder;
RS485Frame frame;

void loop() {
  while (Serial2.available()) {
    uint8_t byte = Serial2.read();
    
    if (decoder.processByte(byte, &frame)) {
      // Complete frame received
      processFrame(&frame);
    }
  }
}
```

### **11.3 Example: Discovery Flow (Complete)**

```cpp
// SLAVE: Send discovery announce
void sendDiscoveryAnnounce() {
  StaticJsonDocument<128> doc;
  
  // Get MAC address
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  
  char macStr[18];
  sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
          mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  
  doc["mac"] = macStr;
  doc["profile"] = getProfileName(myProfile);
  doc["ver"] = FIRMWARE_VERSION;
  
  String payload;
  serializeJson(doc, payload);
  
  Serial.println("→ Sending DISCOVERY_ANNOUNCE");
  Serial.println("  " + payload);
  
  uint8_t frame[262];
  uint16_t frameLen;
  
  encoder.encodeJSON(0x00, 0x01, doc, frame, &frameLen);
  sendFrame(frame, frameLen);
}

// MASTER: Handle discovery announce
void handleDiscoveryAnnounce(RS485Frame* frame) {
  StaticJsonDocument<128> doc;
  deserializeJson(doc, frame->payload, frame->length);
  
  const char* mac = doc["mac"];
  const char* profile = doc["profile"];
  
  Serial.printf("✓ Discovery from MAC %s, profile %s\n", mac, profile);
  
  // Check if already registered
  DeviceRecord* existing = findDeviceByMAC(mac);
  if (existing) {
    Serial.printf("  Device already registered as ID %d\n", existing->id);
    
    // Re-send existing ID
    sendDiscoveryResponse(existing->id, existing->name, profile);
    return;
  }
  
  // New device, add to pending
  PendingDevice pending;
  strncpy(pending.mac, mac, 17);
  strncpy(pending.profile, profile, 15);
  pending.timestamp = millis();
  
  pendingDevices.push_back(pending);
  
  // Notify Display Master
  notifyNewDevicePending(mac, profile);
}

// MASTER: Send discovery response (after user assigns name)
void sendDiscoveryResponse(uint8_t newID, const char* name, 
                          const char* mac, const char* profile) {
  StaticJsonDocument<256> doc;
  
  doc["mac"] = mac;
  doc["assigned_id"] = newID;
  doc["device_name"] = name;
  doc["profile"] = profile;
  doc["timestamp"] = now();  // Unix timestamp
  
  String payload;
  serializeJson(doc, payload);
  
  Serial.printf("→ Sending DISCOVERY_RESPONSE: ID %d = %s\n", newID, name);
  
  uint8_t frame[262];
  uint16_t frameLen;
  
  encoder.encodeJSON(0x00, 0x02, doc, frame, &frameLen);
  sendFrame(frame, &frameLen);
  
  // Add to database (optimistically)
  DeviceRecord record;
  record.id = newID;
  strncpy(record.name, name, 31);
  // ... fill other fields
  
  deviceRegistry[newID] = record;
  markDirty(newID);
}

// SLAVE: Handle discovery response
void handleDiscoveryResponse(RS485Frame* frame) {
  StaticJsonDocument<256> doc;
  deserializeJson(doc, frame->payload, frame->length);
  
  const char* mac = doc["mac"];
  
  // Validate MAC matches ours
  char myMAC[18];
  getMACString(myMAC);
  
  if (strcmp(mac, myMAC) != 0) {
    Serial.println("  MAC mismatch, ignoring");
    return;
  }
  
  // Extract assignment
  uint8_t assignedID = doc["assigned_id"];
  const char* deviceName = doc["device_name"];
  
  Serial.printf("✓ Assigned ID %d, name: %s\n", assignedID, deviceName);
  
  // Save to NVS with compare-before-write
  preferences.begin("config", false);
  
  uint8_t oldID = preferences.getUChar("device_id", 0);
  if (oldID != assignedID) {
    preferences.putUChar("device_id", assignedID);
    Serial.println("  → ID saved to NVS");
  } else {
    Serial.println("  → ID unchanged, skip NVS write");
  }
  
  String oldName = preferences.getString("device_name", "");
  if (oldName != String(deviceName)) {
    preferences.putString("device_name", deviceName);
    Serial.println("  → Name saved to NVS");
  } else {
    Serial.println("  → Name unchanged, skip NVS write");
  }
  
  preferences.end();
  
  // Send ACK
  sendACK();
  
  Serial.println("  → Restarting in 2 seconds...");
  delay(2000);
  ESP.restart();
}
```

---

## 📊 BAGIAN 12: PERFORMANCE METRICS

### **12.1 Key Performance Indicators**

```
TARGET METRICS:
┌──────────────────────────────────────────────────────────┐
│ Metric                  │ Target      │ Acceptable      │
├──────────────────────────────────────────────────────────┤
│ Command latency         │ < 500 ms    │ < 2 seconds     │
│ Heartbeat jitter        │ ± 1 s       │ ± 5 seconds     │
│ Discovery time          │ < 10 s      │ < 60 seconds    │
│ Frame error rate        │ < 0.1%      │ < 1%            │
│ Bus utilization         │ < 20%       │ < 50%           │
│ Polling cycle (18 dev)  │ < 2 s       │ < 5 seconds     │
│ Recovery time (failsafe)│ < 5 s       │ < 30 seconds    │
│ Memory usage (slave)    │ < 100 KB    │ < 200 KB        │
│ NVS writes/hour         │ < 10        │ < 60            │
└──────────────────────────────────────────────────────────┘
```

### **12.2 Benchmarking Code**

```cpp
class PerformanceMonitor {
private:
  struct Metrics {
    uint32_t commandCount;
    uint32_t totalLatency;
    uint32_t maxLatency;
    uint32_t minLatency;
    uint32_t errorCount;
  } metrics;
  
public:
  void recordCommand(uint32_t latencyMs, bool success) {
    metrics.commandCount++;
    
    if (success) {
      metrics.totalLatency += latencyMs;
      metrics.maxLatency = max(metrics.maxLatency, latencyMs);
      
      if (metrics.minLatency == 0 || latencyMs < metrics.minLatency) {
        metrics.minLatency = latencyMs;
      }
    } else {
      metrics.errorCount++;
    }
  }
  
  void printReport() {
    Serial.println("\n╔═══════════════════════════════════╗");
    Serial.println("║   PERFORMANCE REPORT              ║");
    Serial.println("╠═══════════════════════════════════╣");
    Serial.printf("║ Commands:      %8d         ║\n", metrics.commandCount);
    Serial.printf("║ Errors:        %8d         ║\n", metrics.errorCount);
    
    float errorRate = (float)metrics.errorCount / metrics.commandCount * 100;
    Serial.printf("║ Error Rate:    %7.2f%%        ║\n", errorRate);
    
    if (metrics.commandCount > metrics.errorCount) {
      uint32_t successCount = metrics.commandCount - metrics.errorCount;
      uint32_t avgLatency = metrics.totalLatency / successCount;
      
      Serial.printf("║ Avg Latency:   %6d ms        ║\n", avgLatency);
      Serial.printf("║ Min Latency:   %6d ms        ║\n", metrics.minLatency);
      Serial.printf("║ Max Latency:   %6d ms        ║\n", metrics.maxLatency);
    }
    
    Serial.println("╚═══════════════════════════════════╝\n");
  }
};
```

---

## 🔚 BAGIAN 13: APPENDIX & REFERENCES

### **13.1 ASCII Table (Relevant Characters)**

| Hex | Dec | Char | Usage in Protocol |
|-----|-----|------|-------------------|
| 0x0A | 10 | \n | END_DELIM |
| 0x7E | 126 | ~ | START_DELIM |
| 0xAA | 170 | ª | HEARTBEAT payload (typical) |

### **13.2 Related Standards**

- **TIA/EIA-485-A:** Electrical Characteristics of Generators and Receivers for Use in Balanced Digital Multipoint Systems
- **Modbus RTU:** Serial communication protocol (CRC16-MODBUS origin)
- **ISO/IEC 14908:** Control network protocol (reference for multi-master)

### **13.3 Useful Tools**

**Hardware:**
- Logic Analyzer (Saleae, DSLogic) - Protocol decode
- Oscilloscope - Signal integrity check
- RS-485 USB adapter - PC interface for testing
- Multimeter - Bus voltage measurement

**Software:**
- Termite / RealTerm - Serial terminal with hex view
- Wireshark (with custom dissector) - Protocol analysis
- Python pyserial - Scripting test scenarios
- PlatformIO - Firmware development

### **13.4 Version History**

```
v1.3 (27 Desember 2025) - CURRENT
  • Added manual override protocol details
  • Clarified DPDT mode switch behavior
  • Updated STATUS_REPORT payload for mode indication
  • Added performance metrics section
  • Enhanced error handling strategies

v1.2 (26 Desember 2025)
  • Added FORCE_RESET command (optional)
  • Enhanced CRC implementation details
  • Added example code for discovery flow
  • Clarified timing requirements

v1.1 (25 Desember 2025)
  • Initial protocol definition
  • Basic command set (0x01-0x08)
  • Frame format specification
```

---

```
╔═══════════════════════════════════════════════════════════════╗
║                                                               ║
║                   END OF DOCUMENT                             ║
║        SPESIFIKASI TEKNIS PROTOKOL v1.3 - COMPLETE           ║
║                                                               ║
║  Dokumen ini adalah SPESIFIKASI PROTOKOL yang STABIL.        ║
║  Implementasi firmware HARUS mengikuti byte-level detail.    ║
║                                                               ║
║  Next: PANDUAN INTEGRASI HARDWARE v1.1                       ║
║                                                               ║
╚═══════════════════════════════════════════════════════════════╝
```

---

**DOKUMEN 2 SELESAI**