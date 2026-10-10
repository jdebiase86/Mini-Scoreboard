// The score animations. Everything is drawn straight to the screen with shapes (rays, tape, a ball on an arc,
// confetti): no big buffers, no pictures but the team logos. A scene is laid down once (and again halfway, turned a
// little), and the little moving parts are drawn over it a step at a time.
#include "mn_fx.h"
#include "mn_lcd.h"
#include "mn_ui.h"
#include "mn_logo.h"
#include <math.h>

static FxSpec spec;
static bool active = false;
static uint32_t t0 = 0, lastStep = 0, seed = 1;
static int stage = -1;          // which part of the scene has been drawn

static uint32_t rnd() { seed = seed * 1664525u + 1013904223u; return seed >> 8; }
static int rnd(int n) { return n > 0 ? (int)(rnd() % (uint32_t)n) : 0; }

// ------------------------------------------------------------------ colours
static uint16_t from(uint32_t c, uint16_t dflt, bool lift) {
  if (!c) return dflt;
  int r = (c >> 16) & 255, g = (c >> 8) & 255, b = c & 255;
  if (lift && r + g + b < 200) { r += (255 - r) / 3; g += (255 - g) / 3; b += (255 - b) / 3; }
  return rgb(r, g, b);
}
static uint16_t shade(uint32_t c, uint16_t dflt, int pct) {
  if (!c) return dflt;
  int r = (c >> 16) & 255, g = (c >> 8) & 255, b = c & 255;
  return rgb(r * pct / 100, g * pct / 100, b * pct / 100);
}
static uint16_t mineCol() { return from(spec.mine.color, rgb(30, 80, 170), true); }
static uint16_t mineDark() { return shade(spec.mine.color, rgb(12, 30, 80), 40); }
static uint16_t themCol() { return from(spec.them.color, rgb(190, 40, 50), true); }
static const uint16_t BLK = 0x0000;
static uint16_t amber() { return rgb(250, 200, 20); }

// ------------------------------------------------------------------ pieces
static const int CX = 240;

static void rays(uint16_t c1, uint16_t c2, int cy, int rot) {
  lcd.fillScreen(c2);
  const float R = 560.0f;
  for (int k = 0; k < 12; k++) {
    float a0 = (rot + k * 30) * 0.0174533f, a1 = (rot + k * 30 + 15) * 0.0174533f;
    lcd.fillTriangle(CX, cy, CX + (int)(R * cosf(a0)), cy + (int)(R * sinf(a0)), CX + (int)(R * cosf(a1)), cy + (int)(R * sinf(a1)), c1);
  }
}

static void stripes(uint16_t c1, uint16_t c2, int shift) {
  lcd.fillScreen(c2);
  for (int k = -8; k < 24; k += 2) {
    int x = k * 34 + shift;
    lcd.fillTriangle(x, 0, x + 34, 0, x + 34 - 190, 320, c1);
    lcd.fillTriangle(x, 0, x - 190, 320, x + 34 - 190, 320, c1);
  }
}

static void bricks(uint16_t c1, uint16_t c2) {
  lcd.fillScreen(c2);
  for (int r = 0; r < 12; r++) {
    int off = (r % 2) * 30;
    for (int x = -60 + off; x < 480; x += 60) lcd.fillRect(x + 2, r * 27 + 2, 56, 23, ((r + x / 60) % 3) ? c1 : shade(0x2860c0, c1, 100));
  }
}

static void tapeEdge(int y) {
  lcd.fillRect(0, y, 480, 10, amber());
  for (int x = -10; x < 490; x += 20) lcd.fillTriangle(x, y + 10, x + 10, y, x + 18, y, BLK), lcd.fillTriangle(x, y + 10, x + 18, y, x + 8, y + 10, BLK);
}

static void tapeScene(bool warnSign) {
  lcd.fillScreen(rgb(14, 14, 18));
  lcd.fillRect(0, 108, 480, 104, rgb(16, 16, 18));
  tapeEdge(98);
  tapeEdge(212);
  if (warnSign) {
    lcd.fillTriangle(62, 122, 100, 186, 24, 186, amber());
    uiText(F_B36, "!", 62, 164, rgb(16, 16, 18), amber(), middle_center);
  }
}

static void octagon(int cx, int cy, int r, uint16_t col) {
  for (int k = 0; k < 8; k++) {
    float a0 = (22.5f + k * 45) * 0.0174533f, a1 = (22.5f + (k + 1) * 45) * 0.0174533f;
    lcd.fillTriangle(cx, cy, cx + (int)(r * cosf(a0)), cy + (int)(r * sinf(a0)), cx + (int)(r * cosf(a1)), cy + (int)(r * sinf(a1)), col);
  }
}

static void starburst(int cx, int cy, uint16_t col) {
  for (int k = 0; k < 16; k++) {
    float a0 = (k * 22.5f) * 0.0174533f, am = (k * 22.5f + 11.25f) * 0.0174533f, a1 = ((k + 1) * 22.5f) * 0.0174533f;
    int r1 = 128, r2 = 92;
    lcd.fillTriangle(cx, cy, cx + (int)(r1 * cosf(a0)), cy + (int)(r1 * sinf(a0)), cx + (int)(r2 * cosf(am)), cy + (int)(r2 * sinf(am)), col);
    lcd.fillTriangle(cx, cy, cx + (int)(r2 * cosf(am)), cy + (int)(r2 * sinf(am)), cx + (int)(r1 * cosf(a1)), cy + (int)(r1 * sinf(a1)), col);
  }
}

static void football(int cx, int cy, int w) {
  int h = w * 6 / 10;
  lcd.fillEllipse(cx, cy, w / 2, h / 2, rgb(140, 80, 40));
  lcd.drawFastHLine(cx - w / 5, cy, w * 2 / 5, C_WHITE);
  for (int i = -2; i <= 2; i++) lcd.drawFastVLine(cx + i * w / 11, cy - 2, 5, C_WHITE);
}

// a logo on a dark disc with a white ring, so its soft edges never meet the rays
static void badge(const TeamSide& s, int cx, int cy, int size, bool bigDisc = true) {
  int r = size / 2 + 12;
  if (bigDisc) {
    lcd.fillCircle(cx, cy, r + 4, C_WHITE);
    lcd.fillCircle(cx, cy, r, rgb(12, 14, 22));
  }
  if (!logoDrawOver(s, cx, cy, size > 90 ? 112 : 76, rgb(12, 14, 22))) {
    uiText(F_B36, s.abbr, cx, cy, C_WHITE, rgb(12, 14, 22), middle_center);
  }
}

// the big word on a black plate, in the biggest font that fits
static void word(const char* s, int y, uint16_t col) {
  FontId f = F_B36;   // (the biggest font only has digits)
  useFont(f);
  if (lcd.textWidth(s) > 430) { f = F_B24; useFont(f); }
  int w = lcd.textWidth(s) + 40, h = f == F_B36 ? 54 : 40;
  lcd.fillRoundRect(CX - w / 2, y - h / 2, w, h, 14, BLK);
  uiText(f, s, CX, y, col, BLK, middle_center);
}

static void pill(const char* s, int y = 294, uint16_t col = 0) {
  if (!s[0]) return;
  if (!col) col = C_YELLOW;
  useFont(F_B16);
  int w = lcd.textWidth(s) + 40;
  if (w > 470) w = 470;
  lcd.fillRoundRect(CX - w / 2, y - 16, w, 32, 16, BLK);
  uiText(F_B16, s, CX, y, col, BLK, middle_center);
}

static void sparkle(int n, uint16_t a, uint16_t b) {
  for (int i = 0; i < n; i++) {
    int x = rnd(480), y = rnd(250);
    if (x > 60 && x < 420 && y > 130 && y < 260) continue;   // not over the words
    lcd.fillRect(x, y, 4, 4, (i & 1) ? a : b);
  }
}

// ------------------------------------------------------------------ the field goal
static const int POST_X = 240, POST_Y = 150;
static int ballPx = -1, ballPy = 0, ballPw = 0;
static bool wide = false;
static int wideDir = 1;

static uint16_t turfCol(int i) { return (i & 1) ? rgb(36, 118, 56) : rgb(28, 98, 46); }
static void turf(int x, int y, int w, int h) {
  int y1 = y + h;
  if (y < 120) {   // the dark sky above the field
    int e = y1 < 120 ? y1 : 120;
    lcd.fillRect(x, y, w, e - y, rgb(8, 22, 14));
  }
  float top = 120;
  for (int i = 0; i < 14; i++) {
    int sy = (int)top, sh = 12 + i;
    top += sh + 2;
    int a = sy > y ? sy : y, b = sy + sh + 2 < y1 ? sy + sh + 2 : y1;
    if (b > a) lcd.fillRect(x, a, w, b - a, turfCol(i));
  }
}
static void posts(int spread, int height, int thick, uint16_t col) {
  lcd.fillRect(POST_X - thick / 2, POST_Y, thick, height / 2, col);
  lcd.fillRect(POST_X - spread, POST_Y - thick / 2, spread * 2, thick, col);
  for (int s = -1; s <= 1; s += 2) lcd.fillRect(POST_X + s * spread - thick / 2, POST_Y - height, thick, height, col);
}
static void ballAt(int x, int y, int w) {
  if (ballPx >= 0) {
    turf(ballPx - ballPw, ballPy - ballPw, ballPw * 2 + 2, ballPw * 2 + 2);
    posts(40, 56, 3, C_YELLOW);
  }
  football(x, y, w);
  ballPx = x; ballPy = y; ballPw = w / 2 + 3;
}

static void fgBackdrop() {
  turf(0, 0, 480, 320);
  posts(40, 56, 3, C_YELLOW);
  lcd.fillRect(0, 0, 480, 52, BLK);
  uiText(F_B24, "FIELD GOAL TRY", 250, 20, C_YELLOW, BLK, middle_center);
  uiText(F_B16, spec.mine.abbr, 44, 26, C_WHITE, BLK, middle_center);
  ballPx = -1;
  football(CX, 262, 24);
  ballPx = CX; ballPy = 262; ballPw = 15;
}

// ------------------------------------------------------------------ lengths
uint32_t fxLength(FxKind k) {
  switch (k) {
    case FX_FIELDGOAL: return 6200;
    case FX_NOGOOD: return 5600;
    case FX_TOUCHDOWN: case FX_GOAL: case FX_HOMERUN: return 4600;
    case FX_WIN: return 5400;
    case FX_RUN: case FX_THREE: return 3400;
    case FX_KICKOFF: case FX_QUARTER: return 3200;
    case FX_FIRSTDOWN: return 2200;
    case FX_FLAG: return 3000;
    default: return 3600;
  }
}

bool fxActive() { return active; }
void fxStop() { active = false; }

void fxStart(const FxSpec& f) {
  spec = f;
  active = true;
  t0 = millis();
  lastStep = 0;
  stage = -1;
  seed = (uint32_t)millis() * 2654435761u + 12345;
  wide = f.kind == FX_NOGOOD;
  wideDir = rnd(2) ? 1 : -1;
  ballPx = -1;
}

// ------------------------------------------------------------------ the scenes
// stage 0: the scene; stage 1: the scene again, turned a little (halfway); the rest is drawn over them
static void scene(int phase, uint32_t t) {
  const int rot = phase * 7;
  switch (spec.kind) {
    case FX_TOUCHDOWN: case FX_GOAL: case FX_THREE: case FX_RUN: case FX_FIRSTDOWN: case FX_KICKOFF: {
      rays(mineCol(), mineDark(), 120, rot);
      if (spec.kind == FX_KICKOFF) {
        badge(spec.mine, 120, 118, 112); badge(spec.them, 360, 118, 112);
      } else {
        badge(spec.mine, CX, 120, 112);
      }
      if (spec.kind == FX_GOAL) {   // the goal lights
        for (int x : {28, 452}) { lcd.fillCircle(x, 54, 20, rgb(255, 40, 40)); lcd.fillCircle(x, 54, 8, rgb(255, 190, 190)); }
      }
      break;
    }
    case FX_HOMERUN: {
      rays(mineCol(), mineDark(), 300, rot);
      lcd.fillRect(0, 196, 480, 124, rgb(30, 100, 48)); lcd.fillRect(0, 150, 480, 46, rgb(18, 56, 40)); lcd.fillRect(0, 194, 480, 6, C_YELLOW);
      badge(spec.mine, CX, 82, 76);
      break;
    }
    case FX_PICKED: stripes(rgb(30, 70, 150), mineDark(), phase * 10); badge(spec.mine, 260, 112, 112); break;
    case FX_FUMBLE: {
      rays(mineCol(), mineDark(), 120, rot);
      badge(spec.mine, CX, 112, 112);
      football(70, 60, 34); football(150, 170, 38); football(410, 70, 30); football(380, 180, 34);
      break;
    }
    case FX_SACK: {
      rays(mineCol(), mineDark(), 120, rot);
      starburst(CX, 120, C_YELLOW);
      logoDrawOver(spec.mine, CX, 120, 76, C_YELLOW);
      break;
    }
    case FX_STOPPED: {
      lcd.fillScreen(mineDark());
      octagon(CX, 118, 112, C_WHITE); octagon(CX, 118, 102, rgb(200, 30, 36));
      uiText(F_B36, "STOP", CX, 118, C_WHITE, rgb(200, 30, 36), middle_center);
      logoDrawOver(spec.mine, 66, 250, 76, mineDark());
      break;
    }
    case FX_STONEWALL: bricks(shade(spec.mine.color, rgb(20, 40, 110), 55), mineDark()); badge(spec.mine, CX, 112, 112); break;
    case FX_PUNT: rays(shade(spec.mine.color, rgb(30, 70, 150), 85), mineDark(), 130, rot); badge(spec.mine, CX, 112, 112); break;
    case FX_WENTFORIT: {
      lcd.fillScreen(rgb(24, 120, 56));
      for (int x = 0; x < 480; x += 60) lcd.fillRect(x + (phase ? 30 : 0), 0, 30, 320, rgb(30, 140, 64));
      badge(spec.mine, CX, 112, 112);
      break;
    }
    case FX_NOPUNT: lcd.fillScreen(rgb(44, 46, 54)); badge(spec.mine, CX, 112, 112); break;
    case FX_QUARTER: {
      lcd.fillScreen(rgb(14, 16, 24));
      lcd.fillRect(0, 0, 480, 6, mineCol()); lcd.fillRect(0, 314, 480, 6, mineCol());
      badge(spec.mine, 90, 118, 76, false); badge(spec.them, 390, 118, 76, false);
      char a[8]; snprintf(a, sizeof(a), "%d", (int)spec.mine.score);
      char b[8]; snprintf(b, sizeof(b), "%d", (int)spec.them.score);
      uiText(F_B62, a, 190, 118, C_WHITE, rgb(14, 16, 24), middle_center);
      uiText(F_B62, b, 290, 118, C_WHITE, rgb(14, 16, 24), middle_center);
      break;
    }
    case FX_FLAG: {
      lcd.fillScreen(rgb(30, 30, 10));
      for (int x = -40; x < 520; x += 80) lcd.fillTriangle(x, 0, x + 40, 0, x + 120, 320, rgb(40, 40, 12));
      // a yellow flag on a stick
      lcd.fillRect(176, 40, 6, 150, C_WHITE);
      lcd.fillTriangle(182, 44, 300, 70, 182, 110, amber());
      lcd.fillTriangle(182, 110, 300, 70, 262, 120, amber());
      break;
    }
    case FX_WIN: {
      lcd.fillScreen(rgb(12, 22, 60));
      badge(spec.mine, CX, 112, 112);
      break;
    }
    case FX_THEIRSCORE: case FX_TURNOVER: case FX_NOGOOD: {
      tapeScene(true);
      break;
    }
    default: lcd.fillScreen(C_BG);
  }
  (void)t;
}

// words and the line under them, after the scene has landed
static void captions(uint32_t t) {
  switch (spec.kind) {
    case FX_THEIRSCORE: case FX_TURNOVER: case FX_NOGOOD: {
      uiText(F_B36, spec.word, 340, 142, amber(), rgb(16, 16, 18), middle_center);
      if (!logoDrawOver(spec.them, 168, 160, 76, rgb(16, 16, 18))) uiText(F_B24, spec.them.abbr, 168, 160, C_WHITE, rgb(16, 16, 18), middle_center);
      uiText(F_B16, spec.sub, 340, 189, C_WHITE, rgb(16, 16, 18), middle_center);
      return;
    }
    case FX_FLAG: word("FLAG", 220, amber()); pill(spec.sub, 294); return;
    case FX_QUARTER: word(spec.word, 220, C_WHITE); pill(spec.sub, 294); return;
    case FX_STOPPED: word(spec.word, 250, C_WHITE); pill(spec.sub, 294); return;
    case FX_NOPUNT: word(spec.word, 236, rgb(190, 192, 200)); pill(spec.sub, 294, C_WHITE); return;
    case FX_FIRSTDOWN: word(spec.word, 238, C_WHITE); pill(spec.sub, 294); return;
    default: break;
  }
  word(spec.word, spec.kind == FX_HOMERUN ? 244 : 238, C_WHITE);
  pill(spec.sub, 294);
  (void)t;
}

// the home run's ball, along its arc, and the win's confetti, and the little sparks
struct Bit { int16_t x, y, vy; uint16_t col; uint8_t w, h; };
static Bit bits[56];
static int nbits = 0;

static void confettiInit() {
  static const uint16_t cols[] = {0xF800, 0xFFE0, 0x07FF, 0xF81F, 0xFFFF, 0x07E0, 0xFD20};
  nbits = 56;
  for (int i = 0; i < nbits; i++) {
    bits[i].x = rnd(2) ? rnd(130) : 350 + rnd(130);
    bits[i].y = -rnd(300);
    bits[i].vy = 3 + rnd(5);
    bits[i].col = cols[rnd(7)];
    bits[i].w = 4 + rnd(4); bits[i].h = 6 + rnd(6);
  }
}
static void confettiStep() {
  const uint16_t bg = rgb(12, 22, 60);
  for (int i = 0; i < nbits; i++) {
    Bit& b = bits[i];
    if (b.y >= 0) lcd.fillRect(b.x, b.y, b.w, b.h, bg);
    b.y += b.vy;
    b.x += rnd(3) - 1;
    if (b.y > 320) { b.y = -10; b.x = rnd(2) ? rnd(130) : 350 + rnd(130); }
    if (b.y >= 0) lcd.fillRect(b.x, b.y, b.w, b.h, b.col);
  }
}

static void overlay(uint32_t t) {
  switch (spec.kind) {
    case FX_TOUCHDOWN: case FX_GOAL: case FX_THREE: case FX_RUN: case FX_FUMBLE: case FX_PUNT: case FX_PICKED: case FX_SACK:
      sparkle(6, C_WHITE, C_YELLOW);
      break;
    case FX_HOMERUN: {
      // the ball over the fence: dots along an arc, one more each step
      float u = (float)(t < 4000 ? t : 4000) / 4000.0f;
      int x = 30 + (int)(u * 420), y = 236 - (int)(sinf(u * 3.14159f) * 190);
      lcd.fillCircle(x, y, 4, C_WHITE);
      if (u > 0.92f) football(x, y, 18);
      sparkle(3, C_WHITE, C_YELLOW);
      break;
    }
    case FX_WIN: confettiStep(); break;
    case FX_WENTFORIT: case FX_KICKOFF: sparkle(3, C_WHITE, C_YELLOW); break;
    default: break;
  }
}

// ------------------------------------------------------------------ the field goal's own sequence
static bool fgStep(uint32_t t) {
  const bool good = spec.kind == FX_FIELDGOAL;
  const uint32_t KICK0 = 700, KICK1 = 2900;
  if (t < KICK1 + 200) {
    if (stage < 0) { fgBackdrop(); stage = 0; }
    if (t >= KICK0) {
      float u = (float)(t - KICK0) / (KICK1 - KICK0);
      if (u > 1) u = 1;
      int x = CX + (good ? 0 : (int)(wideDir * u * u * 120));
      int y = 262 - (int)(u * 150) - (int)(sinf(u * 3.14159f) * 34);
      int w = 26 - (int)(u * 13);
      static int lastDot = 0;
      if (ballPx >= 0 && (int)((t - KICK0) / 220) != lastDot) { lastDot = (int)((t - KICK0) / 220); lcd.fillCircle(ballPx, ballPy, 2, C_WHITE); }
      ballAt(x, y, w);
      if (t < KICK0 + 100) lastDot = 0;
    }
    return true;
  }
  if (t < KICK1 + 320) {
    if (stage < 1) { lcd.fillScreen(C_WHITE); stage = 1; }   // the flash
    return true;
  }
  if (good) {
    if (stage < 2) {
      rays(mineCol(), mineDark(), 140, 0);
      posts(105, 130, 7, C_YELLOW);
      lcd.fillRect(CX - 105, POST_Y - 4, 210, 8, C_YELLOW);
      football(CX, 104, 32);
      stage = 2;
    }
    if (stage == 2 && t > KICK1 + 700) { word(spec.word, 214, C_WHITE); pill(spec.sub, 294); stage = 3; }
    sparkle(4, C_WHITE, C_YELLOW);
    return true;
  }
  if (stage < 2) { tapeScene(true); stage = 2; captions(t); }
  return true;
}

bool fxStep() {
  if (!active) return false;
  uint32_t t = millis() - t0;
  if (t >= fxLength(spec.kind)) { active = false; return false; }
  if (lastStep && millis() - lastStep < 70) return true;
  lastStep = millis();
  if (spec.kind == FX_FIELDGOAL || spec.kind == FX_NOGOOD) return fgStep(t);
  if (stage < 0) {
    scene(0, t);
    stage = 0;
    if (spec.kind == FX_WIN) confettiInit();
  } else if (stage == 0 && t >= 300) {
    captions(t);
    stage = 1;
  } else if (stage == 1 && t >= 2100 && spec.kind != FX_WIN && spec.kind != FX_HOMERUN && spec.kind != FX_THEIRSCORE &&
             spec.kind != FX_TURNOVER && spec.kind != FX_QUARTER && spec.kind != FX_FLAG && spec.kind != FX_NOPUNT) {
    scene(1, t);        // turned a little
    captions(t);
    stage = 2;
  }
  if (stage >= 1) overlay(t);
  return true;
}
