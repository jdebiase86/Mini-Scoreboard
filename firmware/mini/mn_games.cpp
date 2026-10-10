#include "mn_games.h"
#include "mn_ui.h"
#include "mn_lcd.h"
#ifndef MN_HOST
#include <Preferences.h>
#include <esp_system.h>
#else
#include <map>
#include <string>
#include <stdlib.h>
#endif

#ifndef MN_GAMES_HELPERS_ONLY
static const Minigame* const GAMES[] = {&GAME_KICK,  &GAME_FREETHROW, &GAME_MEMORY,   &GAME_SUDOKU,   &GAME_SNAKE,    &GAME_2048,
                                        &GAME_SIMON, &GAME_CONNECT4,  &GAME_TRIVIA,   &GAME_REACTION, &GAME_TOSS,
                                        &GAME_BASKET};
static const int NGAMES = sizeof(GAMES) / sizeof(GAMES[0]);

#endif

// ------------------------------------------------------------------ saved
#ifndef MN_HOST
static Preferences prefs;
static bool prefsOpen = false;
static void openPrefs() { if (!prefsOpen) { prefs.begin("games", false); prefsOpen = true; } }
int gGet(const char* key, int dflt) { openPrefs(); return prefs.getInt(key, dflt); }
void gPut(const char* key, int value) { openPrefs(); if (prefs.getInt(key, 0x7fffffff) != value) prefs.putInt(key, value); }
bool gGetBlob(const char* key, void* buf, size_t n) { openPrefs(); return prefs.getBytes(key, buf, n) == n; }
void gPutBlob(const char* key, const void* buf, size_t n) { openPrefs(); prefs.putBytes(key, buf, n); }
uint32_t gRand(uint32_t n) { return n ? esp_random() % n : 0; }
#else
static std::map<std::string, int> hostInts;
int gGet(const char* key, int dflt) { auto it = hostInts.find(key); return it == hostInts.end() ? dflt : it->second; }
void gPut(const char* key, int value) { hostInts[key] = value; }
bool gGetBlob(const char*, void*, size_t) { return false; }
void gPutBlob(const char*, const void*, size_t) {}
uint32_t gRand(uint32_t n) { return n ? (uint32_t)rand() % n : 0; }
#endif

// ----------------------------------------------------------------- helpers
bool gIn(int x, int y, int x0, int y0, int x1, int y1) { return x >= x0 && x < x1 && y >= y0 && y < y1; }

void gTopBar(const char* title, const String& right) {
  uiTile(6, 3, 96, 29, C_TILE, C_EDGE, 13, 1);
  lcd.fillTriangle(18, 16, 26, 9, 26, 23, C_WHITE);
  uiText(F_B12, "BACK", 60, 16, C_WHITE, C_TILE, middle_center);
  uiText(F_B16, title, 112, 16, C_WHITE, C_BG, middle_left);
  lcd.fillRect(300, 3, 178, 26, C_BG);
  if (right.length()) uiText(F_B16, right, 472, 16, C_YELLOW, C_BG, middle_right);
}

bool gBackHit(int x, int y) { return x < 104 && y < 34; }

void gButton(int x0, int y0, int x1, int y1, const String& label, uint16_t fill, uint16_t ink, int font) {
  uiTile(x0, y0, x1, y1, fill, C_EDGE, 10, 1);
  uiText(font == 0 ? F_B12 : font == 2 ? F_B24 : F_B16, label, (x0 + x1) / 2, (y0 + y1) / 2, ink, fill, middle_center);
}

void gBanner(const String& text, uint16_t col, int y) {
  lcd.fillRect(0, y - 26, 480, 52, C_BG);
  lcd.fillRect(0, y - 26, 480, 3, col);
  lcd.fillRect(0, y + 23, 480, 3, col);
  uiText(F_B36, text, 240, y, col, C_BG, middle_center);
}

void gStars(int cx, int cy, int stars, int of) {
  for (int i = 0; i < of; i++) {
    int x = cx + (i - (of - 1) * 0.5f) * 18;
    uint16_t c = i < stars ? C_YELLOW : C_DIM;
    lcd.fillTriangle(x, cy - 8, x - 7, cy + 5, x + 7, cy + 5, c);
    lcd.fillTriangle(x, cy + 8, x - 7, cy - 3, x + 7, cy - 3, c);
  }
}

#ifndef MN_GAMES_HELPERS_ONLY
// ------------------------------------------------------------------- menu
static int menuPage = 0, current = -1;
static const int PER_PAGE = 6;
static int pages() { return (NGAMES + PER_PAGE - 1) / PER_PAGE; }

static void tileRect(int i, int& x0, int& y0, int& x1, int& y1) {
  int col = i % 3, row = i / 3;
  x0 = 6 + col * 158;
  y0 = 40 + row * 140;
  x1 = x0 + 152;
  y1 = y0 + 134;
}

void gamesMenu() {
  current = -1;
  lcd.fillScreen(C_BG);
  gTopBar("Games", "");
  if (pages() > 1) {
    uiText(F_S13, String(menuPage + 1) + " / " + String(pages()), 396, 16, C_GREY, C_BG, middle_center);
    uiTile(340, 4, 372, 28, C_TILE, C_EDGE, 8, 1);
    lcd.fillTriangle(361, 10, 361, 22, 351, 16, menuPage > 0 ? C_WHITE : C_DIM);
    uiTile(430, 4, 462, 28, C_TILE, C_EDGE, 8, 1);
    lcd.fillTriangle(441, 10, 441, 22, 451, 16, menuPage < pages() - 1 ? C_WHITE : C_DIM);
  }
  for (int k = 0; k < PER_PAGE; k++) {
    int i = menuPage * PER_PAGE + k;
    if (i >= NGAMES) break;
    int x0, y0, x1, y1;
    tileRect(k, x0, y0, x1, y1);
    int cx = (x0 + x1) / 2;
    uiTile(x0, y0, x1, y1, C_TILE, C_EDGE, 14, 1);
    GAMES[i]->icon(cx, y0 + 44);
    uiText(F_B16, GAMES[i]->name, cx, y0 + 92, C_WHITE, C_TILE, middle_center);
    uiText(F_M12, GAMES[i]->blurb, cx, y0 + 112, C_GREY, C_TILE, middle_center);
    char key[16];
    snprintf(key, sizeof(key), "%s_lv", GAMES[i]->key);
    int lv = gGet(key, 0);
    if (lv > 0) {
      String t = String("LV ") + String(lv);
      useFont(F_B12);
      int w = lcd.textWidth(t.c_str()) + 14;
      uiTile(x0 + 8, y0 + 8, x0 + 8 + w, y0 + 26, C_AUTO_BG, C_AUTO_EDGE, 9, 1);
      uiText(F_B12, t, x0 + 8 + w / 2, y0 + 17, C_WHITE, C_AUTO_BG, middle_center);
    }
  }
}

void gamesMenuSwipe(TouchEvent ev) {
  int old = menuPage;
  if (ev == T_SWIPE_LEFT && menuPage < pages() - 1) menuPage++;
  if (ev == T_SWIPE_RIGHT && menuPage > 0) menuPage--;
  if (old != menuPage) gamesMenu();
}

GamesResult gamesMenuTap(int x, int y) {
  if (gBackHit(x, y)) return GR_EXIT;
  if (pages() > 1 && y < 34) {
    if (x >= 330 && x < 380) { gamesMenuSwipe(T_SWIPE_RIGHT); return GR_NONE; }
    if (x >= 420) { gamesMenuSwipe(T_SWIPE_LEFT); return GR_NONE; }
  }
  for (int k = 0; k < PER_PAGE; k++) {
    int i = menuPage * PER_PAGE + k;
    if (i >= NGAMES) break;
    int x0, y0, x1, y1;
    tileRect(k, x0, y0, x1, y1);
    if (gIn(x, y, x0, y0, x1, y1)) {
      current = i;
      GAMES[i]->open();
      return GR_OPENED;
    }
  }
  return GR_NONE;
}

// ---------------------------------------------------------------- playing
bool gamesPlaying() { return current >= 0; }

bool gamesTap(int x, int y) {
  if (current < 0) return true;
  if (gBackHit(x, y)) { current = -1; return true; }
  if (GAMES[current]->tap) GAMES[current]->tap(x, y);
  return false;
}
void gamesSwipe(TouchEvent ev) { if (current >= 0 && GAMES[current]->swipe) GAMES[current]->swipe(ev); }
void gamesGesture(const TouchGesture& g) { if (current >= 0 && GAMES[current]->gesture) GAMES[current]->gesture(g); }
void gamesLoop() { if (current >= 0 && GAMES[current]->loop) GAMES[current]->loop(); }
#endif
