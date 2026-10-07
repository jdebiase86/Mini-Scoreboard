/*
  Mini Scoreboard hardware test - 4.0" ESP32-32E display (ST7796S or ST7796U,
  XPT2046 resistive touch). Hosyond / iPistBit / LCDwiki E32R40T boards.

  Proves the screen, touch, LED, speaker, battery reading and Wi-Fi radio
  all work before the real firmware goes on. It walks through eight tests
  and says on the screen what you should be seeing. "Test 3 of 8" in the
  top corner tells you where you are. Everything is also printed to the
  Serial Monitor (115200 baud).

    1. CORNERS   Red top-left, green top-right, blue bottom-left, white
                 bottom-right, thin grey border all round. If the colours are
                 wrong, press the board's BOOT button to try the next colour
                 mode (1 to 4); the number shows in the middle.
    2. COLOURS   Whole screen red, green, blue, white, black, then smooth
                 colour bars.
    3. LIGHT     Backlight steps down to dim and back up.
    4. TOUCH     Tap the four arrows as they appear (press firmly), then
                 draw with a finger. Tap DONE.
    5. LED       The little LED on the board goes red, green, blue.
    6. SPEAKER   Two rounds of three beeps (only with a speaker plugged in).
    7. BATTERY   Shows the battery voltage the board reads.
    8. WI-FI     Counts the Wi-Fi networks it can hear.

  Pins from the LCDwiki E32R40T table. The screen's reset is tied to the
  board's reset, so there's no reset pin.

  Needs the LovyanGFX library (Arduino IDE -> Tools -> Manage Libraries ->
  "LovyanGFX"). Board: "ESP32 Dev Module". Or skip the IDE: flash the ready
  file in flash/ (see flash/README.md).
*/

#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <WiFi.h>
#include <Preferences.h>

#define PIN_SCK   14
#define PIN_MOSI  13
#define PIN_MISO  12
#define PIN_DC     2
#define PIN_CS    15
#define PIN_BL    27
#define PIN_T_CS  33
#define PIN_T_IRQ 36
#define PIN_LED_R 22   // the RGB LED is lit by pulling a pin LOW
#define PIN_LED_G 16
#define PIN_LED_B 17
#define PIN_DAC   26   // speaker
#define PIN_AMP    4   // speaker amplifier on/off
#define PIN_BAT   34   // battery voltage (through a divider)
#define PIN_BOOT   0

class LGFX : public lgfx::LGFX_Device {
 public:
  lgfx::Panel_ST7796 panel;
  lgfx::Bus_SPI bus;
  lgfx::Light_PWM light;
  lgfx::Touch_XPT2046 touch;

  LGFX() {
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
      c.x_min = 300; c.x_max = 3900;
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
};

static LGFX lcd;
static int colourMode = 0;   // 0-3: bit 0 = swap red/blue, bit 1 = invert
static const int NTESTS = 8;
static const uint16_t GREY = 0x7BEF;

static void applyColourMode() {
  auto c = lcd.panel.config();
  c.rgb_order = colourMode & 1;
  lcd.panel.config(c);
  lcd.init();
  lcd.setRotation(1);
  lcd.invertDisplay(colourMode & 2);
}

static void header(int n, const char* title) {
  lcd.fillScreen(TFT_BLACK);
  lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  lcd.setFont(&fonts::FreeSansBold18pt7b);
  lcd.setTextDatum(top_left);
  lcd.drawString(title, 14, 10);
  lcd.setFont(&fonts::FreeSans9pt7b);
  lcd.setTextColor(GREY, TFT_BLACK);
  lcd.setTextDatum(top_right);
  char t[24];
  snprintf(t, sizeof(t), "Test %d of %d", n, NTESTS);
  lcd.drawString(t, 466, 16);
  Serial.printf("\n== Test %d of %d: %s\n", n, NTESTS, title);
}

// a line of help text; y advances by the line height
static int say(int y, const char* s, uint16_t col = TFT_WHITE) {
  lcd.setFont(&fonts::FreeSans12pt7b);
  lcd.setTextColor(col, TFT_BLACK);
  lcd.setTextDatum(top_left);
  lcd.drawString(s, 14, y);
  Serial.printf("   %s\n", s);
  return y + 30;
}

static void led(bool r, bool g, bool b) {
  digitalWrite(PIN_LED_R, r ? LOW : HIGH);
  digitalWrite(PIN_LED_G, g ? LOW : HIGH);
  digitalWrite(PIN_LED_B, b ? LOW : HIGH);
}

static bool bootPressed() {
  if (digitalRead(PIN_BOOT) != LOW) return false;
  delay(30);
  while (digitalRead(PIN_BOOT) == LOW) delay(10);
  return true;
}

// ---------------------------------------------------------------- 1 corners
static void drawCorners() {
  lcd.fillScreen(TFT_BLACK);
  lcd.drawRect(0, 0, 480, 320, GREY);
  lcd.fillRect(1, 1, 70, 70, TFT_RED);
  lcd.fillRect(409, 1, 70, 70, TFT_GREEN);
  lcd.fillRect(1, 249, 70, 70, TFT_BLUE);
  lcd.fillRect(409, 249, 70, 70, TFT_WHITE);
  lcd.setFont(&fonts::FreeSansBold12pt7b);
  lcd.setTextDatum(middle_center);
  lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  lcd.drawString("RED", 140, 36);
  lcd.drawString("GREEN", 340, 36);
  lcd.drawString("BLUE", 140, 284);
  lcd.drawString("WHITE", 340, 284);
  lcd.setFont(&fonts::FreeSansBold18pt7b);
  lcd.drawString("MINI SCOREBOARD", 240, 120);
  char m[32];
  snprintf(m, sizeof(m), "Colour mode %d of 4", colourMode + 1);
  lcd.setFont(&fonts::FreeSans12pt7b);
  lcd.setTextColor(TFT_YELLOW, TFT_BLACK);
  lcd.drawString(m, 240, 168);
  lcd.setTextColor(GREY, TFT_BLACK);
  lcd.drawString("Wrong colours? Press BOOT.", 240, 200);
  lcd.drawString("Test 1 of 8", 240, 230);
  Serial.printf("\n== Test 1 of 8: corners. Colour mode %d of 4. Red top-left, green top-right,\n"
                "   blue bottom-left, white bottom-right, black background. Press BOOT to change.\n", colourMode + 1);
}

static void testCorners() {
  drawCorners();
  // 12 seconds, longer while BOOT is being used
  uint32_t until = millis() + 12000;
  while ((int32_t)(millis() - until) < 0) {
    if (bootPressed()) {
      colourMode = (colourMode + 1) % 4;
      Preferences p;
      p.begin("hwtest", false);
      p.putInt("colour", colourMode);
      p.end();
      applyColourMode();
      drawCorners();
      until = millis() + 12000;
    }
    delay(10);
  }
}

// ---------------------------------------------------------------- 2 colours
static void testColours() {
  header(2, "Colours");
  say(70, "Next the whole screen turns red, green,");
  say(100, "blue, white and black. Every dot should");
  say(130, "light, with no stripes or dark patches.");
  delay(3500);
  struct { uint16_t c; const char* n; } fills[] = {
      {TFT_RED, "red"}, {TFT_GREEN, "green"}, {TFT_BLUE, "blue"}, {TFT_WHITE, "white"}, {TFT_BLACK, "black"}};
  for (auto& f : fills) {
    Serial.printf("   all %s\n", f.n);
    lcd.fillScreen(f.c);
    delay(1500);
  }
  // smooth bars: no banding, no gaps
  Serial.println("   colour bars: smooth fades, top to bottom red, green, blue, grey");
  for (int x = 0; x < 480; x++) {
    uint8_t v = x * 255 / 479;
    lcd.drawFastVLine(x, 0, 80, lcd.color565(v, 0, 0));
    lcd.drawFastVLine(x, 80, 80, lcd.color565(0, v, 0));
    lcd.drawFastVLine(x, 160, 80, lcd.color565(0, 0, v));
    lcd.drawFastVLine(x, 240, 80, lcd.color565(v, v, v));
  }
  delay(4000);
}

// ------------------------------------------------------------------ 3 light
static void testLight() {
  header(3, "Backlight");
  say(70, "The screen dims down step by step,");
  say(100, "then comes back up to full.");
  lcd.fillRect(40, 170, 400, 100, TFT_WHITE);
  delay(2000);
  static const uint8_t steps[] = {255, 180, 120, 70, 35, 15, 35, 70, 120, 180, 255};
  for (uint8_t b : steps) {
    lcd.setBrightness(b);
    lcd.fillRect(40, 280, 400, 30, TFT_BLACK);
    char t[24];
    snprintf(t, sizeof(t), "Brightness %d%%", b * 100 / 255);
    lcd.setFont(&fonts::FreeSans12pt7b);
    lcd.setTextColor(TFT_YELLOW, TFT_BLACK);
    lcd.setTextDatum(top_center);
    lcd.drawString(t, 240, 282);
    Serial.printf("   %s\n", t);
    delay(900);
  }
}

// ------------------------------------------------------------------ 4 touch
static void testTouch() {
  header(4, "Touch");
  int y = say(70, "Tap the middle of each arrow as it");
  y = say(y, "shows up. Press firmly with a fingertip");
  y = say(y, "or a fingernail. Starts in 4 seconds.");
  delay(4000);
  uint16_t cal[8];
  lcd.calibrateTouch(cal, TFT_WHITE, TFT_BLACK, 30);
  Serial.printf("   calibration: %u %u %u %u %u %u %u %u\n", cal[0], cal[1], cal[2], cal[3], cal[4], cal[5], cal[6],
                cal[7]);

  // draw pad: dots follow the finger; each corner box lights when tapped
  header(4, "Touch");
  say(60, "Draw with a finger. Dots should land");
  say(90, "right under it. Tap each corner box.");
  struct Box { int x, y; bool hit; } boxes[] = {{4, 130, false}, {396, 130, false}, {4, 236, false}, {396, 236, false}};
  for (auto& b : boxes) lcd.drawRoundRect(b.x, b.y, 80, 80, 10, GREY);
  lcd.fillRoundRect(190, 250, 100, 60, 10, TFT_DARKGREEN);
  lcd.setFont(&fonts::FreeSansBold12pt7b);
  lcd.setTextColor(TFT_WHITE, TFT_DARKGREEN);
  lcd.setTextDatum(middle_center);
  lcd.drawString("DONE", 240, 280);
  uint32_t lastTouch = millis();
  int32_t x, y2;
  while (millis() - lastTouch < 60000) {
    if (bootPressed()) break;
    if (lcd.getTouch(&x, &y2)) {
      lastTouch = millis();
      if (x >= 190 && x < 290 && y2 >= 250 && y2 < 310) {
        Serial.println("   DONE tapped");
        break;
      }
      lcd.fillCircle(x, y2, 3, TFT_YELLOW);
      for (auto& b : boxes)
        if (!b.hit && x >= b.x && x < b.x + 80 && y2 >= b.y && y2 < b.y + 80) {
          b.hit = true;
          lcd.fillRoundRect(b.x, b.y, 80, 80, 10, TFT_GREEN);
          Serial.printf("   corner box at %d,%d hit\n", b.x, b.y);
        }
      static uint32_t lastPrint = 0;
      if (millis() - lastPrint > 300) {
        lastPrint = millis();
        Serial.printf("   touch at %d,%d\n", (int)x, (int)y2);
      }
    }
    delay(5);
  }
}

// -------------------------------------------------------------------- 5 LED
static void testLed() {
  header(5, "LED");
  int y = say(70, "Look at the little LED on the board.");
  y = say(y, "It should match the word below.");
  struct { bool r, g, b; uint16_t c; const char* n; } seq[] = {
      {true, false, false, TFT_RED, "RED"}, {false, true, false, TFT_GREEN, "GREEN"},
      {false, false, true, TFT_BLUE, "BLUE"}, {false, false, false, GREY, "OFF"}};
  lcd.setFont(&fonts::FreeSansBold24pt7b);
  lcd.setTextDatum(middle_center);
  for (auto& s : seq) {
    led(s.r, s.g, s.b);
    lcd.fillRect(0, 170, 480, 100, TFT_BLACK);
    lcd.setTextColor(s.c, TFT_BLACK);
    lcd.drawString(s.n, 240, 220);
    Serial.printf("   LED %s\n", s.n);
    delay(2000);
  }
  led(false, false, false);
}

// ---------------------------------------------------------------- 6 speaker
// three beeps on the DAC pin, a square wave (works on every core version)
static void beeps(int ampLevel) {
  digitalWrite(PIN_AMP, ampLevel);
  delay(50);
  for (int n = 0; n < 3; n++) {
    uint32_t end = micros() + 250000;
    while ((int32_t)(micros() - end) < 0) {
      dacWrite(PIN_DAC, 160);
      delayMicroseconds(568);   // about 880 Hz, quiet-ish
      dacWrite(PIN_DAC, 96);
      delayMicroseconds(568);
    }
    dacWrite(PIN_DAC, 128);
    delay(250);
  }
  digitalWrite(PIN_AMP, ampLevel ? LOW : HIGH);   // amp off again
}

static void testSpeaker() {
  header(6, "Speaker");
  int y = say(70, "Only with the speaker plugged in.");
  y = say(y, "Two rounds of three beeps. Which round");
  y = say(y, "did you hear: 1, 2, or both?");
  delay(2500);
  lcd.setFont(&fonts::FreeSansBold24pt7b);
  lcd.setTextDatum(middle_center);
  lcd.setTextColor(TFT_YELLOW, TFT_BLACK);
  lcd.fillRect(0, 170, 480, 100, TFT_BLACK);
  lcd.drawString("ROUND 1", 240, 230);
  Serial.println("   round 1 (amp pin LOW)");
  beeps(LOW);
  delay(800);
  lcd.fillRect(0, 170, 480, 100, TFT_BLACK);
  lcd.drawString("ROUND 2", 240, 230);
  Serial.println("   round 2 (amp pin HIGH)");
  beeps(HIGH);
  delay(800);
  dacWrite(PIN_DAC, 0);
}

// ---------------------------------------------------------------- 7 battery
static void testBattery() {
  header(7, "Battery");
  say(70, "What the board reads on its battery pin.");
  say(100, "No battery plugged in: any number is fine.", GREY);
  say(130, "With a battery: between 3.3 and 4.2 volts.", GREY);
  analogSetPinAttenuation(PIN_BAT, ADC_11db);
  lcd.setFont(&fonts::FreeSansBold24pt7b);
  lcd.setTextDatum(middle_center);
  for (int i = 0; i < 8; i++) {
    uint32_t mv = 0;
    for (int k = 0; k < 16; k++) mv += analogReadMilliVolts(PIN_BAT);
    mv /= 16;
    char t[40];
    snprintf(t, sizeof(t), "%.2f V", mv * 2 / 1000.0);
    lcd.fillRect(0, 190, 480, 70, TFT_BLACK);
    lcd.setTextColor(TFT_GREEN, TFT_BLACK);
    lcd.drawString(t, 240, 225);
    lcd.setFont(&fonts::FreeSans9pt7b);
    lcd.setTextColor(GREY, TFT_BLACK);
    char raw[40];
    snprintf(raw, sizeof(raw), "pin reads %u mV", (unsigned)mv);
    lcd.fillRect(0, 270, 480, 30, TFT_BLACK);
    lcd.drawString(raw, 240, 285);
    lcd.setFont(&fonts::FreeSansBold24pt7b);
    Serial.printf("   battery %s (%s)\n", t, raw);
    delay(1000);
  }
}

// ------------------------------------------------------------------- 8 wifi
static void testWifi() {
  header(8, "Wi-Fi");
  say(70, "Listening for Wi-Fi networks...");
  WiFi.mode(WIFI_STA);
  int n = WiFi.scanNetworks();
  char t[48];
  snprintf(t, sizeof(t), "Found %d network%s", n, n == 1 ? "" : "s");
  int y = say(110, t, n > 0 ? TFT_GREEN : TFT_RED);
  int strongest = -200;
  for (int i = 0; i < n; i++) if (WiFi.RSSI(i) > strongest) strongest = WiFi.RSSI(i);
  if (n > 0) {
    const char* how = strongest > -60 ? "strong" : strongest > -75 ? "good" : "weak";
    snprintf(t, sizeof(t), "Strongest signal: %s (%d)", how, strongest);
    y = say(y, t);
  }
  WiFi.scanDelete();
  WiFi.mode(WIFI_OFF);
  y = say(y + 20, "All done. Starting over in 10 seconds.", GREY);
  delay(10000);
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\nMini Scoreboard hardware test");
  pinMode(PIN_BOOT, INPUT_PULLUP);
  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT);
  pinMode(PIN_LED_B, OUTPUT);
  led(false, false, false);
  pinMode(PIN_AMP, OUTPUT);
  digitalWrite(PIN_AMP, HIGH);
  Preferences p;
  p.begin("hwtest", true);
  colourMode = p.getInt("colour", 0) & 3;
  p.end();
  applyColourMode();
  lcd.setBrightness(255);
}

void loop() {
  testCorners();
  testColours();
  testLight();
  testTouch();
  testLed();
  testSpeaker();
  testBattery();
  testWifi();
}
