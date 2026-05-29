#pragma once

#include <Arduino.h>

#define OLED_SDA 4
#define OLED_SCL 5

#define BTN_UP    13
#define BTN_DOWN  12
#define BTN_OK    14
#define BTN_BACK  16

#define PIN_READ_WRITE 0
#define PIN_EMU        2

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define MAX_KEYS 10
#define BUILTIN_KEYS_COUNT 27

struct Key {
  uint8_t bytes[8];
  char name[16];
};

enum MenuState {
  MAIN_MENU,
  READ_MENU,
  WRITE_MENU,
  EMULATE_MENU,
  BRUTFORCE_MENU,
  DELETE_MENU,
  VIEW_MENU,
  READ_ACTIVE,
  WRITE_ACTIVE,
  EMULATE_ACTIVE,
  BRUTFORCE_ACTIVE,
  DELETE_CONFIRM,
  COPY_CONFIRM,
  SHOW_CODE
};

extern const uint8_t builtinBrutforceKeys[BUILTIN_KEYS_COUNT][8];
extern const unsigned char image_iButtonKey_bits[];
extern const unsigned char image_paint_4_bits[];