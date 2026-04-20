# Hardware Adaptation — Smart Mosque Ecosystem v1.2

Dokumen ini merangkum perubahan integrasi ESP32 X8 Relay Modbus
sebagai pengganti modul relay 8ch hardware murni (tanpa ESP32).

---

## Perubahan Utama v1.2

### Arsitektur Bus RS485 — DIPISAH

```
[Logic Master ESP32-S3 IoT Logger]
│
├── Serial2 (GPIO15/16) @ 115200 bps ──── BUS A ──── [HwThinker Slave Kipas]
│   Smart Mosque Protocol (custom)                     Modbus addr: dinamis
│                                                      TX=GPIO19, RX=GPIO18
│
└── Serial1 (GPIO1/2)  @ 9600 bps  ──── BUS B ──── [ESP32 X8 Relay Lampu]
    Modbus RTU                      │               Modbus addr: 0x02
    DE/RE: GPIO5                    │               TX=GPIO19, RX=GPIO18
                                    │               ShiftReg 74HC595
                                    │
                                    ├────────────── [ESP32 X8 Relay Sound]
                                    │               Modbus addr: 0x03
                                    │               TX=GPIO19, RX=GPIO18
                                    │               ShiftReg 74HC595
                                    │
                                    └────────────── [SHT20 XY-MD02]
                                                    Modbus addr: 0x01
                                                    Sensor suhu & kelembaban
```

### Alasan Pemisahan Bus
- Smart Mosque Protocol berjalan @ 115200 bps
- ESP32 X8 Relay dan SHT20 berjalan @ 9600 bps
- Tidak bisa mix baud rate dalam satu bus RS485
- Pemisahan juga menghilangkan risiko tabrakan frame antar protokol

---

## Peta Modul Hardware v1.2

| Peran | Modul | Board | Bus |
|---|---|---|---|
| Logic Master | ESP32-S3 N16R8 IoT Logger RS485 Plus | ESP32-S3 | — |
| Slave Kipas | HwThinker 4-relay ESP32 | ESP32 WROOM-32 | Bus A |
| **Slave Lampu** | **ESP32 X8 Relay Modbus (303e32dc812)** | **ESP32 WROOM-32E** | **Bus B** |
| **Slave Sound** | **ESP32 X8 Relay Modbus (303e32dc812)** | **ESP32 WROOM-32E** | **Bus B** |
| Sensor Suhu | SHT20 XY-MD02 RS485 | — | Bus B |

> ⚠️ v1.1 menggunakan modul relay 8ch hardware murni (tanpa ESP32).
> v1.2 menggantinya dengan ESP32 X8 Relay Modbus yang memiliki firmware sendiri.

---

## Perubahan Per File

### `firmware/logic_master/include/logic_master.h`
| Yang Berubah | v1.1 | v1.2 |
|---|---|---|
| Firmware version | 1.1.0 | **1.2.0** |
| Pin RS485 Modbus | (tidak ada, shared Serial2) | **PIN_MODBUS_TX=GPIO1, PIN_MODBUS_RX=GPIO2, PIN_MODBUS_DE_RE=GPIO5** |
| Baud rate Modbus | (tidak ada) | **MODBUS_BAUD_RATE=9600** |
| Baud rate Bus A | (tidak ada) | **SMART_MOSQUE_BAUD_RATE=115200** |
| Fungsi baru | — | `modbusX8WriteRelays()`, `modbusX8ReadStatus()` |
| Fungsi baru | — | `setX8Relay()`, `setX8RelayOne()`, `readX8RelayStatus()` |

### `firmware/logic_master/src/modbus_handler.cpp`
| Yang Berubah | v1.1 | v1.2 |
|---|---|---|
| Serial yang digunakan | Serial2 (shared) | **Serial1 (Bus B dedicated)** |
| Init Serial | (tidak ada) | **Serial1.begin(9600, ..., GPIO2, GPIO1)** |
| DE/RE control | tidak ada | **digitalWrite GPIO5 HIGH/LOW** |
| Fungsi relay | FC05 single coil | **FC15 Write Multiple Coils (X8 protocol)** |
| Fungsi baru | — | `modbusX8WriteRelays(addr, mask)` |
| Fungsi baru | — | `modbusX8ReadStatus(addr, memaddr, outVal)` |

### `firmware/logic_master/src/command_router.cpp`
| Yang Berubah | v1.1 | v1.2 |
|---|---|---|
| `setModbusRelay8ch()` | FC05 per coil | **wrapper → setX8RelayOne()** |
| `setAllModbusRelays8ch()` | FC0F standar | **wrapper → setX8Relay()** |
| Fungsi baru | — | `setX8Relay(addr, mask)` |
| Fungsi baru | — | `setX8RelayOne(addr, index, state)` |
| Fungsi baru | — | `readX8RelayStatus(addr, outMask)` |

### `firmware/logic_master/src/main.cpp`
| Yang Berubah | v1.1 | v1.2 |
|---|---|---|
| Init RS485 | Serial2 @ 115200 saja | **Serial2 Bus A @ 115200 + Serial1 Bus B @ 9600 via initModbus()** |
| printSystemInfo | 1 baris RS485 | **Tampilkan Bus A dan Bus B** |

### File Baru
| File | Keterangan |
|---|---|
| `firmware/slave-node/src/node_slave_lampu_final.ino` | Firmware ESP32 X8 Relay — Node Lampu (addr 0x02) |
| `firmware/slave-node/src/node_slave_sound_final.ino` | Firmware ESP32 X8 Relay — Node Sound (addr 0x03) |
| `firmware/logic_master/src/command_router_x8_patch.cpp` | Patch command router untuk X8 relay |

---

## Protokol ESP32 X8 Relay (303e32dc812)

Board ini menggunakan protokol **custom** berdasarkan Modbus RTU:

### FC15 Write Multiple Coils (kontrol relay)
```
TX: [addr][0x0F][0x00][0x00][0x00][0x08][0x01][MASK][CRC_L][CRC_H]
RX: [addr][0x0F][0x00][0x00][0x00][0x08][CRC_L][CRC_H]

MASK = bitmask 8 relay
  bit 0 = Relay 1
  bit 1 = Relay 2
  ...
  bit 7 = Relay 8

Contoh:
  0x01 = 00000001b → hanya Relay 1 ON
  0x0F = 00001111b → Relay 1-4 ON
  0xFF = 11111111b → semua ON
  0x00 = 00000000b → semua OFF
```

### FC02 Read Discrete Inputs (baca status)
```
TX: [addr][0x02][0x00][MEM][0x00][0x01][CRC_L][CRC_H]
RX: [addr][0x02][0x01][VAL][CRC_L][CRC_H]

MEM = 0x00 → VAL = status input optocoupler (val_in)
MEM = 0x01 → VAL = status relay saat ini (val_out)
```

---

## Wiring Cepat v1.2

### Logic Master (ESP32-S3 IoT Logger RS485 Plus)
| Terminal/Pin | Tujuan |
|---|---|
| RS485 A+ (onboard) | Bus A → HwThinker kipas |
| RS485 B- (onboard) | Bus A → HwThinker kipas |
| GPIO1 (TX Serial1) | MAX485 Bus B → DI pin |
| GPIO2 (RX Serial1) | MAX485 Bus B → RO pin |
| GPIO5 (DE/RE) | MAX485 Bus B → DE+RE pin |
| MAX485 Bus B A+ | Bus B → X8 Lampu, X8 Sound, SHT20 |
| MAX485 Bus B B- | Bus B → X8 Lampu, X8 Sound, SHT20 |

### ESP32 X8 Relay Lampu (addr 0x02)
| Terminal | Hubungkan ke |
|---|---|
| A+ (RS485) | Bus B A+ |
| B- (RS485) | Bus B B- |
| VCC | 12V atau 24V DC |
| GND | GND bersama |

### ESP32 X8 Relay Sound (addr 0x03)
| Terminal | Hubungkan ke |
|---|---|
| A+ (RS485) | Bus B A+ |
| B- (RS485) | Bus B B- |
| VCC | 12V atau 24V DC |
| GND | GND bersama |

> ⚠️ **Penting:** Set Modbus address di firmware sebelum deploy:
> - Node Lampu: `mb_addr = 0x02`
> - Node Sound: `mb_addr = 0x03`
> Pastikan tidak ada dua device dengan address yang sama di Bus B.

---

## Contoh Penggunaan dari Logic Master

```cpp
// Nyalakan Lampu 1, 2, 3 (relay 1, 2, 3 = bit 0, 1, 2)
setX8Relay(MODBUS_ADDR_RELAY_LIGHT_DEFAULT, 0b00000111);

// Nyalakan Speaker Zone 5 saja
setX8Relay(MODBUS_ADDR_RELAY_SOUND_DEFAULT, 0b00010000);

// Matikan semua lampu
setX8Relay(MODBUS_ADDR_RELAY_LIGHT_DEFAULT, 0x00);

// Toggle satu lampu (pertahankan state lainnya)
setX8RelayOne(MODBUS_ADDR_RELAY_LIGHT_DEFAULT, 2, true);  // Lampu 3 ON

// Baca state relay lampu saat ini
uint8_t currentState;
readX8RelayStatus(MODBUS_ADDR_RELAY_LIGHT_DEFAULT, &currentState);
```
