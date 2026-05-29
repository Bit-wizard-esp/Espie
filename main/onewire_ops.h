#pragma once

#include <Arduino.h>

bool readKey(uint8_t* buffer);
bool writeKeyToDevice(uint8_t* data);
void emulateKey(int index);
void emulateBrutforceKey(const uint8_t* keyData);
void stopEmulation();
void buildBrutforceList();
void startBrutforce();
void stopBrutforce();
void copyKeyToAnother(int sourceIndex);