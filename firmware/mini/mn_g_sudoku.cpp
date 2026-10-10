// Sudoku: three levels, a number pad, notes, hints, wrong numbers show in red,
// and the game is kept if you leave. The puzzles are mixed up from a bank of
// ready-made ones (every one has exactly one answer).
#include "mn_games.h"
#include "mn_ui.h"
#include "mn_lcd.h"

namespace sdk {
// ------------------------------------------------------------- the puzzles
// mn_sudoku_bank.h holds ready-made puzzles (each with exactly one answer, made on a computer). Every game mixes one
// up: the digits are renamed, and rows / columns are shuffled inside their bands, bands and stacks swapped, and the
// grid maybe turned on its side. All of that keeps the single answer, so there are billions of different-looking games
// and nothing heavy runs on the board.
#include "mn_sudoku_bank.h"
void perm3(int* p) { p[0] = 0; p[1] = 1; p[2] = 2; for (int i = 2; i > 0; i--) { int j = gRand(i + 1), t = p[i]; p[i] = p[j]; p[j] = t; } }
void makePuzzle(int level, uint8_t* sol, uint8_t* puz) {
  if (level < 0 || level > 2) level = 0;
  int pick = gRand(SDK_BANK_N);
  const char* ps = SDK_BANK[level][pick][0];
  const char* ss = SDK_BANK[level][pick][1];
  int digit[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  for (int i = 9; i > 1; i--) { int j = 1 + gRand(i), t = digit[i]; digit[i] = digit[j]; digit[j] = t; }
  int band[3], stack[3], rowIn[3][3], colIn[3][3];
  perm3(band); perm3(stack);
  for (int i = 0; i < 3; i++) { perm3(rowIn[i]); perm3(colIn[i]); }
  bool turn = gRand(2);
  for (int r = 0; r < 9; r++) {
    for (int c = 0; c < 9; c++) {
      int sr = band[r / 3] * 3 + rowIn[band[r / 3]][r % 3];
      int sc = stack[c / 3] * 3 + colIn[stack[c / 3]][c % 3];
      if (turn) { int t = sr; sr = sc; sc = t; }
      puz[r * 9 + c] = digit[ps[sr * 9 + sc] - '0'] * (ps[sr * 9 + sc] != '0');
      sol[r * 9 + c] = digit[ss[sr * 9 + sc] - '0'];
    }
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
