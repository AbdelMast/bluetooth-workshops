// ============================================================
// Slimme pakketbox - ESP32 BLE GATT server
// - Servo simuleert het slot (2 vaste standen, zie config.h)
// - Automatisch openen/sluiten via millis() (nooit delay())
// - Handmatige bediening via BLE (onWrite callback)
// - Statusbevestiging via BLE notify + seriële monitor
// Documentatie ESP32 BLE: BLEDevice.h / BLEServer.h zijn onderdeel van de ESP32 Arduino core.
// ============================================================
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>   // CCCD descriptor (0x2902): nodig om notificaties te kunnen abonneren
#include <ESP32Servo.h>
#include "config.h"

// ---------- Globale toestand ----------
Servo lockServo;
BLECharacteristic *pStatusCharacteristic = nullptr;

bool deviceConnected = false;
bool boxOpen = false;

// De BLE-callback draait in de BLE-taak. We zetten daar enkel een vlag; de loop() voert
// het commando uit. Zo blijft alle motor- en notify-logica op één plek (setLock).
enum Command : int8_t { CMD_NONE = -1, CMD_CLOSE = 0, CMD_OPEN = 1 };
volatile Command pendingCommand = CMD_NONE;

unsigned long lastAutoOpen = 0; // tijdstip van laatste automatische opening
unsigned long openedAt = 0;     // tijdstip waarop de box open ging

// ---------- Status doorgeven (BLE + seriële monitor) ----------
void publishStatus(const char *source) {
  const char *msg = boxOpen ? STATUS_OPEN_TEXT : STATUS_CLOSED_TEXT;

  Serial.printf("[STATUS] %s (bron: %s)\n", msg, source);

  pStatusCharacteristic->setValue(msg);   // waarde in de characteristic zetten (ook voor Read)
  if (deviceConnected) {
    pStatusCharacteristic->notify();      // actief pushen naar de gsm
  }
}

// ---------- Slot aansturen: de enige plek waar de servo beweegt ----------
void setLock(bool open, const char *source) {
  boxOpen = open;
  lockServo.write(open ? SERVO_OPEN_ANGLE : SERVO_CLOSED_ANGLE);
  if (open) {
    openedAt = millis(); // starttijd voor automatisch sluiten
  }
  publishStatus(source);
}

// ---------- BLE callbacks (event-driven) ----------
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) override {
    deviceConnected = true;
    Serial.println("[EVENT] Smartphone verbonden");
  }
  void onDisconnect(BLEServer *pServer) override {
    deviceConnected = false;
    Serial.println("[EVENT] Verbinding verbroken, advertising herstarten...");
    pServer->startAdvertising(); // anders blijft de ESP32 onvindbaar
  }
};

class LockCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) override {
    // getValue() geeft std::string terug (niet Arduino String)
    std::string rx = pCharacteristic->getValue();
    if (rx.length() == 0) return;

    // Accepteert zowel de byte 1/0 (Uint8Array vanuit JavaScript) als het teken '1'/'0'
    char b = rx[0];
    if (b == 1 || b == '1')      pendingCommand = CMD_OPEN;
    else if (b == 0 || b == '0') pendingCommand = CMD_CLOSE;
  }
};

// ---------- Setup-functies ----------
void setupBle() {
  BLEDevice::init(BLE_DEVICE_NAME);

  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Status: ESP32 -> gsm (READ voor de beginwaarde, NOTIFY voor live updates)
  pStatusCharacteristic = pService->createCharacteristic(
      STATUS_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  pStatusCharacteristic->addDescriptor(new BLE2902());

  // Slotcommando: gsm -> ESP32 (WRITE)
  BLECharacteristic *pLockCharacteristic = pService->createCharacteristic(
      LOCK_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_WRITE);
  pLockCharacteristic->setCallbacks(new LockCallbacks());

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  BLEDevice::startAdvertising();
}

void setupServo() {
  ESP32PWM::allocateTimer(0);
  lockServo.setPeriodHertz(50); // standaard servofrequentie
  lockServo.attach(SERVO_PIN);
}

// ---------- Loop-functies (non-blocking) ----------
void handleBleCommand() {
  if (pendingCommand == CMD_NONE) return;
  Command cmd = pendingCommand;
  pendingCommand = CMD_NONE;
  setLock(cmd == CMD_OPEN, "handmatig via BLE");
}

void handleAutomaticSchedule() {
  unsigned long now = millis();

  if (!boxOpen && now - lastAutoOpen >= AUTO_OPEN_INTERVAL_MS) {
    lastAutoOpen = now;
    setLock(true, "automatisch interval");
  } else if (boxOpen && now - openedAt >= AUTO_CLOSE_DELAY_MS) {
    setLock(false, "automatisch sluiten");
  }
}

void setup() {
  Serial.begin(115200);
  setupServo();
  setupBle();
  setLock(false, "opstart"); // startpositie: gesloten
  Serial.println("Pakketbox actief. Wachten op verbinding...");
}

void loop() {
  handleBleCommand();
  handleAutomaticSchedule();
  // Bewust geen delay(): de loop draait op volle snelheid
}