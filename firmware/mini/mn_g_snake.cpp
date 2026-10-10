// Snake: swipe to steer, or tap above / below / left / right of the head. The
// walls wrap round. It speeds up as your score grows.
#include "mn_games.h"
#include "mn_ui.h"
#include "mn_lcd.h"

namespace {
const int CS = 16, COLS = 30, ROWS = 17, OX = 0, OY = 40;
uint8_t bx[COLS * ROWS], by[COLS * ROWS];
int len = 0, dx = 1, dy = 0, nextDx = 1, nextDy = 0, fx = 0, fy = 0, score = 0, best = 0;
bool running = false, over = false;
uint32_t lastStep = 0;

int stepMs() { int l = score / 50; int ms = 170 - l * 11; return ms < 70 ? 70 : ms; }

void cell(int x, int y, uint16_t c, bool round = false) {
  if (round) { lcd.fillRect(OX + x * CS, OY + y * CS, CS, CS, C_BG); lcd.fillCircle(OX + x * CS + CS / 2, OY + y * CS + CS / 2, CS / 2 - 1, c); }
  else lcd.fillRect(OX + x * CS + 1, OY + y * CS + 1, CS - 2, CS - 2, c);
}

void clearCell(int x, int y) { lcd.fillRect(OX + x * CS, OY + y * CS, CS, CS, C_BG); }

void placeFood() {
  for (int tries = 0; tries < 200; tries++) {
    fx = gRand(COLS); fy = gRand(ROWS);
    bool onSnake = false;
    for (int i = 0; i < len; i++) if (bx[i] == fx && by[i] == fy) { onSnake = true; break; }
    if (!onSnake) break;
  }
  cell(fx, fy, rgb(235, 60, 60), true);
}

void header() { gTopBar((String("Snake  L") + String(score / 50 + 1)).c_str(), String("Score ") + String(score) + "  Best " + String(best)); }

void start() {
  lcd.fillScreen(C_BG);
  lcd.drawRect(OX, OY - 1, COLS * CS, ROWS * CS + 2, C_EDGE);
  len = 4; dx = nextDx = 1; dy = nextDy = 0; score = 0; over = false; running = false;
  for (int i = 0; i < len; i++) { bx[i] = 10 - i; by[i] = ROWS / 2; }
  best = gGet("snake_best", 0);
  header();
  for (int i = len - 1; i >= 0; i--) cell(bx[i], by[i], i == 0 ? rgb(120, 240, 150) : rgb(40, 170, 90));
  placeFood();
  uiText(F_B16, "Swipe or tap to steer. Tap to start.", 240, 190, C_WHITE, C_BG, middle_center);
}

void turn(int ndx, int ndy) {
  if (ndx == -dx && ndy == -dy) return;   // no turning back on yourself
  nextDx = ndx; nextDy = ndy;
}

void tap(int x, int y) {
  if (over) { start(); return; }
  if (!running) {
    running = true;
    lastStep = millis();
    lcd.fillRect(40, 176, 400, 30, C_BG);
    // put back anything the message covered
    for (int i = len - 1; i >= 0; i--) cell(bx[i], by[i], i == 0 ? rgb(120, 240, 150) : rgb(40, 170, 90));
    cell(fx, fy, rgb(235, 60, 60), true);
    return;
  }
  int hx = OX + bx[0] * CS + CS / 2, hy = OY + by[0] * CS + CS / 2;
  if (dx != 0) { if (y < hy) turn(0, -1); else turn(0, 1); }
  else { if (x < hx) turn(-1, 0); else turn(1, 0); }
}

void swipe(TouchEvent ev) {
  if (ev == T_SWIPE_LEFT) turn(-1, 0);
  else if (ev == T_SWIPE_RIGHT) turn(1, 0);
  else if (ev == T_SWIPE_UP) turn(0, -1);
  else if (ev == T_SWIPE_DOWN) turn(0, 1);
}

void tick() {
  if (!running || over) return;
  uint32_t now = millis();
  if (now - lastStep < (uint32_t)stepMs()) return;
  lastStep = now;
  dx = nextDx; dy = nextDy;
  int nx = (bx[0] + dx + COLS) % COLS, ny = (by[0] + dy + ROWS) % ROWS;
  bool eat = nx == fx && ny == fy;
  // running into yourself (the tail moves away unless you just ate)
  for (int i = 0; i < len - (eat ? 0 : 1); i++) {
    if (bx[i] == nx && by[i] == ny) {
      over = true; running = false;
      if (score > best) { best = score; gPut("snake_best", best); }
      gPut("snake_lv", score / 50 + 1);
      header();
      gBanner("GAME OVER", C_RED, 150);
      uiText(F_S13, String("Score ") + String(score) + "   Tap to play again", 240, 186, C_WHITE, C_BG, middle_center);
      return;
    }
  }
  if (!eat) { clearCell(bx[len - 1], by[len - 1]); }
  else len++;
  for (int i = len - 1; i > 0; i--) { bx[i] = bx[i - 1]; by[i] = by[i - 1]; }
  bx[0] = nx; by[0] = ny;
  cell(bx[1], by[1], rgb(40, 170, 90));
  cell(nx, ny, rgb(120, 240, 150));
  if (eat) {
    score += 10;
    if (score > best) { best = score; gPut("snake_best", best); }   // kept the moment it's beaten, even if you leave mid-game
    header();
    placeFood();
  }
}

void open() { start(); }

void icon(int cx, int cy) {
  for (int i = 0; i < 5; i++) lcd.fillRoundRect(cx - 30 + i * 12, cy + (i % 2 ? -6 : 6), 11, 11, 3, i == 4 ? rgb(120, 240, 150) : rgb(40, 170, 90));
  lcd.fillCircle(cx + 26, cy - 14, 6, rgb(235, 60, 60));
}
}  // namespace

const Minigame GAME_SNAKE = {"Snake", "Swipe to steer", "snake", icon, open, tap, swipe, nullptr, tick};
