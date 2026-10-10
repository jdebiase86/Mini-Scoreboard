// Cheer Simon: watch the colours light up, then tap them back in the same
// order. One more every round. (Sounds come when the speaker is in.)
#include "mn_games.h"
#include "mn_ui.h"
#include "mn_lcd.h"

namespace {
enum Phase { IDLE, SHOW_GAP, SHOW_LIT, WAIT_INPUT, LIT_TAP, WRONG, NEXT };
Phase phase = IDLE;
uint8_t seq[60];
int len = 0, showAt = 0, inputAt = 0, best = 0, litPad = -1;
uint32_t t0 = 0;

const int PX[4] = {10, 250, 10, 250}, PY[4] = {44, 44, 184, 184};
const int PW = 220, PH = 130;

uint16_t padCol(int i, bool lit) {
  static const uint8_t C[4][3] = {{220, 50, 60}, {50, 190, 90}, {50, 110, 230}, {245, 200, 40}};
  float k = lit ? 1.0f : 0.38f;
  return rgb(C[i][0] * k, C[i][1] * k, C[i][2] * k);
}

void drawPad(int i, bool lit) {
  uiTile(PX[i], PY[i], PX[i] + PW, PY[i] + PH, padCol(i, lit), lit ? C_WHITE : C_EDGE, 18, lit ? 3 : 1);
}

void drawAll() {
  for (int i = 0; i < 4; i++) drawPad(i, false);
}

void status(const char* s, uint16_t col) {
  lcd.fillRect(100, 3, 200, 26, C_BG);
  uiText(F_B16, s, 200, 16, col, C_BG, middle_center);
}

void header() { gTopBar("Cheer Simon", String("Round ") + String(len) + "  Best " + String(best)); }

int litMs() { int m = 480 - len * 14; return m < 180 ? 180 : m; }

void startRound() {
  seq[len++] = gRand(4);
  showAt = 0; inputAt = 0;
  header();
  status("Watch...", C_WHITE);
  phase = SHOW_GAP;
  t0 = millis();
}

void newGame() {
  len = 0;
  best = gGet("simon_best", 0);
  lcd.fillScreen(C_BG);
  header();
  drawAll();
  uiText(F_B18, "Tap to start", 240, 180, C_WHITE, C_BG, middle_center);
  phase = IDLE;
}

void open() { newGame(); }

void gameOver() {
  phase = WRONG;
  int score = len - 1;
  if (score > best) { best = score; gPut("simon_best", best); }
  gPut("simon_lv", score / 5 + 1);
  header();
  gBanner(String("Oops!  You got ") + String(score), C_RED, 160);
  uiText(F_S13, "Tap to play again", 240, 196, C_WHITE, C_BG, middle_center);
  t0 = millis();
}

void tap(int x, int y) {
  if (phase == IDLE || phase == WRONG) { if (phase == WRONG && millis() - t0 < 600) return; len = 0; lcd.fillScreen(C_BG); header(); drawAll(); startRound(); return; }
  if (phase != WAIT_INPUT) return;
  for (int i = 0; i < 4; i++) {
    if (gIn(x, y, PX[i], PY[i], PX[i] + PW, PY[i] + PH)) {
      litPad = i;
      drawPad(i, true);
      t0 = millis();
      if (seq[inputAt] != i) { gameOver(); return; }
      inputAt++;
      phase = LIT_TAP;
      return;
    }
  }
}

void tick() {
  uint32_t now = millis();
  switch (phase) {
    case SHOW_GAP:
      if (now - t0 > (showAt == 0 ? 700u : 200u)) {
        litPad = seq[showAt];
        drawPad(litPad, true);
        t0 = now;
        phase = SHOW_LIT;
      }
      break;
    case SHOW_LIT:
      if (now - t0 > (uint32_t)litMs()) {
        drawPad(litPad, false);
        t0 = now;
        showAt++;
        if (showAt >= len) { phase = WAIT_INPUT; status("Your turn!", C_GREEN); }
        else phase = SHOW_GAP;
      }
      break;
    case LIT_TAP:
      if (now - t0 > 180) {
        drawPad(litPad, false);
        if (inputAt >= len) {
          phase = NEXT;
          t0 = now;
          if (len % 5 == 0) gBanner("GO TEAM!", C_YELLOW, 150);
        } else phase = WAIT_INPUT;
      }
      break;
    case NEXT:
      if (now - t0 > 800) {
        if (len % 5 == 0) { lcd.fillScreen(C_BG); header(); drawAll(); }
        if (len >= 58) { len = 0; }
        startRound();
      }
      break;
    default:
      break;
  }
}

void icon(int cx, int cy) {
  for (int i = 0; i < 4; i++) {
    int x0 = cx - 28 + (i % 2) * 29, y0 = cy - 26 + (i / 2) * 27;
    uiTile(x0, y0, x0 + 26, y0 + 25, padCol(i, true), padCol(i, true), 6, 1);
  }
}
}  // namespace

const Minigame GAME_SIMON = {"Cheer Simon", "Copy the colours", "simon", icon, open, tap, nullptr, nullptr, tick};
