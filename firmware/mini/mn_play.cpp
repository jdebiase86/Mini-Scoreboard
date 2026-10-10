#include "mn_play.h"
#include "mn_ui.h"
#include "mn_logo.h"
#include "mn_lcd.h"
#include "mn_settings.h"

static void text(FontId f, const String& s, int x, int y, uint16_t col, uint16_t bg, textdatum_t d) {
  uiText(f, s, x, y, col, bg, d);
}

static uint32_t fnv(uint32_t h, const void* p, size_t n) {
  const uint8_t* b = (const uint8_t*)p;
  while (n--) { h ^= *b++; h *= 16777619u; }
  return h;
}

static uint32_t gameHash(uint32_t h, const Game& g, bool known) {
  h = fnv(h, &known, 1);
  if (!known) return h;
  h = fnv(h, &g.state, 1);
  h = fnv(h, &g.start, sizeof(g.start));
  h = fnv(h, &g.period, 1);
  h = fnv(h, g.clock, strlen(g.clock));
  h = fnv(h, g.detail, strlen(g.detail));
  h = fnv(h, &g.away.score, sizeof(int16_t));
  h = fnv(h, &g.home.score, sizeof(int16_t));
  h = fnv(h, g.away.abbr, strlen(g.away.abbr));
  h = fnv(h, g.home.abbr, strlen(g.home.abbr));
  h = fnv(h, g.away.rec, strlen(g.away.rec));
  h = fnv(h, g.home.rec, strlen(g.home.rec));
  h = fnv(h, g.net, strlen(g.net));
  return h;
}

uint32_t playTileSig(int team, const Game& g, bool known) {
  uint32_t v = logoVersion();
  return gameHash(fnv(2166136261u, &team, sizeof(team)), g, known) ^ (v * 2654435761u);
}
uint32_t playGameSig(const Game& g, bool known) {
  uint32_t v = logoVersion();
  return gameHash(2166136261u, g, known) ^ (v * 2654435761u);
}

// -------------------------------------------------------------- the words
static const char* ordinal(int n) {
  static const char* const O[] = {"", "1st", "2nd", "3rd"};
  return n >= 1 && n <= 3 ? O[n] : "OT";
}

// the period as ESPN counts it, in this sport's words
static String periodWord(const Game& g) {
  char b[8];
  switch (g.league) {
    case L_NFL: case L_CFB: case L_NBA:
      if (g.period <= 4) { snprintf(b, sizeof(b), "Q%d", g.period); return b; }
      if (g.period == 5) return "OT";
      snprintf(b, sizeof(b), "%dOT", g.period - 4);
      return b;
    case L_NHL:
      return ordinal(g.period);
    default:
      return g.detail;
  }
}

static bool special(const Game& g) {   // ESPN's own words say more than a period and a clock
  return g.league == L_MLB || strstr(g.detail, "End") || strstr(g.detail, "Half") || strstr(g.detail, "Delay") ||
         strstr(g.detail, "Postponed") || strstr(g.detail, "Suspend") || strstr(g.detail, "Cancel") ||
         strstr(g.detail, "Interm");
}

String playStatus(const Game& g) {
  if (special(g)) return g.detail;
  return periodWord(g) + " " + g.clock;
}

static const char* const DAYS[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
static const char* const MONTHS[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

static String timeOfDay(const struct tm& lt) {
  int h = lt.tm_hour % 12;
  char b[16];
  snprintf(b, sizeof(b), "%d:%02d %s", h ? h : 12, lt.tm_min, lt.tm_hour < 12 ? "AM" : "PM");
  return b;
}

static String startDay(const Game& g) {      // "Wed"
  struct tm lt;
  time_t t = g.start;
  localtime_r(&t, &lt);
  return DAYS[lt.tm_wday % 7];
}
static String startTime(const Game& g) {     // "7:08 PM"
  struct tm lt;
  time_t t = g.start;
  localtime_r(&t, &lt);
  return timeOfDay(lt);
}

static String startDate(const Game& g) {    // "Wed, Oct 7"
  struct tm lt;
  time_t t = g.start;
  localtime_r(&t, &lt);
  char b[20];
  snprintf(b, sizeof(b), "%s, %s %d", DAYS[lt.tm_wday % 7], MONTHS[lt.tm_mon % 12], lt.tm_mday);
  return b;
}

static int dayNumber(time_t t) {             // whole local days
  struct tm lt;
  localtime_r(&t, &lt);
  return lt.tm_year * 366 + lt.tm_yday;
}

// ------------------------------------------------------------- the pieces
static int pill(int x, int y, const String& s, uint16_t bg, uint16_t fg) {   // returns its width
  useFont(F_B12);
  int w = lcd.textWidth(s.c_str()) + 16;
  uiTile(x, y, x + w, y + 20, bg, bg, 10, 1);
  text(F_B12, s, x + w / 2, y + 10, fg, bg, middle_center);
  return w;
}

static void letters(const char* abbr, int cx, int cy, FontId f, uint16_t bg) {
  text(f, abbr, cx, cy, C_WHITE, bg, middle_center);
}

static bool mineWon(const Game& g) {
  return g.state == GS_POST && g.mine().score > g.them().score;
}

// ------------------------------------------------------------------- tile
void playTile(int x0, int y0, int x1, int y1, int team, const Game& g, bool known) {
  int cx = (x0 + x1) / 2;
  bool live = known && g.state == GS_LIVE;
  uiTile(x0, y0, x1, y1, C_TILE, live ? C_RED : C_EDGE, 14, live ? 2 : 1);
  // the logo (ESPN's) when it's here, the letters until then
  bool haveLogo = false;
  if (known && g.state != GS_NONE) haveLogo = logoDraw(g.mine(), cx, y0 + 46, 76, C_TILE);
  if (!haveLogo) letters(TEAMS[team].abbr, cx, y0 + 46, F_B36, C_TILE);
  if (live) pill(x0 + 8, y0 + 8, "LIVE", C_RED, C_WHITE);

  String big, small;
  uint16_t smallCol = C_GREY;
  FontId bigFont = F_B18;
  if (!known) {
    big = "Loading";
    bigFont = F_B16;
    smallCol = C_DIM;
  } else if (g.state == GS_NONE) {
    big = "No game";
    small = "coming up";
    bigFont = F_B16;
  } else if (g.state == GS_LIVE) {
    big = String((int)g.mine().score) + " - " + String((int)g.them().score);
    bigFont = F_B24;
    small = playStatus(g) + "  vs " + g.them().abbr;
  } else if (g.state == GS_POST) {
    big = String("Final ") + String((int)g.mine().score) + "-" + String((int)g.them().score);
    bool won = mineWon(g), lost = g.mine().score < g.them().score;
    small = String(won ? "Win" : lost ? "Loss" : "Tie") + " vs " + g.them().abbr;
    smallCol = won ? C_GREEN : C_GREY;
  } else {
    big = startDay(g) + " " + startTime(g);
    small = String(g.mineHome ? "vs " : "at ") + g.them().name;
  }
  text(bigFont, big, cx, y0 + 104, C_WHITE, C_TILE, middle_center);
  text(F_S13, small, cx, y0 + 125, smallCol, C_TILE, middle_center);
}

// -------------------------------------------------------------- game screen
static const int HB_Y = 274;

void playAutoTag(int secs) {
  lcd.fillRect(250, 278, 226, 36, C_BG);
  if (secs < 0) return;
  String s = String("AUTO  next game in ") + String(secs) + "s";
  useFont(F_B12);
  int w = lcd.textWidth(s.c_str()) + 20;
  uiTile(472 - w, 284, 472, 306, C_AUTO_BG, C_AUTO_BG, 11, 1);
  text(F_B12, s, 472 - w / 2, 295, C_AUTO_INK, C_AUTO_BG, middle_center);
}

void playGame(int team, const Game& g, bool known, int autoSecs) {
  lcd.fillScreen(C_BG);
  uiHomeButton();
  playAutoTag(autoSecs);
  if (!known || g.state == GS_NONE) {
    letters(TEAMS[team].abbr, 240, 100, F_B36, C_BG);
    text(F_B24, TEAMS[team].name, 240, 150, C_WHITE, C_BG, middle_center);
    text(F_M15, known ? "No game coming up" : "Getting the score...", 240, 190, C_GREY, C_BG, middle_center);
    return;
  }
  time_t now = mnTime();
  // the top bar: what kind of game, which league, who's showing it
  uint16_t blue = rgb(30, 80, 170);
  int pw;
  if (g.state == GS_LIVE) pw = pill(10, 8, "LIVE", C_RED, C_WHITE);
  else if (g.state == GS_POST) pw = pill(10, 8, "FINAL", rgb(60, 66, 82), C_WHITE);
  else {
    int dd = dayNumber(g.start) - dayNumber(now);
    pw = pill(10, 8, dd <= 0 ? "TODAY" : dd == 1 ? "TOMORROW" : startDay(g), blue, C_WHITE);
  }
  text(F_S13, LEAGUE_NAMES[g.league], 10 + pw + 10, 18, C_GREY, C_BG, middle_left);
  if (g.net[0]) text(F_S13, g.net, 470, 18, C_GREY, C_BG, middle_right);

  // the two teams
  const TeamSide& a = g.away;
  const TeamSide& h = g.home;
  if (!logoDraw(a, 70, 98, 112, C_BG)) letters(a.abbr, 70, 98, F_B36, C_BG);
  if (!logoDraw(h, 410, 98, 112, C_BG)) letters(h.abbr, 410, 98, F_B36, C_BG);
  String an = String(a.name), hn = String(h.name);
  if (a.rec[0]) an += String("  ") + a.rec;
  if (h.rec[0]) hn += String("  ") + h.rec;
  text(F_S13, an, 70, 166, C_GREY, C_BG, middle_center);
  text(F_S13, hn, 410, 166, C_GREY, C_BG, middle_center);

  if (g.state == GS_PRE) {
    text(F_S13, startDate(g), 240, 70, C_GREY, C_BG, middle_center);
    text(F_B36, startTime(g), 240, 102, C_WHITE, C_BG, middle_center);
    return;
  }
  // scores: big, or smaller when they run to three digits
  bool wide = a.score >= 100 || h.score >= 100;
  FontId sf = wide ? F_B36 : F_B62;
  bool fin = g.state == GS_POST;
  uint16_t ca = fin && a.score < h.score ? C_DIM : C_WHITE;
  uint16_t ch = fin && h.score < a.score ? C_DIM : C_WHITE;
  text(sf, String(a.score), 168, 96, ca, C_BG, middle_center);
  text(sf, String(h.score), 312, 96, ch, C_BG, middle_center);
  if (fin) {
    text(F_B18, "FINAL", 240, 96, C_WHITE, C_BG, middle_center);
    if (mineWon(g)) text(F_B12, "WIN", g.mineHome ? 312 : 168, 134, C_GREEN, C_BG, middle_center);
  } else if (special(g)) {
    text(F_B16, g.detail, 240, 96, C_WHITE, C_BG, middle_center);
  } else {
    text(F_S13, periodWord(g), 240, 76, C_GREY, C_BG, middle_center);
    text(F_B24, g.clock, 240, 100, C_WHITE, C_BG, middle_center);
  }
}
