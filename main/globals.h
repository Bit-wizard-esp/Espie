#pragma once

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <OneWire.h>
#include <OneWireHub.h>
#include <DS2401.h>
#include <EEPROM.h>
#include "config.h"

extern Adafruit_SSD1306 display;
extern OneWire ow;
extern OneWireHub hub;
extern DS2401* currentKey;

extern Key keys[MAX_KEYS];
extern int keyCount;
extern int selectedKey;

extern uint8_t brutforceList[BUILTIN_KEYS_COUNT + MAX_KEYS][8];
extern int brutforceTotalCount;

extern MenuState currentState;
extern int menuIndex;
extern int selectedKeyIndex;
extern int writeSourceIndex;

extern bool emuActive;
extern unsigned long emuStartTime;
extern const unsigned long EMU_DURATION;

extern bool brutforceActive;
extern int brutforceCurrentIndex;
extern int brutforceRepeatCount;
extern unsigned long lastBrutforceTime;
extern const unsigned long BRUTFORCE_DELAY;

extern unsigned long lastTimeUp;
extern unsigned long lastTimeDown;
extern unsigned long lastTimeOk;
extern unsigned long lastTimeBack;
extern const unsigned long DEBOUNCE;

extern bool prevUp;
extern bool prevDown;
extern bool prevOk;
extern bool prevBack;

extern bool waitingForKey;