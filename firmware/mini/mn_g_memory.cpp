// Memory match with the team logos already stored on the board (your teams
// and their opponents; plain symbols fill in if there aren't enough). Levels
// get harder: more cards, then a move limit, then a timer as well.
#include "mn_games.h"
#include "mn_ui.h"
#include "mn_lcd.h"
#include "mn_logo.h"
#include "mn_net.h"
#include "mn_settings.h"

namespace {
struct Face {
  TeamSide side;       // a logo, or
  int symbol = -1;     // a plain symbol 0-7 when there's no logo for it
  uint16_t col = 0;
};

const int MAXFACE = 12;
Face pool[MAXFACE];
int npool = 0;

struct Level { int pairs, cols, rows, moves, seconds; };
// pairs, grid, move limit (0 = none), seconds (0 = none)
const Level LEVELS[] = {{3, 3, 2, 0, 0},  {4, 4, 2, 0, 0},  {6, 4, 3, 0, 0},   {6, 4, 3, 18, 0},
                        {6, 4, 3, 15, 60}, {6, 4, 3, 13, 45}, {6, 4, 3, 12, 40}, {6, 4, 3, 11, 35}};
const int NLEVELS = sizeof(LEVELS) / sizeof(LEVELS[0]);

struct Card { uint8_t face; uint8_t state; };   // state 0 down, 1 up, 2 matched
Card cards[12];
int level = 1, nCards = 0, cols = 0, rows = 0, moves = 0, flipped[2], nFlipped = 0;
int matched = 0, moveLimit = 0, seconds = 0;
uint32_t startedAt = 0, checkAt = 0, shownLeft = 9999;
bool over = false, won = false;
const int CW = 108, CH = 80, GAP = 8;

void addSide(const TeamSide& s) {
  if (!s.abbr[0] || npool >= MAXFACE) return;
  for (int i = 0; i < npool; i++) if (!strcmp(pool[i].side.abbr, s.abbr)) return;
  pool[npool].side = s;
  pool[npool].symbol = -1;
  pool[npool].col = s.color ? rgb((s.color >> 16) & 255, (s.color >> 8) & 255, s.color & 255) : rgb(60, 80, 140);
  npool++;
}

void buildPool() {
  npool = 0;
  for (int i = 0; i < settings.npicks; i++) {
    Game g;
    if (netGame(i, g) && g.state != GS_NONE) { addSide(g.mine()); addSide(g.them()); }
  }
  // my teams with no game yet: their letters
  for (int i = 0; i < settings.npicks && npool < MAXFACE; i++) {
    TeamSide s;
    strncpy(s.abbr, TEAMS[settings.picks[i]].abbr, 7);
    addSide(s);
  }
}

void symbol(int k, int cx, int cy, uint16_t c) {
  switch (k % 8) {
    case 0: lcd.fillTriangle(cx, cy - 22, cx - 20, cy + 14, cx + 20, cy + 14, c); lcd.fillTriangle(cx, cy + 22, cx - 20, cy - 10, cx + 20, cy - 10, c); break;
    case 1: lcd.fillCircle(cx, cy, 22, c); lcd.fillCircle(cx, cy, 10, C_TILE_HI); break;
    case 2: lcd.fillRect(cx - 20, cy - 20, 40, 40, c); break;
    case 3: lcd.fillTriangle(cx, cy - 22, cx - 24, cy + 20, cx + 24, cy + 20, c); break;
    case 4: lcd.fillTriangle(cx, cy - 24, cx - 20, cy, cx + 20, cy, c); lcd.fillTriangle(cx, cy + 24, cx - 20, cy, cx + 20, cy, c); break;
    case 5: lcd.fillRect(cx - 8, cy - 22, 16, 44, c); lcd.fillRect(cx - 22, cy - 8, 44, 16, c); break;
    case 6: lcd.fillTriangle(cx + 6, cy - 24, cx - 14, cy + 4, cx + 2, cy + 4, c); lcd.fillTriangle(cx - 6, cy + 24, cx + 14, cy - 4, cx - 2, cy - 4, c); break;
    default: lcd.fillCircle(cx - 11, cy - 8, 12, c); lcd.fillCircle(cx + 11, cy - 8, 12, c); lcd.fillTriangle(cx - 22, cy - 2, cx + 22, cy - 2, cx, cy + 22, c); break;
  }
}

void cardRect(int i, int& x0, int& y0) {
  int w = cols * (CW + GAP) - GAP, h = rows * (CH + GAP) - GAP;
  x0 = (480 - w) / 2 + (i % cols) * (CW + GAP);
  y0 = 36 + (284 - h) / 2 + 6 + (i / cols) * (CH + GAP);
}

void drawCard(int i) {
  int x0, y0;
  cardRect(i, x0, y0);
  int x1 = x0 + CW, y1 = y0 + CH, cx = x0 + CW / 2, cy = y0 + CH / 2;
  const Card& c = cards[i];
  if (c.state == 0) {
    uiTile(x0, y0, x1, y1, rgb(30, 50, 100), rgb(70, 110, 190), 12, 2);
    uiText(F_B36, "?", cx, cy, rgb(120, 160, 230), rgb(30, 50, 100), middle_center);
    return;
  }
  uint16_t bg = c.state == 2 ? rgb(22, 60, 40) : C_TILE_HI;
  uiTile(x0, y0, x1, y1, bg, c.state == 2 ? C_GREEN : C_WHITE, 12, 2);
  const Face& f = pool[c.face];
  if (f.symbol >= 0) { symbol(f.symbol, cx, cy, f.col); return; }
  if (!logoDraw(f.side, cx, cy, 76, bg)) {   // no logo yet: the letters on the team colour
    lcd.fillRoundRect(x0 + 14, y0 + 12, CW - 28, CH - 24, 10, f.col);
    uiText(F_B24, f.side.abbr, cx, cy, C_WHITE, f.col, middle_center);
  }
}

void status() {
  String r;
  if (moveLimit) r = String("Moves ") + String(moves) + "/" + String(moveLimit);
  else r = String("Moves ") + String(moves);
  if (seconds) {
    int left = seconds - (int)((millis() - startedAt) / 1000);
    if (left < 0) left = 0;
    r += String("  ") + String(left) + "s";
  }
  gTopBar((String("Memory  L") + String(level)).c_str(), r);
}

void deal() {
  buildPool();
  const Level& L = LEVELS[level - 1 < NLEVELS ? level - 1 : NLEVELS - 1];
  int pairs = L.pairs;
  // the faces: logos first, then symbols to make up the number
  Face use[12];
  int n = 0;
  for (int i = 0; i < npool && n < pairs; i++) use[n++] = pool[i];
  for (int s = 0; n < pairs; s++, n++) { use[n] = Face(); use[n].symbol = s; use[n].col = s % 2 ? rgb(250, 210, 40) : rgb(70, 190, 120); }
  for (int i = 0; i < n; i++) pool[i] = use[i];
  npool = n;
  nCards = pairs * 2;
  cols = L.cols; rows = L.rows; moveLimit = L.moves; seconds = L.seconds;
  for (int i = 0; i < nCards; i++) { cards[i].face = i / 2; cards[i].state = 0; }
  for (int i = nCards - 1; i > 0; i--) { int j = gRand(i + 1); Card t = cards[i]; cards[i] = cards[j]; cards[j] = t; }
  moves = 0; nFlipped = 0; matched = 0; over = false; won = false;
  startedAt = millis(); checkAt = 0; shownLeft = 9999;
  lcd.fillScreen(C_BG);
  status();
  for (int i = 0; i < nCards; i++) drawCard(i);
}

void open() {
  level = gGet("mem_lv", 1);
  if (level < 1) level = 1;
  deal();
}

void finish(bool win) {
  over = true; won = win;
  if (win) {
    const Level& L = LEVELS[level - 1 < NLEVELS ? level - 1 : NLEVELS - 1];
    int stars = moves <= L.pairs + 1 ? 3 : moves <= L.pairs + 3 ? 2 : 1;
    gBanner(String("YOU WIN!"), C_GREEN, 178);
    gStars(240, 214, stars);
    uiText(F_S13, "Tap for the next level", 240, 238, C_WHITE, C_BG, middle_center);
    level++;
    gPut("mem_lv", level);
  } else {
    gBanner(String("TIME'S UP"), C_RED, 178);
    uiText(F_S13, "Tap to try this level again", 240, 214, C_WHITE, C_BG, middle_center);
  }
}

void tap(int x, int y) {
  if (over) { deal(); return; }
  if (nFlipped >= 2) return;
  for (int i = 0; i < nCards; i++) {
    int x0, y0;
    cardRect(i, x0, y0);
    if (gIn(x, y, x0 - 3, y0 - 3, x0 + CW + 3, y0 + CH + 3) && cards[i].state == 0) {
      cards[i].state = 1;
      drawCard(i);
      flipped[nFlipped++] = i;
      if (nFlipped == 2) { moves++; checkAt = millis() + 750; status(); }
      return;
    }
  }
}

void tick() {
  if (over) return;
  if (nFlipped == 2 && millis() >= checkAt) {
    int a = flipped[0], b = flipped[1];
    if (cards[a].face == cards[b].face) { cards[a].state = cards[b].state = 2; matched++; }
    else cards[a].state = cards[b].state = 0;
    drawCard(a); drawCard(b);
    nFlipped = 0;
    if (matched * 2 >= nCards) { finish(true); return; }
    if (moveLimit && moves >= moveLimit) { finish(false); return; }
  }
  if (seconds) {
    int left = seconds - (int)((millis() - startedAt) / 1000);
    if (left != (int)shownLeft) { shownLeft = left; status(); }
    if (left <= 0) finish(false);
  }
}

void icon(int cx, int cy) {
  for (int i = 0; i < 2; i++) {
    uiTile(cx - 38 + i * 40, cy - 18, cx - 6 + i * 40, cy + 18, i ? C_TILE_HI : rgb(30, 50, 100), i ? C_WHITE : rgb(70, 110, 190), 6, 2);
  }
  lcd.fillCircle(cx + 18, cy, 8, C_YELLOW);
}
}  // namespace

const Minigame GAME_MEMORY = {"Logo Match", "Find the pairs", "mem", icon, open, tap, nullptr, nullptr, tick};
