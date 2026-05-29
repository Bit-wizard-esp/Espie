#pragma once

#include <Arduino.h>
#include "config.h"

bool isKeyEmpty(uint8_t* buffer);
void loadKeys();
void saveKey(int index);
void saveAllKeysToEEPROM();
void deleteKey(int index);
void addNewKey(uint8_t* buffer, String name);