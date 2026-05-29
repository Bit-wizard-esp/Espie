#pragma once

#include <Arduino.h>

void showBootLogo();
void showMessage(String msg, int delayTime);
void showKeyCode(uint8_t* buffer, String title);
void drawKeyList(String title, int selected, bool showSelectHint);
void drawMainMenu();
void drawReadMenu();
void drawWriteMenu();
void drawEmulateMenu();
void drawBrutforceMenu();
void drawDeleteMenu();
void drawViewMenu();
void drawReadActive();
void drawWriteActive();
void drawEmulateActive();
void drawBrutforceActive();
void drawDeleteConfirm();
void drawCopyConfirm();
void drawShowCode();
void updateDisplay();