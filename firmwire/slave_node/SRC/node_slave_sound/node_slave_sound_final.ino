/**
 * ═══════════════════════════════════════════════════════════════
 *  SMART MOSQUE — NODE SLAVE SOUND
 *  Board  : ESP32 X8 Relay Modbus (303e32dc812)
 *  Addr   : 0x03 (harus sesuai MODBUS_ADDR_RELAY_SOUND_DEFAULT)
 *
 *  Identik dengan node_slave_lampu_final.ino
 *  Perbedaan HANYA pada: mb_addr = 0x03 dan label Serial
 *
 *  MAPPING RELAY:
 *    Relay 1 (bit 0) = Speaker Zone 1
 *    Relay 2 (bit 1) = Speaker Zone 2
 *    Relay 3 (bit 2) = Speaker Zone 3
 *    Relay 4 (bit 3) = Speaker Zone 4
 *    Relay 5 (bit 4) = Speaker Zone 5
 *    Relay 6 (bit 5) = Speaker Zone 6
 *    Relay 7 (bit 6) = Speaker Zone 7
 *    Relay 8 (bit 7) = Speaker Zone 8
 * ═══════════════════════════════════════════════════════════════
 */

uint8_t mb_addr = 0x03;  // Node Sound ← BEDA dari Lampu

int rxPin    = 18;
int txPin    = 19;
int txenPin  = 32;
int clockPin = 26;
int latchPin = 25;
int dataPin  = 33;
int inPin    = 27;
int oePin    = 13;

uint8_t val_in  = 0x00;
uint8_t val_out = 0x00;

uint8_t outMapping[8] = {7, 6, 5, 4, 3, 2, 1, 0};
uint8_t inMapping[8]  = {6, 4, 2, 0, 1, 3, 5, 7};

uint16_t crc16_update(uint16_t crc, uint8_t a) {
    crc ^= (uint16_t)a;
    for (int i = 0; i < 8; ++i) {
        if (crc & 1) crc = (crc >> 1) ^ 0xA001;
        else         crc = (crc >> 1);
    }
    return crc;
}

uint8_t shiftInOut(uint8_t outP, uint8_t inP, uint8_t clkP, uint8_t out) {
    uint8_t in = 0;
    for (int i = 0; i < 8; i++) {
        if (!digitalRead(inP)) in |= (1 << inMapping[i]);
        digitalWrite(outP, !(out & (1 << outMapping[i])));
        digitalWrite(clkP, HIGH);
        digitalWrite(clkP, LOW);
    }
    digitalWrite(latchPin, LOW);
    digitalWrite(latchPin, HIGH);
    return in;
}

void mb_fc15_write_coils(uint8_t *frame_rx) {
    uint16_t crc = 0xFFFF;
    uint8_t frame_tx[8];

    val_out = frame_rx[7];

    Serial.printf("[SOUND] FC15 → mask=0x%02X (", val_out);
    for (int i = 7; i >= 0; i--) Serial.print((val_out >> i) & 1);
    Serial.println("b)");

    frame_tx[0] = mb_addr;
    frame_tx[1] = 0x0F;
    frame_tx[2] = 0x00;
    frame_tx[3] = 0x00;
    frame_tx[4] = 0x00;
    frame_tx[5] = 0x08;
    for (int i = 0; i < 6; i++) crc = crc16_update(crc, frame_tx[i]);
    frame_tx[6] = (crc & 0xFF);
    frame_tx[7] = ((crc >> 8) & 0xFF);

    digitalWrite(txenPin, HIGH);
    delayMicroseconds(200);
    Serial2.write(frame_tx, 8);
    Serial2.flush();
    digitalWrite(txenPin, LOW);
}

void mb_fc02_read_inputs(uint8_t *frame_rx) {
    uint16_t crc = 0xFFFF;
    uint8_t frame_tx[6];
    uint8_t memaddr = frame_rx[3];

    frame_tx[0] = mb_addr;
    frame_tx[1] = 0x02;
    frame_tx[2] = 0x01;

    if (memaddr == 0x00) {
        frame_tx[3] = val_in;
        Serial.printf("[SOUND] FC02 memaddr=0x00 → input=0x%02X\n", val_in);
    } else if (memaddr == 0x01) {
        frame_tx[3] = val_out;
        Serial.printf("[SOUND] FC02 memaddr=0x01 → relay=0x%02X\n", val_out);
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

void setup() {
    Serial.begin(115200);
    Serial.println("\n═══════════════════════════════════");
    Serial.println("  SMART MOSQUE — NODE SLAVE SOUND");
    Serial.printf ("  Modbus addr : 0x%02X\n", mb_addr);
    Serial.println("  RS485 RX=GPIO18  TX=GPIO19");
    Serial.println("  DE/RE=GPIO32     OE=GPIO13");
    Serial.println("═══════════════════════════════════");

    pinMode(txenPin, OUTPUT);
    digitalWrite(txenPin, LOW);

    pinMode(latchPin, OUTPUT);
    pinMode(clockPin, OUTPUT);
    pinMode(dataPin,  OUTPUT);
    pinMode(inPin,    INPUT);
    pinMode(oePin,    OUTPUT);

    shiftInOut(dataPin, inPin, clockPin, 0x00);
    digitalWrite(oePin, LOW);

    Serial2.begin(9600, SERIAL_8N1, rxPin, txPin);
    Serial2.setTimeout(10);

    Serial.println("✓ Semua relay OFF");
    Serial.println("✓ Menunggu perintah Logic Master...\n");
    delay(100);
}

void loop() {
    val_in = shiftInOut(dataPin, inPin, clockPin, val_out);

    digitalWrite(txenPin, LOW);
    if (Serial2.available() > 0) {
        uint8_t frame_rx[12];
        uint8_t flen = Serial2.readBytes(frame_rx, 12);

        if (flen < 4) return;

        uint16_t crc = 0xFFFF;
        for (int i = 0; i < flen - 2; i++) crc = crc16_update(crc, frame_rx[i]);

        bool addr_ok = (frame_rx[0] == mb_addr);
        bool crc_ok  = (frame_rx[flen - 2] == (crc & 0xFF)) &&
                       (frame_rx[flen - 1] == ((crc >> 8) & 0xFF));

        if (!addr_ok || !crc_ok) return;

        switch (frame_rx[1]) {
            case 0x0F:
                mb_fc15_write_coils(frame_rx);
                break;
            case 0x02:
                mb_fc02_read_inputs(frame_rx);
                break;
            default:
                Serial.printf("[SOUND] FC tidak dikenal: 0x%02X\n", frame_rx[1]);
                break;
        }
    }
}
