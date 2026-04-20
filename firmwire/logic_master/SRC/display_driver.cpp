/**
 * ═══════════════════════════════════════════════════════════════
 *  LOGIC MASTER - DISPLAY DRIVER
 *
 *  CATATAN: Board ESP32-S3 N16R8 IoT Logger RS485 Plus tidak
 *  memiliki layar display yang terhubung langsung.
 *  File ini berisi stub kosong agar tidak ada linker error.
 *
 *  Fungsi display (LVGL, ILI9488, dsb) ada di firmware
 *  display_master yang terpisah, berjalan di board sendiri.
 * ═══════════════════════════════════════════════════════════════
 */

#include "../include/logic_master.h"

// Stub: Logic Master tidak punya display lokal
void initDisplay()        { Serial.println("ℹ Display: tidak ada (IoT Logger board)"); }
void updateStatusDisplay(){ /* tidak ada display */ }
void showBootScreen()     { /* tidak ada display */ }
void showDeviceList()     { /* tidak ada display */ }
void showSystemStats()    { /* tidak ada display */ }
void showPendingDevices() { /* tidak ada display */ }
