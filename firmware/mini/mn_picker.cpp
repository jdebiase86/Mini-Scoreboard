#include "mn_picker.h"
#include "mn_ui.h"
#include "mn_settings.h"
#include "mn_log.h"

enum Screen { S_LEAGUES, S_CONFS, S_LIST };
static Screen screen = S_LEAGUES;
static int league = 0;         // League for the list
static int conf = -1;          // college: index into CONFS, -1 = not college
static int page = 0;
static int picks[MAX_PICKS], npicks = 0;   // the working copy
static uint32_t fullAt = 0;    // when "10 is the most" was shown

// College conferences by ESPN group number (mn_teams.h); the last one takes
// everything else (Notre Dame, Army, Navy)
struct Conf { const char* name; int group; };
static const Conf CONFS[] = {{"SEC", 8}, {"BIG TEN", 5}, {"ACC", 1}, {"BIG 12", 4}, {"OTHERS", -1}};
static const int NCONFS = sizeof(CONFS) / sizeof(CONFS[0]);
static const char* const LEAGUE_TILE[L_COUNT] = {"NFL", "COLLEGE", "MLB", "NHL", "NBA"};

static const int PER_PAGE = 10;

static bool inConf(int t, int c) {
  int g = TEAMS[t].group;
  if (CONFS[c].group >= 0) return g == CONFS[c].group;
  for (int k = 0; k < NCONFS - 1; k++) if (g == CONFS[k].group) return false;
  return true;
}

// the teams the list shows, in team-list order
static int listTeams(int* out) {
  int n = 0;
  for (int t = 0; t < NTEAMS; t++) {
    if (TEAMS[t].league != league) continue;
    if (league == L_CFB && conf >= 0 && !inConf(t, conf)) continue;
    out[n++] = t;
  }
  return n;
}

static bool picked(int t) {
  for (int i = 0; i < npicks; i++) if (picks[i] == t) return true;
  return false;
}

static int countWhere(bool (*f)(int, int), int arg) {
  int n = 0;
  for (int i = 0; i < npicks; i++) if (f(picks[i], arg)) n++;
  return n;
}
static bool isLeague(int t, int l) { return TEAMS[t].league == l; }
static bool isConf(int t, int c) { return TEAMS[t].league == L_CFB && inConf(t, c); }

static String pickedText(int n) { return n ? String(n) + " picked" : String("none yet"); }

// ------------------------------------------------------------------ drawing
static void slotRect(int i, int& x0, int& y0, int& x1, int& y1) {   // same grid as the home screen
  x0 = 6 + (i % 3) * 158;
  y0 = 34 + (i / 3) * 143;
  x1 = x0 + 152;
  y1 = y0 + 137;
}

static void backButton() {
  uiTile(6, 3, 96, 29, C_TILE, C_EDGE, 13);
  lcd.fillTriangle(18, 16, 26, 9, 26, 23, C_WHITE);
  uiText(F_B12, "BACK", 60, 16, C_WHITE, C_TILE, middle_center);
}
static bool backHit(int x, int y) { return x < 112 && y < 34; }

static void headerCount(int n) {
  lcd.fillRect(330, 4, 146, 26, C_BG);
  if (fullAt && millis() - fullAt < 2500)
    uiText(F_B12, "10 is the most", 470, 16, C_YELLOW, C_BG, middle_right);
  else
    uiText(F_B12, pickedText(n), 470, 16, n ? C_GREEN : C_DIM, C_BG, middle_right);
}

static void drawLeagues() {
  lcd.fillScreen(C_BG);
  uiText(F_B16, "Pick your teams", 12, 16, C_WHITE, C_BG, middle_left);
  uiText(F_S13, String(npicks) + " of " + String(MAX_PICKS) + " picked", 470, 16, C_GREY, C_BG, middle_right);
  for (int l = 0; l < L_COUNT; l++) {
    int x0, y0, x1, y1;
    slotRect(l, x0, y0, x1, y1);
    int cx = (x0 + x1) / 2, n = countWhere(isLeague, l);
    uiTile(x0, y0, x1, y1, C_TILE, C_EDGE);
    uiText(strlen(LEAGUE_TILE[l]) < 5 ? F_B36 : F_B24, LEAGUE_TILE[l], cx, y0 + 60, C_WHITE, C_TILE, middle_center);
    uiText(n ? F_B12 : F_S13, pickedText(n), cx, y0 + 100, n ? C_GREEN : C_DIM, C_TILE, middle_center);
  }
  int x0, y0, x1, y1;
  slotRect(5, x0, y0, x1, y1);
  uint16_t bg = rgb(22, 70, 40);
  uiTile(x0, y0, x1, y1, bg, C_GREEN, 14, 2);
  int cx = (x0 + x1) / 2;
  lcd.drawWideLine(cx - 22, y0 + 52, cx - 6, y0 + 68, 3.5f, C_WHITE);   // a tick
  lcd.drawWideLine(cx - 6, y0 + 68, cx + 24, y0 + 36, 3.5f, C_WHITE);
  uiText(F_B24, "DONE", cx, y0 + 98, C_WHITE, bg, middle_center);
  uiText(F_M12, "save and go home", cx, y0 + 120, rgb(170, 220, 185), bg, middle_center);
}

static void drawConfs() {
  lcd.fillScreen(C_BG);
  backButton();
  uiText(F_B16, "College football", 240, 16, C_WHITE, C_BG, middle_center);
  headerCount(countWhere(isLeague, L_CFB));
  for (int c = 0; c < NCONFS; c++) {
    int x0, y0, x1, y1;
    slotRect(c, x0, y0, x1, y1);
    int cx = (x0 + x1) / 2, n = countWhere(isConf, c);
    uiTile(x0, y0, x1, y1, C_TILE, C_EDGE);
    uiText(strlen(CONFS[c].name) < 6 ? F_B36 : F_B24, CONFS[c].name, cx, y0 + 60, C_WHITE, C_TILE, middle_center);
    if (n) uiText(F_B12, pickedText(n), cx, y0 + 100, C_GREEN, C_TILE, middle_center);
    else if (CONFS[c].group < 0) uiText(F_M12, "Notre Dame & more", cx, y0 + 100, C_GREY, C_TILE, middle_center);
  }
}

static void rowRect(int i, int& x0, int& y0) {
  x0 = 6 + (i / 5) * 202;
  y0 = 36 + (i % 5) * 56;
}

static void drawRow(int i, int t) {
  int x0, y0;
  rowRect(i, x0, y0);
  bool on = picked(t);
  uint16_t bg = on ? C_TILE_HI : C_TILE;
  lcd.fillRect(x0, y0, 196, 50, C_BG);
  uiTile(x0, y0, x0 + 196, y0 + 50, bg, on ? C_GREEN : C_EDGE, 10, on ? 2 : 1);
  int bx = x0 + 12, by = y0 + 14;
  if (on) {
    lcd.fillRoundRect(bx, by, 22, 22, 5, C_GREEN);
    lcd.drawWideLine(bx + 5, by + 11, bx + 9, by + 16, 1.5f, C_WHITE);
    lcd.drawWideLine(bx + 9, by + 16, bx + 17, by + 6, 1.5f, C_WHITE);
  } else {
    lcd.drawRoundRect(bx, by, 22, 22, 5, C_GREY);
    lcd.drawRoundRect(bx + 1, by + 1, 20, 20, 4, C_GREY);
  }
  useFont(F_M15);
  String nm = TEAMS[t].name;
  FontId f = lcd.textWidth(nm.c_str()) <= 144 ? F_M15 : F_S13;
  uiText(f, nm, x0 + 44, y0 + 25, C_WHITE, bg, middle_left);
}

static void drawArrow(bool up, bool active) {
  int y0 = up ? 36 : 204;
  uiTile(414, y0, 474, y0 + 106, C_TILE, C_EDGE, 12);
  int cy = y0 + 53;
  uint16_t col = active ? C_WHITE : C_DIM;
  if (up) lcd.fillTriangle(444, cy - 16, 424, cy + 10, 464, cy + 10, col);
  else lcd.fillTriangle(444, cy + 16, 424, cy - 10, 464, cy - 10, col);
}

static void drawList() {
  static int teams[NTEAMS];
  int n = listTeams(teams);
  int pages = (n + PER_PAGE - 1) / PER_PAGE;
  if (page >= pages) page = pages - 1;
  if (page < 0) page = 0;
  lcd.fillScreen(C_BG);
  backButton();
  String title = league == L_CFB && conf >= 0 ? String(CONFS[conf].name) : String(LEAGUE_NAMES[league]);
  if (title == "OTHERS") title = "Other schools";
  uiText(F_B16, title, 240, 16, C_WHITE, C_BG, middle_center);
  headerCount(league == L_CFB && conf >= 0 ? countWhere(isConf, conf) : countWhere(isLeague, league));
  for (int i = 0; i < PER_PAGE && page * PER_PAGE + i < n; i++) drawRow(i, teams[page * PER_PAGE + i]);
  if (pages > 1) {
    drawArrow(true, page > 0);
    drawArrow(false, page < pages - 1);
    uiText(F_S13, String(page + 1) + " of " + String(pages), 444, 172, C_GREY, C_BG, middle_center);
  }
}

void pickerDraw() {
  if (screen == S_LEAGUES) drawLeagues();
  else if (screen == S_CONFS) drawConfs();
  else drawList();
}

void pickerStart() {
  npicks = settings.npicks;
  memcpy(picks, settings.picks, sizeof(int) * npicks);
  screen = S_LEAGUES;
  fullAt = 0;
  pickerDraw();
}

// ---------------------------------------------------------------------- taps
static int slotHit(int x, int y) {
  for (int i = 0; i < 6; i++) {
    int x0, y0, x1, y1;
    slotRect(i, x0, y0, x1, y1);
    if (x >= x0 && x < x1 && y >= y0 && y < y1) return i;
  }
  return -1;
}

static void flashSlot(int i) {
  int x0, y0, x1, y1;
  slotRect(i, x0, y0, x1, y1);
  for (int w = 0; w < 2; w++) lcd.drawRoundRect(x0 + w, y0 + w, x1 - x0 - 2 * w, y1 - y0 - 2 * w, 14 - w, C_WHITE);
  delay(90);
}

static void toggle(int t) {
  for (int i = 0; i < npicks; i++)
    if (picks[i] == t) {
      memmove(&picks[i], &picks[i + 1], sizeof(int) * (npicks - i - 1));
      npicks--;
      return;
    }
  if (npicks >= MAX_PICKS) { fullAt = millis(); return; }
  picks[npicks++] = t;
}

bool pickerTap(int x, int y) {
  switch (screen) {
    case S_LEAGUES: {
      int s = slotHit(x, y);
      if (s < 0) return false;
      flashSlot(s);
      if (s == 5) {   // DONE
        String keys;
        for (int i = 0; i < npicks; i++) { if (i) keys += ","; keys += teamKey(picks[i]); }
        settings.setPicksFromString(keys);
        settings.save();
        mnLog("teams picked on the screen: %s", keys.c_str());
        return true;
      }
      league = s;
      page = 0;
      conf = -1;
      screen = league == L_CFB ? S_CONFS : S_LIST;
      pickerDraw();
      return false;
    }
    case S_CONFS: {
      if (backHit(x, y)) { screen = S_LEAGUES; pickerDraw(); return false; }
      int s = slotHit(x, y);
      if (s < 0 || s >= NCONFS) return false;
      flashSlot(s);
      conf = s;
      page = 0;
      screen = S_LIST;
      pickerDraw();
      return false;
    }
    case S_LIST: {
      if (backHit(x, y)) {
        screen = league == L_CFB ? S_CONFS : S_LEAGUES;
        pickerDraw();
        return false;
      }
      static int teams[NTEAMS];
      int n = listTeams(teams);
      int pages = (n + PER_PAGE - 1) / PER_PAGE;
      if (x >= 406) {   // the arrow column (a bit of slack to the left)
        int was = page;
        if (y >= 30 && y < 150 && page > 0) page--;
        if (y >= 196 && page < pages - 1) page++;
        if (page != was) pickerDraw();
        return false;
      }
      for (int i = 0; i < PER_PAGE && page * PER_PAGE + i < n; i++) {
        int x0, y0;
        rowRect(i, x0, y0);
        if (x >= x0 && x < x0 + 200 && y >= y0 - 3 && y < y0 + 53) {
          int t = teams[page * PER_PAGE + i];
          toggle(t);
          drawRow(i, t);
          headerCount(league == L_CFB && conf >= 0 ? countWhere(isConf, conf) : countWhere(isLeague, league));
          return false;
        }
      }
      return false;
    }
  }
  return false;
}

void pickerLoop() {
  if (!fullAt || millis() - fullAt < 2500) return;
  fullAt = 0;
  if (screen == S_LIST) headerCount(league == L_CFB && conf >= 0 ? countWhere(isConf, conf) : countWhere(isLeague, league));
}

void pickerSwipe(bool up) {
  if (screen != S_LIST) return;
  static int teams[NTEAMS];
  int pages = (listTeams(teams) + PER_PAGE - 1) / PER_PAGE;
  int was = page;
  if (up && page < pages - 1) page++;
  if (!up && page > 0) page--;
  if (page != was) pickerDraw();
}
