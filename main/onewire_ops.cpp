#include "onewire_ops.h"
#include "globals.h"
#include "display.h"
#include "config.h"

bool readKey(uint8_t* buffer) {
  uint8_t addr[8];
  ow.reset_search();
  if (ow.search(addr)) {
    for (int i = 0; i < 8; i++) buffer[i] = addr[i];
    ow.reset_search();
    return true;
  }
  return false;
}

bool writeKeyToDevice(uint8_t* data) {
  showMessage("Place target key\non RX pin", 1500);
  delay(1000);
  showMessage("Writing...", 500);

  pinMode(PIN_READ_WRITE, OUTPUT);
  ow.reset();
  delay(10);
  ow.write(0x99);
  for (int i = 0; i < 8; i++) { ow.write(data[i]); delay(1); }
  delay(100);
  ow.reset();
  ow.write(0xC3);
  ow.write(0x00);
  ow.write(0x00);
  delay(200);

  uint8_t crc = 0;
  for (int i = 0; i < 7; i++) crc = ow.crc8(&data[i], 1);

  delay(500);
  pinMode(PIN_READ_WRITE, INPUT_PULLUP);

  uint8_t verify[8];
  if (readKey(verify)) {
    if (memcmp(data, verify, 8) == 0) { showMessage("Write verified!", 1000); return true; }
  }
  showMessage("Write failed!\nCheck key", 1500);
  return false;
}

void emulateKey(int index) {
  if (currentKey != nullptr) { hub.detach(*currentKey); delete currentKey; currentKey = nullptr; }
  if (index >= 0 && index < keyCount && keyCount > 0) {
    currentKey = new DS2401(
      keys[index].bytes[0], keys[index].bytes[1], keys[index].bytes[2],
      keys[index].bytes[3], keys[index].bytes[4], keys[index].bytes[5],
      keys[index].bytes[6]
    );
    hub.attach(*currentKey);
    emuActive = true;
    emuStartTime = millis();
  } else {
    emuActive = false;
  }
}

void emulateBrutforceKey(const uint8_t* keyData) {
  if (currentKey != nullptr) { hub.detach(*currentKey); delete currentKey; currentKey = nullptr; }
  currentKey = new DS2401(
    keyData[0], keyData[1], keyData[2],
    keyData[3], keyData[4], keyData[5], keyData[6]
  );
  hub.attach(*currentKey);
  emuActive = true;
}

void stopEmulation() {
  if (currentKey != nullptr) { hub.detach(*currentKey); delete currentKey; currentKey = nullptr; }
  emuActive = false;
  brutforceActive = false;
}

void buildBrutforceList() {
  brutforceTotalCount = 0;
  for (int i = 0; i < BUILTIN_KEYS_COUNT; i++) {
    memcpy(brutforceList[brutforceTotalCount++], builtinBrutforceKeys[i], 8);
  }
  for (int i = 0; i < keyCount; i++) {
    bool dup = false;
    for (int j = 0; j < brutforceTotalCount; j++) {
      if (memcmp(brutforceList[j], keys[i].bytes, 8) == 0) { dup = true; break; }
    }
    if (!dup && brutforceTotalCount < (BUILTIN_KEYS_COUNT + MAX_KEYS)) {
      memcpy(brutforceList[brutforceTotalCount++], keys[i].bytes, 8);
    }
  }
}

void startBrutforce() {
  buildBrutforceList();
  if (brutforceTotalCount == 0) { showMessage("No keys to try!", 1500); return; }
  brutforceActive = true;
  brutforceCurrentIndex = 0;
  brutforceRepeatCount = 0;
  emulateBrutforceKey(brutforceList[0]);
  lastBrutforceTime = millis();
}

void stopBrutforce() {
  brutforceActive = false;
  stopEmulation();
}

void copyKeyToAnother(int sourceIndex) {
  if (writeKeyToDevice(keys[sourceIndex].bytes))
    showMessage("Key copied!\nFrom: " + String(keys[sourceIndex].name), 1500);
}