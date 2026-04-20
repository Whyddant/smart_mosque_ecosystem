# 🔌 DOKUMEN 3: PANDUAN INTEGRASI HARDWARE v1.1

> Catatan konsistensi versi: Nama file direkomendasikan menjadi "DOKUMEN 3 PANDUAN INTEGRASI HARDWARE v1.1.md" agar sesuai dengan versi pada header dan referensi lintas dokumen.

```
╔═══════════════════════════════════════════════════════════════╗
║                                                               ║
║   PANDUAN INTEGRASI HARDWARE v1.1                            ║
║   SMART MOSQUE ECOSYSTEM                                      ║
║                                                               ║
║   Status: Panduan Instalasi Lengkap                          ║
║   Berdasarkan: Kebenaran Tunggal v1.4, STP v1.3             ║
║   Terakhir Diupdate: 27 Desember 2025                        ║
║                                                               ║
╚═══════════════════════════════════════════════════════════════╝
```

---

## 🎯 BAGIAN 1: TUJUAN DAN RUANG LINGKUP

### **1.1 Target Audience**

**Dokumen ini untuk:**
- **Installer:** Teknisi yang melakukan pemasangan fisik
- **Maintenance Technician:** Teknisi perawatan rutin
- **Project Supervisor:** Penanggung jawab kualitas instalasi
- **Anda sendiri:** Sebagai project owner dan implementer

**Prasyarat pengetahuan:**
- Kelistrikan dasar (AC 220V, grounding)
- Pengalaman pasang MCB/wiring rumah
- Familiar dengan multimeter
- Bisa baca diagram wiring
- (Optional) Pengalaman solder komponen elektronik

### **1.2 Prinsip Instalasi**

```
1. SAFETY FIRST
   • Matikan MCB sebelum wiring
   • Gunakan alat pelindung (sarung tangan, safety glasses)
   • Patuhi PUIL (Persyaratan Umum Instalasi Listrik)
   • Test ground continuity

2. MODULARITY
   • Setiap node independen
   • Ganti komponen tanpa ganggu sistem
   • Easy troubleshooting

3. MAINTAINABILITY
   • Semua kabel rapi dan terlabel
   • Dokumentasi as-built wiring
   • Spare parts inventory

4. SCALABILITY
   • Desain support penambahan node
   • No major rewiring untuk expansion
   • Bus topology linear (daisy-chain)
```

### **1.3 Referensi Dokumen**

| Dokumen | Bagian | Topik Relevan |
|---------|--------|---------------|
| [KT-2.1] | Arsitektur | Diagram sistem keseluruhan |
| [KT-3.2] | Manual Control | Parallel wiring & DPDT |
| [KT-6.1] | Hardware | Spesifikasi komponen |
| [STP-2.1] | Physical Layer | RS-485 electrical specs |

---

## 📦 BAGIAN 2: BILL OF MATERIALS (BOM)

### **2.1 Master Logic Controller**

```
┌────────────────────────────────────────────────────────────┐
│ MASTER LOGIC CONTROLLER - COMPONENT LIST                   │
├────────────────────────────────────────────────────────────┤
│ Item                  │ Specification          │ Qty │ Price│
├────────────────────────────────────────────────────────────┤
│ ESP32-S3 DevKit       │ WROOM-N16R8           │  1  │ 150k │
│                       │ 16MB Flash, 8MB PSRAM  │     │      │
├────────────────────────────────────────────────────────────┤
│ MAX3485 Module        │ 3.3V/5V compatible    │  1  │  15k │
│                       │ RS-485 transceiver     │     │      │
├────────────────────────────────────────────────────────────┤
│ Status Display        │ ILI9488 4" TFT        │  1  │ 120k │
│                       │ 480x320, SPI interface │     │      │
├────────────────────────────────────────────────────────────┤
│ Power Supply          │ HLK-PM03              │  1  │  75k │
│                       │ 220VAC to 5VDC/3A     │     │      │
├────────────────────────────────────────────────────────────┤
│ Enclosure             │ IP54 Plastic Box      │  1  │  75k │
│                       │ 150x100x80mm          │     │      │
├────────────────────────────────────────────────────────────┤
│ Terminal Blocks       │ 5.08mm pitch, 10A     │ 10  │  20k │
├────────────────────────────────────────────────────────────┤
│ DIN Rail              │ 35mm, 10cm length     │  1  │  15k │
├────────────────────────────────────────────────────────────┤
│ Cable Glands          │ PG9 for power         │  2  │  10k │
│                       │ PG7 for RS-485        │  1  │   5k │
├────────────────────────────────────────────────────────────┤
│ MOV Varistor          │ 275VAC surge protect  │  1  │   5k │
├────────────────────────────────────────────────────────────┤
│ Capacitors            │ 1000µF/16V electrolyt │  1  │   3k │
│                       │ 100nF ceramic          │  2  │   2k │
├────────────────────────────────────────────────────────────┤
│ Heat Sink (optional)  │ Aluminium + paste     │  1  │   5k │
├────────────────────────────────────────────────────────────┤
│ TOTAL PER MASTER LOGIC                          │     │ 500k │
└────────────────────────────────────────────────────────────┘
```

**Notes:**
- ESP32-S3 wajib punya PSRAM untuk database
- HLK-PM03 lebih reliable daripada PSU murah
- Enclosure IP54 untuk proteksi debu/cipratan air
- Heatsink optional, tapi recommended kalau ruangan panas

---

### **2.2 Master Display Dashboard**

```
┌────────────────────────────────────────────────────────────┐
│ MASTER DISPLAY - COMPONENT LIST                            │
├────────────────────────────────────────────────────────────┤
│ Item                  │ Specification          │ Qty │ Price│
├────────────────────────────────────────────────────────────┤
│ Main Board            │ Waveshare ESP32-S3    │  1  │ 800k │
│                       │ Touch-LCD-7 (SKU:27078)│     │      │
│                       │ Built-in: ESP32-S3,    │     │      │
│                       │ 800x480 touch, RS-485  │     │      │
├────────────────────────────────────────────────────────────┤
│ Wall Mount            │ VESA 75x75 + arm      │  1  │ 150k │
├────────────────────────────────────────────────────────────┤
│ Power Supply          │ 5VDC/3A, UL/CE cert   │  1  │  85k │
│                       │ Quality penting!       │     │      │
├────────────────────────────────────────────────────────────┤
│ Cable Management      │ Cable channel 20x10mm │ 2m  │  20k │
├────────────────────────────────────────────────────────────┤
│ Screen Protector      │ Tempered glass        │  1  │  50k │
│                       │ Anti-gores            │     │      │
├────────────────────────────────────────────────────────────┤
│ Cable Glands          │ PG9 for power         │  1  │   5k │
│                       │ PG7 for RS-485        │  1  │   5k │
├────────────────────────────────────────────────────────────┤
│ TOTAL PER DISPLAY MASTER                        │     │1,115k│
└────────────────────────────────────────────────────────────┘
```

**Notes:**
- Waveshare board sudah all-in-one (ESP32 + LCD + Touch + RS-485)
- PSU quality penting untuk layar (ripple rendah)
- Screen protector wajib untuk touchscreen publik
- VESA mount untuk pasang di dinding profesional

---

### **2.3 Slave Node (Per Device)**

```
┌────────────────────────────────────────────────────────────┐
│ SLAVE NODE - COMPONENT LIST (GENERIC)                      │
├────────────────────────────────────────────────────────────┤
│ Item                  │ Specification          │ Qty │ Price│
├────────────────────────────────────────────────────────────┤
│ ESP32 DevKit          │ ESP32 DevKit V1       │  1  │ 120k │
│                       │ 30-pin, 4MB Flash     │     │      │
├────────────────────────────────────────────────────────────┤
│ MAX3485 Module        │ RS-485 transceiver    │  1  │  15k │
├────────────────────────────────────────────────────────────┤
│ SSR Module            │ 8CH G3MB-202P         │  1  │ 150k │
│                       │ 5V control, 2A load   │     │      │
│                       │ Zero-cross switching   │     │      │
├────────────────────────────────────────────────────────────┤
│ Power Supply          │ HLK-PM03              │  1  │  75k │
│                       │ 5VDC/3A               │     │      │
├────────────────────────────────────────────────────────────┤
│ Enclosure             │ IP65 Waterproof Box   │  1  │  65k │
│                       │ 150x100x80mm          │     │      │
├────────────────────────────────────────────────────────────┤
│ Terminal Blocks       │ 2-pin input           │  4  │   8k │
│                       │ 3-pin output          │  8  │  16k │
├────────────────────────────────────────────────────────────┤
│ RC Snubber (per SSR)  │ 100Ω 2W + 0.1µF/630V │  8  │  24k │
│                       │ For inductive loads   │     │      │
├────────────────────────────────────────────────────────────┤
│ MOV Varistor          │ 275VAC                │  1  │   5k │
├────────────────────────────────────────────────────────────┤
│ Fuse                  │ 5A fast-blow          │  1  │   2k │
│ Fuse Holder           │ PCB mount             │  1  │   3k │
├────────────────────────────────────────────────────────────┤
│ Heatsink              │ Aluminium 100x50x10mm │  1  │  15k │
│ Thermal Adhesive      │ For SSR module        │  1  │  10k │
├────────────────────────────────────────────────────────────┤
│ Capacitors            │ 1000µF + 100nF        │  1  │   5k │
├────────────────────────────────────────────────────────────┤
│ Cable Glands          │ PG9 AC in/out         │  2  │  10k │
│                       │ PG7 RS-485            │  1  │   5k │
├────────────────────────────────────────────────────────────┤
│ TOTAL PER SLAVE NODE (GENERIC)                 │     │ 528k │
└────────────────────────────────────────────────────────────┘

┌────────────────────────────────────────────────────────────┐
│ TAMBAHAN UNTUK KIPAS (FAN_4CH) - ADD TO ABOVE             │
├────────────────────────────────────────────────────────────┤
│ Mode Switch (DPDT)    │ 6-pin, 10A/250VAC     │  1  │  25k │
│ Switch Box            │ Plastic 50x50x30mm    │  1  │  10k │
│ Micro Switch (detect) │ SPDT for GPIO35       │  1  │   5k │
│ Label Plate           │ AUTO/MANUAL marker    │  1  │   5k │
│ Extra Cable           │ 3-core 0.5mm²         │ 1m  │  10k │
├────────────────────────────────────────────────────────────┤
│ TOTAL TAMBAHAN FOR FAN                          │     │  55k │
├────────────────────────────────────────────────────────────┤
│ TOTAL KIPAS NODE (528k + 55k)                  │     │ 583k │
└────────────────────────────────────────────────────────────┘
```

**Critical Notes:**
- **G3MB-202P** wajib pakai heatsink (3W dissipation per SSR)
- **RC Snubber** mandatory untuk kipas/motor (inductive load)
- **IP65 enclosure** untuk area dekat air wudhu/outdoor
- **Mode switch** hanya untuk kipas (DPDT 6-pin changeover)
- ⚠ Beban induktif kipas dapat memiliki arus inrush signifikan; verifikasi arus nominal vs derating SSR. Pertimbangkan SSR/triac khusus induktif 5–10A atau kontaktor mini bila diperlukan.

---

### **2.4 Kabel & Aksesoris Sistem**

```
┌────────────────────────────────────────────────────────────┐
│ CABLES & ACCESSORIES                                        │
├────────────────────────────────────────────────────────────┤
│ Item                  │ Specification          │ Qty │ Price│
├────────────────────────────────────────────────────────────┤
│ UTP Cable             │ Cat6, solid copper    │100m │ 600k │
│                       │ CCA NOT recommended    │     │      │
├────────────────────────────────────────────────────────────┤
│ RJ45 Connectors       │ Cat6, shielded        │ 30  │  60k │
├────────────────────────────────────────────────────────────┤
│ Power Cable           │ NYM 2x1.5mm²          │ 50m │ 250k │
│                       │ For AC distribution    │     │      │
├────────────────────────────────────────────────────────────┤
│ Cable Ties            │ Nylon, various sizes  │100  │  20k │
├────────────────────────────────────────────────────────────┤
│ Cable Markers         │ Self-laminating 1-50  │  1  │  30k │
├────────────────────────────────────────────────────────────┤
│ Wire Duct             │ 40x40mm PVC           │10m  │ 100k │
├────────────────────────────────────────────────────────────┤
│ Termination Resistor  │ 120Ω, 1/4W            │  4  │   4k │
│                       │ 2 for bus ends + spare │     │      │
├────────────────────────────────────────────────────────────┤
│ Heat Shrink Tube      │ Various sizes         │  1  │  20k │
├────────────────────────────────────────────────────────────┤
│ Solder & Flux         │ For connections       │  1  │  30k │
├────────────────────────────────────────────────────────────┤
│ TOTAL CABLES & ACCESSORIES                      │     │1,114k│
└────────────────────────────────────────────────────────────┘
```

---

### **2.5 Tools yang Dibutuhkan**

```
┌────────────────────────────────────────────────────────────┐
│ REQUIRED TOOLS                                              │
├────────────────────────────────────────────────────────────┤
│ Tool                  │ Specification          │ Price      │
├────────────────────────────────────────────────────────────┤
│ Multimeter            │ Digital, continuity    │  150k      │
├────────────────────────────────────────────────────────────┤
│ Crimping Tool         │ For RJ45               │  150k      │
├────────────────────────────────────────────────────────────┤
│ Cable Tester          │ Network cable tester   │   75k      │
├────────────────────────────────────────────────────────────┤
│ Wire Stripper         │ Automatic, adjustable  │   50k      │
├────────────────────────────────────────────────────────────┤
│ Screwdriver Set       │ Phillips, flat         │   75k      │
├────────────────────────────────────────────────────────────┤
│ Soldering Iron        │ 60W with stand         │  100k      │
├────────────────────────────────────────────────────────────┤
│ Drill & Bits          │ 6mm, 8mm, 10mm         │  200k      │
├────────────────────────────────────────────────────────────┤
│ Level                 │ Small bubble level     │   20k      │
├────────────────────────────────────────────────────────────┤
│ Label Printer         │ Brother P-touch        │  500k      │
│                       │ (nice to have)         │            │
├────────────────────────────────────────────────────────────┤
│ TOTAL TOOLS                                     │ ~1,320k    │
│ (One-time investment)                           │            │
└────────────────────────────────────────────────────────────┘
```

---

### **2.6 Complete System BOM Summary**

```
┌────────────────────────────────────────────────────────────┐
│ COMPLETE SYSTEM COST BREAKDOWN                              │
├────────────────────────────────────────────────────────────┤
│ Component                    │ Qty    │ Unit    │ Total    │
├────────────────────────────────────────────────────────────┤
│ Master Logic Controller      │  1     │  500k   │   500k   │
│ Master Display Dashboard     │  1     │1,115k   │ 1,115k   │
│ Slave Kipas (FAN_4CH)        │ 13     │  583k   │ 7,579k   │
│ Slave Lampu (LIGHT_8CH)      │  4     │  528k   │ 2,112k   │
│ Slave Sound (SOUND_8CH)      │  1     │  528k   │   528k   │
│ Cables & Accessories         │  1     │1,114k   │ 1,114k   │
├────────────────────────────────────────────────────────────┤
│ SUBTOTAL (Hardware)                            │ 12,948k   │
├────────────────────────────────────────────────────────────┤
│ Tools (one-time investment)  │  1     │1,320k   │ 1,320k   │
├────────────────────────────────────────────────────────────┤
│ GRAND TOTAL                                    │ 14,268k   │
│ (≈ Rp 14.3 juta)                                          │
└────────────────────────────────────────────────────────────┘

NOTES:
• Harga estimasi (Desember 2025)
• Belum termasuk ongkos pasang
• Spare parts recommended: +10% (~Rp 1.3 juta)
• Total dengan spare: ~Rp 15.6 juta
```

---

## 🔌 BAGIAN 3: DIAGRAM PENGAWATAN DETAIL

### **3.1 Master Logic Controller Wiring**

```
┌─────────────────────────────────────────────────────────────┐
│          MASTER LOGIC CONTROLLER - WIRING DIAGRAM           │
└─────────────────────────────────────────────────────────────┘

        220VAC INPUT
            │
            ▼
    ┌───────────────┐
    │   AC INPUT    │
    │   PROTECTION  │
    ├───────────────┤
    │               │
    │  L ─┬─ MOV ─┐ │
    │     │  275V │ │
    │  N ─┴───────┘ │
    │     │         │
    │  E ─┴─ Ground│
    │               │
    └───────┬───────┘
            │
            ▼
    ┌───────────────┐
    │  HLK-PM03 PSU │
    │ 220VAC→5VDC/3A│
    ├───────────────┤
    │  L  N  E  │ ○○│
    │  │  │  │  │ 5V│
    │  └──┴──┴──┴─GND
    └───────┬───────┘
            │
        5VDC/GND
            │
      ┌─────┴─────┬──────────┬──────────┐
      │           │          │          │
      ▼           ▼          ▼          ▼
  ┌────────┐ ┌────────┐ ┌───────┐ ┌────────┐
  │ESP32-S3│ │MAX3485 │ │ILI9488│ │ Caps   │
  │ DevKit │ │  IC    │ │  4"   │ │1000µF +│
  │        │ │        │ │Display│ │ 100nF  │
  └────────┘ └────────┘ └───────┘ └────────┘

ESP32-S3 PIN CONNECTIONS:
┌─────────────────────────────────────────────────────────────┐
│ GPIO │ Function      │ Connected To                         │
├─────────────────────────────────────────────────────────────┤
│  16  │ RS485 RX      │ MAX3485 RO (pin 1)                  │
│  17  │ RS485 TX      │ MAX3485 DI (pin 4)                  │
│   4  │ RS485 DE/RE   │ MAX3485 DE & RE (pins 2,3)          │
│  ─────────────────────────────────────────────────────────  │
│   5  │ TFT_CS        │ ILI9488 CS                          │
│  21  │ TFT_DC        │ ILI9488 DC/RS                       │
│  22  │ TFT_RST       │ ILI9488 RESET                       │
│  23  │ TFT_MOSI      │ ILI9488 SDI (MOSI)                  │
│  18  │ TFT_CLK       │ ILI9488 SCK                         │
│  ─────────────────────────────────────────────────────────  │
│  33  │ UART_RX       │ To Display Master TX (optional)     │
│  32  │ UART_TX       │ To Display Master RX (optional)     │
│  ─────────────────────────────────────────────────────────  │
│   2  │ LED_STATUS    │ Onboard LED (status indicator)      │
│  15  │ LED_RS485_ACT │ External LED (optional)             │
└─────────────────────────────────────────────────────────────┘

MAX3485 TO RS-485 BUS:
┌─────────────────────────────────────────────────────────────┐
│                                                              │
│  MAX3485 Pin 6 (A) ──→ Terminal "RS485_A+"                 │
│                    └──→ UTP Blue-White (to slaves)          │
│                                                              │
│  MAX3485 Pin 7 (B) ──→ Terminal "RS485_B-"                 │
│                    └──→ UTP Green-White (to slaves)         │
│                                                              │
│  MAX3485 Pin 5 (GND) ─→ Terminal "RS485_GND"               │
│                    └──→ UTP Brown pair (to slaves)          │
│                                                              │
│  [120Ω Termination] Between A-B (first node in chain)      │
│                                                              │
└─────────────────────────────────────────────────────────────┘

POWER DISTRIBUTION:
┌─────────────────────────────────────────────────────────────┐
│                                                              │
│  5VDC ──┬─[1000µF]─┬──→ ESP32 VIN (5V)                     │
│         │          │                                         │
│         ├──────────┼──→ MAX3485 VCC (3.3V from ESP32)      │
│         │          │                                         │
│         └─[100nF]──┴──→ ILI9488 VCC (3.3V from ESP32)      │
│                                                              │
│  GND ───┴──────────┴──→ Common GND (star point)            │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

---

### **3.2 Slave Node Wiring (Generic)**

```
┌─────────────────────────────────────────────────────────────┐
│             SLAVE NODE - COMPLETE WIRING DIAGRAM            │
└─────────────────────────────────────────────────────────────┘

    220VAC INPUT (from MCB)
        │
        ▼
┌───────────────────┐
│  INPUT PROTECTION │
├───────────────────┤
│ L ─┬─ FUSE 5A ─┐  │
│    │           │  │
│    ├─ MOV ─────┤  │
│    │   275V    │  │
│ N ─┴───────────┘  │
│    │              │
│ E ─┴─ Ground      │
└────────┬──────────┘
         │
    ┌────┴────┐
    │         │
    ▼         ▼
┌─────────┐ ┌──────────────┐
│HLK-PM03 │ │ TO LOAD      │
│220V→5V  │ │ (via SSR)    │
└────┬────┘ └──────────────┘
     │
   5VDC/GND
     │
 ┌───┴───┬────────┬────────┐
 │       │        │        │
 ▼       ▼        ▼        ▼
┌───┐ ┌──────┐ ┌────┐ ┌──────┐
│ESP│ │MAX485│ │8CH │ │ Caps │
│32 │ │      │ │SSR │ │      │
└───┘ └──────┘ └────┘ └──────┘

ESP32 PIN CONNECTIONS (STANDARD FOR ALL SLAVES):
┌─────────────────────────────────────────────────────────────┐
│ GPIO │ Function      │ Connected To                         │
├─────────────────────────────────────────────────────────────┤
│  16  │ RS485 RX      │ MAX3485 RO                          │
│  17  │ RS485 TX      │ MAX3485 DI                          │
│   4  │ RS485 DE/RE   │ MAX3485 DE & RE                     │
│  ─────────────────────────────────────────────────────────  │
│  12  │ RELAY_1       │ SSR CH1 input (LOW/Lampu CH1)       │
│  13  │ RELAY_2       │ SSR CH2 input (MED/Lampu CH2)       │
│  14  │ RELAY_3       │ SSR CH3 input (HIGH/Lampu CH3)      │
│  15  │ RELAY_4       │ SSR CH4 input (Reserve/Lampu CH4)   │
│  25  │ RELAY_5       │ SSR CH5 input (Lampu CH5)           │
│  26  │ RELAY_6       │ SSR CH6 input (Lampu CH6)           │
│  27  │ RELAY_7       │ SSR CH7 input (Lampu CH7)           │
│  32  │ RELAY_8       │ SSR CH8 input (Lampu CH8)           │
│  ─────────────────────────────────────────────────────────  │
│  35  │ MODE_DETECT   │ DPDT mode switch (FAN only)         │
│   2  │ LED_STATUS    │ Onboard LED (mode indicator)        │
└─────────────────────────────────────────────────────────────┘

8CH SSR MODULE CONNECTIONS:
┌─────────────────────────────────────────────────────────────┐
│                                                              │
│  INPUT SIDE (Control):                                       │
│    VCC ←── 5V from PSU                                      │
│    GND ←── Common GND                                       │
│    IN1-8 ←── ESP32 GPIO 12,13,14,15,25,26,27,32           │
│                                                              │
│  OUTPUT SIDE (Load):                                         │
│    Each SSR has: [Common] [NO]                              │
│                                                              │
│    SSR1: COM1 ←── Manual Switch / 220V                     │
│          NO1  ──→ Load 1                                    │
│                                                              │
│    SSR2-8: Similar wiring                                   │
│                                                              │
│  RC SNUBBER (per SSR, for inductive loads):                │
│    Between NO and Load: 100Ω (2W) + 0.1µF (630VAC)        │
│                                                              │
└─────────────────────────────────────────────────────────────┘

LOAD WIRING (Parallel with Manual):
┌─────────────────────────────────────────────────────────────┐
│                                                              │
│  220VAC L ──┬─── Manual Switch ───┬──→ Load                │
│             │                       │                        │
│             └─── SSR Output (NO) ──┘                        │
│                                                              │
│  220VAC N ───────────────────────────→ Load                │
│
│                                                              │
│  Logic: OR gate (salah satu ON → Load ON)                  │
│                                                              │
│  For KIPAS (with DPDT mode switch):                        │
│    See section 3.3 for detailed DPDT wiring                │
│                                                              │
└─────────────────────────────────────────────────────────────┘

RS-485 BUS CONNECTION (DAISY-CHAIN):
┌─────────────────────────────────────────────────────────────┐
│                                                              │
│  FROM PREVIOUS NODE:                                         │
│    UTP Blue-White   ──→ MAX3485 Pin 6 (A)                  │
│    UTP Green-White  ──→ MAX3485 Pin 7 (B)                  │
│    UTP Brown pair   ──→ MAX3485 Pin 5 (GND)                │
│                                                              │
│  TO NEXT NODE:                                               │
│    MAX3485 Pin 6 (A)   ──→ UTP Blue-White                  │
│    MAX3485 Pin 7 (B)   ──→ UTP Green-White                 │
│    MAX3485 Pin 5 (GND) ──→ UTP Brown pair                  │
│                                                              │
│  TERMINATION (LAST NODE ONLY):                              │
│    120Ω resistor between Pin 6 (A) and Pin 7 (B)           │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

---

### **3.3 Kipas Node Wiring (FAN_4CH dengan DPDT)**

```
┌─────────────────────────────────────────────────────────────┐
│        KIPAS (FAN_4CH) - DPDT MODE SWITCH WIRING           │
└─────────────────────────────────────────────────────────────┘

DPDT MODE SWITCH (6-pin Changeover):
┌─────────────────────────────────────────────────────────────┐
│                                                              │
│     Pin Layout:                                              │
│                                                              │
│       1 (AUTO L)     2 (COM L)      3 (MANUAL L)           │
│          ○              ○               ○                    │
│          │              │               │                    │
│          │        ┌─────┴─────┐        │                    │
│          │        │  TOGGLE   │        │                    │
│          │        │  SWITCH   │        │                    │
│          │        └─────┬─────┘        │                    │
│          │              │               │                    │
│          ○              ○               ○                    │
│       4 (AUTO N)     5 (COM N)      6 (MANUAL N)           │
│                                                              │
└─────────────────────────────────────────────────────────────┘

TERMINAL CONNECTIONS:
┌─────────────────────────────────────────────────────────────┐
│                                                              │
│  INPUT (Power Source):                                       │
│    220VAC L ──→ Pin 2 (COM L)                               │
│    220VAC N ──→ Pin 5 (COM N)                               │
│                                                              │
│  AUTO PATH:                                                  │
│    Pin 1 (AUTO L) ──┬──→ SSR_LOW common                    │
│                     ├──→ SSR_MED common                     │
│                     └──→ SSR_HIGH common                    │
│                                                              │
│    Pin 4 (AUTO N) ──┬──→ SSR_LOW input side                │
│                     ├──→ SSR_MED input side                 │
│                     └──→ SSR_HIGH input side                │
│                                                              │
│  MANUAL PATH:                                                │
│    Pin 3 (MANUAL L) ──→ Speed Switch input                 │
│    Pin 6 (MANUAL N) ──→ Motor Neutral (direct)             │
│                                                              │
└─────────────────────────────────────────────────────────────┘

SPEED SWITCH (3-way Rotary):
┌─────────────────────────────────────────────────────────────┐
│                                                              │
│    Input: Pin 3 (MANUAL L) from DPDT                        │
│                                                              │
│    Output positions:                                         │
│      Position 1 ──→ Motor LOW winding                       │
│      Position 2 ──→ Motor MED winding                       │
│      Position 3 ──→ Motor HIGH winding                      │
│                                                              │
│    Motor Neutral ←─ Pin 6 (MANUAL N) from DPDT             │
│                                                              │
└─────────────────────────────────────────────────────────────┘

SSR TO MOTOR WIRING (Parallel with Speed Switch):
┌─────────────────────────────────────────────────────────────┐
│                                                              │
│  Motor LOW winding ──┬── Speed Switch Position 1           │
│                      └── SSR_LOW output (NO)                │
│                                                              │
│  Motor MED winding ──┬── Speed Switch Position 2           │
│                      └── SSR_MED output (NO)                │
│                                                              │
│  Motor HIGH winding ─┬── Speed Switch Position 3           │
│                      └── SSR_HIGH output (NO)               │
│                                                              │
│  Motor Neutral ──────┴── DPDT Pin 6 or Pin 4               │
│                          (depending on mode)                │
│                                                              │
└─────────────────────────────────────────────────────────────┘

MODE DETECTION CIRCUIT:
┌─────────────────────────────────────────────────────────────┐
│                                                              │
│  OPTION A: Voltage Sensing (via optocoupler)               │
│    DPDT Pin 1 (AUTO L) ──→ [Voltage Divider] ──→ Opto     │
│                                                     │        │
│                                              Opto output     │
│                                                     │        │
│                                             ESP32 GPIO35    │
│                                                              │
│  OPTION B: Micro Switch (Recommended - Simpler)            │
│    Add small SPDT micro switch mechanically coupled:        │
│                                                              │
│      Micro Switch:                                           │
│        Common ←── 3.3V from ESP32                           │
│        NO (AUTO) ──→ GPIO35 (pulled HIGH in AUTO)          │
│        NC (MANUAL) ──→ GND (pulled LOW in MANUAL)          │
│                                                              │
│      Result:                                                 │
│        DPDT in AUTO → Micro NO closed → GPIO35 = HIGH      │
│        DPDT in MANUAL → Micro NC closed → GPIO35 = LOW     │
│                                                              │
└─────────────────────────────────────────────────────────────┘

BEHAVIOR SUMMARY:
┌─────────────────────────────────────────────────────────────┐
│                                                              │
│  DPDT Position: AUTO                                         │
│    • Pin 2→1, 5→4: SSR path powered                        │
│    • Pin 3, 6: Open (speed switch isolated)                │
│    • GPIO35: HIGH                                            │
│    • Control: ESP32 via RS-485 commands                     │
│                                                              │
│  DPDT Position: MANUAL                                       │
│    • Pin 2→3, 5→6: Speed switch powered                    │
│    • Pin 1, 4: Open (SSR isolated)                         │
│    • GPIO35: LOW                                             │
│    • Control: Physical speed switch (direct)                │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

---

### **3.4 Complete System Topology**

```
┌─────────────────────────────────────────────────────────────┐
│           COMPLETE SYSTEM WIRING TOPOLOGY                    │
└─────────────────────────────────────────────────────────────┘

POWER DISTRIBUTION:
┌─────────────────────────────────────────────────────────────┐
│                                                              │
│         MAIN MCB PANEL (220VAC)                             │
│                │                                             │
│     ┌──────────┴──────────┬───────────┬──────────┐         │
│     │                      │           │          │         │
│     ▼                      ▼           ▼          ▼         │
│ Master Logic          Display      Slave 1   ... Slave 18  │
│ (HLK-PM03)           (PSU 5V/3A)  (HLK-PM03)              │
│     │                      │           │          │         │
│     └──────────┬───────────┴───────────┴──────────┘         │
│                │                                             │
│            All 5VDC                                          │
│                                                              │
└─────────────────────────────────────────────────────────────┘

RS-485 BUS (LINEAR DAISY-CHAIN):
┌─────────────────────────────────────────────────────────────┐
│                                                              │
│  [120Ω]                                                      │
│    │                                                         │
│    ▼                                                         │
│  ┌──────────┐    ┌──────────┐    ┌──────────┐             │
│  │  Master  │───→│ Display  │───→│ Slave 1  │───→ ...     │
│  │  Logic   │    │  Master  │    │          │             │
│  │ (ID:254) │    │ (ID:255) │    │  (ID:1)  │             │
│  └──────────┘    └──────────┘    └──────────┘             │
│                                                              │
│    ... ───→ Slave 17 ───→ Slave 18                         │
│              (ID:17)      (ID:18)                           │
│                              │                               │
│                              ▼                               │
│                            [120Ω]                            │
│                                                              │
│  UTP Cat6 Cable:                                            │
│    Blue-White  = RS-485 A+                                  │
│    Green-White = RS-485 B-                                  │
│    Brown pair  = Common GND                                 │
│                                                              │
│  CRITICAL RULES:                                             │
│    • NO star/branch topology                                 │
│    • Termination ONLY at first & last node                  │
│    • Max cable length: 200m total                           │
│    • Max node-to-node distance: 50m                         │
│                                                              │
└─────────────────────────────────────────────────────────────┘

LOAD CONNECTIONS (Example for 1 Kipas + 1 Lampu):
┌─────────────────────────────────────────────────────────────┐
│                                                              │
│  SLAVE 1 (Kipas Saf 1):                                     │
│    220VAC ──→ DPDT Mode Switch ──→ Motor 3-speed           │
│              (AUTO/MANUAL)                                   │
│                                                              │
│  SLAVE 14 (Lampu Utama Kanan):                             │
│    CH1: 220VAC ──┬─ Manual Switch ─┬──→ Lampu Depan       │
│                  └─ SSR CH1 ────────┘                       │
│                                                              │
│    CH2: 220VAC ──┬─ Manual Switch ─┬──→ Lampu Tengah      │
│                  └─ SSR CH2 ────────┘                       │
│                                                              │
│    CH3-8: Similar wiring                                     │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

---

## 🔧 BAGIAN 4: PANDUAN PEMASANGAN STEP-BY-STEP

### **4.1 Phase 1: Persiapan & Site Survey**

#### **4.1.1 Site Survey Checklist**

```
PRE-INSTALLATION SURVEY:
┌─────────────────────────────────────────────────────────────┐
│                                                              │
│ [ ] 1. Ukur jarak antar lokasi slave                        │
│       • Buat sketch layout masjid                            │
│       • Tandai posisi setiap kipas/lampu                     │
│       • Hitung total panjang kabel yang dibutuhkan           │
│                                                              │
│ [ ] 2. Identifikasi rute kabel RS-485                       │
│       • Hindari parallel dengan kabel power >50cm            │
│       • Cari jalur existing (cable tray/conduit)            │
│       • Tandai crossing points dengan AC cable               │
│                                                              │
│ [ ] 3. Tentukan lokasi Master Logic                         │
│       • Dekat panel MCB utama                                │
│       • Akses mudah untuk maintenance                        │
│       • Suhu ruangan < 40°C                                  │
│       • Ada power outlet 220VAC                              │
│                                                              │
│ [ ] 4. Tentukan lokasi Display Master                       │
│       • Ruang mihrab/imam (untuk display utama)             │
│       • Kantor pengurus (untuk monitoring)                   │
│       • Tinggi 150-160cm dari lantai                         │
│       • Tidak silau dari cahaya jendela                      │
│       • Ada power outlet & WiFi coverage                     │
│                                                              │
│ [ ] 5. Cek existing manual switches                         │
│       • Lokasi setiap saklar manual                          │
│       • Jenis saklar (toggle/rotary/push button)            │
│       • Kondisi wiring (bisa tap parallel?)                  │
│                                                              │
│ [ ] 6. Identifikasi titik mounting enclosure                │
│       • Slave kipas: Dekat motor (max 2m kabel)             │
│       • Slave lampu: Panel distribusi lampu                  │
│       • Slave sound: Ruang audio/amplifier                   │
│       • Cek struktur dinding (beton/gypsum)                  │
│                                                              │
│ [ ] 7. Environment check                                     │
│       • Suhu ambient (kipas outdoor?)                        │
│       • Kelembaban (dekat area wudhu?)                       │
│       • Exposure air/debu                                    │
│       • Akses untuk maintenance                              │
│                                                              │
│ [ ] 8. Power availability                                    │
│       • Cek MCB capacity (cukup untuk tambahan?)            │
│       • Identifikasi phase L untuk tiap area                 │
│       • Ground continuity test                               │
│                                                              │
│ [ ] 9. Documentation                                         │
│       • Foto lokasi setiap device                            │
│       • Foto existing panel/wiring                           │
│       • Buat as-is diagram                                   │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

#### **4.1.2 Material Preparation**

```
LABELING SYSTEM:
┌─────────────────────────────────────────────────────────────┐
│                                                              │
│  DEVICES:                                                    │
│    • ML-001: Master Logic                                    │
│    • MD-001: Master Display (Mihrab)                        │
│    • SL-001: Slave 1 - Kipas Saf 1 Depan                   │
│    • SL-002: Slave 2 - Kipas Saf 2 Depan                   │
│    • ...                                                     │
│    • SL-014: Slave 14 - Lampu Utama Kanan                  │
│    • ...                                                     │
│    • SL-018: Slave 18 - Audio Mimbar                        │
│                                                              │
│  CABLES (UTP):                                               │
│    • RS485-A: Blue-White (A+)                               │
│    • RS485-B: Green-White (B-)                              │
│    • RS485-GND: Brown pair (Common GND)                     │
│                                                              │
│  CABLES (Power):                                             │
│    • PWR-5V: Red (Positive)                                 │
│    • PWR-GND: Black (Negative)                              │
│    • PWR-220L: Brown (Live)                                 │
│    • PWR-220N: Blue (Neutral)                               │
│    • PWR-PE: Yellow-Green (Earth)                           │
│                                                              │
└─────────────────────────────────────────────────────────────┘

PRE-ASSEMBLY PREPARATION:
┌─────────────────────────────────────────────────────────────┐
│                                                              │
│  [ ] Cut UTP cables to measured lengths + 20% slack         │
│  [ ] Crimp RJ45 connectors (test with cable tester)        │
│  [ ] Prepare power cables (strip, tin ends)                │
│  [ ] Label all cables BEFORE installation                   │
│  [ ] Pre-drill enclosure mounting holes                     │
│  [ ] Prepare RC snubbers (solder components)                │
│  [ ] Test all PSU units (load test)                         │
│  [ ] Flash firmware to all ESP32 boards                     │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

---

### **4.2 Phase 2: Master Logic Installation**

#### **4.2.1 Enclosure Assembly**

```
STEP-BY-STEP ASSEMBLY:
┌─────────────────────────────────────────────────────────────┐
│                                                              │
│  STEP 1: DIN Rail Installation                              │
│    1. Mount DIN rail di bagian bawah enclosure              │
│    2. Gunakan screw M4 (included with rail)                 │
│    3. Pastikan level (gunakan bubble level)                 │
│                                                              │
│  STEP 2: HLK-PM03 Mounting                                  │
│    1. Clip HLK-PM03 ke DIN rail                             │
│    2. Posisi di pojok kiri bawah                            │
│    3. Pastikan kencang (push down firmly)                   │
│                                                              │
│  STEP 3: ESP32-S3 Mounting                                  │
│    1. Gunakan PCB standoff/spacer plastik                   │
│    2. Posisi tengah enclosure (akses GPIO mudah)            │
│    3. Tinggi 15mm dari dasar enclosure                      │
│    4. Screw M3 untuk secure                                 │
│                                                              │
│  STEP 4: MAX3485 Module Mounting                            │
│    1. Dekat ESP32 (short wiring)                            │
│    2. Gunakan double-sided tape atau standoff               │
│    3. Orientation: Pins menghadap ke ESP32                  │
│                                                              │
│  STEP 5: ILI9488 Display Mounting                          │
│    1. Posisi di atas (visible melalui enclosure window)     │
│    2. Secure dengan M2.5 screws                             │
│    3. Jarak 5mm dari enclosure top                          │
│                                                              │
│  STEP 6: Terminal Blocks Installation                       │
│    1. DIN rail terminal blocks di bagian kanan              │
│    2. Label setiap terminal:                                │
│       • AC_IN (L, N, E)                                     │
│       • RS485 (A+, B-, GND)                                 │
│       • STATUS (LED+, LED-)                                 │
│    3. Pastikan jarak cukup untuk wiring                     │
│                                                              │
│  STEP 7: Cable Glands Installation                         │
│    1. Drill lubang diameter sesuai PG size                  │
│    2. PG9 untuk AC power input                              │
│    3. PG7 untuk RS-485 UTP                                  │
│    4. Pasang gland dengan washer & nut                      │
│    5. Test waterproof (coba tarik kabel)                    │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

#### **4.2.2 Internal Wiring**

```
WIRING SEQUENCE (Follow this order to avoid mistakes):
┌─────────────────────────────────────────────────────────────┐
│                                                              │
│  PHASE 1: AC INPUT PROTECTION                               │
│    1. Wire AC_IN terminal:                                  │
│       220VAC L → Terminal L_IN                              │
│       220VAC N → Terminal N_IN                              │
│       PE → Terminal E_IN                                     │
│                                                              │
│    2. Install MOV varistor:                                 │
│       Between L_IN and N_IN (solder or crimp)              │
│       Insulate with heat shrink                             │
│                                                              │
│    3. Wire to HLK-PM03:                                     │
│       L_IN → HLK-PM03 AC_L                                  │
│       N_IN → HLK-PM03 AC_N                                  │
│       E_IN → Enclosure body (screw lug)                     │
│                                                              │
│  PHASE 2: DC POWER DISTRIBUTION                             │
│    4. HLK-PM03 output:                                      │
│       +5V → Bus bar or thick wire (1.5mm²)                 │
│       GND → Bus bar (common star point)                     │
│                                                              │
│    5. Add filter capacitors near PSU output:                │
│       1000µF/16V electrolytic (observe polarity!)          │
│       100nF ceramic (in parallel)                           │
│                                                              │
│    6. Wire to ESP32-S3:                                     │
│       +5V → ESP32 VIN pin                                   │
│       GND → ESP32 GND pin                                   │
│       (Use 0.5mm² wire, short as possible)                  │
│                                                              │
│  PHASE 3: RS-485 CONNECTIONS                                │
│    7. Wire MAX3485 to ESP32:                                │
│       MAX3485 RO  → ESP32 GPIO16 (RX)                      │
│       MAX3485 DI  → ESP32 GPIO17 (TX)                      │
│       MAX3485 DE  → ESP32 GPIO4                            │
│       MAX3485 RE  → ESP32 GPIO4 (bridge with DE)           │
│                                                              │
│    8. Wire MAX3485 to terminal block:                       │
│       MAX3485 A   → Terminal RS485_A                        │
│       MAX3485 B   → Terminal RS485_B                        │
│       MAX3485 GND → Terminal RS485_GND                      │
│                                                              │
│    9. Install 120Ω termination resistor:                    │
│       Between RS485_A and RS485_B terminals                 │
│       (This is FIRST node in bus)                           │
│                                                              │
│  PHASE 4: DISPLAY CONNECTIONS (ILI9488)                    │
│    10. Wire SPI interface (use short wires):                │
│        TFT_VCC → ESP32 3.3V                                 │
│        TFT_GND → ESP32 GND                                  │
│        TFT_CS  → ESP32 GPIO5                                │
│        TFT_DC  → ESP32 GPIO21                               │
│        TFT_RST → ESP32 GPIO22                               │
│        TFT_SDI → ESP32 GPIO23 (MOSI)                        │
│        TFT_SCK → ESP32 GPIO18                               │
│        TFT_LED → 3.3V (backlight, or PWM for dimming)      │
│                                                              │
│  PHASE 5: STATUS LED (Optional)                            │
│    11. External LED indicator:                              │
│        ESP32 GPIO15 → 220Ω resistor → LED Anode            │
│        LED Cathode → GND                                    │
│                                                              │
└─────────────────────────────────────────────────────────────┘

WIRING BEST PRACTICES:
┌─────────────────────────────────────────────────────────────┐
│                                                              │
│  • Use wire ferrules for terminal connections               │
│  • Strain relief for all external cables                    │
│  • Bundle power and signal separately                       │
│  • Leave 10cm service loop inside enclosure                │
│  • Use different color wires (standard code)                │
│  • Label every wire at both ends                            │
│  • No bare conductors exposed                               │
│  • Check continuity before power on                         │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

#### **4.2.3 Initial Power Test**

```
POWER-ON SEQUENCE (SAFETY FIRST!):
┌─────────────────────────────────────────────────────────────┐
│                                                              │
│  PRE-POWER CHECKS:                                           │
│  [ ] Visual inspection (no shorts, proper polarity)         │
│  [ ] Continuity test (GND, 5V rail)                         │
│  [ ] Insulation test (AC L-N, L-E, N-E)                    │
│  [ ] Termination resistor installed (120Ω between A-B)     │
│                                                              │
│  FIRST POWER ON:                                             │
│  1. Disconnect RS-485 bus (test master alone first)         │
│  2. Connect 220VAC power                                     │
│  3. Observe:                                                 │
│     [ ] HLK-PM03 LED nyala hijau                            │
│     [ ] No smoke or burning smell                           │
│     [ ] No unusual sounds                                    │
│                                                              │
│  4. Measure voltages with multimeter:                       │
│     [ ] HLK-PM03 output: 5.0V ± 0.25V                      │
│     [ ] ESP32 VIN: 5.0V ± 0.25V                            │
│     [ ] ESP32 3.3V: 3.3V ± 0.15V                           │
│                                                              │
│  5. Check ESP32 boot:                                       │
│     [ ] Onboard LED berkedip (boot sequence)                │
│     [ ] Serial monitor shows boot messages                  │
│     [ ] ILI9488 display shows splash screen                 │
│                                                              │
│  6. Test RS-485 driver enable:                              │
│     Upload test firmware:                                    │
│                                                              │
│     void setup() {                                           │
│       Serial.begin(115200);                                  │
│       pinMode(4, OUTPUT);   // DE/RE                        │
│       Serial2.begin(115200, SERIAL_8N1, 16, 17);           │
│                                                              │
│       Serial.println("RS-485 Test");                        │
│     }                                                        │
│                                                              │
│     void loop() {                                            │
│       // Test transmit                                       │
│       digitalWrite(4, HIGH);  // Enable TX                  │
│       Serial2.println("TEST");                              │
│       digitalWrite(4, LOW);   // Enable RX                  │
│                                                              │
│       delay(1000);                                           │
│     }                                                        │
│                                                              │
│  7. Measure RS-485 output with oscilloscope:                │
│     [ ] Differential voltage 2-5V                           │
│     [ ] No glitches or noise                                │
│     [ ] Clean waveform                                      │
│                                                              │
│  IF ALL OK:                                                  │
│  [ ] Power off                                               │
│  [ ] Connect RS-485 bus to first slave                      │
│  [ ] Proceed to slave installation                          │
│                                                              │
│  IF ANY ISSUE:                                               │
│  [ ] Power off immediately                                   │
│  [ ] Troubleshoot before proceeding                         │
│  [ ] Document the issue                                     │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

---

### **4.3 Phase 3: Slave Node Installation**

#### **4.3.1 Slave Assembly (Generic Template)**

```
SLAVE NODE ASSEMBLY CHECKLIST:
┌─────────────────────────────────────────────────────────────┐
│                                                              │
│  [ ] Step 1: Prepare enclosure                              │
│      • Clean enclosure interior                              │
│      • Install cable glands (PG9 × 2, PG7 × 1)             │
│      • Drill ventilation holes (4× Ø10mm top & bottom)     │
│                                                              │
│  [ ] Step 2: Mount HLK-PM03 PSU                            │
│      • Position: Bottom left corner                          │
│      • Secure with screws or DIN rail clip                  │
│      • Ensure input/output accessible                       │
│                                                              │
│  [ ] Step 3: Mount 8CH SSR module                          │
│      • Position: Bottom (largest component)                  │
│      • Attach aluminium heatsink with thermal adhesive      │
│      • Leave 5mm gap from enclosure bottom (airflow)        │
│      • Secure with M3 screws                                │
│                                                              │
│  [ ] Step 4: Mount ESP32 DevKit                            │
│      • Position: Top center (away from heat)                │
│      • Use PCB standoffs 10mm height                        │
│      • USB port accessible (for future update)              │
│                                                              │
│  [ ] Step 5: Mount MAX3485 module                          │
│      • Near ESP32 (short wiring)                            │
│      • Double-sided tape or standoff                        │
│                                                              │
│  [ ] Step 6: Install terminal blocks                        │
│      • Right side of enclosure                               │
│      • Label: AC_IN, AC_OUT (8×), RS485                    │
│      • Adequate spacing for thick wires                      │
│                                                              │
│  [ ] Step 7: Install protection components                  │
│      • MOV varistor on AC input (L-N)                       │
│      • Fuse holder with 5A fuse                             │
│      • Output capacitors (1000µF + 100nF)                   │
│                                                              │
│  [ ] Step 8: Install RC snubbers (per SSR output)          │
│      • Solder 100Ω 2W resistor + 0.1µF capacitor          │
│      • Series connection: R-C-R                             │
│      • Insulate with heat shrink                            │
│      • Mount near SSR output terminals                      │
│      • CRITICAL for motor/fan loads!                        │
│                                                              │
│  [ ] Step 9: Wire AC input protection                       │
│      • Cable gland → Fuse → MOV → PSU                      │
│      • PE (Earth) → Enclosure body                          │
│      • Tight connections (torque to spec)                   │
│                                                              │
│  [ ] Step 10: Wire DC distribution                          │
│      • PSU 5V → Capacitors → ESP32 VIN                     │
│      • PSU GND → Common star point                          │
│      • SSR module VCC ← 5V                                  │
│      • All GND to star point                                │
│                                                              │
│  [ ] Step 11: Wire control signals                          │
│      • ESP32 GPIO → SSR inputs (use pin standard!)         │
│      • MAX3485 ↔ ESP32 (RX, TX, DE/RE)                     │
│      • Keep signal wires away from AC                       │
│                                                              │
│  [ ] Step 12: Wire RS-485 bus                              │
│      • UTP from previous node → terminals                   │
│      • MAX3485 A, B, GND                                    │
│      • UTP to next node (daisy-chain)                       │
│      • NO termination (only at last node)                   │
│                                                              │
│  [ ] Step 13: Label everything                              │
│      • Cable labels at both ends                            │
│      • Terminal labels                                       │
│      • Device ID label on enclosure                         │
│                                                              │
│  [ ] Step 14: Final inspection                              │
│      • No loose strands                                      │
│      • Proper strain relief                                  │
│      • Enclosure gasket clean                               │
│      • All screws tight                                      │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

#### **4.3.2 Kipas Node Assembly (Additional Steps)**

```
ADDITIONAL STEPS FOR FAN_4CH:
┌─────────────────────────────────────────────────────────────┐
│                                                              │
│  [ ] Step A: DPDT Mode Switch Installation                  │
│      1. Prepare switch box (50x50x30mm)                     │
│      2. Drill Ø12mm hole for toggle shaft                   │
│      3. Mount DPDT switch with nut & washer                 │
│      4. Label: "AUTO" (up), "MANUAL" (down)                │
│      5. Install near existing speed switch                  │
│                                                              │
│  [ ] Step B: Wire DPDT connections                          │
│      From slave enclosure via cable gland:                  │
│        • 3-core cable (L, N, Signal)                        │
│                                                              │
│      DPDT Pin 2 (COM L) ← 220VAC Live from MCB             │
│      DPDT Pin 5 (COM N) ← 220VAC Neutral from MCB          │
│                                                              │
│      DPDT Pin 1 (AUTO L) → All SSR commons (parallel)      │
│      DPDT Pin 4 (AUTO N) → All SSR commons (parallel)      │
│                                                              │
│      DPDT Pin 3 (MANUAL L) → Speed switch input            │
│      DPDT Pin 6 (MANUAL N) → Motor neutral                 │
│                                                              │
│  [ ] Step C: Wire micro switch (mode detection)            │
│      Option A: Inside DPDT switch box                       │
│        • Micro switch mechanically coupled to DPDT          │
│        • Common ← 3.3V from slave enclosure                 │
│        • NO contact → Wire to GPIO35 (AUTO detect)         │
│        • NC contact → Wire to GND (MANUAL detect)          │
│                                                              │
│      Option B: Inside slave enclosure                       │
│        • Voltage divider from DPDT Pin 1                    │
│        • 1MΩ + optocoupler + GPIO35                         │
│        • More complex, but no mechanical coupling           │
│                                                              │
│  [ ] Step D: Wire speed switch integration                  │
│      Speed switch (existing 3-way):                         │
│        Input ← DPDT Pin 3 (MANUAL L)                        │
│        Output 1 → Motor LOW winding                         │
│        Output 2 → Motor MED winding                         │
│        Output 3 → Motor HIGH winding                        │
│                                                              │
│  [ ] Step E: Wire SSR to motor (parallel)                  │
│      SSR_LOW output (NO) ──┬── Motor LOW winding           │
│      Speed switch pos 1 ────┘                               │
│                                                              │
│      SSR_MED output (NO) ──┬── Motor MED winding           │
│      Speed switch pos 2 ────┘                               │
│                                                              │
│      SSR_HIGH output (NO) ─┬── Motor HIGH winding          │
│      Speed switch pos 3 ────┘                               │
│                                                              │
│      Motor Neutral ← DPDT Pin 4 or Pin 6 (depends on mode) │
│                                                              │
│  [ ] Step F: Test mode switching                            │
│      1. Power off all                                        │
│      2. Set DPDT to AUTO                                     │
│      3. Measure GPIO35: Should be HIGH                       │
│      4. Set DPDT to MANUAL                                   │
│      5. Measure GPIO35: Should be LOW                        │
│      6. Test speed switch in MANUAL (motor should run)      │
│      7. Set to AUTO, test SSR control                       │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

#### **4.3.3 Slave Power-On Test**

Tambahan pengujian termal (wajib untuk kanal kipas):
- Setelah uji fungsional, jalankan 1 kanal kipas pada beban normal selama 30 menit.
- Ukur suhu heatsink SSR dan PSU; target: SSR < 65°C, PSU < 60°C. Jika melebihi, upgrade SSR/peningkatan heatsink atau ubah arsitektur kanal (SSR rating lebih tinggi/kontaktor mini).

```
SLAVE POWER-ON PROCEDURE:
┌─────────────────────────────────────────────────────────────┐
│                                                              │
│  PRE-POWER CHECKS:                                           │
│  [ ] Visual inspection complete                              │
│  [ ] All wiring per diagram                                  │
│  [ ] No shorts (multimeter test)                            │
│  [ ] Correct polarity (AC & DC)                             │
│  [ ] Fuse installed (5A)                                     │
│  [ ] RC snubbers on all SSR outputs                         │
│  [ ] For FAN: Mode switch in AUTO position                  │
│                                                              │
│  INITIAL POWER ON (Slave standalone):                       │
│  1. Disconnect RS-485 bus temporarily                        │
│  2. Disconnect all loads (SSR outputs open)                 │
│  3. Apply 220VAC power                                       │
│                                                              │
│  4. Check immediate response:                                │
│     [ ] PSU LED on (green)                                   │
│     [ ] No smoke, no smell                                   │
│     [ ] No buzzing from PSU                                  │
│                                                              │
│  5. Measure DC voltages:                                     │
│     [ ] PSU output: 5.0V ± 0.25V                            │
│     [ ] ESP32 VIN: 5.0V ± 0.25V                            │
│     [ ] ESP32 3.3V: 3.3V ± 0.15V                           │
│     [ ] SSR module VCC: 5.0V ± 0.25V                       │
│                                                              │
│  6. Check ESP32 boot:                                        │
│     [ ] Connect USB (for serial monitor)                    │
│     [ ] Onboard LED blinks during boot                      │
│     [ ] Serial shows boot messages                          │
│     [ ] If ID=0: LED blinks blue (discovery mode)          │
│                                                              │
│  7. Test relay control (manual):                            │
│     Upload test firmware:                                    │
│                                                              │
│     void setup() {                                           │
│       pinMode(12, OUTPUT);  // Relay 1                      │
│       pinMode(13, OUTPUT);  // Relay 2                      │
│       // ... all relay pins                                 │
│     }                                                        │
│                                                              │
│     void loop() {                                            │
│       // Cycle through all relays                           │
│       for(int i=0; i<8; i++) {                              │
│         digitalWrite(relayPins[i], HIGH);                   │
│         delay(500);                                          │
│         digitalWrite(relayPins[i], LOW);                    │
│         delay(500);                                          │
│       }                                                      │
│     }                                                        │
│                                                              │
│  8. Observe relay operation:                                 │
│     [ ] Hear SSR clicking? (should be silent for G3MB)     │
│     [ ] SSR LED indicators light up sequentially            │
│     [ ] No false triggering                                  │
│     [ ] Smooth operation                                     │
│                                                              │
│  9. FOR FAN: Test mode detection                            │
│     [ ] DPDT in AUTO: GPIO35 reads HIGH                     │
│     [ ] DPDT in MANUAL: GPIO35 reads LOW                    │
│     [ ] LED indicator: Green (AUTO), Yellow (MANUAL)        │
│                                                              │
│  10. Temperature check after 10 minutes:                     │
│      [ ] PSU: Warm but < 60°C (touchable)                  │
│      [ ] ESP32: Warm but < 50°C                             │
│      [ ] SSR heatsink: < 45°C (idle, no load)              │
│                                                              │
│  IF ALL OK:                                                  │
│  [ ] Connect RS-485 to Master Logic                         │
│  [ ] Power on, observe auto-enrollment                      │
│  [ ] Proceed to next slave                                  │
│                                                              │
│  IF ANY ISSUE:                                               │
│  [ ] Power off immediately                                   │
│  [ ] Troubleshoot (see section 7)                           │
│  [ ] Do NOT proceed until resolved                          │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

---

### **4.4 Phase 4: RS-485 Bus Installation**

#### **4.4.1 Cable Preparation**

```
UTP CABLE PREPARATION:
┌─────────────────────────────────────────────────────────────┐
│                                                              │
│  STEP 1: Measure & Cut                                      │
│    • Measure actual distance between nodes                  │
│    • Add 20% slack (coil excess neatly)                     │
│    • Cut cable squarely with sharp cutter                   │
│                                                              │
│  STEP 2: Strip outer jacket                                 │
│    • Strip 30mm from each end                               │
│    • Be careful not to nick inner conductors                │
│    • Remove jacket cleanly                                  │
│                                                              │
│  STEP 3: Identify pairs (T568B standard)                   │
│    Pair 1: Blue-White / Blue                                │
│    Pair 2: Green-White / Green                              │
│    Pair 3: Orange-White / Orange                            │
│    Pair 4: Brown-White / Brown                              │
│                                                              │
│  STEP 4: Select conductors for RS-485                      │
│    A+ (Data+):   Blue-White (Pin 5)                         │
│    B- (Data-):   Green-White (Pin 2)                        │
│    GND (Common): Brown-White + Brown (Pins 7+8, parallel)  │
│    Not used:     Blue, Green, Orange pair                   │
│                                                              │
│  STEP 5: Crimp RJ45 connector (optional for testing)       │
│    OR: Direct termination to screw terminals                │
│                                                              │
│  DIRECT TERMINATION (Recommended for permanent install):    │
│    1. Untwist each pair minimum (< 13mm)                    │
│    2. Strip 7mm from selected conductors                    │
│    3. Twist strands tightly (or tin with solder)           │
│    4. Insert into terminal block                            │
│    5. Tighten screw firmly (0.5 Nm torque)                 │
│    6. Tug test (should not pull out)                        │
│                                                              │
│  STEP 6: Label cable                                        │
│    At each end, use cable marker:                           │
│      "RS485 | FROM: ML-001 | TO: SL-001"                   │
│      Or similar identification                              │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

#### **4.4.2 Bus Topology & Routing**

```
LINEAR DAISY-CHAIN TOPOLOGY:
┌─────────────────────────────────────────────────────────────┐
│                                                              │
│  CORRECT (Linear):                                           │
│                                                              │
│    [Master] ──→ [Slave1] ──→ [Slave2] ──→ ... ──→ [SlaveN] │
│      ↑                                                  ↑    │
│    [120Ω]                                            [120Ω] │
│                                                              │
│  ✓ Advantages:                                               │
│    • Predictable impedance                                  │
│    • Easy troubleshooting                                   │
│    • Optimal signal quality                                 │
│                                                              │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  INCORRECT (Star/Branch):                                    │
│                                                              │
│           ┌─→ [Slave1]                                      │
│    [Master]─→ [Slave2]                                      │
│           └─→ [Slave3]                                      │
│                                                              │
│  ✗ Problems:                                                 │
│    • Reflections (signal integrity issues)                  │
│    • Difficult termination                                  │
│    • Unpredictable errors                                   │
│                                                              │
└─────────────────────────────────────────────────────────────┘

ROUTING BEST PRACTICES:
┌─────────────────────────────────────────────────────────────┐
│                                                              │
│  DO:                                                         │
│  ✓ Use cable trays or conduit                               │
│  ✓ Maintain min 30cm separation from AC power              │
│  ✓ Cross AC cables at 90° angle (if unavoidable)           │
│  ✓ Support cable every 50cm                                 │
│  ✓ Use gentle bends (min radius: 4× cable diameter)        │
│  ✓ Label cable at both ends and every junction             │
│  ✓ Leave service loop at each node                         │
│  ✓ Document actual routing path (as-built)                 │
│                                                              │
│  DON'T:                                                      │
│  ✗ Run parallel to AC power for long distances             │
│  ✗ Make sharp bends (damages conductors)                   │
│  ✗ Use excessive force pulling cable                       │
│  ✗ Staple or nail through cable                            │
│  ✗ Expose to direct sunlight (outdoor)                     │
│  ✗ Route through areas with heavy EMI                      │
│  ✗ Mix with antenna cables                                 │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

#### **4.4.3 Termination & Testing**

```
TERMINATION PROCEDURE:
┌─────────────────────────────────────────────────────────────┐
│                                                              │
│  FIRST NODE (Master Logic):                                 │
│    1. Install 120Ω resistor between A-B terminals          │
│    2. Use 1/4W metal film resistor                          │
│    3. Solder or secure with terminal block                  │
│    4. Measure resistance: Should be 120Ω ± 5%              │
│                                                              │
│  MIDDLE NODES (All Slaves except last):                     │
│    • NO termination resistor                                │
│    • Straight daisy-chain connection only                   │
│    • A to A, B to B, GND to GND                            │
│                                                              │
│  LAST NODE (Slave 18):                                      │
│    1. Install 120Ω resistor between A-B terminals          │
│    2. Same as first node                                    │
│    3. Verify resistance                                     │
│                                                              │
│  VERIFICATION:                                               │
│    Measure total bus impedance (power off all):             │
│      Between Master A-B: Should be ~60Ω                     │
│      (Two 120Ω in parallel = 60Ω)                          │
│                                                              │
└─────────────────────────────────────────────────────────────┘

CONTINUITY TESTING:
┌─────────────────────────────────────────────────────────────┐
│                                                              │
│  Use multimeter in continuity mode:                         │
│                                                              │
│  TEST 1: A-line continuity                                  │
│    Master A+ ↔ Slave1 A+ ↔ ... ↔ Slave18 A+               │
│    Should have continuity (<1Ω)                             │
│                                                              │
│  TEST 2: B-line continuity                                  │
│    Master B- ↔ Slave1 B- ↔ ... ↔ Slave18 B-               │
│    Should have continuity (<1Ω)                             │
│                                                              │
│  TEST 3: GND continuity                                     │
│    Master GND ↔ All slave GNDs                              │
│    Should have continuity (<2Ω)                             │
│                                                              │
│  TEST 4: Isolation check                                    │
│    A-B: Should be 60Ω (terminators in parallel)            │
│    A-GND: Should be open (>1MΩ)                            │
│    B-GND: Should be open (>1MΩ)                            │
│                                                              │
│  TEST 5: No shorts                                          │
│    Check no short between:                                  │
│      • A-B (except through terminators)                     │
│      • A-GND                                                │
│      • B-GND                                                │
│                                                              │
└─────────────────────────────────────────────────────────────┘

SIGNAL QUALITY TEST (with Oscilloscope):
┌─────────────────────────────────────────────────────────────┐
│                                                              │
│  If available, use oscilloscope to check:                   │
│                                                              │
│  SETUP:                                                      │
│    • CH1: Probe A line (differential)                       │
│    • CH2: Probe B line                                      │
│    • Trigger on data edges                                  │
│    • Ground probe to bus GND                                │
│                                                              │
│  EXPECTED WAVEFORM:                                          │
│    • Differential voltage: 2-5V peak-to-peak               │
│    • Clean transitions (no ringing)                         │
│    • Symmetric rise/fall times                              │
│    • No glitches or noise spikes                            │
│                                                              │
│  IF ISSUES OBSERVED:                                         │
│    • Ringing: Check termination (missing or wrong value)   │
│    • Noise: Check grounding, separation from AC             │
│    • Slow edges: Cable too long or wrong type              │
│    • Asymmetry: Polarity reversed (swap A-B)               │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

---

## ⚡ BAGIAN 5: POWER CALCULATION & PROTECTION

### **5.1 Detailed Power Budget**

```
COMPLETE SYSTEM POWER ANALYSIS:
┌─────────────────────────────────────────────────────────────┐
│                                                              │
│  5VDC CONTROL CIRCUIT:                                      │
│  ────────────────────────────────────────────────────────  │
│  Master Logic:                                              │
│    ESP32-S3:      120mA (avg), 240mA (peak WiFi TX)       │
│    MAX3485:        10mA (idle), 15mA (TX)                  │
│    ILI9488 4":    180mA (avg), 250mA (peak backlight)     │
│    Subtotal:      310mA (avg), 505mA (peak)               │
│                                                              │
│  Display Master:                                             │
│    Waveshare 7":  800mA (avg), 1200mA (peak)              │
│                                                              │
│  Per Kipas Node:                                            │
│    ESP32:         120mA                                     │
│    MAX3485:        10mA                                     │
│    4× SSR ctrl:    80mA (20mA each @ 5V)                   │
│    Subtotal:      210mA per node                            │
│    Total 13:    2,730mA                                     │
│                                                              │
│  Per Lampu/Sound Node:                                      │
│    ESP32:         120mA                                     │
│    MAX3485:        10mA                                     │
│    8× SSR ctrl:   160mA (20mA each @ 5V)                   │
│    Subtotal:      290mA per node                            │
│    Total 5:     1,450mA                                     │
│                                                              │
│  ═══════════════════════════════════════════════════════   │
│  TOTAL 5VDC:    5,290mA = 5.3A                             │
│  With 25% margin: 6.6A                                      │
│  Recommended PSU: 5V/8A (centralized)                       │
│                OR: Individual HLK-PM03 per node             │
│  ═══════════════════════════════════════════════════════   │
│                                                              │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  220VAC LOAD CIRCUIT (SSR outputs):                         │
│  ────────────────────────────────────────────────────────  │
│  Assumptions:                                                │
│    • Kipas: 60W per fan (typical ceiling fan)              │
│    • Lampu: 15W per lamp (LED)                              │
│    • Sound: 100W amplifier                                  │
│                                                              │
│  Kipas load:    13 × 60W  = 780W                           │
│  Lampu load:    32 × 15W  = 480W (4 nodes × 8 CH)         │
│  Sound load:     1 × 100W = 100W                            │
│  ═══════════════════════════════════════════════════════   │
│  TOTAL AC LOAD: 1,360W @ 220VAC                            │
│  Current:       1360W / 220V = 6.2A                         │
│  MCB sizing:    10A (C-curve) recommended                   │
│  ═══════════════════════════════════════════════════════   │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

### **5.2 Voltage Drop Calculation**

```
VOLTAGE DROP FOR DC DISTRIBUTION:
┌─────────────────────────────────────────────────────────────┐
│                                                              │
│  Formula: Vdrop = 2 × L × I × R / 1000                     │
│                                                              │
│  Where:                                                      │
│    L = Cable length (meters, one way)                       │
│    I = Current (Amperes)                                    │
│    R = Resistance (Ω/km for given wire size)               │
│                                                              │
│  Example 1: HLK-PM03 to ESP32 (inside enclosure)           │
│    L = 0.3m, I = 0.3A, Wire = 0.5mm² (R = 36 Ω/km)        │
│    Vdrop = 2 × 0.3 × 0.3 × 36 / 1000 = 0.0065V            │
│    Result: 5.00V - 0.01V = 4.99V ✓ (negligible)           │
│                                                              │
│  Example 2: Centralized PSU to distant slave               │
│    L = 30m, I = 0.5A, Wire = 1.5mm² (R = 12.1 Ω/km)      │
│    Vdrop = 2 × 30 × 0.5 × 12.1 / 1000 = 0.363V            │
│    Result: 5.00V - 0.36V = 4.64V ✓ (acceptable)           │
│                                                              │
│  Example 3: Too much drop (bad design)                     │
│    L = 50m, I = 1A, Wire = 0.75mm² (R = 24.5 Ω/km)       │
│    Vdrop = 2 × 50 × 1 × 24.5 / 1000 = 2.45V               │
│    Result: 5.00V - 2.45V = 2.55V ✗ (ESP32 won't boot!)    │
│                                                              │
│  SOLUTION for long distance:                                │
│    Option A: Use thicker wire (2.5mm² → R = 7.41 Ω/km)    │
│    Option B: Local PSU at each slave (recommended)         │
│                                                              │
└─────────────────────────────────────────────────────────────┘

WIRE SIZING TABLE:
┌─────────────────────────────────────────────────────────────┐
│ Wire Size │ Resistance │ Max Current │ Application         │
├─────────────────────────────────────────────────────────────┤
│ 0.5mm²    │ 36.0 Ω/km  │ 3A          │ Short jumpers (<1m) │
│ 0.75mm²   │ 24.5 Ω/km  │ 6A          │ Internal wiring     │
│ 1.0mm²    │ 18.1 Ω/km  │ 10A         │ Short runs (<10m)   │
│ 1.5mm²    │ 12.1 Ω/km  │ 15A         │ Medium runs (<30m)  │
│ 2.5mm²    │  7.41 Ω/km │ 21A         │ Long runs (>30m)    │
└─────────────────────────────────────────────────────────────┘
```

### **5.3 Protection Scheme**

```
MULTI-LAYER PROTECTION SYSTEM:
┌─────────────────────────────────────────────────────────────┐
│                                                              │
│  LAYER 1: AC Input Protection (Per Node)                    │
│  ───────────────────────────────────────────────────────   │
│    Component: MOV Varistor 275VAC                           │
│    Function:  Clamp voltage spikes (lightning, switching)  │
│    Placement: Between L-N at AC input                       │
│    Rating:    Peak surge 3500A, energy 200J                │
│                                                              │
│    Component: Fuse 5A fast-blow                             │
│    Function:  Overcurrent protection                        │
│    Placement: In series with AC Live                        │
│    Action:    Blow in <1 second @ 10A                       │
│                                                              │
│  LAYER 2: SSR Output Protection (Per Channel)              │
│  ───────────────────────────────────────────────────────   │
│    Component: RC Snubber (100Ω 2W + 0.1µF 630VAC)         │
│    Function:  Suppress voltage spike from inductive loads  │
│    Placement: Parallel with SSR output (between NO-Load)   │
│    Critical:  MANDATORY for motor/fan loads                │
│                                                              │
│    Wiring:                                                   │
│      SSR_NO ──┬── [100Ω] ──┬── Load                        │
│               │             │                                │
│               └── [0.1µF] ──┘                               │
│                                                              │
│  LAYER 3: DC Output Protection                             │
│  ───────────────────────────────────────────────────────   │
│    Component: 1000µF electrolytic + 100nF ceramic          │
│    Function:  Filter PSU ripple, decouple noise            │
│    Placement: Parallel with PSU output, near loads         │
│                                                              │
│  LAYER 4: Logic Protection                                  │
│  ───────────────────────────────────────────────────────   │
│    Component: 220Ω series resistor on GPIO inputs          │
│    Function:  Current limiting for GPIO protection         │
│    Placement: Between external signal and ESP32 GPIO       │
│                                                              │
│  LAYER 5: Earth/Ground Protection                          │
│  ───────────────────────────────────────────────────────   │
│    • All enclosures bonded to PE (protective earth)        │
│    • Star point grounding (single point reference)         │
│    • Ground continuity < 0.5Ω                               │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

---

**DOKUMEN 3 AKAN DILANJUTKAN...**

## 🛠️ BAGIAN 7: TROUBLESHOOTING GUIDE

### 7.1 Metode Diagnostik Cepat (First 5 Minutes)

```
ALUR CEPAT DIAGNOSA:
┌─────────────────────────────────────────────────────────────┐
│  1) Identifikasi gejala utama                               │
│     • Tidak bisa kontrol? • Perangkat offline? • Beban mati?│
│  2) Tentukan domain                                         │
│     • Power • Komunikasi RS-485 • Logic/firmware • Wiring    │
│  3) Lakukan uji minimum                                     │
│     • Ukur tegangan • Cek terminasi • Baca LED status       │
│  4) Isolasi masalah                                         │
│     • Uji single node • Putus bus sementara • Swap komponen │
│  5) Dokumentasikan temuan                                   │
│     • Foto • Catat nilai ukur • Kondisi sebelum/sesudah     │
└─────────────────────────────────────────────────────────────┘
```

LED STATUS KONVENSI (semua node):
- Hijau steady: Online & AUTO
- Kuning steady: MANUAL (kipas)
- Merah steady: Fail-safe aktif / fault kritikal
- Biru berkedip cepat: Discovery mode (ID=0)
- Biru berkedip lambat: Waiting bus/timeout

### 7.2 Tabel Gejala → Penyebab → Solusi

```
┌───────────────────────────────────────────────────────────────────────────────────────────────┐
│ Gejala                                      │ Penyebab Mungkin                 │ Solusi         │
├──────────────────────────────────────────────┼──────────────────────────────────┼──��─────────────┤
│ Semua perangkat OFFLINE di dashboard        │ Master mati / RS-485 putus       │ Cek daya Master│
│                                              │ Terminasi hilang                 │ Pastikan 120Ω  │
│                                              │ Polaritas A/B terbalik           │ Tukar A/B      │
├──────────────────────────────────────────────┼──────────────────────────────────┼────────────────┤
│ Hanya satu slave OFFLINE                     │ Kabel putus di titik tsb          │ Uji continuity │
│                                              │ MAX3485 rusak di slave            │ Ganti modul    │
│                                              │ ID bentrok (duplikat)             │ Reset & enroll │
├──────────────────────────────────────────────┼──────────────────────────────────┼────────────────┤
│ Kipas tidak merespons di AUTO                │ DPDT di MANUAL (GPIO35 LOW)       │ Set ke AUTO    │
│                                              │ SSR kanal rusak                   │ Pindah ke kanal│
│                                              │ RC snubber short                   │ Periksa/replace│
├──────────────────────────────────────────────┼──────────────────────────────────┼────────────────┤
│ Kipas bergetar/berdengung di LOW             │ Beban induktif tanpa snubber      │ Pasang RC      │
│                                              │ Netral tidak solid                │ Periksa netral │
├──────────────────────────────────────────────┼──────────────────────────────────┼────────────────┤
│ Lampu vilting/berkedip random                │ Noise di 5V / GND loop            │ Tambah decouple│
│                                              │ SSR bocor (leak)                   │ Ganti SSR      │
├──���───────────────────────────────────────────┼──────────────────────────────────┼────────────────┤
│ Tidak bisa kirim perintah (timeout)          │ DE/RE tidak toggle                │ Cek GPIO4      │
│                                              │ Baud mismatch                      │ Samakan 115200 │
│                                              │ Termination salah                  │ Cek 60Ω total │
├──────────────────────────────────────────────┼──────────────────────────────────┼────────────────┤
│ Display Master tidak terhubung               │ RS-485 longgar                     │ Kencangkan     │
│                                              │ Firmware tidak jalan               │ Reflash        │
│                                              │ Power supply drop                  │ Ganti PSU      │
├──────────────────────────────────────��───────┼──────────────────────────────────┼────────────────┤
│ PSU panas / drop tegangan                    │ Beban melebihi rating              │ Naikkan rating │
│                                              │ Ventilasi buruk                    │ Tambah airflow │
├──────────────────────────────────────────────┼──────────────────────────────────┼────────────────┤
│ MOV sering jebol                             │ Surge berat / kualitas listrik     │ Tambah SPD     │
│                                              │ MOV rating rendah                  │ Pakai kelas lebih│
└───────────────────────────────────────────────────────────────────────────────────────────────┘
```

### 7.3 Prosedur Isolasi Masalah RS-485

- Matikan semua node, nyalakan Master saja → verifikasi TX.
- Nyalakan satu slave terdekat → uji komunikasi dasar.
- Tambahkan node satu-per-satu hingga error muncul → lokasi kesalahan = node terakhir ditambahkan atau segmen kabel antar keduanya.
- Ukur impedansi bus di Master: ~60Ω. Jika jauh berbeda, periksa terminator.
- Swap A/B di satu sisi jika tidak ada komunikasi sama sekali.
- Cek GND bersama; tanpa reference, komunikasi bisa tidak stabil.

### 7.4 Reset & Recovery

- Reset Slave ke Factory (ID=0): Tekan tombol BOOT 5 detik saat power on → LED biru berkedip cepat → siap enrollment.
- Clear NVS Master: Menu Maintenance → Storage → Clear Registry (pastikan backup dulu).
- Re-enrollment massal: Putus bus jadi segmen kecil 3-5 perangkat, enroll per segmen, lalu gabungkan kembali.

### 7.5 Suku Cadang & Swap Cepat

- SIMPAN spare: MAX3485, 1× modul SSR 8CH, 1× HLK-PM03, 1× ESP32 DevKit, resistor 120Ω, RC snubber kit, MOV 275VAC, sekrup & gland.
- Metode swap: Lepas-komponen-sesuai-label, ganti, test lokal, lalu test bus.

---

## 🧰 BAGIAN 8: MAINTENANCE SCHEDULE

### 8.1 Jadwal Perawatan Berkala

```
RINGKASAN INTERVAL:
┌─────────────────────────────────────────────────────────────┐
│ Harian   : Pengecekan visual cepat (dashboard status)       │
│ Mingguan : Uji fungsi acak 10% perangkat                     │
│ Bulanan  : Bersihkan debu, cek suhu operasional              │
│ Triwulan : Kencangkan terminal, uji fail-safe, cek log       │
│ Tahunan  : Ganti MOV preventif, uji isolasi, audit wiring    │
└─────────────────────────────────────────────────────────────┘
```

### 8.2 Checklist Detail per Interval

- Harian:
  - Periksa dashboard: semua device ONLINE, heartbeat normal.
  - Catat anomali (latency, perangkat sering offline sesaat).

- Mingguan:
  - Pilih 2 kipas dan 2 lampu secara acak → uji LOW/MED/HIGH/ON/OFF.
  - Untuk kipas, flip DPDT ke MANUAL lalu kembali ke AUTO, pastikan deteksi.
  - Periksa suhu PSU dan modul SSR dengan infrared thermometer.

- Bulanan:
  - Buka cover, hembuskan debu (angin kering), bersihkan filter/vent.
  - Cek kekencangan sekrup terminal (torque ringan). Jangan over-torque.
  - Verifikasi nilai tegangan 5V pada titik terjauh: 4.8–5.2V.

- Triwulan:
  - Uji fail-safe dengan mematikan Master selama 5 menit.
  - Tarik log error dari Master & Display, analisis pola.
  - Verifikasi terminasi fisik (resistor masih terpasang di ujung-ujung).

- Tahunan:
  - Ganti MOV di node yang sering kena surge (preventif).
  - Uji insulation L-N, L-PE, N-PE pada sampel per panel.
  - Audit rute kabel vs instalasi baru (pastikan tidak jadi star/branch).

### 8.3 Suku Cadang dan Umur Pakai

- MOV 275VAC: ganti 1–2 tahun atau pasca kejadian petir.
- HLK-PM03: umur tipikal 5–7 tahun; monitor ripple dan suhu.
- Modul SSR G3MB: tergantung beban; untuk motor, cek panas berkala; siap spare.
- Konektor terminal: jika sering buka-tutup, per 3 tahun ganti.

### 8.4 Prosedur Backup & Restore

- Backup konfigurasi Master & Display via menu Maintenance → Export (JSON) setiap triwulan.
- Simpan di 3 lokasi: laptop teknisi, cloud pengurus, flashdisk cadangan.
- Restore: Import file → Reboot perangkat → Verifikasi device map.

---

## 🛡️ BAGIAN 9: SAFETY & EMERGENCY PROCEDURES

### 9.1 Aturan Umum Keselamatan

- Selalu matikan MCB terkait sebelum bekerja pada kabel/beban AC.
- Gunakan APD: sarung tangan isolasi, kacamata, sepatu safety.
- Ikuti PUIL & standar lokal. Jangan modifikasi panel tanpa izin.
- Pastikan semua enclosure terhubung ke PE (earth) dengan baik.

### 9.2 Penanganan Keadaan Darurat

- Korsleting/Asap dari enclosure:
  - Matikan MCB utama area tersebut segera.
  - Jangan sentuh perangkat panas; tunggu dingin, lalu inspeksi.
  - Periksa kerusakan: PSU, MOV, kabel meleleh, SSR short.
  - Ganti komponen rusak, lakukan uji isolasi sebelum menghidupkan.

- Petir/Surge besar (banyak perangkat reset):
  - Periksa MOV visual (menghitam/retak → ganti).
  - Ukur tegangan 5V dan 3.3V; pastikan dalam rentang.
  - Verifikasi RS-485: impedansi 60Ω, kontinuitas OK.

- Orang tersengat listrik:
  - Putus sumber listrik, jangan sentuh korban langsung.
  - Ikuti prosedur P3K setempat, hubungi layanan darurat.

### 9.3 Lockout/Tagout (LOTO) Sederhana

- Saat pekerjaan besar, pasang tag “JANGAN DIHIDUPKAN” di MCB terkait.
- Kunci boks MCB jika tersedia. Catat waktu dan penanggung jawab.

### 9.4 Bekerja di Ketinggian (Kipas Plafon)

- Gunakan tangga stabil; ada pendamping yang memegangi.
- Matikan beban, pastikan motor benar-benar berhenti sebelum bekerja.
- Pastikan penempatan enclosure tidak menghalangi akses evakuasi.

---

## 📑 BAGIAN 10: DOKUMENTASI & LAMPIRAN

### 10.1 Template As-Built Documentation

```
AS-BUILT FORM (per perangkat):
- Device ID/Name   : SL-0XX / [Nama Deskriptif]
- Lokasi           : [Ruang/Saf/Koordinat]
- Tipe Profil      : FAN_4CH / LIGHT_8CH / SOUND_8CH
- Sumber Daya      : HLK-PM03 / PSU terpusat
- RS-485           : A=Blue-White, B=Green-White, GND=Brown pair
- Terminasi        : Ya / Tidak (120Ω)
- Mode Switch      : Ya / Tidak (untuk kipas)
- Firmware Version : vX.Y.Z
- Tanggal Pasang   : DD-MM-YYYY
- Teknisi          : [Nama]
- Catatan          : [Observasi khusus]
```

### 10.2 Tabel Mapping ID dan Lokasi (Contoh)

```
ID  Nama                          Lokasi            Profil
01  Kipas Saf 1 Depan             Saf 1             FAN_4CH
02  Kipas Saf 2 Depan             Saf 2             FAN_4CH
...
14  Lampu Utama Kanan             Panel Lampu Kanan LIGHT_8CH
...
18  Audio Mimbar                  Ruang Audio       SOUND_8CH
```

### 10.3 Checklist Serah Terima (Commissioning Report)

- Semua bagian 6 lulus: Test 1–7 PASS.
- Backup konfigurasi diekspor dan diserahkan (USB + cloud).
- Peta kabel dan foto instalasi diserahkan.
- Buku panduan ini dicetak + softcopy.

### 10.4 Spesifikasi Teknis Ringkas (Rujukan Cepat)

- Layer Fisik RS-485: 115200 8N1, bus linear, terminasi 120Ω di ujung.
- Pinout standar ESP32 untuk slave: RX=16, TX=17, DE/RE=4, Relay=12/13/14/15/25/26/27/32, Mode Detect=35.
- Proteksi wajib: MOV L–N, Fuse 5A, RC snubber per kanal induktif, decoupling 1000µF+100nF.

### 10.5 Catatan Perubahan Dokumen (Changelog)

- v1.1 (27-12-2025): Tambah Bagian 6–10, perbaikan diagram RS-485, klarifikasi DPDT.
- v1.0 (25-12-2025): Rilis awal Bagian 1–5.

---

Dokumen 3: Panduan Integrasi Hardware v1.1 selesai. Simpan dan distribusikan kepada tim instalasi dan maintenance. Pastikan semua teknisi membaca Bagian 7–9 sebelum bekerja di lapangan.

Dokumen ini sudah mencapai panjang signifikan. Saya akan melanjutkan dengan bagian-bagian berikutnya:
- Bagian 6: Commissioning & Testing
- Bagian 7: Troubleshooting Guide
- Bagian 8: Maintenance Schedule
- Bagian 9: Safety Procedures
- Bagian 10: Documentation & Appendix

Apakah saya lanjutkan sekarang, atau Anda ingin review dulu bagian 1-5?
```

---

## 🔍 BAGIAN 6: COMMISSIONING & TESTING PROCEDURE

### **6.1 Pre-Commissioning Checklist**

```
CRITICAL PRE-CHECKS (DO NOT SKIP):
┌─────────────────────────────────────────────────────────────┐
│                                                              │
│  PHASE 1: VISUAL INSPECTION                                 │
│  ───────────────────────────────────────────────────────   │
│  [ ] All enclosures properly mounted & leveled              │
│  [ ] No exposed conductors or bare wires                    │
│  [ ] All cable labels present and readable                  │
│  [ ] Cable management neat and secure                       │
│  [ ] Ground connections tight (torque check)                │
│  [ ] Fuses installed with correct rating                    │
│  [ ] MOV varistors installed at AC inputs                   │
│  [ ] RC snubbers on all SSR outputs (fan/motor loads)      │
│  [ ] Termination resistors: 120Ω at first & last node     │
│  [ ] All cable glands tight (no water ingress)             │
│  [ ] Ventilation holes clear (not blocked)                 │
│  [ ] For KIPAS: Mode switch accessible & labeled           │
│                                                              │
│  PHASE 2: ELECTRICAL SAFETY TEST                           │
│  ───────────────────────────────────────────────────────   │
│  [ ] Insulation resistance test:                            │
│      • AC L to Ground: > 100MΩ                              │
│      • AC N to Ground: > 100MΩ                              │
│      • AC L to N: > 100MΩ (power off)                      │
│                                                              │
│  [ ] Earth continuity test:                                 │
│      • All enclosures to main earth: < 0.5Ω               │
│      • PE conductor continuous                              │
│                                                              │
│  [ ] Polarity check (multimeter):                           │
│      • L to N: ~220VAC                                      │
│      • L to E: ~220VAC                                      │
│      • N to E: < 5VAC                                       │
│                                                              │
│  [ ] AC voltage within range:                               │
│      • 210-230VAC nominal (Indonesia standard)             │
│      • Stable (no significant fluctuation)                  │
│                                                              │
│  [ ] DC voltage check (after PSU power-on):                │
│      • All PSU outputs: 5.0V ± 0.25V                       │
│                                                              │
│  PHASE 3: COMMUNICATION TEST                                │
│  ───────────────────────────────────────────────────────   │
│  [ ] RS-485 continuity (power off):                         │
│      • A-A through all nodes: < 2Ω                          │
│      • B-B through all nodes: < 2Ω                          │
│      • GND-GND through all nodes: < 3Ω                      │
│                                                              │
│  [ ] RS-485 isolation check:                                │
│      • A to GND: > 1MΩ (open circuit)                      │
│      • B to GND: > 1MΩ (open circuit)                      │
│                                                              │
│  [ ] Termination check:                                     │
│      • Measure A-B at Master: ~60Ω                          │
│        (Two 120Ω terminators in parallel)                   │
│                                                              │
│  [ ] Ground loop check (power on):                          │
│      • Measure AC voltage between node grounds: < 1VAC     │
│      • If > 1VAC: Ground loop present, fix grounding       │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

### **6.2 Step-by-Step Commissioning**

#### **6.2.1 Master Logic Power-Up**

```
MASTER LOGIC COMMISSIONING:
┌─────────────────────────────────────────────────────────────┐
│                                                              │
│  STEP 1: Initial Power-On                                   │
│    1. Ensure all slaves disconnected from bus               │
│    2. Apply 220VAC to Master Logic only                     │
│    3. Observe boot sequence:                                │
│       [ ] PSU LED on (green)                                │
│       [ ] ESP32 onboard LED blinks (boot)                   │
│       [ ] ILI9488 display shows splash screen               │
│       [ ] Serial monitor shows boot messages                │
│                                                              │
│  STEP 2: Verify NVS & Database                             │
│    Serial output should show:                               │
│      "Loading device registry from NVS..."                  │
│      "0 devices found" (if first boot)                      │
│      OR "X devices loaded" (if reboot)                      │
│                                                              │
│  STEP 3: Test Status Display                               │
│    ILI9488 should show:                                     │
│      • System name: "Smart Mosque"                          │
│      • Status: "Master Logic Ready"                         │
│      • Device count: 0                                       │
│      • RS-485: "Listening..."                               │
│                                                              │
│  STEP 4: Test RS-485 Transmit                              │
│    Upload test firmware to send test pattern:               │
│                                                              │
│    void loop() {                                            │
│      static int count = 0;                                  │
│      digitalWrite(4, HIGH);  // Enable TX                   │
│      Serial2.printf("TEST %d\n", count++);                 │
│      digitalWrite(4, LOW);   // Enable RX                   │
│      delay(1000);                                           │
│    }                                                        │
│                                                              │
│    Use oscilloscope or second device to verify TX works.   │
│                                                              │
│  STEP 5: Start Heartbeat                                   │
│    Load production firmware:                                │
│      • Should start broadcasting heartbeat every 60s        │
│      • Check with logic analyzer or serial monitor          │
│      • Frame: 7E 00 04 01 AA [CRC] 0A                      │
│                                                              │
│  EXPECTED STATE:                                             │
│    ✓ Master Logic ready for slave enrollment               │
│    ✓ Heartbeat broadcasting                                 │
│    ✓ Database empty (0 devices)                            │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

#### **6.2.2 Display Master Power-Up**

```
DISPLAY MASTER COMMISSIONING:
┌─────────────────────────────────────────────────────────────┐
│                                                              │
│  STEP 1: Initial Power-On                                   │
│    1. Power up Waveshare display                            │
│    2. Observe boot sequence:                                │
│       [ ] Backlight on                                       │
│       [ ] Touch screen responsive                            │
│       [ ] LVGL UI loads                                      │
│       [ ] WiFi connects (if configured)                     │
│                                                              │
│  STEP 2: Test Touch Calibration                            │
│    • Touch each corner of screen                            │
│    • Verify touch accuracy                                  │
│    • Re-calibrate if needed (LVGL settings)                │
│                                                              │
│  STEP 3: Connect to Master Logic                           │
│    Option A: Via RS-485 (built-in)                         │
│      • Connect UTP to Master Logic RS-485 terminals         │
│      • Display should show "Connected to Master"            │
│                                                              │
│    Option B: Via Serial UART (dedicated)                    │
│      • GPIO33/32 connection                                 │
│      • Faster data sync                                     │
│      • Optional but recommended                             │
│                                                              │
│  STEP 4: Test Web Dashboard (WiFi)                         │
│    1. Connect to WiFi network                               │
│    2. Note IP address shown on display                      │
│    3. Open browser: http://[IP_ADDRESS]                     │
│    4. Should see dashboard interface                        │
│    5. Test WebSocket real-time updates                      │
│                                                              │
│  STEP 5: Verify Communication                              │
│    Display should show:                                     │
│      • Master Logic: Online                                 │
│      • Heartbeat received: [timestamp]                      │
│      • Device count: 0                                       │
│      • Pending enrollments: None                            │
│                                                              │
│  EXPECTED STATE:                                             │
│    ✓ Display Master online                                  │
│    ✓ Connected to Master Logic                              │
│    ✓ Web dashboard accessible                               │
│    ✓ Ready for device enrollment                            │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

#### **6.2.3 Slave Enrollment (One-by-One)**

```
SLAVE ENROLLMENT PROCEDURE:
┌─────────────────────────────────────────────────────────────┐
│                                                              │
│  FOR EACH SLAVE (Start with Slave 1):                      │
│                                                              │
│  STEP 1: Pre-Enrollment Check                              │
│    [ ] Slave enclosure properly assembled                   │
│    [ ] All wiring verified per diagram                      │
│    [ ] Firmware flashed (ID should be 0)                    │
│    [ ] For KIPAS: Mode switch in AUTO position             │
│                                                              │
│  STEP 2: Connect to RS-485 Bus                             │
│    • Connect UTP from previous node (or Master)             │
│    • Connect UTP to next node (daisy-chain)                 │
│    • Do NOT install terminator (unless this is last node)  │
│                                                              │
│  STEP 3: Power On Slave                                    │
│    1. Apply 220VAC power                                    │
│    2. Observe LED indicator:                                │
│       • Should blink BLUE (discovery mode)                  │
│       • Blink rate: ~500ms (2 Hz)                           │
│                                                              │
│  STEP 4: Monitor Master Logic Serial                       │
│    Within 3 seconds, should see:                            │
│      "✓ DISCOVERY_ANNOUNCE received"                       │
│      "  MAC: XX:XX:XX:XX:XX:XX"                            │
│      "  Profile: FAN_4CH" (or LIGHT/SOUND)                 │
│      "  Added to pending devices"                           │
│                                                              │
│  STEP 5: Display Master Shows Popup                        │
│    Touchscreen shows:                                       │
│      ┌─────────────────────────────────────┐              │
│      │  🆕 New Device Detected              │              │
│      │                                       │              │
│      │  MAC: XX:XX:XX:XX:XX:XX              │              │
│      │  Type: KIPAS (4-Channel)             │              │
│      │                                       │              │
│      │  Assign Name:                         │              │
│      │  [Kipas Saf 1 Depan___________]     │              │
│      │                                       │              │
│      │     [Cancel]        [Confirm]        │              │
│      └─────────────────────────────────────┘              │
│                                                              │
│  STEP 6: User Assigns Name                                 │
│    1. Touch name field (keyboard appears)                   │
│    2. Enter descriptive name (max 31 characters)           │
│    3. Touch "Confirm"                                       │
│                                                              │
│  STEP 7: Master Sends DISCOVERY_RESPONSE                   │
│    Serial output:                                           │
│      "→ Sending DISCOVERY_RESPONSE"                        │
│      "  Assigned ID: 1"                                     │
│      "  Name: Kipas Saf 1 Depan"                           │
│                                                              │
│  STEP 8: Slave Receives & Restarts                         │
│    Slave serial output:                                     │
│      "✓ Assigned ID 1, name: Kipas Saf 1 Depan"          │
│      "  → ID saved to NVS"                                 │
│      "  → Name saved to NVS"                               │
│      "  → Restarting in 2 seconds..."                      │
│                                                              │
│    LED changes: BLUE blinking → OFF → GREEN steady         │
│                                                              │
│  STEP 9: Slave Boots with New ID                           │
│    After restart:                                           │
│      "Booting as ID 1: Kipas Saf 1 Depan"                 │
│      "Profile: FAN_4CH"                                     │
│      "Mode: AUTO"                                            │
│      "→ Sending STATUS_REPORT to Master"                   │
│                                                              │
│  STEP 10: Verify on Dashboard                              │
│    Display Master shows:                                    │
│      Device List:                                            │
│      ┌─────────────────────────────────────┐              │
│      │ 🌀 Kipas Saf 1 Depan        [ID: 1]│              │
│      │    Status: ● ONLINE                 │              │
│      │    Mode: 🟢 AUTO                    │              │
│      │    Speed: OFF                       │              │
│      │    [OFF] [LOW] [MED] [HIGH]        │              │
│      └─────────────────────────────────────┘              │
│                                                              │
│  STEP 11: Test Basic Control                               │
│    1. Touch "LOW" button on dashboard                      │
│    2. Observe:                                              │
│       [ ] Dashboard updates to "Speed: LOW"                │
│       [ ] Slave SSR_LOW LED turns on                       │
│       [ ] Motor runs at LOW speed (if connected)           │
│       [ ] Serial shows "→ SSR_LOW activated"               │
│                                                              │
│    3. Touch "OFF" button                                    │
│    4. Verify motor stops                                    │
│                                                              │
│  STEP 12: For KIPAS - Test Mode Switch                     │
│    1. Flip DPDT to MANUAL position                         │
│    2. Observe:                                              │
│       [ ] LED changes GREEN → YELLOW                        │
│       [ ] Dashboard shows "⚠ MANUAL MODE"                  │
│       [ ] Control buttons grayed out (disabled)            │
│       [ ] Serial: "Mode change: AUTO → MANUAL"             │
│                                                              │
│    3. Test physical speed switch                           │
│       [ ] Motor responds to speed switch                   │
│       [ ] Dashboard shows "Speed: UNKNOWN"                 │
│                                                              │
│    4. Flip DPDT back to AUTO                                │
│       [ ] LED changes YELLOW → GREEN                        │
│       [ ] Dashboard enables control buttons                │
│       [ ] Can control via dashboard again                  │
│                                                              │
│  STEP 13: Document & Proceed                               │
│    [ ] Take photo of device ID label                        │
│    [ ] Update as-built documentation                        │
│    [ ] Proceed to next slave                                │
│                                                              │
│  REPEAT STEPS 1-13 FOR ALL 18 SLAVES                       │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

### **6.3 System Integration Testing**

```
INTEGRATION TEST SCENARIOS:
┌─────────────────────────────────────────────────────────────┐
│                                                              │
│  TEST 1: Simultaneous Control                               │
│  ───────────────────────────────────────────────────────   │
│  Objective: Verify all devices respond to commands         │
│                                                              │
│  Procedure:                                                  │
│    1. From dashboard, command all kipas to MED             │
│    2. Observe all 13 kipas SSR activate                     │
│    3. Measure response time (should be < 3 seconds)        │
│    4. Verify dashboard updates all status                   │
│                                                              │
│  Expected Result:                                            │
│    ✓ All kipas respond                                      │
│    ✓ No command lost                                        │
│    ✓ Dashboard synced                                       │
│                                                              │
│  Pass Criteria: 100% success rate                          │
│                                                              │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  TEST 2: Heartbeat Monitoring                              │
│  ───────────────────────────────────────────────────────   │
│  Objective: Verify heartbeat mechanism working              │
│                                                              │
│  Procedure:                                                  │
│    1. Note timestamp of last heartbeat (dashboard)         │
│    2. Wait 60 seconds                                       │
│    3. Verify new heartbeat received                         │
│    4. Check all slaves still online                         │
│                                                              │
│  Expected Result:                                            │
│    ✓ Heartbeat interval: 57-63 seconds                     │
│    ✓ All slaves receive it                                  │
│    ✓ No device marked offline                               │
│                                                              │
│  Pass Criteria: Consistent 60s ± 5% interval               │
│                                                              │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  TEST 3: Fail-Safe Trigger                                 │
│  ───────────────────────────────────────────────────────   │
│  Objective: Verify fail-safe behavior                       │
│                                                              │
│  Procedure:                                                  │
│    1. Select 1 test slave (kipas recommended)              │
│    2. Note current state                                    │
│    3. Disconnect Master Logic power (simulate crash)       │
│    4. Monitor slave for 5 minutes                           │
│    5. At 300 seconds, observe fail-safe trigger            │
│                                                              │
│  Expected Result:                                            │
│    ✓ At t=300s: Slave LED turns RED                        │
│    ✓ KIPAS: All relays turn OFF                            │
│    ✓ Serial: "⚠ FAIL-SAFE TIMEOUT!"                       │
│    ✓ Dashboard shows "FAIL-SAFE" (if Display still online) │
│                                                              │
│  Pass Criteria: Fail-safe triggers at exactly 300s ± 5s    │
│                                                              │
│  Recovery Test:                                              │
│    6. Restore Master Logic power                            │
│    7. Observe slave recovery                                │
│                                                              │
│  Expected Result:                                            │
│    ✓ Within 60s: Heartbeat received                        │
│    ✓ Slave exits fail-safe (LED RED → GREEN)              │
│    ✓ Serial: "✓ Heartbeat restored"                        │
│    ✓ Slave reports FAILSAFE → AUTO                         │
│                                                              │
│  Pass Criteria: Auto-recovery within 60 seconds            │
│                                                              │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  TEST 4: Manual Override (KIPAS only)                      │
│  ───────────────────────────────────────────────────────   │
│  Objective: Verify manual mode operation                    │
│                                                              │
│  Procedure:                                                  │
│    1. Set kipas to HIGH via dashboard                       │
│    2. Verify motor at HIGH speed                            │
│    3. Flip DPDT mode switch to MANUAL                       │
│    4. Observe:                                              │
│       • LED: GREEN → YELLOW                                 │
│       • Dashboard: Shows MANUAL warning                     │
│    5. Try to command from dashboard                         │
│    6. Verify command rejected                               │
│    7. Use physical speed switch                             │
│    8. Verify motor responds to speed switch                 │
│    9. Flip back to AUTO                                     │
│    10. Verify dashboard control restored                    │
│                                                              │
│  Expected Result:                                            │
│    ✓ Mode switch detected instantly                        │
│    ✓ Dashboard commands rejected in MANUAL                 │
│    ✓ Physical switch works in MANUAL                       │
│    ✓ Dashboard control works in AUTO                        │
│                                                              │
│  Pass Criteria: All modes function correctly               │
│                                                              │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  TEST 5: Power Cycle Recovery                              │
│  ───────────────────────────────────────────────────────   │
│  Objective: Verify NVS persistence                          │
│                                                              │
│  Procedure:                                                  │
│    1. Note all device IDs and names from dashboard         │
│    2. Power off ALL devices (full system shutdown)         │
│    3. Wait 30 seconds                                       │
│    4. Power on Master Logic first                           │
│    5. Power on Display Master                               │
│    6. Power on all Slaves                                   │
│    7. Wait for full system boot (~2 minutes)               │
│    8. Verify all devices re-appear with correct IDs        │
│                                                              │
│  Expected Result:                                            │
│    ✓ All devices boot with assigned IDs                    │
│    ✓ Names preserved (not "Device 1", etc.)               │
│    ✓ No re-enrollment needed                                │
│    ✓ System fully operational after boot                   │
│                                                              │
│  Pass Criteria: 100% data persistence                      │
│                                                              │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  TEST 6: Communication Error Handling                      │
│  ───────────────────────────────────────────────────────   │
│  Objective: Verify retry and error recovery                 │
│                                                              │
│  Procedure:                                                  │
│    1. Temporarily disconnect RS-485 wire from 1 slave      │
│    2. Try to control that slave from dashboard             │
│    3. Observe retry attempts (check serial log)            │
│    4. After 3 retries, slave should be marked offline      │
│    5. Reconnect RS-485 wire                                 │
│    6. Verify slave comes back online                        │
│                                                              │
│  Expected Result:                                            │
│    ✓ 3 retry attempts with exponential backoff            │
│    ✓ Slave marked offline after retries exhausted         │
│    ✓ Auto-recovery when connection restored                │
│    ✓ No system crash or hang                               │
│                                                              │
│  Pass Criteria: Graceful error handling                    │
│                                                              │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  TEST 7: Load Testing                                       │
│  ───────────────────────────────────────────────────────   │
│  Objective: Verify system under full load                   │
│                                                              │
│  Procedure:                                                  │
│    1. Connect all actual loads (motors, lamps, speakers)   │
│    2. Turn ON all devices simultaneously                    │
│    3. Monitor for 1 hour continuous operation               │
│    4. Check temperatures:                                   │
│       • PSU: Should be warm but < 60°C                     │
│       • SSR heatsink: < 55°C (with load)                   │
│       • Enclosures: Warm but safe to touch                 │
│    5. Monitor for:                                          │
│       • False triggers                                      │
│       • Communication errors                                │
│       • Relay failures                                      │
│       • Power supply issues                                 │
│                                                              │
│  Expected Result:                                            │
│    ✓ Stable operation for 1 hour                           │
│    ✓ No overheating                                         │
│    ✓ No false triggers or errors                           │
│    ✓ All loads function correctly                          │
│                                                              │
│  Pass Criteria: 60 minutes fault-free operation            │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

---

## 🛠️ BAGIAN 7: TROUBLESHOOTING GUIDE

### 7.1 Metode Diagnostik Cepat (First 5 Minutes)

```
ALUR CEPAT DIAGNOSA:
┌─────────────────────────────────────────────────────────────┐
│  1) Identifikasi gejala utama                               │
│     • Tidak bisa kontrol? • Perangkat offline? • Beban mati?│
│  2) Tentukan domain                                         │
│     • Power • Komunikasi RS-485 • Logic/firmware • Wiring    │
│  3) Lakukan uji minimum                                     │
│     • Ukur tegangan • Cek terminasi • Baca LED status       │
│  4) Isolasi masalah                                         │
│     • Uji single node • Putus bus sementara • Swap komponen │
│  5) Dokumentasikan temuan                                   │
│     • Foto • Catat nilai ukur • Kondisi sebelum/sesudah     │
└─────────────────────────────────────────────────────────────┘
```

LED STATUS KONVENSI (semua node):
- Hijau steady: Online & AUTO
- Kuning steady: MANUAL (kipas)
- Merah steady: Fail-safe aktif / fault kritikal
- Biru berkedip cepat: Discovery mode (ID=0)
- Biru berkedip lambat: Waiting bus/timeout

### 7.2 Tabel Gejala → Penyebab → Solusi

```
┌───────────────────────────────────────────────────────────────────────────────────────────────┐
│ Gejala                                      │ Penyebab Mungkin                 │ Solusi         │
├──────────────────────────────────────────────┼──────────────────────────────────┼────────────────┤
│ Semua perangkat OFFLINE di dashboard        │ Master mati / RS-485 putus       │ Cek daya Master│
│                                              │ Terminasi hilang                 │ Pastikan 120Ω  │
│                                              │ Polaritas A/B terbalik           │ Tukar A/B      │
├──────────────────────────────────────────────┼──────────────────────────────────┼────────────────┤
│ Hanya satu slave OFFLINE                     │ Kabel putus di titik tsb          │ Uji continuity │
│                                              │ MAX3485 rusak di slave            │ Ganti modul    │
│                                              │ ID bentrok (duplikat)             │ Reset & enroll │
├─────────────────────────────────────���────────┼──────────────────────────────────┼────────────────┤
│ Kipas tidak merespons di AUTO                │ DPDT di MANUAL (GPIO35 LOW)       │ Set ke AUTO    │
│                                              │ SSR kanal rusak                   │ Pindah ke kanal│
│                                              │ RC snubber short                   │ Periksa/replace│
├──────────────────────────────────────────────┼──────────────────────────────────┼────────────────┤
│ Kipas bergetar/berdengung di LOW             │ Beban induktif tanpa snubber      │ Pasang RC      │
│                                              │ Netral tidak solid                │ Periksa netral │
├──────────────────────────────────────────────┼──────────────────────────────────┼────────────────┤
│ Lampu vilting/berkedip random                │ Noise di 5V / GND loop            │ Tambah decouple│
│                                              │ SSR bocor (leak)                   │ Ganti SSR      │
├──────────────────────────────────────────────┼──────────────────────────────────┼────────────────┤
│ Tidak bisa kirim perintah (timeout)          │ DE/RE tidak toggle                │ Cek GPIO4      │
│                                              │ Baud mismatch                      │ Samakan 115200 │
│                                              │ Termination salah                  │ Cek 60Ω total │
├──────────────────────────────────────────────┼──────────────────────────────────┼────────────────┤
│ Display Master tidak terhubung               │ RS-485 longgar                     │ Kencangkan     │
│                                              │ Firmware tidak jalan               │ Reflash        │
│                                              │ Power supply drop                  │ Ganti PSU      │
├──────────────────────────────────────────────┼──────────────────────────────────┼────────────────┤
│ PSU panas / drop tegangan                    │ Beban melebihi rating              │ Naikkan rating │
│                                              │ Ventilasi buruk                    │ Tambah airflow │
├──────────────────────────────────────────────┼──────────────────────────────────┼────────────────┤
│ MOV sering jebol                             │ Surge berat / kualitas listrik     │ Tambah SPD     │
│                                              │ MOV rating rendah                  │ Pakai kelas lebih│
└─────────────────────────────────────────────────────────────────────────────────────────���─────┘
```

### 7.3 Prosedur Isolasi Masalah RS-485

- Matikan semua node, nyalakan Master saja → verifikasi TX.
- Nyalakan satu slave terdekat → uji komunikasi dasar.
- Tambahkan node satu-per-satu hingga error muncul → lokasi kesalahan = node terakhir ditambahkan atau segmen kabel antar keduanya.
- Ukur impedansi bus di Master: ~60Ω. Jika jauh berbeda, periksa terminator.
- Swap A/B di satu sisi jika tidak ada komunikasi sama sekali.
- Cek GND bersama; tanpa reference, komunikasi bisa tidak stabil.

### 7.4 Reset & Recovery

- Reset Slave ke Factory (ID=0): Tekan tombol BOOT 5 detik saat power on → LED biru berkedip cepat → siap enrollment.
- Clear NVS Master: Menu Maintenance → Storage → Clear Registry (pastikan backup dulu).
- Re-enrollment massal: Putus bus jadi segmen kecil 3-5 perangkat, enroll per segmen, lalu gabungkan kembali.

### 7.5 Suku Cadang & Swap Cepat

- SIMPAN spare: MAX3485, 1× modul SSR 8CH, 1× HLK-PM03, 1× ESP32 DevKit, resistor 120Ω, RC snubber kit, MOV 275VAC, sekrup & gland.
- Metode swap: Lepas-komponen-sesuai-label, ganti, test lokal, lalu test bus.

---

## 🧰 BAGIAN 8: MAINTENANCE SCHEDULE

### 8.1 Jadwal Perawatan Berkala

```
RINGKASAN INTERVAL:
┌─────────────────────────────────────────��───────────────────┐
│ Harian   : Pengecekan visual cepat (dashboard status)       │
│ Mingguan : Uji fungsi acak 10% perangkat                     │
│ Bulanan  : Bersihkan debu, cek suhu operasional              │
│ Triwulan : Kencangkan terminal, uji fail-safe, cek log       │
│ Tahunan  : Ganti MOV preventif, uji isolasi, audit wiring    │
└─────────────────────────────────────────────────────────────┘
```

### 8.2 Checklist Detail per Interval

- Harian:
  - Periksa dashboard: semua device ONLINE, heartbeat normal.
  - Catat anomali (latency, perangkat sering offline sesaat).

- Mingguan:
  - Pilih 2 kipas dan 2 lampu secara acak → uji LOW/MED/HIGH/ON/OFF.
  - Untuk kipas, flip DPDT ke MANUAL lalu kembali ke AUTO, pastikan deteksi.
  - Periksa suhu PSU dan modul SSR dengan infrared thermometer.

- Bulanan:
  - Buka cover, hembuskan debu (angin kering), bersihkan filter/vent.
  - Cek kekencangan sekrup terminal (torque ringan). Jangan over-torque.
  - Verifikasi nilai tegangan 5V pada titik terjauh: 4.8–5.2V.

- Triwulan:
  - Uji fail-safe dengan mematikan Master selama 5 menit.
  - Tarik log error dari Master & Display, analisis pola.
  - Verifikasi terminasi fisik (resistor masih terpasang di ujung-ujung).

- Tahunan:
  - Ganti MOV di node yang sering kena surge (preventif).
  - Uji insulation L-N, L-PE, N-PE pada sampel per panel.
  - Audit rute kabel vs instalasi baru (pastikan tidak jadi star/branch).

### 8.3 Suku Cadang dan Umur Pakai

- MOV 275VAC: ganti 1–2 tahun atau pasca kejadian petir.
- HLK-PM03: umur tipikal 5–7 tahun; monitor ripple dan suhu.
- Modul SSR G3MB: tergantung beban; untuk motor, cek panas berkala; siap spare.
- Konektor terminal: jika sering buka-tutup, per 3 tahun ganti.

### 8.4 Prosedur Backup & Restore

- Backup konfigurasi Master & Display via menu Maintenance → Export (JSON) setiap triwulan.
- Simpan di 3 lokasi: laptop teknisi, cloud pengurus, flashdisk cadangan.
- Restore: Import file → Reboot perangkat → Verifikasi device map.

---

## 🛡️ BAGIAN 9: SAFETY & EMERGENCY PROCEDURES

### 9.1 Aturan Umum Keselamatan

- Selalu matikan MCB terkait sebelum bekerja pada kabel/beban AC.
- Gunakan APD: sarung tangan isolasi, kacamata, sepatu safety.
- Ikuti PUIL & standar lokal. Jangan modifikasi panel tanpa izin.
- Pastikan semua enclosure terhubung ke PE (earth) dengan baik.

### 9.2 Penanganan Keadaan Darurat

- Korsleting/Asap dari enclosure:
  - Matikan MCB utama area tersebut segera.
  - Jangan sentuh perangkat panas; tunggu dingin, lalu inspeksi.
  - Periksa kerusakan: PSU, MOV, kabel meleleh, SSR short.
  - Ganti komponen rusak, lakukan uji isolasi sebelum menghidupkan.

- Petir/Surge besar (banyak perangkat reset):
  - Periksa MOV visual (menghitam/retak → ganti).
  - Ukur tegangan 5V dan 3.3V; pastikan dalam rentang.
  - Verifikasi RS-485: impedansi 60Ω, kontinuitas OK.

- Orang tersengat listrik:
  - Putus sumber listrik, jangan sentuh korban langsung.
  - Ikuti prosedur P3K setempat, hubungi layanan darurat.

### 9.3 Lockout/Tagout (LOTO) Sederhana

- Saat pekerjaan besar, pasang tag “JANGAN DIHIDUPKAN” di MCB terkait.
- Kunci boks MCB jika tersedia. Catat waktu dan penanggung jawab.

### 9.4 Bekerja di Ketinggian (Kipas Plafon)

- Gunakan tangga stabil; ada pendamping yang memegangi.
- Matikan beban, pastikan motor benar-benar berhenti sebelum bekerja.
- Pastikan penempatan enclosure tidak menghalangi akses evakuasi.

---

## 📑 BAGIAN 10: DOKUMENTASI & LAMPIRAN

### 10.1 Template As-Built Documentation

```
AS-BUILT FORM (per perangkat):
- Device ID/Name   : SL-0XX / [Nama Deskriptif]
- Lokasi           : [Ruang/Saf/Koordinat]
- Tipe Profil      : FAN_4CH / LIGHT_8CH / SOUND_8CH
- Sumber Daya      : HLK-PM03 / PSU terpusat
- RS-485           : A=Blue-White, B=Green-White, GND=Brown pair
- Terminasi        : Ya / Tidak (120Ω)
- Mode Switch      : Ya / Tidak (untuk kipas)
- Firmware Version : vX.Y.Z
- Tanggal Pasang   : DD-MM-YYYY
- Teknisi          : [Nama]
- Catatan          : [Observasi khusus]
```

### 10.2 Tabel Mapping ID dan Lokasi (Contoh)

```
ID  Nama                          Lokasi            Profil
01  Kipas Saf 1 Depan             Saf 1             FAN_4CH
02  Kipas Saf 2 Depan             Saf 2             FAN_4CH
...
14  Lampu Utama Kanan             Panel Lampu Kanan LIGHT_8CH
...
18  Audio Mimbar                  Ruang Audio       SOUND_8CH
```

### 10.3 Checklist Serah Terima (Commissioning Report)

- Semua bagian 6 lulus: Test 1–7 PASS.
- Backup konfigurasi diekspor dan diserahkan (USB + cloud).
- Peta kabel dan foto instalasi diserahkan.
- Buku panduan ini dicetak + softcopy.

### 10.4 Spesifikasi Teknis Ringkas (Rujukan Cepat)

- Layer Fisik RS-485: 115200 8N1, bus linear, terminasi 120Ω di ujung.
- Pinout standar ESP32 untuk slave: RX=16, TX=17, DE/RE=4, Relay=12/13/14/15/25/26/27/32, Mode Detect=35.
- Proteksi wajib: MOV L–N, Fuse 5A, RC snubber per kanal induktif, decoupling 1000µF+100nF.

### 10.5 Catatan Perubahan Dokumen (Changelog)

- v1.1 (27-12-2025): Tambah Bagian 6–10, perbaikan diagram RS-485, klarifikasi DPDT.
- v1.0 (25-12-2025): Rilis awal Bagian 1–5.

---

Dokumen 3: Panduan Integrasi Hardware v1.1 selesai. Simpan dan distribusikan kepada tim instalasi dan maintenance. Pastikan semua teknisi membaca Bagian 7–9 sebelum bekerja di lapangan.