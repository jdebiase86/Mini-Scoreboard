// The board: 4.0" ESP32-32E display (Hosyond / iPistBit / LCDwiki E32R40T).
// ESP32-D0WD-V3, 4 MB flash, no PSRAM. ST7796S or ST7796U screen, XPT2046
// resistive touch, both on one SPI bus. The screen's reset is tied to the
// board's reset. Pins from the LCDwiki table, checked with
// firmware/mini_hw_test.
#pragma once

#define PIN_SCK   14
#define PIN_MOSI  13
#define PIN_MISO  12
#define PIN_DC     2
#define PIN_CS    15
#define PIN_BL    27   // backlight, PWM
#define PIN_T_CS  33   // touch
#define PIN_T_IRQ 36
#define PIN_LED_R 22   // RGB LED, lit by pulling the pin LOW
#define PIN_LED_G 16
#define PIN_LED_B 17
#define PIN_DAC   26   // speaker
#define PIN_AMP    4   // speaker amplifier on/off
#define PIN_BAT   34   // battery voltage through a divider
#define PIN_BOOT   0   // the BOOT button ("B" tab on the case)

#define SCREEN_W 480
#define SCREEN_H 320
