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

// Записывает один байт побитово напрямую через пин (протокол DS1990A)
static void iButtonWriteByte(uint8_t data) {
  for (int bit = 0; bit < 8; bit++) {
    if (data & 1) {
      // Логическая 1: длинный импульс LOW ~60мкс
      digitalWrite(PIN_READ_WRITE, LOW);
      pinMode(PIN_READ_WRITE, OUTPUT);
      delayMicroseconds(60);
      pinMode(PIN_READ_WRITE, INPUT);
      digitalWrite(PIN_READ_WRITE, HIGH);
      delay(10);
    } else {
      // Логический 0: короткий импульс LOW
      digitalWrite(PIN_READ_WRITE, LOW);
      pinMode(PIN_READ_WRITE, OUTPUT);
      pinMode(PIN_READ_WRITE, INPUT);
      digitalWrite(PIN_READ_WRITE, HIGH);
      delay(10);
    }
    data >>= 1;
  }
}

bool writeKeyToDevice(uint8_t* data) {
  showMessage("Place blank key\non RX pin", 2000);
  delay(500);
  showMessage("Writing...", 0);

  // Шаг 1: Разблокировка записи — команда 0xD1 + логический 0
  ow.skip();
  ow.reset();
  ow.write(0xD1);
  digitalWrite(PIN_READ_WRITE, LOW);
  pinMode(PIN_READ_WRITE, OUTPUT);
  delayMicroseconds(60);           // лог. 0 = длинный LOW
  pinMode(PIN_READ_WRITE, INPUT);
  digitalWrite(PIN_READ_WRITE, HIGH);
  delay(10);

  // Шаг 2: Запись 8 байт — команда 0xD5 + данные побитово
  ow.skip();
  ow.reset();
  ow.write(0xD5);
  for (int i = 0; i < 8; i++) {
    iButtonWriteByte(data[i]);
  }

  // Шаг 3: Фиксация записи — команда 0xD1 + логическая 1
  ow.reset();
  ow.write(0xD1);
  digitalWrite(PIN_READ_WRITE, LOW);
  pinMode(PIN_READ_WRITE, OUTPUT);
  delayMicroseconds(10);           // лог. 1 = короткий LOW
  pinMode(PIN_READ_WRITE, INPUT);
  digitalWrite(PIN_READ_WRITE, HIGH);
  delay(10);

  // Шаг 4: Верификация
  delay(500);
  uint8_t verify[8];
  if (readKey(verify)) {
    if (memcmp(data, verify, 8) == 0) {
      showMessage("Write OK!\nVerified!", 1500);
      return true;
    }
  }
  showMessage("Write FAILED!\nCheck key/pin", 2000);
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