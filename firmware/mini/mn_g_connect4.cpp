// Connect Four against the mini. Three levels (the mini looks 1, 4 or 7 moves
// ahead); beat one to unlock the next.
#include "mn_games.h"
#include "mn_ui.h"
#include "mn_lcd.h"

namespace {
const int COLS = 7, ROWS = 6, CS = 40, OX = 16, OY = 54;
int8_t bd[COLS][ROWS];       // 0 empty, 1 you, 2 the mini; [col][row], row 0 = the bottom
int diff = 0;                // 0 easy, 1 medium, 2 hard
int turn = 1;                // whose go
bool over = false;
int winCells[4][2], hasWin = 0;
// a falling piece
bool dropping = false;
int dropCol, dropRow, dropAt, dropWho;
uint32_t dropT = 0, aiAt = 0;
bool aiWaiting = false;

int height(int c) { for (int r = 0; r < ROWS; r++) if (!bd[c][r]) return r; return ROWS; }

bool four(int who, int c0, int r0, int dc, int dr, int out[4][2]) {
  for (int k = 0; k < 4; k++) {
    int c = c0 + dc * k, r = r0 + dr * k;
    if (c < 0 || c >= COLS || r < 0 || r >= ROWS || bd[c][r] != who) return false;
  }
  if (out) for (int k = 0; k < 4; k++) { out[k][0] = c0 + dc * k; out[k][1] = r0 + dr * k; }
  return true;
}

bool wins(int who, int out[4][2]) {
  for (int c = 0; c < COLS; c++)
    for (int r = 0; r < ROWS; r++)
      if (four(who, c, r, 1, 0, out) || four(who, c, r, 0, 1, out) || four(who, c, r, 1, 1, out) || four(who, c, r, 1, -1, out)) return true;
  return false;
}

bool full() { for (int c = 0; c < COLS; c++) if (height(c) < ROWS) return false; return true; }

// how good the board is for `who` (windows of four)
int evaluate(int who) {
  int opp = 3 - who, score = 0;
  for (int r = 0; r < ROWS; r++) if (bd[3][r] == who) score += 3;
  static const int DC[4] = {1, 0, 1, 1}, DR[4] = {0, 1, 1, -1};
  for (int c = 0; c < COLS; c++)
    for (int r = 0; r < ROWS; r++)
      for (int d = 0; d < 4; d++) {
        int ec = c + DC[d] * 3, er = r + DR[d] * 3;
        if (ec < 0 || ec >= COLS || er < 0 || er >= ROWS) continue;
        int mine = 0, theirs = 0;
        for (int k = 0; k < 4; k++) { int v = bd[c + DC[d] * k][r + DR[d] * k]; if (v == who) mine++; else if (v == opp) theirs++; }
        if (theirs == 0) score += mine == 3 ? 5 : mine == 2 ? 2 : 0;
        if (mine == 0) score -= theirs == 3 ? 6 : theirs == 2 ? 2 : 0;
      }
  return score;
}

const int ORDER[COLS] = {3, 2, 4, 1, 5, 0, 6};

int negamax(int who, int depth, int alpha, int beta) {
  if (wins(3 - who, nullptr)) return -10000 - depth;   // the other side just won
  if (full()) return 0;
  if (depth == 0) return evaluate(who) - evaluate(3 - who);
  int best = -100000;
  for (int k = 0; k < COLS; k++) {
    int c = ORDER[k], h = height(c);
    if (h >= ROWS) continue;
    bd[c][h] = who;
    int v = -negamax(3 - who, depth - 1, -beta, -alpha);
    bd[c][h] = 0;
    if (v > best) best = v;
    if (v > alpha) alpha = v;
    if (alpha >= beta) break;
  }
  return best;
}

int aiMove() {
  int depth = diff == 0 ? 1 : diff == 1 ? 4 : 7;
  int bestC = -1, bestV = -1000000;
  // easy: sometimes just a random column
  if (diff == 0 && gRand(3) == 0) { int tries = 0; while (tries++ < 20) { int c = gRand(COLS); if (height(c) < ROWS) return c; } }
  for (int k = 0; k < COLS; k++) {
    int c = ORDER[k], h = height(c);
    if (h >= ROWS) continue;
    bd[c][h] = 2;
    int v = -negamax(1, depth - 1, -100000, 100000);
    bd[c][h] = 0;
    if (bestC < 0 || v > bestV) { bestV = v; bestC = c; }
  }
  return bestC;
}

// ------------------------------------------------------------------ drawing
void drawCell(int c, int r, int who, bool win = false) {
  int x = OX + c * CS, y = OY + (ROWS - 1 - r) * CS;
  lcd.fillRect(x, y, CS, CS, rgb(24, 60, 150));
  uint16_t col = who == 1 ? rgb(230, 60, 60) : who == 2 ? rgb(250, 210, 40) : C_BG;
  lcd.fillCircle(x + CS / 2, y + CS / 2, CS / 2 - 4, col);
  if (win) lcd.drawCircle(x + CS / 2, y + CS / 2, CS / 2 - 2, C_WHITE);
}

void drawBoard() {
  for (int c = 0; c < COLS; c++) for (int r = 0; r < ROWS; r++) drawCell(c, r, bd[c][r]);
}

void panel(const char* msg, uint16_t col) {
  lcd.fillRect(310, 36, 170, 284, C_BG);
  static const char* const N[] = {"EASY", "MEDIUM", "HARD"};
  for (int d = 0; d < 3; d++) {
    bool open = d == 0 || gGet(d == 1 ? "c4_e" : "c4_m", 0) > 0;
    gButton(318, 50 + d * 40, 470, 84 + d * 40, N[d], d == diff ? C_AUTO_EDGE : open ? C_TILE : C_BG, open ? C_WHITE : C_DIM, 0);
  }
  uiText(F_S13, "Win to unlock the next", 395, 178, C_DIM, C_BG, middle_center);
  uiText(F_B16, msg, 395, 214, col, C_BG, middle_center);
  gButton(318, 250, 470, 290, "NEW GAME", C_TILE_HI, C_WHITE, 1);
}

void newGame() {
  memset(bd, 0, sizeof(bd));
  turn = 1; over = false; hasWin = 0; dropping = false; aiWaiting = false;
  lcd.fillScreen(C_BG);
  gTopBar("Connect Four", "");
  lcd.fillRect(OX - 4, OY - 4, COLS * CS + 8, ROWS * CS + 8, rgb(24, 60, 150));
  drawBoard();
  panel("Your turn (red)", rgb(230, 100, 100));
}

void open() {
  diff = gGet("c4_d", 0);
  newGame();
}

void finishTurn() {
  int w[4][2];
  if (wins(turn, w)) {
    over = true;
    for (int k = 0; k < 4; k++) drawCell(w[k][0], w[k][1], turn, true);
    if (turn == 1) {
      panel("You win!", C_GREEN);
      const char* key = diff == 0 ? "c4_e" : diff == 1 ? "c4_m" : "c4_h";
      gPut(key, gGet(key, 0) + 1);
      gPut("c4_lv", gGet("c4_e", 0) + gGet("c4_m", 0) + gGet("c4_h", 0) + 1);
    } else panel("The mini wins", C_RED);
    return;
  }
  if (full()) { over = true; panel("A draw", C_YELLOW); return; }
  turn = 3 - turn;
  if (turn == 2) { aiWaiting = true; aiAt = millis() + 450; panel("Thinking...", C_GREY); }
  else panel("Your turn (red)", rgb(230, 100, 100));
}

void startDrop(int col, int who) {
  dropping = true; dropCol = col; dropRow = height(col); dropWho = who; dropAt = ROWS; dropT = millis();
}

void tap(int x, int y) {
  if (gIn(x, y, 312, 46, 480, 170)) {
    for (int d = 0; d < 3; d++) {
      bool open = d == 0 || gGet(d == 1 ? "c4_e" : "c4_m", 0) > 0;
      if (open && gIn(x, y, 312, 46 + d * 40, 480, 88 + d * 40)) { diff = d; gPut("c4_d", d); newGame(); return; }
    }
    return;
  }
  if (gIn(x, y, 312, 246, 480, 294)) { newGame(); return; }
  if (over || dropping || aiWaiting || turn != 1) return;
  if (!gIn(x, y, OX - 6, 36, OX + COLS * CS + 6, 320)) return;
  int c = (x - OX) / CS;
  if (c < 0 || c >= COLS || height(c) >= ROWS) return;
  startDrop(c, 1);
}

void tick() {
  uint32_t now = millis();
  if (dropping && now - dropT > 45) {
    dropT = now;
    // the piece falls one cell at a time
    if (dropAt < ROWS) drawCell(dropCol, dropAt, 0);
    dropAt--;
    if (dropAt <= dropRow) {
      dropping = false;
      bd[dropCol][dropRow] = dropWho;
      drawCell(dropCol, dropRow, dropWho);
      finishTurn();
    } else drawCell(dropCol, dropAt, dropWho);
    return;
  }
  if (aiWaiting && now >= aiAt && !dropping) {
    aiWaiting = false;
    int c = aiMove();
    if (c >= 0) startDrop(c, 2);
  }
}

void icon(int cx, int cy) {
  uiTile(cx - 32, cy - 24, cx + 32, cy + 24, rgb(24, 60, 150), rgb(24, 60, 150), 6, 1);
  for (int i = 0; i < 4; i++) for (int j = 0; j < 3; j++) {
    int v = (i + j) % 3;
    lcd.fillCircle(cx - 22 + i * 15, cy - 14 + j * 14, 5, v == 0 ? rgb(230, 60, 60) : v == 1 ? rgb(250, 210, 40) : C_BG);
  }
}
}  // namespace

const Minigame GAME_CONNECT4 = {"Connect Four", "Beat the mini", "c4", icon, open, tap, nullptr, nullptr, tick};
