#include <EEPROM.h>
#include "config.h"
#include "globals.h"
#include "storage.h"
#include "display.h"
#include "onewire_ops.h"

bool isKeyEmpty(uint8_t* buffer) {
  for (int i = 0; i < 8; i++) {
    if (buffer[i] != 0xFF) return false;
  }
  return true;
}

void saveAllKeysToEEPROM() {
  EEPROM.begin(512);
  EEPROM.write(0, keyCount);
  for (int i = 0; i < keyCount; i++) {
    int addr = 1 + i * 24;
    for (int j = 0; j < 8;  j++) EEPROM.write(addr + j,     keys[i].bytes[j]);
    for (int j = 0; j < 16; j++) EEPROM.write(addr + 8 + j, keys[i].name[j]);
  }
  for (int i = keyCount; i < MAX_KEYS; i++) {
    int addr = 1 + i * 24;
    for (int j = 0; j < 24; j++) EEPROM.write(addr + j, 0xFF);
  }
  EEPROM.commit();
}

void loadKeys() {
  EEPROM.begin(512);
  int rawCount = EEPROM.read(0);
  if (rawCount > MAX_KEYS || rawCount < 0) rawCount = 0;

  Key validKeys[MAX_KEYS];
  int validCount = 0;

  for (int i = 0; i < rawCount && i < MAX_KEYS; i++) {
    int addr = 1 + i * 24;
    uint8_t tempBytes[8];
    bool isEmpty = true;
    for (int j = 0; j < 8; j++) {
      tempBytes[j] = EEPROM.read(addr + j);
      if (tempBytes[j] != 0xFF) isEmpty = false;
    }
    if (!isEmpty) {
      for (int j = 0; j < 8;  j++) validKeys[validCount].bytes[j] = tempBytes[j];
      for (int j = 0; j < 16; j++) validKeys[validCount].name[j]  = EEPROM.read(addr + 8 + j);
      validKeys[validCount].name[15] = '\0';
      validCount++;
    }
  }

  keyCount = validCount;
  for (int i = 0; i < keyCount; i++) keys[i] = validKeys[i];
  if (rawCount != keyCount) saveAllKeysToEEPROM();
}

void saveKey(int index) {
  if (index < 0 || index >= MAX_KEYS) return;
  EEPROM.begin(512);
  int addr = 1 + index * 24;
  for (int j = 0; j < 8;  j++) EEPROM.write(addr + j,     keys[index].bytes[j]);
  for (int j = 0; j < 16; j++) EEPROM.write(addr + 8 + j, keys[index].name[j]);
  EEPROM.commit();
}

void addNewKey(uint8_t* buffer, String name) {
  if (isKeyEmpty(buffer)) { showMessage("Empty key ignored", 1000); return; }
  for (int i = 0; i < keyCount; i++) {
    if (memcmp(keys[i].bytes, buffer, 8) == 0) { showMessage("Key already exists!", 1500); return; }
  }
  if (keyCount >= MAX_KEYS) { showMessage("Memory full!\nMax 10 keys", 1500); return; }

  memcpy(keys[keyCount].bytes, buffer, 8);
  if (name.length() == 0) {
    char tempName[16];
    sprintf(tempName, "%02X%02X%02X", buffer[0], buffer[1], buffer[2]);
    name = String(tempName);
  }
  name.toCharArray(keys[keyCount].name, 16);
  keyCount++;
  saveKey(keyCount - 1);
  saveAllKeysToEEPROM();
  showMessage("Key saved!\n" + name + "\n(" + String(keyCount) + "/10)", 1000);
}

void deleteKey(int index) {
  if (keyCount == 0 || index < 0 || index >= keyCount) return;
  if (emuActive && currentKey != nullptr) stopEmulation();

  for (int i = index; i < keyCount - 1; i++) keys[i] = keys[i + 1];
  keyCount--;
  if (keyCount >= 0 && keyCount < MAX_KEYS) memset(&keys[keyCount], 0, sizeof(Key));

  EEPROM.begin(512);
  EEPROM.write(0, keyCount);
  for (int i = 0; i < keyCount; i++) saveKey(i);
  for (int i = keyCount; i < MAX_KEYS; i++) {
    int addr = 1 + i * 24;
    for (int j = 0; j < 24; j++) EEPROM.write(addr + j, 0xFF);
  }
  EEPROM.commit();

  if (keyCount == 0) {
    emuActive = false;
    if (currentKey != nullptr) { hub.detach(*currentKey); delete currentKey; currentKey = nullptr; }
  }
}