/**
 * ═══════════════════════════════════════════════════════════════
 *  SMART MOSQUE — NODE SLAVE LAMPU
 *  Board  : ESP32 X8 Relay Modbus (303e32dc812)
 *  Addr   : 0x02 (set mb_addr di bawah)
 *  Bus    : Modbus RTU via RS485 Built-in @ 9600 bps
 *
 *  PINOUT TERVERIFIKASI (source asli multigcs):
 *    RS485 RX   : GPIO18  (ModBUS RX label board)
 *    RS485 TX   : GPIO19  (ModBUS TX label board)
 *    RS485 DE   : GPIO32  (TX Enable)
 *    SR Clock   : GPIO26  (74HC595 SCK)
 *    SR Latch   : GPIO25  (74HC595 RCK)
 *    SR Data Out: GPIO33  (74HC595 SER → relay)
 *    SR Data In : GPIO27  (74HC165 SER ← input optocoupler)
 *    SR OE      : GPIO13  (74HC595 OE, LOW = aktif)
 *
 *  PROTOKOL (dari Logic Master):
 *    FC15 (0x0F) Write Multiple Coils:
 *      [addr][0x0F][0x00][0x00][0x00][0x08][0x01][MASK][CRC_L][CRC_H]
 *      MASK = bitmask 8 relay (bit0=Lampu1 ... bit7=Lampu8)
 *    FC02 (0x02) Read Discrete Inputs:
 *      [addr][0x02][0x00][MEM][0x00][0x01][CRC_L][CRC_H]
 *      MEM=0x00 → baca input, MEM=0x01 → baca relay state
 *
 *  MAPPING RELAY:
 *    Relay 1 (bit 0) = Lampu 1
 *    Relay 2 (bit 1) = Lampu 2
 *    Relay 3 (bit 2) = Lampu 3
 *    Relay 4 (bit 3) = Lampu 4
 *    Relay 5 (bit 4) = Lampu 5
 *    Relay 6 (bit 5) = Lampu 6
 *    Relay 7 (bit 6) = Lampu 7
 *    Relay 8 (bit 7) = Lampu 8
 *
 *  TIDAK menggunakan library Modbus eksternal.
 *  CRC16 diimplementasikan sendiri (kompatibel standar Modbus RTU).
 * ═══════════════════════════════════════════════════════════════
 */

// ── MODBUS ADDRESS ────────────────────────────────────────────
// Harus sesuai dengan MODBUS_ADDR_RELAY_LIGHT_DEFAULT di Logic Master
uint8_t mb_addr = 0x02;  // Node Lampu

// ── PIN DEFINITIONS (terverifikasi dari source asli) ──────────
int rxPin    = 18;   // RS485 RX
int txPin    = 19;   // RS485 TX
int txenPin  = 32;   // RS485 TX Enable (DE/RE)
int clockPin = 26;   // 74HC595 SCK
int latchPin = 25;   // 74HC595 RCK
int dataPin  = 33;   // 74HC595 SER (output → relay)
int inPin    = 27;   // 74HC165 SER (input ← optocoupler)
int oePin    = 13;   // 74HC595 OE (LOW = aktif)

// ── STATE ─────────────────────────────────────────────────────
uint8_t val_in  = 0x00;  // Bitmask input optocoupler
uint8_t val_out = 0x00;  // Bitmask relay (bit0=relay1 ... bit7=relay8)

// Mapping dari source asli (urutan bit fisik board)
uint8_t outMapping[8] = {7, 6, 5, 4, 3, 2, 1, 0};
uint8_t inMapping[8]  = {6, 4, 2, 0, 1, 3, 5, 7};

// ══════════════════════════════════════════════════════════════
// CRC16 MODBUS
// ══════════════════════════════════════════════════════════════

uint16_t crc16_update(uint16_t crc, uint8_t a) {
    crc ^= (uint16_t)a;
    for (int i = 0; i < 8; ++i) {
        if (crc & 1) crc = (crc >> 1) ^ 0xA001;
        else         crc = (crc >> 1);
    }
    return crc;
}

// ══════════════════════════════════════════════════════════════
// SHIFT REGISTER — tulis relay & baca input sekaligus
// ══════════════════════════════════════════════════════════════

uint8_t shiftInOut(uint8_t outP, uint8_t inP, uint8_t clkP, uint8_t out) {
    uint8_t in = 0;
    for (int i = 0; i < 8; i++) {
        if (!digitalRead(inP)) in |= (1 << inMapping[i]);
        digitalWrite(outP, !(out & (1 << outMapping[i])));  // active LOW
        digitalWrite(clkP, HIGH);
        digitalWrite(clkP, LOW);
    }
    digitalWrite(latchPin, LOW);
    digitalWrite(latchPin, HIGH);
    return in;
}

// ══════════════════════════════════════════════════════════════
// FC15 — Write Multiple Coils (kontrol relay dari Logic Master)
// ══════════════════════════════════════════════════════════════

void mb_fc15_write_coils(uint8_t *frame_rx) {
    uint16_t crc = 0xFFFF;
    uint8_t frame_tx[8];

    // Ambil bitmask relay dari byte ke-8 frame (index 7)
    val_out = frame_rx[7];

    Serial.printf("[LAMPU] FC15 → mask=0x%02X (", val_out);
    for (int i = 7; i >= 0; i--) Serial.print((val_out >> i) & 1);
    Serial.println("b)");

    // Response frame
    frame_tx[0] = mb_addr;
    frame_tx[1] = 0x0F;   // FC15
    frame_tx[2] = 0x00;   // Start coil Hi
    frame_tx[3] = 0x00;   // Start coil Lo
    frame_tx[4] = 0x00;   // Qty Hi
    frame_tx[5] = 0x08;   // Qty Lo (8 coil)
    for (int i = 0; i < 6; i++) crc = crc16_update(crc, frame_tx[i]);
    frame_tx[6] = (crc & 0xFF);
    frame_tx[7] = ((crc >> 8) & 0xFF);

    digitalWrite(txenPin, HIGH);
    delayMicroseconds(200);
    Serial2.write(frame_tx, 8);
    Serial2.flush();
    digitalWrite(txenPin, LOW);
}

// ══════════════════════════════════════════════════════════════
// FC02 — Read Discrete Inputs (baca status ke Logic Master)
// ══════════════════════════════════════════════════════════════

void mb_fc02_read_inputs(uint8_t *frame_rx) {
    uint16_t crc = 0xFFFF;
    uint8_t frame_tx[6];
    uint8_t memaddr = frame_rx[3];

    frame_tx[0] = mb_addr;
    frame_tx[1] = 0x02;   // FC02
    frame_tx[2] = 0x01;   // Byte count

    if (memaddr == 0x00) {
        frame_tx[3] = val_in;   // Baca input optocoupler
        Serial.printf("[LAMPU] FC02 memaddr=0x00 → input=0x%02X\n", val_in);
    } else if (memaddr == 0x01) {
        frame_tx[3] = val_out;  // Baca relay state
        Serial.printf("[LAMPU] FC02 memaddr=0x01 → relay=0x%02X\n", val_out);
    } else {
        frame_tx[3] = 0x00;
    }

    for (int i = 0; i < 4; i++) crc = crc16_update(crc, frame_tx[i]);
    frame_tx[4] = (crc & 0xFF);
    frame_tx[5] = ((crc >> 8) & 0xFF);

    digitalWrite(txenPin, HIGH);
    delayMicroseconds(200);
    Serial2.write(frame_tx, 6);
    Serial2.flush();
    digitalWrite(txenPin, LOW);
}

// ══════════════════════════════════════════════════════════════
// SETUP
// ══════════════════════════════════════════════════════════════

void setup() {
    Serial.begin(115200);
    Serial.println("\n═══════════════════════════════════");
    Serial.println("  SMART MOSQUE — NODE SLAVE LAMPU");
    Serial.printf ("  Modbus addr : 0x%02X\n", mb_addr);
    Serial.println("  RS485 RX=GPIO18  TX=GPIO19");
    Serial.println("  DE/RE=GPIO32     OE=GPIO13");
    Serial.println("═══════════════════════════════════");

    // RS485 direction
    pinMode(txenPin, OUTPUT);
    digitalWrite(txenPin, LOW);  // Default: receive

    // Shift register
    pinMode(latchPin, OUTPUT);
    pinMode(clockPin, OUTPUT);
    pinMode(dataPin,  OUTPUT);
    pinMode(inPin,    INPUT);
    pinMode(oePin,    OUTPUT);

    // Semua relay OFF saat boot
    shiftInOut(dataPin, inPin, clockPin, 0x00);
    digitalWrite(oePin, LOW);  // Aktifkan output

    // RS485 Serial @ 9600 (sesuai Bus B Logic Master)
    Serial2.begin(9600, SERIAL_8N1, rxPin, txPin);
    Serial2.setTimeout(10);

    Serial.println("✓ Semua relay OFF");
    Serial.println("✓ Menunggu perintah Logic Master...\n");
    delay(100);
}

// ══════════════════════════════════════════════════════════════
// LOOP
// ══════════════════════════════════════════════════════════════

void loop() {
    // Update shift register: tulis relay & baca input
    val_in = shiftInOut(dataPin, inPin, clockPin, val_out);

    // Modbus listener
    digitalWrite(txenPin, LOW);
    if (Serial2.available() > 0) {
        uint8_t frame_rx[12];
        uint8_t flen = Serial2.readBytes(frame_rx, 12);

        if (flen < 4) return;  // Frame terlalu pendek

        // Validasi CRC
        uint16_t crc = 0xFFFF;
        for (int i = 0; i < flen - 2; i++) crc = crc16_update(crc, frame_rx[i]);

        bool addr_ok = (frame_rx[0] == mb_addr);
        bool crc_ok  = (frame_rx[flen - 2] == (crc & 0xFF)) &&
                       (frame_rx[flen - 1] == ((crc >> 8) & 0xFF));

        if (!addr_ok || !crc_ok) return;  // Bukan untuk kita / CRC error

        // Dispatch berdasarkan Function Code
        switch (frame_rx[1]) {
            case 0x0F:  // FC15 Write Multiple Coils
                mb_fc15_write_coils(frame_rx);
                break;
            case 0x02:  // FC02 Read Discrete Inputs
                mb_fc02_read_inputs(frame_rx);
                break;
            default:
                // FC tidak didukung — abaikan
                Serial.printf("[LAMPU] FC tidak dikenal: 0x%02X\n", frame_rx[1]);
                break;
        }
    }
}
