// The screen and touch driver (LovyanGFX), the colours from the mock-ups and
// the smooth Inter fonts.
#pragma once
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include "mn_hw.h"

#ifdef MN_HOST
// on a computer (firmware/hosttest/render_screens.cpp) the screen is a picture
class LGFX : public lgfx::LGFX_Sprite {};
#else
class LGFX : public lgfx::LGFX_Device {
 public:
  lgfx::Panel_ST7796 panel;   // drives both the ST7796S and the ST7796U
  lgfx::Bus_SPI bus;
  lgfx::Light_PWM light;
  lgfx::Touch_XPT2046 touch;
  LGFX();
};
#endif

extern LGFX lcd;

// colourMode 0-3 (bit 0 = red/blue swapped, bit 1 = inverted): screens from
// different batches differ; the hardware test finds the right one.
// flip = turned upside down (USB-C on the other side).
void lcdBegin(int colourMode, bool flip);
void lcdBrightness(uint8_t level);   // 0-255

// The mock-up colours (design/mock_mini.py)
uint16_t rgb(uint8_t r, uint8_t g, uint8_t b);
extern uint16_t C_BG, C_TILE, C_TILE_HI, C_EDGE, C_WHITE, C_GREY, C_DIM, C_RED, C_GREEN, C_YELLOW,
    C_AUTO_BG, C_AUTO_EDGE, C_AUTO_INK;

enum FontId { F_B12, F_B16, F_B18, F_B24, F_B36, F_S13, F_M12, F_M15, F_B62, F_COUNT };
void useFont(FontId f);
