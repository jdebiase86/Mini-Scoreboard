// Reaction test, and a coin and dice for settling who goes first.
#include "mn_games.h"
#include "mn_ui.h"
#include "mn_lcd.h"
#include <math.h>

namespace {
// ---------------------------------------------------------------- reaction
enum RPhase { R_IDLE, R_WAIT, R_GO, R_SHOW } rp = R_IDLE;
uint32_t rT = 0, rGoAt = 0;
int rBest = 0, rTimes[5], rN = 0, rLast = 0;

void rScreen(uint16_t col, const char* big, const char* small) {
  lcd.fillRect(0, 36, 480, 284, col);
  uiText(F_B36, big, 240, 150, C_WHITE, col, middle_center);
  if (small) uiText(F_B16, small, 240, 196, C_WHITE, col, middle_center);
}

void rHeader() {
  String r = rBest ? String("Best ") + String(rBest) + " ms" : String("");
  gTopBar("Reaction", r);
}

void rOpen() {
  rBest = gGet("react_best", 0);
  rN = 0;
  lcd.fillScreen(C_BG);
  rHeader();
  rScreen(rgb(30, 50, 100), "TAP TO START", "Wait for green, then tap fast");
  rp = R_IDLE;
}

void rStartWait() {
  rScreen(rgb(150, 30, 36), "WAIT...", "Tap when it turns green");
  rp = R_WAIT;
  rT = millis();
  rGoAt = millis() + 1500 + gRand(2800);
}

void rTap(int, int) {
  uint32_t now = millis();
  if (rp == R_IDLE || rp == R_SHOW) { if (rp == R_SHOW && now - rT < 400) return; rStartWait(); return; }
  if (rp == R_WAIT) {
    rScreen(rgb(150, 100, 20), "TOO SOON!", "Tap to try again");
    rp = R_SHOW; rT = now;
    return;
  }
  if (rp == R_GO) {
    int ms = (int)(now - rT);
    rLast = ms;
    rTimes[rN % 5] = ms; rN++;
    bool newBest = !rBest || ms < rBest;
    if (newBest) { rBest = ms; gPut("react_best", rBest); }
    int stars = ms < 220 ? 3 : ms < 300 ? 2 : ms < 450 ? 1 : 0;
    gPut("react_lv", 1 + (rBest < 450) + (rBest < 300) + (rBest < 220));
    rScreen(rgb(24, 70, 120), (String(ms) + " ms").c_str(), ms < 220 ? "Lightning!" : ms < 300 ? "Quick!" : ms < 450 ? "Not bad" : "Keep trying");
    if (newBest) uiText(F_B16, "NEW BEST!", 240, 230, C_YELLOW, rgb(24, 70, 120), middle_center);
    gStars(240, 262, stars);
    rHeader();
    rp = R_SHOW; rT = now;
  }
}

void rLoop() {
  if (rp == R_WAIT && millis() >= rGoAt) {
    rScreen(rgb(30, 160, 80), "TAP NOW!", nullptr);
    rp = R_GO;
    rT = millis();
  }
}

void rIcon(int cx, int cy) {
  lcd.fillCircle(cx, cy, 24, rgb(30, 160, 80));
  lcd.fillTriangle(cx + 4, cy - 14, cx - 8, cy + 2, cx + 1, cy + 2, C_WHITE);
  lcd.fillTriangle(cx - 4, cy + 14, cx + 8, cy - 2, cx - 1, cy - 2, C_WHITE);
}

// ------------------------------------------------------------ coin and dice
enum TPhase { T_IDLE, T_COIN, T_DIE, T_DONE } tp = T_IDLE;
int tFrame = 0, tResult = 0, tFace = 1;
uint32_t tT = 0;
bool lastWasCoin = true;

void tButtons() {
  gButton(40, 262, 230, 308, "FLIP A COIN", C_TILE_HI, C_WHITE, 1);
  gButton(250, 262, 440, 308, "ROLL A DIE", C_TILE_HI, C_WHITE, 1);
}

void coinDraw(int squash, bool heads) {
  lcd.fillRect(100, 60, 280, 190, C_BG);
  int ry = squash < 4 ? 4 : squash;
  uint16_t c = rgb(235, 190, 60);
  lcd.fillEllipse(240, 150, 70, ry, c);
  lcd.drawEllipse(240, 150, 70, ry, rgb(150, 110, 20));
  if (ry > 40) uiText(F_B36, heads ? "H" : "T", 240, 150, rgb(120, 80, 10), c, middle_center);
}

void dieDraw(int face) {
  lcd.fillRect(100, 60, 280, 190, C_BG);
  uiTile(180, 80, 300, 200, C_WHITE, rgb(150, 150, 160), 16, 2);
  static const int8_t P[7][6][2] = {{{0, 0}}, {{0, 0}}, {{-1, -1}, {1, 1}}, {{-1, -1}, {0, 0}, {1, 1}},
                                    {{-1, -1}, {1, -1}, {-1, 1}, {1, 1}}, {{-1, -1}, {1, -1}, {0, 0}, {-1, 1}, {1, 1}},
                                    {{-1, -1}, {1, -1}, {-1, 0}, {1, 0}, {-1, 1}, {1, 1}}};
  static const int N[7] = {0, 1, 2, 3, 4, 5, 6};
  for (int i = 0; i < N[face]; i++) lcd.fillCircle(240 + P[face][i][0] * 30, 140 + P[face][i][1] * 30, 9, rgb(30, 30, 40));
}

void tOpen() {
  lcd.fillScreen(C_BG);
  gTopBar("Coin & Dice", "");
  uiText(F_B18, "Who goes first?", 240, 130, C_WHITE, C_BG, middle_center);
  tButtons();
  tp = T_IDLE;
}

void tTap(int x, int y) {
  if (tp == T_COIN || tp == T_DIE) return;
  if (gIn(x, y, 30, 256, 236, 314)) { tp = T_COIN; tFrame = 0; tT = millis(); tResult = gRand(2); gPut("toss_lv", gGet("toss_lv", 0) + 1); }
  else if (gIn(x, y, 244, 256, 450, 314)) { tp = T_DIE; tFrame = 0; tT = millis(); tResult = 1 + gRand(6); gPut("toss_lv", gGet("toss_lv", 0) + 1); }
}

void tLoop() {
  uint32_t now = millis();
  if (tp == T_COIN && now - tT > 45) {
    tT = now; tFrame++;
    int squash = (int)(70 * fabsf(cosf(tFrame * 0.55f)));
    coinDraw(squash, (tFrame / 6) % 2 == 0);
    if (tFrame >= 22) {
      tp = T_DONE;
      coinDraw(70, tResult == 0);
      uiText(F_B24, tResult == 0 ? "HEADS" : "TAILS", 240, 236, C_YELLOW, C_BG, middle_center);
    }
  } else if (tp == T_DIE && now - tT > 70) {
    tT = now; tFrame++;
    tFace = 1 + gRand(6);
    if (tFrame >= 12) { tFace = tResult; tp = T_DONE; }
    dieDraw(tFace);
    if (tp == T_DONE) uiText(F_B24, String(tResult), 240, 236, C_YELLOW, C_BG, middle_center);
  }
}

void tIcon(int cx, int cy) {
  lcd.fillEllipse(cx - 14, cy, 20, 20, rgb(235, 190, 60));
  uiTile(cx + 2, cy - 18, cx + 36, cy + 16, C_WHITE, rgb(150, 150, 160), 6, 1);
  lcd.fillCircle(cx + 19, cy - 1, 3, rgb(30, 30, 40));
}
}  // namespace

const Minigame GAME_REACTION = {"Reaction", "How fast are you?", "react", rIcon, rOpen, rTap, nullptr, nullptr, rLoop};
const Minigame GAME_TOSS = {"Coin & Dice", "Who goes first?", "toss", tIcon, tOpen, tTap, nullptr, nullptr, tLoop};
