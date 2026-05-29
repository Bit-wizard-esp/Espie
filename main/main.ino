#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <OneWire.h>
#include <OneWireHub.h>
#include <DS2401.h>
#include <EEPROM.h>

#include "config.h"
#include "globals.h"
#include "storage.h"
#include "display.h"
#include "onewire_ops.h"

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
OneWire ow(PIN_READ_WRITE);
OneWireHub hub(PIN_EMU);
DS2401* currentKey = nullptr;

Key keys[MAX_KEYS];
int keyCount = 0;
int selectedKey = 0;

uint8_t brutforceList[BUILTIN_KEYS_COUNT + MAX_KEYS][8];
int brutforceTotalCount = 0;

MenuState currentState = MAIN_MENU;
int menuIndex = 0;
int selectedKeyIndex = 0;
int writeSourceIndex = -1;

bool emuActive = false;
unsigned long emuStartTime = 0;
const unsigned long EMU_DURATION = 10000;

bool brutforceActive = false;
int brutforceCurrentIndex = 0;
int brutforceRepeatCount = 0;
unsigned long lastBrutforceTime = 0;
const unsigned long BRUTFORCE_DELAY = 500;

unsigned long lastTimeUp   = 0;
unsigned long lastTimeDown = 0;
unsigned long lastTimeOk   = 0;
unsigned long lastTimeBack = 0;
const unsigned long DEBOUNCE = 200;

bool prevUp   = HIGH;
bool prevDown = HIGH;
bool prevOk   = HIGH;
bool prevBack = HIGH;

bool waitingForKey = false;


void handleBackButton() {
  switch (currentState) {
    case MAIN_MENU:
      break;

    case READ_MENU:
    case BRUTFORCE_MENU:
    case READ_ACTIVE:
    case BRUTFORCE_ACTIVE:
      if (brutforceActive) { stopBrutforce(); showMessage("Brutforce stopped", 1000); }
      waitingForKey = false;
      currentState = MAIN_MENU;
      menuIndex = 0;
      break;

    case WRITE_MENU:
    case EMULATE_MENU:
    case DELETE_MENU:
    case VIEW_MENU:
    case WRITE_ACTIVE:
    case EMULATE_ACTIVE:
      if (emuActive) stopEmulation();
      currentState = MAIN_MENU;
      menuIndex = 0;
      selectedKeyIndex = 0;
      break;

    case DELETE_CONFIRM:
      currentState = DELETE_MENU;
      break;

    case COPY_CONFIRM:
      currentState = WRITE_MENU;
      break;

    default:
      currentState = MAIN_MENU;
      menuIndex = 0;
      break;
  }
  updateDisplay();
}

void handleUpButton() {
  switch (currentState) {
    case MAIN_MENU:
      menuIndex = (menuIndex > 0) ? menuIndex - 1 : 5;
      break;
    case WRITE_MENU: case EMULATE_MENU: case DELETE_MENU: case VIEW_MENU:
      if (keyCount > 0)
        selectedKeyIndex = (selectedKeyIndex > 0) ? selectedKeyIndex - 1 : keyCount - 1;
      break;
    default: break;
  }
  updateDisplay();
}

void handleDownButton() {
  switch (currentState) {
    case MAIN_MENU:
      menuIndex = (menuIndex < 5) ? menuIndex + 1 : 0;
      break;
    case WRITE_MENU: case EMULATE_MENU: case DELETE_MENU: case VIEW_MENU:
      if (keyCount > 0)
        selectedKeyIndex = (selectedKeyIndex < keyCount - 1) ? selectedKeyIndex + 1 : 0;
      break;
    default: break;
  }
  updateDisplay();
}

void handleOkButton() {
  switch (currentState) {
    case MAIN_MENU:
      switch (menuIndex) {
        case 0: currentState = READ_MENU; break;
        case 1:
          if (keyCount > 0) { currentState = WRITE_MENU; selectedKeyIndex = 0; }
          else showMessage("No keys in memory!", 1500);
          break;
        case 2:
          if (keyCount > 0) { currentState = EMULATE_MENU; selectedKeyIndex = 0; }
          else showMessage("No keys in memory!", 1500);
          break;
        case 3: currentState = BRUTFORCE_MENU; break;
        case 4:
          if (keyCount > 0) { currentState = DELETE_MENU; selectedKeyIndex = 0; }
          else showMessage("No keys to delete", 1500);
          break;
        case 5:
          if (keyCount > 0) { currentState = VIEW_MENU; selectedKeyIndex = 0; }
          else showMessage("No keys in memory!", 1500);
          break;
      }
      updateDisplay();
      break;

    case READ_MENU:
      currentState = READ_ACTIVE;
      waitingForKey = true;
      updateDisplay();
      break;

    case WRITE_MENU:
      if (keyCount > 0) { currentState = COPY_CONFIRM; updateDisplay(); }
      break;

    case COPY_CONFIRM:
      currentState = WRITE_ACTIVE;
      updateDisplay();
      copyKeyToAnother(selectedKeyIndex);
      currentState = MAIN_MENU;
      menuIndex = 0;
      updateDisplay();
      break;

    case EMULATE_MENU:
      if (keyCount > 0) {
        currentState = EMULATE_ACTIVE;
        updateDisplay();
        emulateKey(selectedKeyIndex);
        showMessage("Emulating:\n" + String(keys[selectedKeyIndex].name) + "\non TX (GPIO2)\nfor 10 seconds", 2000);
        currentState = MAIN_MENU;
        menuIndex = 0;
        updateDisplay();
      }
      break;

    case BRUTFORCE_MENU:
      currentState = BRUTFORCE_ACTIVE;
      updateDisplay();
      startBrutforce();
      break;

    case DELETE_MENU:
      if (keyCount > 0) { currentState = DELETE_CONFIRM; updateDisplay(); }
      break;

    case DELETE_CONFIRM:
      deleteKey(selectedKeyIndex);
      showMessage("Key deleted!\nKeys: " + String(keyCount) + "/" + String(MAX_KEYS), 800);
      if (keyCount > 0) {
        if (selectedKeyIndex >= keyCount) selectedKeyIndex = keyCount - 1;
        currentState = DELETE_MENU;
      } else {
        selectedKeyIndex = 0;
        currentState = MAIN_MENU;
        menuIndex = 0;
      }
      updateDisplay();
      break;

    case VIEW_MENU:
      if (keyCount > 0 && selectedKeyIndex < keyCount) { currentState = SHOW_CODE; updateDisplay(); }
      break;

    default: break;
  }
}

void handleButtons() {
  unsigned long now = millis();

  bool curUp   = digitalRead(BTN_UP);
  bool curDown = digitalRead(BTN_DOWN);
  bool curOk   = digitalRead(BTN_OK);
  bool curBack = digitalRead(BTN_BACK);

  bool pressedUp   = (curUp   == LOW && prevUp   == HIGH && now - lastTimeUp   > DEBOUNCE);
  bool pressedDown = (curDown == LOW && prevDown == HIGH && now - lastTimeDown > DEBOUNCE);
  bool pressedOk   = (curOk   == LOW && prevOk   == HIGH && now - lastTimeOk   > DEBOUNCE);
  bool pressedBack = (curBack == LOW && prevBack == HIGH && now - lastTimeBack > DEBOUNCE);

  if (pressedUp)   lastTimeUp   = now;
  if (pressedDown) lastTimeDown = now;
  if (pressedOk)   lastTimeOk   = now;
  if (pressedBack) lastTimeBack = now;

  prevUp   = curUp;
  prevDown = curDown;
  prevOk   = curOk;
  prevBack = curBack;

  if (currentState == SHOW_CODE) {
    if (pressedOk)   { currentState = VIEW_MENU;  updateDisplay(); }
    if (pressedBack) { currentState = MAIN_MENU; menuIndex = 0; updateDisplay(); }
    return;
  }

  if (pressedBack) { handleBackButton(); return; }
  if (pressedUp)   handleUpButton();
  if (pressedDown) handleDownButton();
  if (pressedOk)   handleOkButton();
}

void setup() {
  Serial.begin(115200);

  pinMode(BTN_UP,   INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_OK,   INPUT_PULLUP);
  pinMode(BTN_BACK, INPUT_PULLUP);

  pinMode(PIN_READ_WRITE, INPUT_PULLUP);
  pinMode(PIN_EMU,        INPUT_PULLUP);

  Wire.begin(OLED_SDA, OLED_SCL);
  Wire.setClock(800000);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    Serial.println("SSD1306 allocation failed");

  display.ssd1306_command(SSD1306_DISPLAYON);
  display.clearDisplay();

  showBootLogo();
  loadKeys();

  menuIndex = 0;
  selectedKeyIndex = 0;
  waitingForKey = false;

  prevUp   = digitalRead(BTN_UP);
  prevDown = digitalRead(BTN_DOWN);
  prevOk   = digitalRead(BTN_OK);
  prevBack = digitalRead(BTN_BACK);

  updateDisplay();
}

void loop() {
  handleButtons();

  if (currentState == READ_ACTIVE && waitingForKey) {
    uint8_t newKey[8];
    if (readKey(newKey)) {
      waitingForKey = false;
      showKeyCode(newKey, "READ KEY:");

      bool decisionMade = false;
      unsigned long showStartTime = millis();

      while (!decisionMade) {
        if (digitalRead(BTN_OK) == LOW && millis() - lastTimeOk > DEBOUNCE) {
          lastTimeOk = millis();
          addNewKey(newKey, "");
          decisionMade = true;
        }
        if (digitalRead(BTN_BACK) == LOW && millis() - lastTimeBack > DEBOUNCE) {
          lastTimeBack = millis();
          showMessage("Key not saved", 800);
          decisionMade = true;
        }
        if (millis() - showStartTime > 10000) {
          showMessage("Timeout\nKey not saved", 1000);
          decisionMade = true;
        }
        delay(50);
      }

      currentState = MAIN_MENU;
      menuIndex = 0;
      updateDisplay();
    }
    delay(100);
  }

  if (brutforceActive) {
    if (millis() - lastBrutforceTime >= BRUTFORCE_DELAY) {
      lastBrutforceTime = millis();
      brutforceCurrentIndex++;
      if (brutforceCurrentIndex >= brutforceTotalCount) {
        stopBrutforce();
        showMessage("Brutforce complete!\nAll " + String(brutforceTotalCount) + " keys tried", 2000);
        currentState = MAIN_MENU;
        updateDisplay();
      } else {
        emulateBrutforceKey(brutforceList[brutforceCurrentIndex]);
        updateDisplay();
      }
    }
    if (emuActive && currentKey != nullptr) hub.poll();
  }
  else if (emuActive && currentKey != nullptr) {
    if (millis() - emuStartTime >= EMU_DURATION) {
      stopEmulation();
      updateDisplay();
    } else {
      hub.poll();
    }
  }

  delay(10);
}