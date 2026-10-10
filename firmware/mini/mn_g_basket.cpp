// Basket Toss: the bases throw a flyer up and she comes down somewhere else.
// Slide your thumb to move the two bases under her and catch her. Three
// misses and it's over. It gets harder as you go: faster and wider tosses,
// narrower arms, a gust of wind in the air from level 4, and no landing
// marker from level 4.
#include "mn_games.h"
#include "mn_ui.h"
#include "mn_lcd.h"
#include <math.h>

#ifdef MN_HOST
bool touchHeldAt(int& x, int& y);
#else
#include "mn_touch.h"
#endif

namespace {
enum Phase { IDLE, READY, FLY, CAUGHT, MISSED, OVER };
Phase phase = IDLE;

const int FLOOR_Y = 296;      // the mat
const int BAR_Y = 250;        // the arms
const int REST_Y = BAR_Y - 34;  // where she stands on the arms
const float SC = 1.5f;        // drawing size of the people
const int TOP_Y = 40;

float baseX = 240, targetX = 240;
int score = 0, best = 0, caught = 0, lives = 3, streak = 0;
uint32_t lastFrame = 0, phaseAt = 0;
// the toss
float x0 = 240, xEnd = 240, tossS = 1.6f, apexH = 170;
float turns = 1;
float gustTo = 0;
bool gust = false;
float fx = 240, fy = REST_Y, fa = 0;   // where the flyer is drawn, and her angle
int markerX = -1;
int prevFx = -999, prevFy = 0;
int prevBx = -999;
int popX = 0, confX = -999;
uint32_t tossAt = 0;
bool gustShown = false;

int level() { int l = 1 + caught / 5; return l > 12 ? 12 : l; }
float halfWidth() { int l = level(); return 58.0f - (l > 9 ? 8 : l - 1) * 2.0f; }
float tossSecs() { float s = 1.95f - 0.09f * (level() - 1); return s < 0.85f ? 0.85f : s; }
float tossRange() { float r = 70.0f + 14.0f * (level() - 1); return r > 210 ? 210 : r; }
const float BASE_SPEED = 340.0f;   // pixels per second the bases can move

uint16_t SKIN() { return rgb(246, 200, 160); }

void rot(float px, float py, float a, int cx, int cy, int& ox, int& oy) {
  float c = cosf(a), s = sinf(a);
  px *= SC; py *= SC;
  ox = cx + (int)lroundf(px * c - py * s);
  oy = cy + (int)lroundf(px * s + py * c);
}

void thick(int ax, int ay, int bx, int by, uint16_t c) {
  lcd.drawLine(ax, ay, bx, by, c); lcd.drawLine(ax + 1, ay, bx + 1, by, c); lcd.drawLine(ax, ay + 1, bx, by + 1, c);
}

void flyer(int cx, int cy, float a) {
  int ax, ay, bx, by;
  uint16_t suit = rgb(240, 70, 90), pom = C_YELLOW, white = C_WHITE;
  // legs
  rot(-2, 6, a, cx, cy, ax, ay); rot(-4, 21, a, cx, cy, bx, by); thick(ax, ay, bx, by, white);
  rot(2, 6, a, cx, cy, ax, ay); rot(4, 21, a, cx, cy, bx, by); thick(ax, ay, bx, by, white);
  // body
  int tx[4], ty[4];
  rot(-5, -10, a, cx, cy, tx[0], ty[0]); rot(5, -10, a, cx, cy, tx[1], ty[1]);
  rot(5, 7, a, cx, cy, tx[2], ty[2]); rot(-5, 7, a, cx, cy, tx[3], ty[3]);
  lcd.fillTriangle(tx[0], ty[0], tx[1], ty[1], tx[2], ty[2], suit);
  lcd.fillTriangle(tx[0], ty[0], tx[2], ty[2], tx[3], ty[3], suit);
  // arms up in a high V, with poms
  for (int side = -1; side <= 1; side += 2) {
    rot(side * 4, -9, a, cx, cy, ax, ay); rot(side * 14, -23, a, cx, cy, bx, by);
    thick(ax, ay, bx, by, SKIN());
    lcd.fillCircle(bx, by, 6, pom); lcd.fillCircle(bx, by, 3, rgb(255, 235, 140));
  }
  // head, ponytail and bow
  rot(0, -17, a, cx, cy, ax, ay);
  lcd.fillCircle(ax, ay, 9, SKIN());
  rot(7, -18, a, cx, cy, bx, by); thick(ax, ay - 1, bx, by, rgb(120, 70, 40)); lcd.fillCircle(bx, by, 3, rgb(120, 70, 40));
  rot(5, -21, a, cx, cy, bx, by); lcd.fillCircle(bx, by, 4, rgb(255, 120, 190));
}

void cradle(int dip) {
  int cx = (int)baseX;
  int hw = (int)halfWidth();
  // two bases and the arms between them
  for (int i = 0; i < 2; i++) {
    int hx = cx + (i ? hw - 14 : -(hw - 14));
    uint16_t shirt = i ? rgb(255, 255, 255) : rgb(60, 140, 255);
    lcd.fillCircle(hx, BAR_Y + 10 + dip, 9, SKIN());
    lcd.fillRoundRect(hx - 9, BAR_Y + 20 + dip, 18, 22, 5, shirt);
    lcd.fillRect(hx - 7, BAR_Y + 42, 6, FLOOR_Y - BAR_Y - 42, rgb(70, 76, 96));
    lcd.fillRect(hx + 1, BAR_Y + 42, 6, FLOOR_Y - BAR_Y - 42, rgb(70, 76, 96));
  }
  lcd.fillRoundRect(cx - hw + 4, BAR_Y + dip, 2 * hw - 8, 9, 4, SKIN());
}

void eraseCradle() {
  lcd.fillRect(prevBx - 84, BAR_Y - 6, 168, FLOOR_Y - BAR_Y + 6, C_BG);
}

void eraseFlyer() {
  if (prevFx < -900) return;
  int y0 = prevFy - 56 < TOP_Y ? TOP_Y : prevFy - 56;
  int y1 = prevFy + 56 > FLOOR_Y ? FLOOR_Y : prevFy + 56;
  if (y1 > y0) lcd.fillRect(prevFx - 56, y0, 112, y1 - y0, C_BG);
  prevFx = -999;
}

void hearts() {
  lcd.fillRect(4, 40, 100, 20, C_BG);
  for (int i = 0; i < 3; i++) {
    int x = 16 + i * 26;
    uint16_t c = i < lives ? rgb(240, 70, 90) : C_DIM;
    lcd.fillCircle(x - 4, 50, 5, c); lcd.fillCircle(x + 4, 50, 5, c);
    lcd.fillTriangle(x - 9, 52, x + 9, 52, x, 62, c);
  }
}

void header() {
  gTopBar((String("Basket Toss  L") + String(level())).c_str(), String("Score ") + String(score) + "  Best " + String(best));
}

void floor_() {
  lcd.fillRect(0, FLOOR_Y, 480, 320 - FLOOR_Y, rgb(30, 60, 130));
  lcd.fillRect(0, FLOOR_Y, 480, 3, C_WHITE);
  for (int x = 20; x < 480; x += 60) lcd.fillRect(x, FLOOR_Y + 14, 30, 2, rgb(70, 110, 190));
}

void marker(bool on) {
  // the landing spot, a ring on the mat: levels 1 to 3 only
  if (markerX >= 0) { lcd.fillRect(markerX - 24, FLOOR_Y + 5, 48, 14, rgb(30, 60, 130)); for (int x = 20; x < 480; x += 60) if (x + 30 > markerX - 24 && x < markerX + 24) lcd.fillRect(x, FLOOR_Y + 14, 30, 2, rgb(70, 110, 190)); }
  markerX = -1;
  if (on && level() <= 3) { markerX = (int)xEnd; lcd.drawEllipse(markerX, FLOOR_Y + 11, 20, 5, C_YELLOW); lcd.drawEllipse(markerX, FLOOR_Y + 11, 19, 4, C_YELLOW); }
}

void pop(const String& s, uint16_t col, int x) {
  lcd.fillRect(popX - 130, 100, 260, 40, C_BG);
  popX = x < 140 ? 140 : x > 340 ? 340 : x;
  uiText(F_B36, s, popX, 120, col, C_BG, middle_center);
}
void popOff() { lcd.fillRect(popX - 130, 100, 260, 40, C_BG); lcd.fillRect(confX - 110, 140, 220, 100, C_BG); confX = -999; }
void confetti(int cx) {
  confX = cx;
  static const uint16_t cols[] = {0xF800, 0xFFE0, 0x07FF, 0xF81F, 0xFFFF, 0x07E0};
  for (int i = 0; i < 26; i++) {
    int x = cx - 100 + (int)gRand(200), y = 146 + (int)gRand(90);
    lcd.fillRect(x, y, 5, 5, cols[gRand(6)]);
  }
}

void clearAll() {
  lcd.fillRect(0, TOP_Y, 480, FLOOR_Y - TOP_Y, C_BG);
  prevFx = -999; prevBx = (int)baseX;
}

void drawStatic() {
  lcd.fillScreen(C_BG);
  header();
  floor_();
  hearts();
  markerX = -1;
}

void startToss() {
  x0 = baseX;
  // a landing spot at least half the range away, on a side with room
  float r = tossRange();
  float d = r * (0.5f + 0.5f * (gRand(1000) / 1000.0f));
  float sign = gRand(2) ? 1 : -1;
  float hw = halfWidth() + 14;
  if (x0 + sign * d > 480 - hw || x0 + sign * d < hw) sign = -sign;
  xEnd = x0 + sign * d;
  if (xEnd > 480 - hw) xEnd = 480 - hw;
  if (xEnd < hw) xEnd = hw;
  gust = level() >= 4 && gRand(100) < 55;
  gustShown = false;
  if (gust) {
    float w = 30.0f + 8.0f * (level() - 4); if (w > 100) w = 100;
    float gs = gRand(2) ? 1 : -1;
    gustTo = xEnd + gs * w;
    if (gustTo > 480 - hw || gustTo < hw) gustTo = xEnd - gs * w;
    if (gustTo > 480 - hw) gustTo = 480 - hw;
    if (gustTo < hw) gustTo = hw;
  }
  tossS = tossSecs();
  apexH = 105 + gRand(25);
  turns = level() < 3 ? 1 : level() < 7 ? 2 : 3;
  tossAt = millis();
  phase = FLY;
  popOff();
  marker(true);
}

float landingX() { return gust ? gustTo : xEnd; }

void beginReady() {
  phase = READY; phaseAt = millis();
  fa = 0; fx = baseX; fy = REST_Y;
}

void gameOver() {
  phase = OVER; phaseAt = millis();
  eraseFlyer(); marker(false);
  header();
  gPut("basket_lv", level());
  gBanner("GAME OVER", C_RED, 140);
  uiText(F_B16, String("You caught ") + String(caught) + "   Score " + String(score), 240, 184, C_WHITE, C_BG, middle_center);
  uiText(F_S13, "Tap to play again", 240, 208, C_GREY, C_BG, middle_center);
}

void newGame() {
  score = 0; caught = 0; lives = 3; streak = 0;
  best = gGet("basket_best", 0);
  baseX = targetX = 240;
  drawStatic();
  cradle(0); prevBx = (int)baseX;
  beginReady();
  flyer((int)fx, (int)fy, 0); prevFx = (int)fx; prevFy = (int)fy;
  phase = IDLE;
  uiText(F_B16, "Slide your thumb to move the bases.", 240, 120, C_WHITE, C_BG, middle_center);
  uiText(F_B16, "Catch the flyer! Tap to start.", 240, 146, C_YELLOW, C_BG, middle_center);
}

void open() { newGame(); }

void tap(int x, int y) {
  (void)y;
  if (phase == IDLE) {
    lcd.fillRect(20, 108, 440, 52, C_BG);
    beginReady();
    return;
  }
  if (phase == OVER) { if (millis() - phaseAt > 600) newGame(); return; }
  targetX = (float)x;   // a quick tap moves the bases there too
}

// where the flyer is at u = 0..1 through the toss
void flightAt(float u, float& x, float& y, float& a) {
  float xm = x0 + (xEnd - x0) * 0.5f;
  if (gust && u > 0.5f) x = xm + (gustTo - xm) * ((u - 0.5f) / 0.5f);
  else x = x0 + (xEnd - x0) * u;
  y = REST_Y - apexH * 4.0f * u * (1.0f - u);
  float sm = u / 0.85f; if (sm > 1) sm = 1;
  float e = sm * sm * (3 - 2 * sm);
  a = 6.2831853f * turns * e;
}

void caughtIt(bool perfect) {
  phase = CAUGHT; phaseAt = millis();
  caught++;
  streak = perfect ? streak + 1 : 0;
  if (caught % 10 == 0 && lives < 3) { lives++; hearts(); }   // a spare life every ten catches
  int pts = 1 + (perfect ? 1 : 0) + (streak >= 3 ? 1 : 0);
  score += pts;
  if (score > best) { best = score; gPut("basket_best", best); }
  gPut("basket_lv", level());
  header();
  const char* w = streak >= 3 ? "ON FIRE!" : perfect ? "PERFECT!" : caught % 5 == 0 ? "LEVEL UP!" : "NICE!";
  eraseFlyer(); marker(false);
  eraseCradle(); cradle(4); prevBx = (int)baseX;
  fa = 0; fx = baseX; fy = REST_Y;
  flyer((int)fx, (int)fy, 0); prevFx = (int)fx; prevFy = (int)fy;
  pop(w, perfect ? C_YELLOW : C_GREEN, (int)baseX);
  if (perfect || caught % 5 == 0) confetti((int)baseX);
}

void missed() {
  phase = MISSED; phaseAt = millis();
  lives--; streak = 0;
  hearts();
  eraseFlyer(); marker(false);
  // she lands on the mat
  int lx = (int)landingX();
  flyer(lx, FLOOR_Y - 8, 1.57f);
  prevFx = -999;
  pop("OOPS!", C_RED, lx);
}

void tick() {
  uint32_t now = millis();
  if (lastFrame && now - lastFrame < 22) return;
  float dt = lastFrame ? (now - lastFrame) / 1000.0f : 0.022f;
  if (dt > 0.08f) dt = 0.08f;
  lastFrame = now;
  if (phase == OVER) return;
  // the bases follow the thumb
  int tx, ty;
  if (touchHeldAt(tx, ty)) targetX = (float)tx;
  float hw = halfWidth();
  if (targetX < hw) targetX = hw;
  if (targetX > 480 - hw) targetX = 480 - hw;
  float step = BASE_SPEED * dt, dx = targetX - baseX;
  if (dx > step) dx = step; else if (dx < -step) dx = -step;
  float oldBase = baseX;
  baseX += dx;
  bool moved = (int)baseX != (int)oldBase;

  if (phase == IDLE || phase == READY || phase == CAUGHT || phase == MISSED) {
    if (moved || phase == READY || phase == CAUGHT) {
      // the flyer rides the bases until she is thrown
      eraseCradle(); eraseFlyer();
      cradle(phase == CAUGHT && now - phaseAt < 160 ? 4 : 0);
      prevBx = (int)baseX;
      if (phase != MISSED && phase != IDLE) { fx = baseX; fy = REST_Y; flyer((int)fx, (int)fy, 0); prevFx = (int)fx; prevFy = (int)fy; }
      else if (phase == IDLE) { fx = baseX; flyer((int)fx, (int)fy, 0); prevFx = (int)fx; prevFy = (int)fy; }
    }
    if (phase == READY && now - phaseAt > 700) startToss();
    if (phase == CAUGHT && now - phaseAt > 650) { popOff(); if (caught % 5 == 0) header(); beginReady(); }
    if (phase == MISSED && now - phaseAt > 1100) {
      popOff();
      lcd.fillRect(0, BAR_Y - 12, 480, FLOOR_Y - BAR_Y + 12, C_BG); floor_();   // she walks off
      if (lives <= 0) gameOver(); else beginReady();
    }
    return;
  }
  // in the air
  float u = (now - tossAt) / (tossS * 1000.0f);
  if (u > 1) u = 1;
  float x, y, a;
  flightAt(u, x, y, a);
  eraseFlyer();
  if (moved) { eraseCradle(); cradle(0); prevBx = (int)baseX; }
  else { cradle(0); }
  if (gust && !gustShown && u >= 0.5f) {
    gustShown = true;
    marker(false);
    bool right = gustTo > xEnd;
    uiText(F_B16, right ? "WIND >>" : "<< WIND", 240, 74, C_YELLOW, C_BG, middle_center);
  }
  if (gustShown && u > 0.8f) lcd.fillRect(160, 62, 160, 24, C_BG);
  flyer((int)x, (int)y, a);
  if (x < 190 && y < 130) hearts();   // she flew over them
  prevFx = (int)x; prevFy = (int)y;
  fx = x; fy = y; fa = a;
  if (u >= 1.0f) {
    lcd.fillRect(160, 62, 160, 24, C_BG);
    float off = fabsf(baseX - landingX());
    if (off <= halfWidth() + 4) caughtIt(off <= 12);
    else missed();
  }
}

void icon(int cx, int cy) {
  flyer(cx, cy - 12, 0.0f);
  lcd.fillRoundRect(cx - 26, cy + 20, 52, 5, 2, SKIN());
  lcd.fillCircle(cx - 26, cy + 24, 5, SKIN());
  lcd.fillCircle(cx + 26, cy + 24, 5, SKIN());
}
}  // namespace

#ifdef MN_HOST
float basketLanding() { return landingX(); }   // for the picture maker and the tests: where she will come down
#endif

const Minigame GAME_BASKET = {"Basket Toss", "Catch the flyer", "basket", icon, open, tap, nullptr, nullptr, tick};
