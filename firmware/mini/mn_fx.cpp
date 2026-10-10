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
static uint16_t stageCol = 0;
static int nsp = 0;
static uint32_t lastTick = 0, nxtA = 0, blkA = 0, nxtB = 0, blkB = 0, altG = 99;

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
    case FX_TOUCHDOWN: return 4600;
    case FX_GOAL: return 4800;
    case FX_HOMERUN: return 5200;
    case FX_THREE: return 4400;
    case FX_WIN: return 5600;
    case FX_RUN: return 3400;
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
  lastTick = 0; nxtA = nxtB = blkA = blkB = 0; altG = 99;
  nsp = 0;
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
    bits[i].x = rnd(2) ? rnd(84) : 396 + rnd(80);
    bits[i].y = -rnd(300);
    bits[i].vy = 3 + rnd(5);
    bits[i].col = cols[rnd(7)];
    bits[i].w = 4 + rnd(4); bits[i].h = 6 + rnd(6);
  }
}
static void confettiStep() {
  const uint16_t bg = stageCol ? stageCol : rgb(12, 22, 60);
  for (int i = 0; i < nbits; i++) {
    Bit& b = bits[i];
    if (b.y >= 0) lcd.fillRect(b.x, b.y, b.w, b.h, bg);
    b.y += b.vy;
    b.x += rnd(3) - 1;
    if (b.y > 320) { b.y = -10; b.x = rnd(2) ? rnd(84) : 396 + rnd(80); }
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


// ================================================================== the big moments (after the LED board's animations)
// A dark stage, fireworks that fade as they fall, shock-wave rings, white flashes, and words that blink between the team's
// colour and white. Everything is a function of the time since the start, drawn in small pieces over a flat colour (so
// what moves can be rubbed out cleanly): nothing is redrawn whole.
static int pX0 = 0, pY0 = 0, pX1 = 0, pY1 = 0;     // sparks stay out of this box (the logo and words)
struct Spark { float x, y, vx, vy, life, decay; uint8_t r, g, b; int16_t ex, ey; uint8_t ew, eh; bool drawn; };
static Spark sp[90];
struct Ring { float cx, cy, t0, speed, maxr, lastR; uint8_t r, g, b; bool used; };
static Ring rg[4];

static void rgbOf(uint32_t c, uint8_t& r, uint8_t& g, uint8_t& b, bool lift) {
  r = (c >> 16) & 255; g = (c >> 8) & 255; b = c & 255;
  int m = r > g ? (r > b ? r : b) : (g > b ? g : b);
  if (!c) { r = g = b = 255; return; }
  if (lift && m < 150) { float f = 200.0f / (m ? m : 1); r = r * f > 255 ? 255 : (uint8_t)(r * f); g = g * f > 255 ? 255 : (uint8_t)(g * f); b = b * f > 255 ? 255 : (uint8_t)(b * f); }
}
static float frnd() { return (rnd() & 0xFFFF) / 65536.0f; }

static void paletteOf(uint8_t pal[5][3], int& n) {
  n = 0;
  rgbOf(spec.mine.color, pal[0][0], pal[0][1], pal[0][2], true); n = 1;
  static const uint8_t extra[4][3] = {{255, 215, 0}, {255, 255, 255}, {255, 110, 30}, {255, 255, 255}};
  for (int i = 0; i < 3; i++) { pal[n][0] = extra[i][0]; pal[n][1] = extra[i][1]; pal[n][2] = extra[i][2]; n++; }
}

static void blast(float cx, float cy, float power) {
  uint8_t pal[5][3]; int np; paletteOf(pal, np);
  const float cnt[3] = {30 * power, 22 * power, 14 * power}, spd[3] = {7.4f, 4.4f, 2.1f};
  for (int k = 0; k < 3; k++) {
    for (int i = 0; i < (int)cnt[k] && nsp < 90; i++) {
      // reuse a dead slot
      int slot = -1;
      for (int j = 0; j < nsp; j++) if (sp[j].life <= 0 && !sp[j].drawn) { slot = j; break; }
      if (slot < 0) slot = nsp++;
      Spark& p = sp[slot];
      float ang = frnd() * 6.2832f, s = spd[k] * (0.6f + frnd() * 0.8f);
      p.x = cx; p.y = cy; p.vx = cosf(ang) * s; p.vy = sinf(ang) * s; p.life = 1; p.decay = 0.0075f + frnd() * 0.0065f;
      const uint8_t* c = pal[rnd(np)]; p.r = c[0]; p.g = c[1]; p.b = c[2]; p.drawn = false;
    }
  }
}

// rubbing out a box, but never over the logo and words
static void eraseBox(int x, int y, int w, int h) {
  if (pX1 <= pX0 || x >= pX1 || x + w <= pX0 || y >= pY1 || y + h <= pY0) { lcd.fillRect(x, y, w, h, stageCol); return; }
  if (y < pY0) lcd.fillRect(x, y, w, pY0 - y, stageCol);
  if (y + h > pY1) lcd.fillRect(x, pY1, w, y + h - pY1, stageCol);
  int a = y > pY0 ? y : pY0, b = y + h < pY1 ? y + h : pY1;
  if (b > a) {
    if (x < pX0) lcd.fillRect(x, a, pX0 - x, b - a, stageCol);
    if (x + w > pX1) lcd.fillRect(pX1, a, x + w - pX1, b - a, stageCol);
  }
}

static void sparksStep(int ticks, bool gravity = true) {
  for (int j = 0; j < nsp; j++) {
    Spark& p = sp[j];
    if (p.drawn) { eraseBox(p.ex, p.ey, p.ew, p.eh); p.drawn = false; }
    if (p.life <= 0) continue;
    for (int t = 0; t < ticks; t++) {
      p.x += p.vx; p.y += p.vy; if (gravity) p.vy += 0.085f; p.vx *= 0.985f; p.life -= p.decay;
      if (p.life <= 0) break;
    }
    if (p.life <= 0) continue;
    float b = p.life > 1 ? 1 : p.life;
    int x = (int)p.x, y = (int)p.y;
    if (x < 2 || y < 2 || x > 476 || y > 316) continue;
    if (x > pX0 && x < pX1 && y > pY0 && y < pY1) continue;
    uint16_t c = rgb((uint8_t)(p.r * b), (uint8_t)(p.g * b), (uint8_t)(p.b * b));
    int tx = x - (int)(p.vx * 1.6f), ty = y - (int)(p.vy * 1.6f);
    if (b > 0.35f && !(tx > pX0 && tx < pX1 && ty > pY0 && ty < pY1)) {
      lcd.fillRect(tx, ty, 2, 2, rgb((uint8_t)(p.r * b * 0.4f), (uint8_t)(p.g * b * 0.4f), (uint8_t)(p.b * b * 0.4f)));
    }
    lcd.fillRect(x - 1, y - 1, 3, 3, c);
    int ex = (tx < x ? tx : x) - 2, ey = (ty < y ? ty : y) - 2;
    int ew = abs(tx - x) + 6, eh = abs(ty - y) + 6;
    p.ex = ex; p.ey = ey; p.ew = ew > 40 ? 40 : ew; p.eh = eh > 40 ? 40 : eh; p.drawn = true;
  }
}
static void sparksClear() {
  for (int j = 0; j < nsp; j++) { if (sp[j].drawn) eraseBox(sp[j].ex, sp[j].ey, sp[j].ew, sp[j].eh); sp[j].drawn = false; sp[j].life = 0; }
  nsp = 0;
}

static void ringsAdd(float cx, float cy, float t0, float speed, float maxr, uint8_t r, uint8_t g, uint8_t b) {
  for (Ring& q : rg) if (!q.used) { q = Ring{cx, cy, t0, speed, maxr, 0, r, g, b, true}; return; }
}
static void ringsStep(float t, bool eraseOnly = false) {
  for (Ring& q : rg) {
    if (!q.used) continue;
    if (q.lastR > 1) { lcd.drawCircle((int)q.cx, (int)q.cy, (int)q.lastR, stageCol); lcd.drawCircle((int)q.cx, (int)q.cy, (int)q.lastR - 1, stageCol); lcd.drawCircle((int)q.cx, (int)q.cy, (int)q.lastR + 1, stageCol); }
    float r = (t - q.t0) * q.speed;
    if (eraseOnly || r > q.maxr) { q.used = false; continue; }
    if (r < 2) { q.lastR = 0; continue; }
    float b = 1 - r / q.maxr;
    uint16_t c = rgb((uint8_t)(q.r * b), (uint8_t)(q.g * b), (uint8_t)(q.b * b));
    lcd.drawCircle((int)q.cx, (int)q.cy, (int)r, c); lcd.drawCircle((int)q.cx, (int)q.cy, (int)r - 1, c);
    q.lastR = r;
  }
}

static void bigWord(const char* s, int y, bool alt) { word(s, y, alt ? C_WHITE : (spec.tape ? amber() : mineCol())); }

// the settled end: the logo, the word blinking between white and the team's colour, sparks around them
static void finale(const char* w, const char* sub) {
  lcd.fillScreen(stageCol);
  if (!logoDraw(spec.mine, CX, 112, 112, stageCol)) uiText(F_B36, spec.mine.abbr, CX, 112, C_WHITE, stageCol, middle_center);
  bigWord(w, 232, false);
  pill(sub, 292);
  pX0 = 90; pX1 = 390; pY0 = 36; pY1 = 312;
}

// ---- scene pieces for the home run and the three
static uint16_t crowdDot(int i) { static const uint16_t c[] = {0xF800, 0xFFE0, 0x07FF, 0xFFFF, 0xFD20, 0x07E0, 0xF81F}; return c[i % 7]; }
static void park(int x, int y, int w, int h) {   // the night ballpark, any part of it
  int y1 = y + h;
  auto band = [&](int a, int b, uint16_t col) { int s = a > y ? a : y, e = b < y1 ? b : y1; if (e > s) lcd.fillRect(x, s, w, e - s, col); };
  band(0, 70, rgb(8, 12, 34)); band(70, 120, rgb(14, 20, 52));          // the sky
  band(120, 168, rgb(10, 14, 40));                                      // the crowd
  band(168, 200, rgb(20, 90, 50)); band(198, 204, C_YELLOW);            // the wall and its yellow line
  band(204, 240, rgb(30, 110, 56)); band(240, 270, rgb(26, 98, 48)); band(270, 320, rgb(34, 118, 60));
}
static void parkDetails() {   // lights on the roof line and the crowd, laid down once
  for (int k = 0; k < 9; k++) { int lx = 30 + k * 52; lcd.fillRect(lx, 22, 14, 6, rgb(255, 250, 200)); lcd.fillRect(lx + 2, 28, 10, 2, rgb(160, 150, 100)); }
  for (int i = 0; i < 90; i++) { int cx = 8 + (i * 37) % 464, cy = 126 + (i * 53) % 40; lcd.fillRect(cx, cy, 4, 6, crowdDot(i * 3)); }
}
static void batter(uint16_t jersey, bool swing) {
  lcd.fillRect(100, 252, 12, 50, jersey);          // body
  lcd.fillCircle(106, 244, 8, rgb(240, 190, 150)); // head
  lcd.fillRect(98, 238, 16, 5, jersey);            // cap
  if (swing) lcd.drawWideLine(112, 258, 150, 232, 3, C_WHITE); else lcd.drawWideLine(108, 252, 124, 226, 3, C_WHITE);
}

static void courtBackdrop() {
  lcd.fillScreen(rgb(120, 78, 40));
  for (int x = 0; x < 480; x += 48) lcd.fillRect(x, 0, 2, 320, rgb(104, 66, 34));
  lcd.fillRect(0, 286, 480, 34, rgb(86, 54, 28));
  // the three-point line
  lcd.drawArc(420, 250, 250, 252, 90, 270, C_WHITE);
  // backboard, rim and net
  lcd.fillRect(430, 76, 8, 90, C_WHITE);
  lcd.fillRect(392, 150, 40, 5, rgb(255, 110, 20));
  for (int k = 0; k < 6; k++) lcd.drawLine(394 + k * 7, 155, 398 + k * 5, 188, C_WHITE);
  lcd.drawLine(398, 188, 428, 188, C_WHITE);
}

// ---- one step of a big scene; true = still playing
static bool bigStep(uint32_t t) {
  const uint32_t TICK_MS = 17;
  int ticks = lastTick ? (int)((millis() - lastTick) / TICK_MS) : 1;
  if (ticks < 1) ticks = 1;
  if (ticks > 6) ticks = 6;
  lastTick = millis();
  uint8_t pal[5][3]; int np; paletteOf(pal, np);
  const uint16_t tm = mineCol();
  switch (spec.kind) {
    case FX_FIELDGOAL: case FX_NOGOOD: {
      const bool good = spec.kind == FX_FIELDGOAL;
      const uint32_t K0 = 700, K1 = 2400;
      const uint16_t GROUND = rgb(24, 96, 46);
      static int tx[10], ty[10], tr[10], ntr = 0;
      auto posts = [&]() {
        lcd.fillRect(399, 214, 5, 68, C_YELLOW);
        lcd.fillRect(350, 170, 105, 5, C_YELLOW);
        lcd.fillRect(350, 64, 5, 111, C_YELLOW); lcd.fillRect(450, 64, 5, 111, C_YELLOW);
      };
      if (stage < 0) {
        stageCol = rgb(5, 9, 8); lcd.fillScreen(stageCol);
        lcd.fillRect(0, 282, 480, 38, GROUND); lcd.fillRect(0, 282, 480, 3, rgb(90, 170, 100));
        posts();
        uiText(F_B24, "FIELD GOAL TRY", CX, 26, C_YELLOW, stageCol, middle_center);
        ntr = 0; nsp = 0; pX0 = pX1 = pY0 = pY1 = 0; stage = 0;
        wideDir = rnd(2) ? 1 : -1;
      }
      if (t < K1 + 50) {
        for (int i = 0; i < ntr; i++) lcd.fillCircle(tx[i], ty[i], tr[i] + 3, ty[i] > 276 ? GROUND : stageCol);
        ntr = 0;
        lcd.fillRect(0, 282, 480, 38, GROUND); lcd.fillRect(0, 282, 480, 3, rgb(90, 170, 100));
        posts();
        if (t >= K0) {
          float u = (float)(t - K0) / (K1 - K0); if (u > 1) u = 1;
          int endx = good ? 402 : (wideDir > 0 ? 492 : 318), endy = good ? 112 : 104;
          auto pos = [&](float uu, int& x, int& y) { x = 62 + (int)(uu * (endx - 62)); y = 270 - (int)(uu * (270 - endy)) - (int)(sinf(uu * 3.14159f) * 120); };
          for (int k = 7; k >= 1; k--) {
            float uu = u - k * 0.035f; if (uu < 0) continue;
            int x, y; pos(uu, x, y);
            lcd.fillCircle(x, y, 2, rgb((uint8_t)(200 - k * 22), (uint8_t)(110 - k * 12), (uint8_t)(40)));
            if (ntr < 10) { tx[ntr] = x; ty[ntr] = y; tr[ntr] = 2; ntr++; }
          }
          int x, y; pos(u, x, y);
          int w = 36 - (int)(u * 16), h = w * 6 / 10;
          lcd.fillEllipse(x, y, w / 2, h / 2, rgb(150, 84, 40));
          lcd.drawLine(x - w / 5, y, x + w / 5, y, C_WHITE);
          if (ntr < 10) { tx[ntr] = x; ty[ntr] = y; tr[ntr] = w / 2; ntr++; }
        }
        return true;
      }
      if (stage == 0) {
        stage = 1;
        for (int i = 0; i < ntr; i++) lcd.fillCircle(tx[i], ty[i], tr[i] + 3, ty[i] > 276 ? GROUND : stageCol);
        ntr = 0;
        if (good) lcd.fillRect(356, 66, 94, 104, C_WHITE);   // the flash between the posts
      }
      if (stage == 1 && t >= K1 + 220) {
        stage = 2;
        if (good) { stageCol = rgb(5, 9, 8); blast(402, 120, 1.4f); finale(spec.word, spec.sub); }
        else { tapeScene(true); captions(t); }
      }
      if (stage >= 2 && good) {
        if (t / 130 != blkA) { blkA = t / 130; bigWord(spec.word, 232, blkA % 2); }
        if (t > nxtA && t < 5000) { blast(60 + frnd() * 380, 30 + frnd() * 160, 0.9f); nxtA = t + 260 + (uint32_t)(frnd() * 200); }
        sparksStep(ticks);
      }
      return true;
    }
    case FX_TOUCHDOWN: {
      if (stage < 0) {
        stageCol = rgb(4, 6, 12); lcd.fillScreen(C_WHITE); stage = 0; nsp = 0;
        for (Ring& q : rg) q.used = false;
        pX0 = pX1 = pY0 = pY1 = 0;
      }
      if (stage == 0 && t >= 120) {
        lcd.fillScreen(stageCol);
        blast(CX, 130, 1.3f);
        ringsAdd(CX, 130, (float)t, 0.42f, 420, pal[0][0], pal[0][1], pal[0][2]);
        ringsAdd(CX, 130, (float)t + 110, 0.31f, 380, pal[1][0], pal[1][1], pal[1][2]);
        ringsAdd(CX, 130, (float)t + 230, 0.24f, 330, 255, 255, 255);
        stage = 1;
      }
      if (stage == 1) {
        if (t < 640) ringsStep((float)t);
        else { ringsStep((float)t, true); stage = 2; finale(spec.word, spec.sub); }
      }
      if (stage >= 2) {
        if (t > nxtA && t < 3900) { blast(60 + frnd() * 360, 30 + frnd() * 250, 1.05f); nxtA = t + 240 + (uint32_t)(frnd() * 200); }
        if (t / 130 != blkA) { blkA = t / 130; bigWord(spec.word, 232, blkA % 2); }
      }
      if (stage >= 1) sparksStep(ticks);
      return true;
    }
    case FX_GOAL: {
      const float R = 255;
      if (t < 2000) {
        float ph = fmodf((float)t, 250.0f) / 250.0f;
        float wash = 1 - fabsf(ph - 0.5f) * 2.4f; if (wash < 0) wash = 0;
        lcd.fillScreen(rgb((uint8_t)(R * (0.08f + 0.45f * wash)), (uint8_t)(18 * (0.08f + 0.45f * wash)), (uint8_t)(18 * (0.08f + 0.45f * wash))));
        // two beams turning round the lamp at the top
        float a0 = ((float)t / 250.0f) * 6.2832f;
        for (int k = 0; k < 2; k++) {
          float a = a0 + k * 3.14159f;
          lcd.drawWideLine(CX, 24, CX + (int)(cosf(a) * 520), 24 + (int)(fabsf(sinf(a)) * 470), 5, rgb(255, 90, 60));
        }
        lcd.fillCircle(CX, 24, 22, rgb((uint8_t)(120 + 135 * wash), 30, 30)); lcd.fillCircle(CX, 24, 10, rgb(255, 200, 200));
        lcd.fillRoundRect(CX - 70, 150, 140, 70, 12, BLK);
        uiText(F_B36, "GOAL", CX, 185, wash > 0.35f ? C_WHITE : rgb(255, 40, 40), BLK, middle_center);
        stage = 0;
      } else if (t < 2260) {
        if (stage < 1) { lcd.fillScreen(C_WHITE); stage = 1; }
      } else {
        if (stage < 2) { stageCol = rgb(12, 4, 6); finale("GOAL!", spec.sub); stage = 2; }
        bool alt = (t / 250) % 2;
        if ((t / 250) != altG) {
          altG = t / 250;
          uint16_t edge = alt ? rgb(255, 40, 40) : rgb(120, 20, 20);
          lcd.fillRect(0, 0, 6, 320, edge); lcd.fillRect(474, 0, 6, 320, edge);
          bigWord("GOAL!", 232, alt);
        }
      }
      return true;
    }
    case FX_HOMERUN: {
      const uint32_t HIT = 120, GONE = 1900;
      if (stage < 0) { stageCol = rgb(8, 10, 28); park(0, 0, 480, 320); parkDetails(); batter(mineCol(), false); stage = 0; nsp = 0; pX0 = pX1 = pY0 = pY1 = 0; }
      if (t < GONE) {
        // the ball, along its arc, with a trail
        float u = t < HIT ? 0 : (float)(t - HIT) / (GONE - HIT);
        if (u > 1) u = 1;
        static int lx[9], ly[9], lr[9];
        static int nl = 0;
        if (t <= 40) nl = 0;
        for (int i = 0; i < nl; i++) { park(lx[i] - lr[i] - 1, ly[i] - lr[i] - 1, lr[i] * 2 + 3, lr[i] * 2 + 3); }
        if (u > 0) {
          if (stage == 0) { batter(mineCol(), true); stage = 1; }
          int bx = 118 + (int)(u * 330), by = 244 - (int)(u * 300) + (int)(u * u * 135);
          int br = 8 - (int)(u * 4);
          // the trail: six older positions, fading
          for (int k = 6; k >= 1; k--) {
            float uu = u - k * 0.03f; if (uu < 0) continue;
            int tx = 118 + (int)(uu * 330), ty = 244 - (int)(uu * 300) + (int)(uu * uu * 135);
            uint8_t g = (uint8_t)(220 - k * 28);
            lcd.fillCircle(tx, ty, 2, rgb(g, g, g));
            if (nl < 9) { lx[nl] = tx; ly[nl] = ty; lr[nl] = 2; nl++; }
          }
          lcd.fillCircle(bx, by, br, C_WHITE); lcd.drawCircle(bx, by, br - 3, rgb(210, 60, 60));
          if (nl < 9) { lx[nl] = bx; ly[nl] = by; lr[nl] = br; nl++; }
        }
        // the crack of the bat
        if (t >= HIT && t < HIT + 300) {
          float k = 1 - (float)(t - HIT) / 300;
          uint8_t g = (uint8_t)(255 * k);
          for (int r = 6; r <= 26; r += 5) { lcd.drawLine(150 - r, 232, 150 + r, 232, rgb(g, g, g)); lcd.drawLine(150, 232 - r, 150, 232, rgb(g, g, g)); }
        } else if (t >= HIT + 300 && t < HIT + 340) park(110, 190, 90, 70), batter(mineCol(), true);
        return true;
      }
      if (stage < 2) { stage = 2; lcd.fillScreen(stageCol); nsp = 0; pX0 = 90; pX1 = 390; pY0 = 150; pY1 = 312; }
      if (t < 3600 && t > nxtA) { blast(40 + frnd() * 400, 30 + frnd() * 110, 0.8f); nxtA = t + 280 + (uint32_t)(frnd() * 200); }
      if (t / 200 != blkA || stage == 2) {
        blkA = t / 200;
        bool alt = blkA % 2;
        const char* w = spec.word;
        word(w, 205, alt ? C_WHITE : mineCol());
        if (stage == 2) { pill(spec.sub, 292); stage = 3; }
        if (t >= 3500 && stage == 3) { stage = 4; lcd.fillRect(0, 36, 480, 100, stageCol); logoDraw(spec.mine, CX, 90, 112, stageCol); pX0 = 90; pX1 = 390; pY0 = 24; pY1 = 312; }
      }
      sparksStep(ticks);
      return true;
    }
    case FX_THREE: {
      if (stage < 0) { courtBackdrop(); stageCol = rgb(6, 6, 10); stage = 0; nsp = 0; pX0 = pX1 = pY0 = pY1 = 0; }
      if (t < 1850) {
        static int lx[9], ly[9], nl = 0;
        if (t <= 40) nl = 0;
        for (int i = 0; i < nl; i++) { lcd.fillRect(lx[i] - 12, ly[i] - 12, 25, 25, rgb(120, 78, 40)); }
        if (nl) { lcd.fillRect(0, 286, 480, 34, rgb(86, 54, 28)); }
        lcd.fillRect(430, 76, 8, 90, C_WHITE); lcd.fillRect(392, 150, 40, 5, rgb(255, 110, 20));
        for (int k = 0; k < 6; k++) lcd.drawLine(394 + k * 7, 155, 398 + k * 5, 188, C_WHITE);
        lcd.drawLine(398, 188, 428, 188, C_WHITE);
        nl = 0;
        float u = t < 1300 ? (float)t / 1300 : 1;
        int bx, by;
        if (t < 1300) { bx = 40 + (int)(u * 356); by = 262 - (int)(u * 112) - (int)(sinf(u * 3.14159f) * 110); }
        else if (t < 1650) { float d = (float)(t - 1300) / 350; bx = 396; by = 156 + (int)(d * 60); }
        else { bx = 396; by = 216; }
        if (t < 1300) for (int k = 6; k >= 1; k--) {
          float uu = u - k * 0.03f; if (uu < 0) continue;
          int tx = 40 + (int)(uu * 356), ty = 262 - (int)(uu * 112) - (int)(sinf(uu * 3.14159f) * 110);
          lcd.fillCircle(tx, ty, 2, rgb(255, 160, 60));
          if (nl < 9) { lx[nl] = tx; ly[nl] = ty; nl++; }
        }
        lcd.fillCircle(bx, by, 11, rgb(235, 120, 30)); lcd.drawLine(bx - 11, by, bx + 11, by, rgb(70, 35, 10)); lcd.drawLine(bx, by - 11, bx, by + 11, rgb(70, 35, 10));
        if (nl < 9) { lx[nl] = bx; ly[nl] = by; nl++; }
        if (t >= 1300) { lcd.drawLine(398, 188, 428, 188, C_WHITE); }
        if (t >= 1650 && stage == 0) { lcd.fillScreen(C_WHITE); stage = 1; }
        return true;
      }
      if (stage <= 1) {
        stageCol = rgb(6, 6, 10); lcd.fillScreen(stageCol); stage = 2; blast(CX, 130, 1.2f);
        pX0 = 90; pX1 = 390; pY0 = 110; pY1 = 312;
      }
      if (t < 3300 && t > nxtB) { blast(40 + frnd() * 400, 30 + frnd() * 250, 0.6f); nxtB = t + 220 + (uint32_t)(frnd() * 160); }
      if (t / 200 != blkB || stage == 2) {
        blkB = t / 200;
        bool alt = blkB % 2;
        lcd.fillRoundRect(60, 120, 360, 120, 16, BLK);
        uiText(F_B36, "THREE", CX, 156, alt ? C_WHITE : tm, BLK, middle_center);
        uiText(F_B24, "POINTER", CX, 204, rgb(pal[1][0], pal[1][1], pal[1][2]), BLK, middle_center);
        if (stage == 2) { pill(spec.sub, 292); stage = 3; }
      }
      sparksStep(ticks);
      return true;
    }
    case FX_WIN: {
      if (stage < 0) { stageCol = rgb(8, 14, 40); lcd.fillScreen(stageCol); stage = 0; nsp = 0; confettiInit();
        if (!logoDraw(spec.mine, CX, 112, 112, stageCol)) uiText(F_B36, spec.mine.abbr, CX, 112, C_WHITE, stageCol, middle_center);
        word(spec.word, 232, C_WHITE); pill(spec.sub, 292); pX0 = 90; pX1 = 390; pY0 = 36; pY1 = 312; }
      if (t > nxtA && t < 4800) { blast(50 + frnd() * 380, 40 + frnd() * 150, 0.9f); nxtA = t + 300 + (uint32_t)(frnd() * 220); }
      sparksStep(ticks);
      confettiStep();
      return true;
    }
    default: return false;
  }
}

bool fxStep() {
  if (!active) return false;
  uint32_t t = millis() - t0;
  if (t >= fxLength(spec.kind)) { active = false; return false; }
  if (spec.kind == FX_TOUCHDOWN || spec.kind == FX_GOAL || spec.kind == FX_HOMERUN || spec.kind == FX_THREE || spec.kind == FX_WIN ||
      spec.kind == FX_FIELDGOAL || spec.kind == FX_NOGOOD) {
    if (lastStep && millis() - lastStep < 24) return true;
    lastStep = millis();
    return bigStep(t);
  }
  if (lastStep && millis() - lastStep < 70) return true;
  lastStep = millis();
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
