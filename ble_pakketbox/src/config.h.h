#pragma once
// ============================================================
// config.h - alle instellingen van de slimme pakketbox
// Eén plek om BLE-naam, pins, intervallen en motorstanden aan te passen.
// ============================================================
#include <Arduino.h>

// ---------- BLE ----------
// Herkenbare naam: moet overeenkomen met deviceName in index.html
#define BLE_DEVICE_NAME "Pakketbox-ESP32"

// UUID's: moeten 100% gelijk zijn aan die in index.html
// (zelf nieuwe genereren kan via uuidgenerator.net)
#define SERVICE_UUID             "19b10000-e8f2-537e-4f6c-d104768a1214"
#define STATUS_CHARACTERISTIC_UUID "19b10001-e8f2-537e-4f6c-d104768a1214" // ESP32 -> gsm (READ + NOTIFY)
#define LOCK_CHARACTERISTIC_UUID   "19b10002-e8f2-537e-4f6c-d104768a1214" // gsm -> ESP32 (WRITE)

// ---------- Pinnen ----------
// Kies een vrije GPIO die PWM kan; controleer de pinout van je FireBeetle ESP32-E
#define SERVO_PIN 25

// ---------- Motorstanden (twee vaste waarden) ----------
#define SERVO_CLOSED_ANGLE 0    // graden = pakketbox gesloten
#define SERVO_OPEN_ANGLE   65   // graden = pakketbox open

// ---------- Intervallen (milliseconden, gebruikt met millis()) ----------
#define AUTO_OPEN_INTERVAL_MS 30000UL // elke 30 s een geplande levering: box opent automatisch
#define AUTO_CLOSE_DELAY_MS   10000UL // na 10 s open sluit de box automatisch weer

// ---------- Statusteksten (naar gsm én seriële monitor) ----------
#define STATUS_OPEN_TEXT   "Pakketbox open"
#define STATUS_CLOSED_TEXT "Pakketbox gesloten"
