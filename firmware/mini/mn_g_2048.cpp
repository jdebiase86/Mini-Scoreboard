// 2048: swipe to slide the tiles; two tiles of the same number join into one.
#include "mn_games.h"
#include "mn_ui.h"
#include "mn_lcd.h"

namespace {
uint8_t b[16], prevB[16];
int score = 0, prevScore = 0, best = 0;
bool over = false, hasPrev = false, shownWin = false;
const int TS = 62, TG = 6, TX = 14, TY = 46;

uint16_t tileCol(int e) {
  static const uint8_t C[12][3] = {{40, 46, 60}, {238, 228, 218}, {237, 224, 200}, {242, 177, 121}, {245, 149, 99}, {246, 124, 95},
                                   {246, 94, 59}, {237, 207, 114}, {237, 204, 97}, {237, 200, 80}, {237, 197, 63}, {237, 194, 46}};
  int i = e > 11 ? 11 : e;
  return rgb(C[i][0], C[i][1], C[i][2]);
}

void drawTile(int i) {
  int x0 = TX + (i % 4) * (TS + TG), y0 = TY + (i / 4) * (TS + TG);
  uint16_t c = tileCol(b[i]);
  uiTile(x0, y0, x0 + TS, y0 + TS, c, c, 8, 1);
  if (b[i]) {
    int v = 1 << b[i];
    uiText(v < 1000 ? F_B24 : F_B18, String(v), x0 + TS / 2, y0 + TS / 2, b[i] <= 2 ? rgb(90, 80, 70) : C_WHITE, c, middle_center);
  }
}

void drawAll() {
  for (int i = 0; i < 16; i++) drawTile(i);
}

void panel() {
  lcd.fillRect(300, 36, 180, 284, C_BG);
  uiTile(310, 50, 470, 100, C_TILE, C_EDGE, 10, 1);
  uiText(F_B12, "SCORE", 390, 62, C_GREY, C_TILE, middle_center);
  uiText(F_B24, String(score), 390, 84, C_WHITE, C_TILE, middle_center);
  uiTile(310, 108, 470, 158, C_TILE, C_EDGE, 10, 1);
  uiText(F_B12, "BEST", 390, 120, C_GREY, C_TILE, middle_center);
  uiText(F_B24, String(best), 390, 142, C_YELLOW, C_TILE, middle_center);
  gButton(310, 170, 470, 210, "NEW GAME", C_TILE_HI, C_WHITE, 1);
  gButton(310, 218, 470, 258, "UNDO", hasPrev ? C_TILE_HI : C_BG, hasPrev ? C_WHITE : C_DIM, 1);
  uiText(F_S13, "Swipe to slide the tiles", 390, 284, C_GREY, C_BG, middle_center);
}

void spawn() {
  int empty[16], n = 0;
  for (int i = 0; i < 16; i++) if (!b[i]) empty[n++] = i;
  if (!n) return;
  b[empty[gRand(n)]] = gRand(10) == 0 ? 2 : 1;
}

bool canMove() {
  for (int i = 0; i < 16; i++) {
    if (!b[i]) return true;
    if (i % 4 < 3 && b[i] == b[i + 1]) return true;
    if (i < 12 && b[i] == b[i + 4]) return true;
  }
  return false;
}

// slides one line (4 values) toward index 0; returns the points made
int slide(uint8_t* line) {
  uint8_t t[4]; int n = 0, pts = 0;
  for (int i = 0; i < 4; i++) if (line[i]) t[n++] = line[i];
  for (int i = 0; i + 1 < n; i++) if (t[i] == t[i + 1]) { t[i]++; pts += 1 << t[i]; for (int j = i + 1; j + 1 < n; j++) t[j] = t[j + 1]; n--; }
  for (int i = 0; i < 4; i++) line[i] = i < n ? t[i] : 0;
  return pts;
}

bool move(int dir) {   // 0 left, 1 right, 2 up, 3 down
  uint8_t before[16];
  memcpy(before, b, 16);
  int pts = 0;
  for (int k = 0; k < 4; k++) {
    uint8_t line[4];
    for (int i = 0; i < 4; i++) {
      int idx = dir == 0 ? k * 4 + i : dir == 1 ? k * 4 + 3 - i : dir == 2 ? i * 4 + k : (3 - i) * 4 + k;
      line[i] = b[idx];
    }
    pts += slide(line);
    for (int i = 0; i < 4; i++) {
      int idx = dir == 0 ? k * 4 + i : dir == 1 ? k * 4 + 3 - i : dir == 2 ? i * 4 + k : (3 - i) * 4 + k;
      b[idx] = line[i];
    }
  }
  if (!memcmp(before, b, 16)) return false;
  memcpy(prevB, before, 16);
  prevScore = score;
  hasPrev = true;
  score += pts;
  return true;
}

void newGame() {
  memset(b, 0, 16);
  score = 0; over = false; hasPrev = false; shownWin = false;
  best = gGet("g2048_best", 0);
  spawn(); spawn();
  lcd.fillScreen(C_BG);
  gTopBar("2048", "");
  drawAll();
  panel();
}

void open() { newGame(); }

void tap(int x, int y) {
  if (gIn(x, y, 310, 170, 470, 210)) { newGame(); return; }
  if (gIn(x, y, 310, 218, 470, 258) && hasPrev) {
    memcpy(b, prevB, 16);
    score = prevScore; hasPrev = false; over = false;
    drawAll(); panel();
  }
}

void swipe(TouchEvent ev) {
  if (over) return;
  int dir = ev == T_SWIPE_LEFT ? 0 : ev == T_SWIPE_RIGHT ? 1 : ev == T_SWIPE_UP ? 2 : ev == T_SWIPE_DOWN ? 3 : -1;
  if (dir < 0 || !move(dir)) return;
  spawn();
  if (score > best) { best = score; gPut("g2048_best", best); }
  int top = 0;
  for (int i = 0; i < 16; i++) if (b[i] > top) top = b[i];
  int lv = top > 1 ? top - 1 : 1;   // the biggest tile as the level (8 -> level 2 ... 2048 -> level 10)
  if (lv > gGet("g2048_lv", 0)) gPut("g2048_lv", lv);
  drawAll();
  panel();
  if (top >= 11 && !shownWin) { shownWin = true; gBanner("2048!  KEEP GOING", C_GREEN, 160); }
  else if (!canMove()) {
    over = true;
    gBanner("GAME OVER", C_RED, 160);
  }
}

void icon(int cx, int cy) {
  for (int i = 0; i < 4; i++) {
    int x0 = cx - 26 + (i % 2) * 28, y0 = cy - 26 + (i / 2) * 28;
    uint16_t c = tileCol(i + 1);
    uiTile(x0, y0, x0 + 24, y0 + 24, c, c, 4, 1);
  }
}
}  // namespace

const Minigame GAME_2048 = {"2048", "Swipe and join", "g2048", icon, open, tap, swipe, nullptr, nullptr};
