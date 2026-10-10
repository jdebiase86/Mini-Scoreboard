// Sudoku: three levels, a number pad, notes, hints, wrong numbers show in red,
// and the game is kept if you leave. The puzzles are made on the board (every
// one has exactly one answer).
#include "mn_games.h"
#include "mn_ui.h"
#include "mn_lcd.h"

namespace sdk {
// ------------------------------------------------------------- the maker
struct Solver {
  uint8_t g[81];
  uint16_t row[9], col[9], box[9];
  long nodes, nodeLimit;
  int found, limit;
  bool shuffle;
  uint8_t order[9];

  void load(const uint8_t* src) {
    for (int i = 0; i < 9; i++) row[i] = col[i] = box[i] = 0;
    for (int i = 0; i < 81; i++) {
      g[i] = src[i];
      if (g[i]) { uint16_t b = 1 << g[i]; row[i / 9] |= b; col[i % 9] |= b; box[(i / 27) * 3 + (i % 9) / 3] |= b; }
    }
  }
  // fills / counts: stops at `limit` solutions, or when it has tried too long
  bool run() {
    int best = -1, bestN = 10;
    uint16_t bestMask = 0;
    for (int i = 0; i < 81; i++) {
      if (g[i]) continue;
      uint16_t used = row[i / 9] | col[i % 9] | box[(i / 27) * 3 + (i % 9) / 3];
      uint16_t free = ~used & 0x3FE;
      int n = __builtin_popcount(free);
      if (n < bestN) { best = i; bestN = n; bestMask = free; if (n <= 1) break; }
    }
    if (best < 0) { found++; return found >= limit; }   // full
    if (bestN == 0) return false;
    if (++nodes > nodeLimit) return true;               // gave up
    int digits[9], nd = 0;
    for (int d = 1; d <= 9; d++) if (bestMask & (1 << d)) digits[nd++] = d;
    if (shuffle) for (int i = nd - 1; i > 0; i--) { int j = gRand(i + 1); int t = digits[i]; digits[i] = digits[j]; digits[j] = t; }
    for (int k = 0; k < nd; k++) {
      int d = digits[k];
      uint16_t b = 1 << d;
      int r = best / 9, c = best % 9, bx = (best / 27) * 3 + c / 3;
      g[best] = d; row[r] |= b; col[c] |= b; box[bx] |= b;
      bool stop = run();
      if (shuffle && found >= 1) return true;           // one full grid is enough
      g[best] = 0; row[r] &= ~b; col[c] &= ~b; box[bx] &= ~b;
      if (stop) return true;
    }
    return false;
  }
};

// how many answers (up to 2) a puzzle has; 2 also means "too hard to tell"
int countSolutions(const uint8_t* puz) {
  static Solver s;
  s.load(puz);
  s.nodes = 0; s.nodeLimit = 6000; s.found = 0; s.limit = 2; s.shuffle = false;
  s.run();
  if (s.nodes > s.nodeLimit) return 2;
  return s.found;
}

bool makeSolution(uint8_t* out) {
  static Solver s;
  uint8_t empty[81] = {0};
  s.load(empty);
  s.nodes = 0; s.nodeLimit = 200000; s.found = 0; s.limit = 1; s.shuffle = true;
  // run() leaves the filled grid in s.g when it finds one
  s.run();
  if (!s.found) return false;
  memcpy(out, s.g, 81);
  return true;
}

// givens wanted: easy 38, medium 32, hard 27
void makePuzzle(int level, uint8_t* sol, uint8_t* puz) {
  for (int tries = 0; tries < 5 && !makeSolution(sol); tries++) {}
  memcpy(puz, sol, 81);
  int want = level == 0 ? 38 : level == 1 ? 32 : 27;
  int order[81];
  for (int i = 0; i < 81; i++) order[i] = i;
  for (int i = 80; i > 0; i--) { int j = gRand(i + 1); int t = order[i]; order[i] = order[j]; order[j] = t; }
  int givens = 81;
  for (int k = 0; k < 81 && givens > want; k++) {
    int i = order[k];
    uint8_t keep = puz[i];
    puz[i] = 0;
    if (countSolutions(puz) != 1) puz[i] = keep; else givens--;
  }
}

// ----------------------------------------------------------------- the game
struct Save { uint8_t puz[81], cur[81], sol[81]; uint8_t level, mistakes, hints, done; };
Save sv;
uint16_t notes[81];
int sel = -1;
bool noteMode = false, inMenu = true;
uint32_t startedAt = 0;
const int CELL = 30, GX = 8, GY = 40;

bool hasSave() {
  Save t;
  return gGetBlob("sdk_save", &t, sizeof(t)) && !t.done && t.level < 3;
}

void persist() { gPutBlob("sdk_save", &sv, sizeof(sv)); }

int solved() { int n = 0; for (int i = 0; i < 81; i++) if (sv.cur[i] == sv.sol[i]) n++; return n; }

uint16_t digitCol(int i) {
  if (sv.puz[i]) return C_WHITE;
  return sv.cur[i] == sv.sol[i] ? rgb(120, 180, 255) : rgb(255, 90, 90);
}

void drawCell(int i) {
  int r = i / 9, c = i % 9;
  int x0 = GX + c * CELL, y0 = GY + r * CELL;
  bool isSel = i == sel;
  bool peer = sel >= 0 && !isSel && (sel / 9 == r || sel % 9 == c || ((sel / 27) == (r / 3) && (sel % 9) / 3 == c / 3));
  bool same = sel >= 0 && sv.cur[sel] && sv.cur[i] == sv.cur[sel] && !isSel;
  uint16_t bg = isSel ? rgb(48, 90, 170) : same ? rgb(48, 70, 110) : peer ? rgb(26, 32, 46) : C_BG;
  lcd.fillRect(x0 + 1, y0 + 1, CELL - 1, CELL - 1, bg);
  if (sv.cur[i]) {
    uiText(F_B18, String((int)sv.cur[i]), x0 + CELL / 2 + 1, y0 + CELL / 2 + 1, digitCol(i), bg, middle_center);
  } else if (notes[i]) {
    for (int d = 1; d <= 9; d++)
      if (notes[i] & (1 << d)) uiText(F_M12, String(d), x0 + 5 + ((d - 1) % 3) * 10, y0 + 5 + ((d - 1) / 3) * 10, C_GREY, bg, middle_center);
  }
}

void drawGrid() {
  for (int i = 0; i < 81; i++) drawCell(i);
  for (int k = 0; k <= 9; k++) {
    uint16_t col = k % 3 == 0 ? C_GREY : C_EDGE;
    int w = k % 3 == 0 ? 2 : 1;
    lcd.fillRect(GX + k * CELL - (w - 1), GY, w, 9 * CELL + 1, col);
    lcd.fillRect(GX, GY + k * CELL - (w - 1), 9 * CELL + 1, w, col);
  }
}

String clock() {
  int s = (int)((millis() - startedAt) / 1000);
  char b[12];
  snprintf(b, sizeof(b), "%d:%02d", s / 60, s % 60);
  return b;
}

void panel() {
  lcd.fillRect(290, 36, 190, 284, C_BG);
  for (int d = 1; d <= 9; d++) {
    int x0 = 298 + ((d - 1) % 3) * 60, y0 = 44 + ((d - 1) / 3) * 48;
    // used up (nine placed): dimmed
    int n = 0;
    for (int i = 0; i < 81; i++) if (sv.cur[i] == d && sv.cur[i] == sv.sol[i]) n++;
    gButton(x0, y0, x0 + 54, y0 + 42, String(d), n >= 9 ? C_BG : C_TILE_HI, n >= 9 ? C_DIM : C_WHITE, 2);
  }
  gButton(298, 192, 358, 228, "NOTES", noteMode ? C_AUTO_EDGE : C_TILE, C_WHITE, 0);
  gButton(364, 192, 424, 228, "ERASE", C_TILE, C_WHITE, 0);
  gButton(298, 234, 358, 270, "HINT", C_TILE, C_WHITE, 0);
  gButton(364, 234, 424, 270, "NEW", C_TILE, C_WHITE, 0);
  uiText(F_S13, String("Mistakes ") + String((int)sv.mistakes), 360, 290, sv.mistakes ? rgb(255, 130, 130) : C_GREY, C_BG, middle_center);
}

void header() {
  static const char* const NAMES[] = {"Easy", "Medium", "Hard"};
  gTopBar((String("Sudoku  ") + NAMES[sv.level < 3 ? sv.level : 0]).c_str(), clock());
}

void drawPlay() {
  lcd.fillScreen(C_BG);
  header();
  drawGrid();
  panel();
}

void drawMenu() {
  inMenu = true;
  lcd.fillScreen(C_BG);
  gTopBar("Sudoku", "");
  uiText(F_B18, "Pick a level", 240, 66, C_WHITE, C_BG, middle_center);
  static const char* const NAMES[] = {"EASY", "MEDIUM", "HARD"};
  static const char* const SUB[] = {"38 numbers to start with", "32 numbers to start with", "27 numbers to start with"};
  for (int l = 0; l < 3; l++) {
    int y0 = 90 + l * 62;
    bool open = l == 0 || gGet(l == 1 ? "sdk_e" : "sdk_m", 0) > 0;
    gButton(60, y0, 420, y0 + 52, "", open ? C_TILE : C_BG, C_WHITE, 1);
    uiText(F_B24, NAMES[l], 80, y0 + 20, open ? C_WHITE : C_DIM, open ? C_TILE : C_BG, middle_left);
    uiText(F_S13, open ? SUB[l] : (l == 1 ? "Solve an Easy one to unlock" : "Solve a Medium one to unlock"), 80, y0 + 40, C_GREY, open ? C_TILE : C_BG, middle_left);
    int solvedN = gGet(l == 0 ? "sdk_e" : l == 1 ? "sdk_m" : "sdk_h", 0);
    if (solvedN) uiText(F_B16, String(solvedN) + " solved", 404, y0 + 26, C_YELLOW, C_TILE, middle_right);
  }
  if (hasSave()) gButton(60, 280, 420, 314, "CONTINUE MY GAME", C_AUTO_BG, C_WHITE, 1);
}

void newGame(int level) {
  lcd.fillScreen(C_BG);
  gTopBar("Sudoku", "");
  uiText(F_B24, "Making a puzzle...", 240, 160, C_WHITE, C_BG, middle_center);
  makePuzzle(level, sv.sol, sv.puz);
  memcpy(sv.cur, sv.puz, 81);
  sv.level = level; sv.mistakes = 0; sv.hints = 0; sv.done = 0;
  memset(notes, 0, sizeof(notes));
  sel = -1; noteMode = false; inMenu = false;
  startedAt = millis();
  persist();
  drawPlay();
}

void resume() {
  if (!gGetBlob("sdk_save", &sv, sizeof(sv))) return;
  memset(notes, 0, sizeof(notes));
  sel = -1; noteMode = false; inMenu = false;
  startedAt = millis();
  drawPlay();
}

void open() {
  if (hasSave()) drawMenu(); else drawMenu();
}

void win() {
  sv.done = 1;
  persist();
  const char* key = sv.level == 0 ? "sdk_e" : sv.level == 1 ? "sdk_m" : "sdk_h";
  gPut(key, gGet(key, 0) + 1);
  gPut("sdk_lv", gGet("sdk_e", 0) + gGet("sdk_m", 0) + gGet("sdk_h", 0) + 1);
  int stars = sv.mistakes == 0 && sv.hints == 0 ? 3 : sv.mistakes <= 2 ? 2 : 1;
  gBanner("SOLVED!", C_GREEN, 150);
  gStars(240, 188, stars);
  uiText(F_S13, String("Time ") + clock() + "   Tap to play again", 240, 212, C_WHITE, C_BG, middle_center);
}

void place(int d) {
  if (sel < 0 || sv.puz[sel]) return;
  if (noteMode) {
    if (sv.cur[sel]) return;
    notes[sel] ^= 1 << d;
    drawCell(sel);
    return;
  }
  sv.cur[sel] = d;
  notes[sel] = 0;
  if (d != sv.sol[sel]) sv.mistakes++;
  else {   // a right number: it comes off the notes of its row, column and box
    int r = sel / 9, c = sel % 9;
    for (int i = 0; i < 81; i++) {
      if (i / 9 == r || i % 9 == c || ((i / 27) == (r / 3) && (i % 9) / 3 == c / 3)) { if (notes[i] & (1 << d)) { notes[i] &= ~(1 << d); } }
    }
  }
  persist();
  drawGrid();
  panel();
  if (solved() == 81) win();
}

void hint() {
  int i = sel;
  if (i < 0 || (sv.puz[i]) || sv.cur[i] == sv.sol[i]) {   // the chosen one is fine: help with another empty one
    int start = gRand(81);
    i = -1;
    for (int k = 0; k < 81; k++) { int j = (start + k) % 81; if (sv.cur[j] != sv.sol[j]) { i = j; break; } }
  }
  if (i < 0) return;
  sel = i;
  sv.cur[i] = sv.sol[i];
  notes[i] = 0;
  sv.hints++;
  persist();
  drawGrid();
  panel();
  if (solved() == 81) win();
}

void tap(int x, int y) {
  if (inMenu) {
    for (int l = 0; l < 3; l++) {
      int y0 = 90 + l * 62;
      bool open = l == 0 || gGet(l == 1 ? "sdk_e" : "sdk_m", 0) > 0;
      if (open && gIn(x, y, 60, y0, 420, y0 + 52)) { newGame(l); return; }
    }
    if (hasSave() && gIn(x, y, 60, 280, 420, 314)) resume();
    return;
  }
  if (sv.done) { drawMenu(); return; }
  if (gIn(x, y, GX, GY, GX + 9 * CELL, GY + 9 * CELL)) {
    int c = (x - GX) / CELL, r = (y - GY) / CELL, i = r * 9 + c;
    int old = sel;
    sel = i;
    if (old >= 0) { drawGrid(); } else drawGrid();
    return;
  }
  for (int d = 1; d <= 9; d++) {
    int x0 = 298 + ((d - 1) % 3) * 60, y0 = 44 + ((d - 1) / 3) * 48;
    if (gIn(x, y, x0 - 3, y0 - 3, x0 + 57, y0 + 45)) { place(d); return; }
  }
  if (gIn(x, y, 292, 188, 360, 230)) { noteMode = !noteMode; panel(); }
  else if (gIn(x, y, 360, 188, 430, 230)) {
    if (sel >= 0 && !sv.puz[sel]) { sv.cur[sel] = 0; notes[sel] = 0; persist(); drawGrid(); panel(); }
  } else if (gIn(x, y, 292, 230, 360, 274)) hint();
  else if (gIn(x, y, 360, 230, 430, 274)) drawMenu();
}

void tick() {
  static int lastSec = -1;
  if (inMenu || sv.done) return;
  int s = (int)((millis() - startedAt) / 1000);
  if (s != lastSec) { lastSec = s; header(); }
}

void icon(int cx, int cy) {
  lcd.drawRect(cx - 27, cy - 27, 54, 54, C_WHITE);
  lcd.drawFastVLine(cx - 9, cy - 27, 54, C_GREY); lcd.drawFastVLine(cx + 9, cy - 27, 54, C_GREY);
  lcd.drawFastHLine(cx - 27, cy - 9, 54, C_GREY); lcd.drawFastHLine(cx - 27, cy + 9, 54, C_GREY);
  uiText(F_B18, "5", cx, cy, rgb(120, 180, 255), C_BG, middle_center);
}
}  // namespace sdk

const Minigame GAME_SUDOKU = {"Sudoku", "Think it through", "sdk", sdk::icon, sdk::open, sdk::tap, nullptr, nullptr, sdk::tick};
