#include "mn_touch.h"
#include "mn_lcd.h"
#include "mn_settings.h"
#include "mn_log.h"

static bool down = false;
static int downX, downY;
static uint32_t lastSeen = 0, lastActivity = 0;

void touchBegin() {
  if (settings.hasCal) lcd.setTouchCalibrate(settings.tcal);
}

bool touchDown() { return down; }
uint32_t touchLastActivity() { return lastActivity; }

bool touchPoll(int& x, int& y) {
  int32_t tx, ty;
  bool now = lcd.getTouch(&tx, &ty);
  uint32_t ms = millis();
  if (now) {
    lastSeen = ms;
    lastActivity = ms;
    if (!down) { down = true; downX = tx; downY = ty; }
    return false;
  }
  // a short gap is the screen wobbling, not a lift
  if (down && ms - lastSeen > 60) {
    down = false;
    x = downX;
    y = downY;
    return true;
  }
  return false;
}

static void centred(FontId f, const char* s, int y, uint16_t col) {
  useFont(f);
  lcd.setTextColor(col, C_BG);
  lcd.setTextDatum(middle_center);
  lcd.drawString(s, SCREEN_W / 2, y);
}

void touchCalibrate() {
  for (;;) {
    lcd.fillScreen(C_BG);
    centred(F_B24, "Touch setup", 90, C_WHITE);
    centred(F_M15, "An arrow shows up in each corner.", 140, C_GREY);
    centred(F_M15, "Press the tip of each one firmly", 166, C_GREY);
    centred(F_M15, "with a fingertip or fingernail.", 192, C_GREY);
    centred(F_B16, "Starting in a moment...", 250, C_YELLOW);
    delay(4000);
    lcd.fillScreen(C_BG);
    centred(F_B18, "Press each arrow's tip", SCREEN_H / 2, C_GREY);
    uint16_t cal[8];
    lcd.calibrateTouch(cal, C_WHITE, C_BG, 28);
    mnLog("touch setup: %u %u %u %u %u %u %u %u", cal[0], cal[1], cal[2], cal[3], cal[4], cal[5], cal[6], cal[7]);

    // check: tap the box; it has to land within it
    lcd.fillScreen(C_BG);
    centred(F_B24, "Now tap the green box", 60, C_WHITE);
    const int bx = 190, by = 150, bw = 100, bh = 80;
    lcd.fillRoundRect(bx, by, bw, bh, 14, C_GREEN);
    int x = -1, y = -1;
    uint32_t t0 = millis();
    bool ok = false, tapped = false;
    while (millis() - t0 < 30000) {
      if (touchPoll(x, y)) { tapped = true; ok = x >= bx - 20 && x < bx + bw + 20 && y >= by - 20 && y < by + bh + 20; break; }
      delay(10);
    }
    mnLog("touch check: %s at %d,%d", ok ? "hit" : tapped ? "missed" : "no tap", x, y);
    if (ok) {
      settings.saveCal(cal);
      lcd.fillScreen(C_BG);
      centred(F_B24, "Touch is set", 140, C_GREEN);
      delay(1200);
      return;
    }
    lcd.fillScreen(C_BG);
    centred(F_B24, "That missed the box", 130, C_RED);
    centred(F_M15, "Let's do the arrows again.", 175, C_GREY);
    delay(2500);
  }
}
