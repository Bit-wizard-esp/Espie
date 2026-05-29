#include "display.h"
#include "globals.h"
#include "config.h"
#include "onewire_ops.h"
void showBootLogo() {
  display.clearDisplay();
  display.drawBitmap(0, 10, image_iButtonKey_bits, 49, 44, 1);
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(61, 24);
  display.print("Espie");
  display.setTextSize(1);
  display.setCursor(54, 43);
  display.print("Ibutton tool");
  display.drawBitmap(17, 17, image_paint_4_bits, 88, 31, 0);
  display.display();
  delay(2000);
}

void showMessage(String msg, int delayTime) {
  display.clearDisplay();
  display.setTextSize(1);
  int y = 10, startIdx = 0;
  while (startIdx < (int)msg.length()) {
    int endIdx = msg.indexOf('\n', startIdx);
    if (endIdx == -1) endIdx = msg.length();
    String line = msg.substring(startIdx, endIdx);
    display.setCursor((128 - line.length() * 6) / 2, y);
    display.println(line);
    y += 10;
    startIdx = endIdx + 1;
  }
  display.display();
  if (delayTime > 0) delay(delayTime);
}

void showKeyCode(uint8_t* buffer, String title) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(title);
  display.drawLine(0, 8, 128, 8, SSD1306_WHITE);

  String hexStr = "";
  for (int i = 0; i < 8; i++) {
    if (buffer[i] < 0x10) hexStr += "0";
    hexStr += String(buffer[i], HEX);
    if (i < 7) hexStr += ":";
  }

  display.setCursor(0, 16);
  if (hexStr.length() > 21) {
    display.println(hexStr.substring(0, 21));
    display.setCursor(0, 26);
    display.println(hexStr.substring(21));
  } else {
    display.println(hexStr);
  }

  uint8_t crc = ow.crc8(buffer, 7);
  display.setCursor(0, 40);
  display.print("CRC: ");
  if (crc < 0x10) display.print("0");
  display.print(crc, HEX);
  display.println(crc == buffer[7] ? " OK" : " BAD");

  display.setCursor(0, 52);
  display.print("OK:Save  BACK:Cancel");
  display.display();
}

void drawKeyList(String title, int selected, bool showSelectHint) {
  display.setCursor(0, 0);
  display.println(title);
  display.drawLine(0, 8, 128, 8, SSD1306_WHITE);

  if (keyCount == 0) {
    display.setCursor(20, 30);
    display.println("NO KEYS");
  } else {
    int startIdx = 0;
    if (selected >= 2) startIdx = selected - 1;
    if (startIdx + 3 > keyCount) startIdx = keyCount - 3;
    if (startIdx < 0) startIdx = 0;

    int yPos = 16;
    for (int i = 0; i < 3 && (startIdx + i) < keyCount; i++) {
      int idx = startIdx + i;
      display.setCursor(2, yPos);
      display.print(idx == selected ? ">" : " ");
      String name = keys[idx].name;
      if (name.length() > 14) name = name.substring(0, 12) + "...";
      display.println(name);
      yPos += 12;
    }
  }
  if (showSelectHint) {
    display.setCursor(2, 55);
    display.print("Keys: " + String(keyCount) + "/" + String(MAX_KEYS));
  }
}

void drawMainMenu() {
  display.setCursor(0, 0);
  display.println("iBUTTON TOOL");
  display.drawLine(0, 8, 128, 8, SSD1306_WHITE);

  const char* menuItems[] = {" READ KEY", " WRITE KEY", " EMULATE", " BRUTFORCE", " DELETE", " VIEW CODE"};
  int menuCount = 6;
  int startIdx = 0;
  if (menuIndex >= 2) startIdx = menuIndex - 1;
  if (startIdx + 3 > menuCount) startIdx = menuCount - 3;
  if (startIdx < 0) startIdx = 0;

  int yPos = 16;
  for (int i = 0; i < 3 && (startIdx + i) < menuCount; i++) {
    int idx = startIdx + i;
    display.setCursor(2, yPos);
    display.print(idx == menuIndex ? ">" : " ");
    display.println(menuItems[idx]);
    yPos += 12;
  }
  display.setCursor(2, 55);
  display.print("Keys: " + String(keyCount) + "/" + String(MAX_KEYS));
  if (emuActive || brutforceActive) { display.setCursor(95, 55); display.print("EMU"); }
}

void drawReadMenu() {
  display.setCursor(0, 0); display.println("READ KEY");
  display.drawLine(0, 8, 128, 8, SSD1306_WHITE);
  display.setCursor(0, 18); display.println("Place key on");
  display.setCursor(0, 28); display.println("GPIO0 (RX) pin");
  display.setCursor(0, 48); display.println("OK to start");
  display.setCursor(0, 56); display.print("BACK to cancel");
}

void drawReadActive() {
  display.setCursor(0, 0); display.println("READING...");
  display.drawLine(0, 8, 128, 8, SSD1306_WHITE);
  display.setCursor(0, 18); display.println("Waiting for key");
  display.setCursor(0, 28); display.println("on GPIO0 (RX)");
  display.setCursor(0, 40); display.println("Press BACK");
  display.setCursor(0, 50); display.println("to cancel");
}

void drawWriteMenu()    { drawKeyList("SELECT KEY TO COPY", selectedKeyIndex, true); }
void drawEmulateMenu()  { drawKeyList("SELECT TO EMULATE",  selectedKeyIndex, true); }
void drawDeleteMenu()   { drawKeyList("SELECT TO DELETE",   selectedKeyIndex, true); }
void drawViewMenu()     { drawKeyList("SELECT TO VIEW",     selectedKeyIndex, true); }

void drawBrutforceMenu() {
  buildBrutforceList();
  display.setCursor(0, 0); display.println("BRUTFORCE MODE");
  display.drawLine(0, 8, 128, 8, SSD1306_WHITE);
  display.setCursor(0, 18); display.println("Will try " + String(brutforceTotalCount) + " keys");
  display.setCursor(0, 28); display.println("Each key 1 time");
  display.setCursor(0, 38); display.println("Delay: 500ms");
  display.setCursor(0, 55); display.print("OK to start");
}

void drawWriteActive() {
  display.setCursor(0, 0); display.println("WRITING...");
  display.drawLine(0, 8, 128, 8, SSD1306_WHITE);
  display.setCursor(0, 30); display.println("Please wait");
}

void drawEmulateActive() {
  display.setCursor(0, 0); display.println("EMULATING");
  display.drawLine(0, 8, 128, 8, SSD1306_WHITE);
  display.setCursor(0, 18); display.println("Key: " + String(keys[selectedKeyIndex].name));
  display.setCursor(0, 28); display.println("on TX (GPIO2)");
  display.setCursor(0, 38); display.println("Time: 10 seconds");
}

void drawBrutforceActive() {
  display.setCursor(0, 0); display.println("BRUTFORCE ACTIVE");
  display.drawLine(0, 8, 128, 8, SSD1306_WHITE);
  display.setCursor(0, 18);
  display.print("Key: "); display.print(brutforceCurrentIndex + 1);
  display.print("/"); display.println(brutforceTotalCount);
  display.setCursor(0, 28); display.print("ID: ");
  for (int i = 0; i < 8; i++) {
    if (brutforceList[brutforceCurrentIndex][i] < 0x10) display.print("0");
    display.print(brutforceList[brutforceCurrentIndex][i], HEX);
    if (i < 7) display.print(":");
  }
  display.setCursor(0, 50); display.print("BACK to stop");
}

void drawDeleteConfirm() {
  display.setCursor(0, 0); display.println("CONFIRM DELETE");
  display.drawLine(0, 8, 128, 8, SSD1306_WHITE);
  display.setCursor(0, 18); display.println("Delete:");
  display.setCursor(0, 28); display.println(String(keys[selectedKeyIndex].name));
  display.setCursor(0, 40); display.println("Are you sure?");
  display.setCursor(20, 52); display.print("OK Yes");
  display.setCursor(80, 52); display.print("BACK No");
}

void drawCopyConfirm() {
  display.setCursor(0, 0); display.println("CONFIRM COPY");
  display.drawLine(0, 8, 128, 8, SSD1306_WHITE);
  display.setCursor(0, 18); display.println("Copy to blank key:");
  display.setCursor(0, 28); display.println(String(keys[selectedKeyIndex].name));
  display.setCursor(0, 40); display.println("Are you sure?");
  display.setCursor(20, 52); display.print("OK Yes");
  display.setCursor(80, 52); display.print("BACK No");
}

void drawShowCode() {
  if (keyCount > 0 && selectedKeyIndex < keyCount)
    showKeyCode(keys[selectedKeyIndex].bytes, "KEY CODE:");
}

void updateDisplay() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  switch (currentState) {
    case MAIN_MENU:        drawMainMenu();        break;
    case READ_MENU:        drawReadMenu();        break;
    case WRITE_MENU:       drawWriteMenu();       break;
    case EMULATE_MENU:     drawEmulateMenu();     break;
    case BRUTFORCE_MENU:   drawBrutforceMenu();   break;
    case DELETE_MENU:      drawDeleteMenu();      break;
    case VIEW_MENU:        drawViewMenu();        break;
    case READ_ACTIVE:      drawReadActive();      break;
    case WRITE_ACTIVE:     drawWriteActive();     break;
    case EMULATE_ACTIVE:   drawEmulateActive();   break;
    case BRUTFORCE_ACTIVE: drawBrutforceActive(); break;
    case DELETE_CONFIRM:   drawDeleteConfirm();   break;
    case COPY_CONFIRM:     drawCopyConfirm();     break;
    case SHOW_CODE:        drawShowCode();        break;
  }
  display.display();
}