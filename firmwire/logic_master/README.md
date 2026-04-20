# 🧠 Logic Master Firmware

**Role:** System Coordinator & Brain of Smart Mosque Ecosystem

**Hardware:** ESP32-S3 DevKit + ILI9488 4" Display + MAX3485

**Device ID:** 254 (0xFE) - Fixed, tidak bisa diubah

---

## 📋 Responsibilities

### Core Functions:
1. **Heartbeat Broadcasting**
   - Send heartbeat setiap 60 detik ke semua slaves
   - Prevent fail-safe trigger
   - Timing critical: ±5% tolerance

2. **Device Registry Management**
   - Track 18 slave devices (ID 1-18)
   - Maintain online/offline status
   - Store configuration in NVS
   - RAM cache untuk fast access

3. **Discovery Enrollment**
   - Listen untuk DISCOVERY_ANNOUNCE dari new devices
   - Assign device ID (1-247)
   - Coordinate dengan Display Master untuk user input
   - Send DISCOVERY_RESPONSE

4. **Command Router**
   - Receive commands dari Display Master
   - Route ke target slave(s)
   - Track ACK/NACK responses
   - Implement retry logic

5. **Status Aggregator**
   - Collect STATUS_REPORT dari slaves
   - Update device registry
   - Calculate derived values (online status)
   - Sync ke Display Master

6. **Database Persistence**
   - Periodic NVS flush (5 min interval)
   - Compare-before-write optimization
   - Power-loss safe operations

---

## 🔌 Hardware Connections

### RS-485 (MAX3485):
```
ESP32-S3       MAX3485        RS-485 Bus
─────────────────────────────────────────
GPIO16 (RX) ──► RO
GPIO17 (TX) ──► DI
GPIO4       ──► DE/RE
GND         ──► GND ────────► GND
                A   ────────► A+ (Blue-White)
                B   ────────► B- (Green-White)
```

### Display (ILI9488 4" SPI):
```
ESP32-S3       ILI9488
─────────────────────
GPIO13      ──► MOSI
GPIO12      ──► MISO
GPIO14      ──► SCK
GPIO15      ──► CS
GPIO2       ──► DC
GPIO4       ──► RST (optional)
GND         ──► GND
3.3V        ──► VCC
```

### SD Card (Optional - for logging):
```
ESP32-S3       SD Card
─────────────────────
GPIO13      ──► MOSI
GPIO12      ──► MISO
GPIO14      ──► SCK
GPIO21      ──► CS
```

---

## 📊 Memory Budget

### Device Registry:
```
DeviceRecord: 47 bytes × 247 devices = 11,609 bytes (~11.6KB)
RAM Cache:    Full registry in RAM for fast access
NVS Storage:  Periodic flush (compare-before-write)
```

### Heap Usage Estimate:
```
Device Registry:     ~12 KB
Frame Buffers:       ~2 KB
JSON Buffers:        ~4 KB
Display Buffer:      ~10 KB (partial)
Stack + Overhead:    ~20 KB
─────────────────────────────
Total Heap Usage:    ~48 KB

Available (ESP32-S3): 320 KB SRAM + 8 MB PSRAM
Margin: Excellent (>250 KB free)
```

---

## 🔄 Main Loop Tasks

```cpp
void loop() {
    // Priority 1: Process incoming frames (slaves + display master)
    processRS485Communication();
    
    // Priority 2: Send heartbeat (60s interval, critical timing)
    sendHeartbeatIfDue();
    
    // Priority 3: Process pending enrollments
    processPendingDiscovery();
    
    // Priority 4: Update device online status
    updateDeviceOnlineStatus();
    
    // Priority 5: Flush database to NVS (5 min interval)
    flushDatabaseIfDue();
    
    // Priority 6: Update local display
    updateStatusDisplay();
    
    // Priority 7: Sync with Display Master
    syncToDisplayMaster();
    
    // Priority 8: Handle watchdog
    feedWatchdog();
}
```

---

## 📁 File Structure

```
logic_master/
├── src/
│   ├── main.cpp                 # Setup & main loop
│   ├── device_registry.cpp      # Database management
│   ├── heartbeat.cpp            # Heartbeat broadcaster
│   ├── discovery.cpp            # Enrollment handler
│   ├── command_router.cpp       # Command routing
│   ├── status_aggregator.cpp    # Status collection
│   ├── frame_processing.cpp     # RS-485 parser
│   └── display_driver.cpp       # Local display (4" LCD)
├── include/
│   └── logic_master.h           # Common definitions
├── platformio.ini
└── README.md (this file)
```

---

## 🧪 Testing Strategy

### Phase 1: Unit Testing
- [ ] Device registry CRUD operations
- [ ] Heartbeat timer accuracy
- [ ] Discovery protocol flow
- [ ] NVS persistence

### Phase 2: Integration Testing
- [ ] Communication with 1 slave
- [ ] Communication with 3 slaves
- [ ] Communication with 18 slaves
- [ ] Display Master sync

### Phase 3: Stress Testing
- [ ] 24 hour continuous operation
- [ ] Rapid command sequence
- [ ] Multiple simultaneous enrollments
- [ ] Network saturation

---

## 🚀 Build & Upload

```bash
cd firmware/logic_master

# Build
pio run

# Upload
pio run --target upload

# Monitor
pio device monitor
```

---

## 📝 Configuration

### NVS Namespaces:
- `smart_mosque` - Device registry database
- `system` - System configuration
- `stats` - Runtime statistics

### Fixed Parameters:
- Device ID: 254 (0xFE)
- Heartbeat Interval: 60000 ms (±5%)
- NVS Flush Interval: 300000 ms (5 min)
- Discovery Timeout: 300000 ms (5 min)

---

## 🔧 Development Status

- [ ] Setup & initialization
- [ ] Device registry implementation
- [ ] Heartbeat broadcaster
- [ ] Discovery handler
- [ ] Command router
- [ ] Status aggregator
- [ ] Display driver
- [ ] Testing & validation

**Status:** 🔴 IN DEVELOPMENT

---

*Referensi: [KT-2.2] Logic Master, [STP-5.2] Master State Machine*
