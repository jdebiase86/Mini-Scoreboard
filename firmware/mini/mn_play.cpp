#include "mn_play.h"
#include "mn_ui.h"
#include "mn_logo.h"
#include "mn_lcd.h"
#include "mn_settings.h"
#include "mn_detail.h"

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
  uint32_t h = gameHash(2166136261u, g, known) ^ (v * 2654435761u);
  h = fnv(h, &g.fb.yardLine, sizeof(g.fb.yardLine));
  h = fnv(h, &g.fb.distance, sizeof(g.fb.distance));
  h = fnv(h, &g.fb.down, 1);
  h = fnv(h, &g.fb.possession, 1);
  h = fnv(h, &g.fb.redzone, 1);
  h = fnv(h, &g.fb.toAway, 1);
  h = fnv(h, &g.fb.toHome, 1);
  h = fnv(h, &g.fb.winHome, 1);
  h = fnv(h, &g.fb.driveStart, sizeof(g.fb.driveStart));
  h = fnv(h, g.fb.playId, strlen(g.fb.playId));
  h = fnv(h, &g.det.sig, sizeof(g.det.sig));
  return h;
}

// what needs the whole screen redrawn: the game itself, its teams, its kind
uint32_t playGameShape(const Game& g, bool known) {
  uint32_t h = fnv(2166136261u, &known, 1);
  if (!known) return h;
  h = fnv(h, &g.state, 1);
  h = fnv(h, g.away.abbr, strlen(g.away.abbr));
  h = fnv(h, g.home.abbr, strlen(g.home.abbr));
  h = fnv(h, g.away.rec, strlen(g.away.rec));
  h = fnv(h, g.home.rec, strlen(g.home.rec));
  h = fnv(h, g.net, strlen(g.net));
  h = fnv(h, &g.fb.redzone, 1);   // the RED ZONE tag is up in the top bar
  h = fnv(h, &g.det.has, 1);      // the bottom area fills in when the extras arrive
  uint32_t v = logoVersion();
  return fnv(h, &v, sizeof(v));
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
  if (live && g.fb.has && g.fb.redzone) {   // in the red zone: a red strip with the down
    uiTile(x0 + 10, y0 + 116, x1 - 10, y0 + 133, C_RED, C_RED, 8, 1);
    text(F_B12, String("RED ZONE  ") + g.fb.dd, cx, y0 + 125, C_WHITE, C_RED, middle_center);
  } else {
    text(F_S13, small, cx, y0 + 125, smallCol, C_TILE, middle_center);
  }
}

// -------------------------------------------------------------- game screen
static const int HB_Y = 274;
static char shownPlay[20] = "";
static uint32_t cardUntil = 0;
static bool cardOn = false;
static int lastAutoSecs = -1;

static bool autoTagOn = false;
static char autoLabel[12] = "";
void playAutoLabel(const char* label) { strncpy(autoLabel, label ? label : "", sizeof(autoLabel) - 1); autoLabel[sizeof(autoLabel) - 1] = 0; }

void playAutoTag(int secs) {
  lastAutoSecs = secs;
  autoTagOn = secs >= 0;
  if (cardOn) return;               // the last-play card has that corner for a few seconds
  lcd.fillRect(250, 278, 226, 36, C_BG);
  if (secs < 0) return;
  String s = autoLabel[0] ? String("TICKER ") + autoLabel + "  next in " + String(secs) + "s" : String("AUTO  next game in ") + String(secs) + "s";
  useFont(F_B12);
  int w = lcd.textWidth(s.c_str()) + 20;
  uiTile(472 - w, 284, 472, 306, C_AUTO_BG, C_AUTO_BG, 11, 1);
  text(F_B12, s, 472 - w / 2, 295, C_AUTO_INK, C_AUTO_BG, middle_center);
}

static uint16_t col565(uint32_t c, uint16_t dflt) { return c ? rgb((c >> 16) & 255, (c >> 8) & 255, c & 255) : dflt; }

static bool alike(uint32_t a, uint32_t b) {
  int dr = (int)((a >> 16) & 255) - (int)((b >> 16) & 255), dg = (int)((a >> 8) & 255) - (int)((b >> 8) & 255),
      db = (int)(a & 255) - (int)(b & 255);
  return abs(dr) + abs(dg) + abs(db) < 120;
}

// the two teams' colours: the away team goes light grey when they look alike
static void teamColours(const Game& g, uint16_t& away, uint16_t& home) {
  home = col565(g.home.color, rgb(11, 34, 101));
  away = col565(g.away.color, rgb(0, 76, 84));
  if (g.away.color && g.home.color && alike(g.away.color, g.home.color)) away = rgb(190, 196, 208);
}

void playTeamColours(const Game& g, uint16_t& away, uint16_t& home) { teamColours(g, away, home); }

static void football(int cx, int cy, int w, int h) {
  lcd.fillEllipse(cx, cy, w / 2, h / 2, rgb(140, 80, 40));
  lcd.drawFastHLine(cx - w / 5, cy, w * 2 / 5, C_WHITE);
  for (int i = -2; i <= 2; i++) lcd.drawFastVLine(cx + i * w / 11, cy - 2, 5, C_WHITE);
}

// the field: away end zone left, home right; yard 0 (the home goal line) is on the right
static const int FX0 = 12, FX1 = 468, FY0 = 184, FY1 = 222, EZ = 30;
static const int GX0 = FX0 + EZ, GX1 = FX1 - EZ;
static int fieldX(int yard) { return GX1 - yard * (GX1 - GX0) / 100; }

static void drawField(const Game& g) {
  const FbSit& f = g.fb;
  uint16_t ca, ch;
  teamColours(g, ca, ch);
  uint16_t grass = rgb(38, 120, 52), line = rgb(90, 170, 100);
  lcd.fillRect(GX0, FY0, GX1 - GX0, FY1 - FY0, grass);
  // the 20 yards in front of the goal being attacked: a tint, bright in the red zone
  if (f.possession) {
    uint16_t tint = f.redzone ? rgb(200, 40, 40) : rgb(130, 60, 50);
    int a = f.possession == 2 ? fieldX(100) : fieldX(20);     // home attacks left, away right
    int b = f.possession == 2 ? fieldX(80) : fieldX(0);
    lcd.fillRect(a, FY0, b - a, FY1 - FY0, tint);
  }
  // the drive so far, a lighter band from where it began to the ball
  if (f.yardLine >= 0 && f.yardLine <= 100 && f.driveStart >= 0 && f.driveStart <= 100) {
    int a = fieldX(f.driveStart), b = fieldX(f.yardLine);
    if (a > b) { int t = a; a = b; b = t; }
    if (b - a > 1) lcd.fillRect(a, FY0 + 4, b - a, FY1 - FY0 - 8, rgb(92, 160, 104));
  }
  for (int y = 10; y < 100; y += 10) lcd.drawFastVLine(fieldX(y), FY0, FY1 - FY0, y == 50 ? rgb(160, 210, 165) : line);
  // end zones in team colours
  lcd.fillRoundRect(FX0, FY0, EZ + 6, FY1 - FY0, 6, ca);
  lcd.fillRect(GX0 - 6, FY0, 6, FY1 - FY0, ca);
  lcd.fillRoundRect(GX1 - 6, FY0, EZ + 6, FY1 - FY0, 6, ch);
  lcd.fillRect(GX1, FY0, 6, FY1 - FY0, ch);
  char ab[6];
  snprintf(ab, sizeof(ab), "%.4s", g.away.abbr);
  text(F_B12, ab, (FX0 + GX0) / 2, (FY0 + FY1) / 2, C_WHITE, ca, middle_center);
  snprintf(ab, sizeof(ab), "%.4s", g.home.abbr);
  text(F_B12, ab, (GX1 + FX1) / 2, (FY0 + FY1) / 2, C_WHITE, ch, middle_center);
  if (f.redzone) lcd.drawRoundRect(FX0 - 3, FY0 - 3, FX1 - FX0 + 6, FY1 - FY0 + 6, 8, C_RED);
  // the line to gain (when we know who has the ball), then the ball. During a
  // timeout ESPN leaves out the down and the possession: the ball still shows.
  if (f.yardLine >= 0 && f.yardLine <= 100) {
    int fd = f.possession == 2 ? f.yardLine + f.distance : f.yardLine - f.distance;
    if (f.possession && f.distance > 0 && fd > 0 && fd < 100 && !f.redzone) lcd.drawFastVLine(fieldX(fd), FY0, FY1 - FY0, C_YELLOW);
    int bx = fieldX(f.yardLine);
    lcd.drawFastVLine(bx, FY0, FY1 - FY0, rgb(80, 150, 255));
    football(bx, (FY0 + FY1) / 2, 16, 10);
  }
}

static void drawWinBar(const Game& g) {
  if (g.fb.winHome < 0) return;
  uint16_t ca, ch;
  teamColours(g, ca, ch);
  int away = 100 - g.fb.winHome;
  const int x0 = 12, x1 = 468, y = 227, h = 14;
  int xm = x0 + (x1 - x0) * away / 100;
  lcd.fillRoundRect(x0, y, x1 - x0, h, 7, ch);
  lcd.fillRoundRect(x0, y, (xm - x0 + 7 > 14 ? xm - x0 + 7 : 14), h, 7, ca);
  lcd.fillRect(xm - 1, y, 3, h, C_WHITE);
  text(F_B12, String(away) + "%", x0 + 8, y + h / 2, C_WHITE, ca, middle_left);
  text(F_B12, String(g.fb.winHome) + "%", x1 - 8, y + h / 2, C_WHITE, ch, middle_right);
  text(F_B12, "WIN CHANCE", 240, y + h / 2, rgb(210, 215, 228), (xm < 240 ? ch : ca), middle_center);
}

static void drawTimeouts(int cx, int left) {
  for (int k = 0; k < 3; k++) uiTile(cx - 22 + k * 16, 130, cx - 10 + k * 16, 134, k < left ? C_YELLOW : C_DIM, k < left ? C_YELLOW : C_DIM, 2, 1);
}

// ------------------------------------------------- the area under the scores
// Where a live football game has its field: for everything else the score by
// period (live and final) or the starters and leaders (before the game).
static void leaderRow(const Leader& l, int x0, int y, int w) {
  text(F_B12, l.cat, x0, y, C_YELLOW, C_BG, middle_left);
  String nm = l.name;
  useFont(F_S13);
  while (nm.length() > 4 && lcd.textWidth(nm.c_str()) > 92) nm = nm.substring(0, nm.length() - 1);
  text(F_S13, nm, x0 + 40, y, C_WHITE, C_BG, middle_left);
  String v = l.val;
  while (v.length() > 4 && lcd.textWidth(v.c_str()) > w - 138) v = v.substring(0, v.length() - 1);
  text(F_S13, v, x0 + 136, y, C_GREY, C_BG, middle_left);
}

static void drawBottom(const Game& g) {
  if (!g.det.has) {
    text(F_S13, "Getting the details...", 240, 212, C_DIM, C_BG, middle_center);
    return;
  }
  if (g.state != GS_PRE && (g.det.away.nLines || g.det.home.nLines)) {
    detailLineScore(g, 184);
    if (g.det.venue[0]) text(F_S13, g.det.venue, 240, 250, C_DIM, C_BG, middle_center);
    return;
  }
  // before the game: the starters, then the leaders
  int y = 190;
  bool starters = g.det.away.starter[0] || g.det.home.starter[0];
  if (starters) {
    text(F_B12, "STARTER", 14, y, C_GREY, C_BG, middle_left);
    text(F_B16, g.det.away.starter, 14, y + 18, C_WHITE, C_BG, middle_left);
    text(F_B16, g.det.home.starter, 250, y + 18, C_WHITE, C_BG, middle_left);
    y += 42;
  }
  int rows = starters ? 1 : 3;
  for (int i = 0; i < rows; i++) {
    if (g.det.away.lead[i].cat[0]) leaderRow(g.det.away.lead[i], 14, y + i * 22, 226);
    if (g.det.home.lead[i].cat[0]) leaderRow(g.det.home.lead[i], 250, y + i * 22, 226);
  }
  if (!starters && !g.det.away.lead[0].cat[0] && !g.det.home.lead[0].cat[0] && g.det.venue[0])
    text(F_S13, g.det.venue, 240, 212, C_GREY, C_BG, middle_center);
}

// the LAST PLAY button, bottom middle (the card takes that corner for a few seconds after a play)
static bool lastBtnOn = false;
static void drawLastBtn() {
  uiTile(8, 274, 122, 314, C_TILE, C_EDGE, 12, 1);
  text(F_B12, "LAST PLAY", 65, 294, C_WHITE, C_TILE, middle_center);
}

// the parts of a game screen that move
static void drawDynamic(const Game& g, bool partial) {
  const TeamSide& a = g.away;
  const TeamSide& h = g.home;
  bool fb = g.state == GS_LIVE && g.fb.has;
  bool rz = fb && g.fb.redzone;
  if (partial) {
    lcd.fillRect(128, 50, 224, 126, C_BG);                      // the scores and the middle
    lcd.fillRect(0, 176, 480, 70, C_BG);   // the field and win bar, or the area that stands in for them
  }
  if (g.state == GS_PRE) {
    text(F_S13, startDate(g), 240, 70, C_GREY, C_BG, middle_center);
    text(F_B36, startTime(g), 240, 102, C_WHITE, C_BG, middle_center);
    drawBottom(g);
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
    text(F_S13, periodWord(g), 240, 76, rz ? C_RED : C_GREY, C_BG, middle_center);
    text(F_B24, g.clock, 240, 100, rz ? C_RED : C_WHITE, C_BG, middle_center);
  }
  if (!fb) { drawBottom(g); return; }
  // football: down and distance, where the ball is, timeouts, who has it
  if (g.fb.dd[0]) text(F_B16, g.fb.dd, 240, 122, rz ? C_RED : C_YELLOW, C_BG, middle_center);
  if (g.fb.at[0]) text(F_S13, String("at ") + g.fb.at, 240, 142, C_GREY, C_BG, middle_center);
  if (g.fb.toAway >= 0) drawTimeouts(168, g.fb.toAway);
  if (g.fb.toHome >= 0) drawTimeouts(312, g.fb.toHome);
  if (g.fb.possession) football(g.fb.possession == 2 ? 312 : 168, 150, 22, 13);
  drawField(g);
  drawWinBar(g);
}

// ---------------------------------------------------------- last play card
static const int CX0 = 8, CY0 = 248, CX1 = 472, CY1 = 316;

bool playCardVisible() { return cardOn; }

void playCardHide() {
  if (!cardOn) return;
  cardOn = false;
  lcd.fillRect(CX0 - 4, CY0 - 2, CX1 - CX0 + 8, 320 - CY0 + 2, C_BG);
  if (lastBtnOn) drawLastBtn();
  playAutoTag(lastAutoSecs);
}

void playCardTick() {
  if (cardOn && (int32_t)(millis() - cardUntil) >= 0) playCardHide();
}

static void drawCard(const Game& g) {
  cardOn = true;
  cardUntil = millis() + 7000;
  uiTile(CX0, CY0, CX1, CY1, rgb(34, 40, 54), rgb(90, 100, 125), 14, 1);
  text(F_B12, "LAST PLAY", CX0 + 14, CY0 + 15, C_GREY, rgb(34, 40, 54), middle_left);
  String hdr = String(g.fb.dd) + " " + (g.fb.at[0] ? String("AT ") + g.fb.at : String(""));
  hdr.toUpperCase();
  text(F_B12, hdr, CX1 - 14, CY0 + 15, C_GREY, rgb(34, 40, 54), middle_right);
  // the play, on up to two lines
  String rest = g.fb.play;
  String line[2];
  useFont(F_M15);
  for (int i = 0; i < 2 && rest.length(); i++) {
    int cut = rest.length();
    while (cut > 0 && lcd.textWidth(rest.substring(0, cut).c_str()) > CX1 - CX0 - 28) {
      int sp = rest.lastIndexOf(' ', cut - 1);
      cut = sp > 0 ? sp : cut - 1;
    }
    if (i == 1 && cut < (int)rest.length()) {   // doesn't fit even so: end it with dots
      String t = rest.substring(0, cut);
      while (t.length() > 3 && lcd.textWidth((t + "...").c_str()) > CX1 - CX0 - 28) t = t.substring(0, t.length() - 1);
      line[i] = t + "...";
      rest = "";
      break;
    }
    line[i] = rest.substring(0, cut);
    rest = rest.substring(cut);
    rest.trim();
  }
  int y1 = line[1].length() ? CY0 + 38 : CY0 + 46;
  text(F_M15, line[0], CX0 + 14, y1, C_WHITE, rgb(34, 40, 54), middle_left);
  if (line[1].length()) text(F_M15, line[1], CX0 + 14, CY0 + 56, C_WHITE, rgb(34, 40, 54), middle_left);
}

void playCardHold(const Game& g, uint32_t ms) {
  if (!g.fb.has || !g.fb.play[0]) return;
  drawCard(g);
  cardUntil = millis() + ms;
}

PlayHit playGameHit(int x, int y, const Game& g, bool known) {
  if (!known || g.state == GS_NONE) return PH_NONE;
  if (cardOn && x >= CX0 - 6 && x < CX1 + 6 && y >= CY0 - 6) return PH_CARD;
  if (!cardOn && g.state == GS_LIVE && x < 134 && y >= 266) return PH_LASTPLAY;
  if (!cardOn && autoTagOn && x >= 240 && y >= 270) return PH_AUTOTAG;
  if (y < 40 || y >= 246) return PH_NONE;
  if (g.state == GS_LIVE && g.fb.has) {
    if (y >= 176) return PH_SIT;                       // the field and the win bar
    if (y >= 110 && x >= 128 && x < 352) return PH_SIT;   // down and distance, timeouts
  }
  if (y >= 176 && g.state != GS_PRE && detailHasStats(g)) return PH_STATS;   // the score by period: tap for the team stats
  return PH_TEAMS;
}

void playGame(int team, const Game& g, bool known, int autoSecs, int mode) {
  uint32_t keepUntil = cardOn ? cardUntil : 0;   // a card that's showing keeps its time through a redraw
  if (mode != PG_DYN) {
    lcd.fillScreen(C_BG);
    cardOn = false;
    uiHomeButton();
    lastAutoSecs = autoSecs;
    lastBtnOn = known && g.state == GS_LIVE;
    if (lastBtnOn) drawLastBtn();
    playAutoTag(autoSecs);
  }
  if (!known || g.state == GS_NONE) {
    if (mode != PG_DYN) uiBattery(true);
    letters(TEAMS[team].abbr, 240, 100, F_B36, C_BG);
    text(F_B24, TEAMS[team].name, 240, 150, C_WHITE, C_BG, middle_center);
    text(F_M15, known ? "No game coming up" : "Getting the score...", 240, 190, C_GREY, C_BG, middle_center);
    return;
  }
  bool fb = g.state == GS_LIVE && g.fb.has;
  if (mode != PG_DYN) {
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
    useFont(F_S13);
    int leagueEnd = 10 + pw + 10 + lcd.textWidth(LEAGUE_NAMES[g.league]);
    if (fb && g.fb.redzone) pill(leagueEnd + 10, 8, "RED ZONE", C_RED, C_WHITE);
    else if (g.net[0]) {
      String nt = g.net;
      while (nt.length() > 3 && lcd.textWidth(nt.c_str()) > 300 - leagueEnd - 12) nt = nt.substring(0, nt.length() - 1);
      text(F_S13, nt, 306, 18, C_GREY, C_BG, middle_right);
    }
    uiBattery(true);
    // the two teams
    if (!logoDraw(g.away, 70, 98, 112, C_BG)) letters(g.away.abbr, 70, 98, F_B36, C_BG);
    if (!logoDraw(g.home, 410, 98, 112, C_BG)) letters(g.home.abbr, 410, 98, F_B36, C_BG);
    String an = String(g.away.name), hn = String(g.home.name);
    if (g.away.rec[0]) an += String("  ") + g.away.rec;
    if (g.home.rec[0]) hn += String("  ") + g.home.rec;
    text(F_S13, an, 70, 166, C_GREY, C_BG, middle_center);
    text(F_S13, hn, 410, 166, C_GREY, C_BG, middle_center);
  }
  drawDynamic(g, mode == PG_DYN);
  // a new play: its card, unless the screen was only just opened
  if (fb && g.fb.playId[0]) {
    bool fresh = strcmp(g.fb.playId, shownPlay) != 0;
    strncpy(shownPlay, g.fb.playId, sizeof(shownPlay) - 1);
    if (fresh && mode != PG_OPEN && g.fb.play[0]) { cardOn = false; drawCard(g); }
    else if (keepUntil && mode != PG_OPEN && g.fb.play[0]) { drawCard(g); cardUntil = keepUntil; }   // put it back
  } else {
    shownPlay[0] = 0;
  }
}
