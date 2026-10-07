#include "mn_lcd.h"
#include "mn_fonts.h"

#ifndef MN_HOST
LGFX::LGFX() {
  {
    auto c = bus.config();
    c.spi_host = HSPI_HOST;
    c.spi_mode = 0;
    c.freq_write = 40000000;
    c.freq_read = 16000000;
    c.spi_3wire = false;
    c.use_lock = true;
    c.dma_channel = SPI_DMA_CH_AUTO;
    c.pin_sclk = PIN_SCK;
    c.pin_mosi = PIN_MOSI;
    c.pin_miso = PIN_MISO;
    c.pin_dc = PIN_DC;
    bus.config(c);
    panel.setBus(&bus);
  }
  {
    auto c = panel.config();
    c.pin_cs = PIN_CS;
    c.pin_rst = -1;
    c.pin_busy = -1;
    c.panel_width = 320;
    c.panel_height = 480;
    c.readable = true;
    c.invert = false;
    c.rgb_order = false;
    c.dlen_16bit = false;
    c.bus_shared = true;
    panel.config(c);
  }
  {
    auto c = light.config();
    c.pin_bl = PIN_BL;
    c.invert = false;
    c.freq = 44100;
    c.pwm_channel = 7;
    light.config(c);
    panel.setLight(&light);
  }
  {
    auto c = touch.config();
    c.x_min = 300; c.x_max = 3900;   // replaced by the saved touch setup
    c.y_min = 200; c.y_max = 3800;
    c.pin_int = PIN_T_IRQ;
    c.bus_shared = true;
    c.offset_rotation = 0;
    c.spi_host = HSPI_HOST;
    c.freq = 1000000;
    c.pin_sclk = PIN_SCK;
    c.pin_mosi = PIN_MOSI;
    c.pin_miso = PIN_MISO;
    c.pin_cs = PIN_T_CS;
    touch.config(c);
    panel.setTouch(&touch);
  }
  setPanel(&panel);
}

#endif

LGFX lcd;

uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) { return lgfx::color565(r, g, b); }

uint16_t C_BG = rgb(12, 14, 20), C_TILE = rgb(28, 32, 42), C_TILE_HI = rgb(38, 44, 58), C_EDGE = rgb(52, 58, 74),
         C_WHITE = rgb(240, 242, 246), C_GREY = rgb(150, 156, 170), C_DIM = rgb(100, 106, 120),
         C_RED = rgb(226, 40, 46), C_GREEN = rgb(40, 190, 90), C_YELLOW = rgb(250, 210, 40),
         C_AUTO_BG = rgb(24, 40, 70), C_AUTO_EDGE = rgb(70, 110, 190), C_AUTO_INK = rgb(170, 190, 230);

#ifndef MN_HOST
void lcdBegin(int colourMode, bool flip) {
  auto c = lcd.panel.config();
  c.rgb_order = colourMode & 1;
  lcd.panel.config(c);
  lcd.init();
  lcd.setRotation(flip ? 3 : 1);
  lcd.invertDisplay(colourMode & 2);
  lcd.fillScreen(C_BG);
}

void lcdBrightness(uint8_t level) { lcd.setBrightness(level); }
#endif

// Each font is read straight from flash; only its small glyph table is in RAM
struct LoadedFont {
  lgfx::PointerWrapper data;
  lgfx::VLWfont font;
  bool ok = false;
};
static LoadedFont loaded[F_COUNT];
static const uint8_t* const FONT_DATA[F_COUNT] = {FONT_B12, FONT_B16, FONT_B18, FONT_B24,
                                                  FONT_B36, FONT_S13, FONT_M12, FONT_M15};
static const uint32_t FONT_LEN[F_COUNT] = {sizeof(FONT_B12), sizeof(FONT_B16), sizeof(FONT_B18), sizeof(FONT_B24),
                                           sizeof(FONT_B36), sizeof(FONT_S13), sizeof(FONT_M12), sizeof(FONT_M15)};

void useFont(FontId f) {
  LoadedFont& lf = loaded[f];
  if (!lf.ok) {
    lf.data.set(FONT_DATA[f], FONT_LEN[f]);
    lf.ok = lf.font.loadFont(&lf.data);
  }
  if (lf.ok) lcd.setFont(&lf.font);
  else lcd.setFont(&fonts::FreeSans12pt7b);
}
