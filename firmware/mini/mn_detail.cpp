#include "mn_detail.h"
#include "mn_play.h"
#include "mn_logo.h"
#include "mn_ui.h"
#include "mn_lcd.h"

static uint32_t openedAt = 0, touchedAt = 0;

void detailOpen(DetailKind) { openedAt = touchedAt = millis(); }
void detailTouched() { touchedAt = millis(); }
bool detailExpired() { return millis() - touchedAt > 45000; }

uint32_t detailSig(const Game& g, bool known) {
  // anything the cards show: the game screen's own signature plus the extras
  uint32_t h = playGameSig(g, known);
  h = (h ^ g.det.sig) * 16777619u;
  h = (h ^ (g.det.has ? 1u : 0u)) * 16777619u;
  // the "no more details" notice appears 9 seconds in
  h = (h ^ (millis() - openedAt > 9000 ? 1u : 0u)) * 16777619u;
  return h;
}

bool detailTap(int x, int y) { return x < 104 && y < 34; }

// ------------------------------------------------------------------ helpers
static void backButton() {
  uiTile(6, 3, 96, 29, C_TILE, C_EDGE, 13, 1);
  lcd.fillTriangle(18, 16, 26, 9, 26, 23, C_WHITE);
  uiText(F_B12, "BACK", 60, 16, C_WHITE, C_TILE, middle_center);
}

static void fit(String& s, FontId f, int maxW) {
  useFont(f);
  while (s.length() > 4 && lcd.textWidth(s.c_str()) > maxW) s = s.substring(0, s.length() - 1);
}

// cuts text into lines no wider than maxW
static int wrap(const String& text, FontId f, int maxW, String* out, int maxLines) {
  useFont(f);
  String rest = text;
  int n = 0;
  while (rest.length() && n < maxLines) {
    int cut = rest.length();
    while (cut > 1 && lcd.textWidth(rest.substring(0, cut).c_str()) > maxW) {
      int sp = rest.lastIndexOf(' ', cut - 1);
      cut = sp > 0 ? sp : cut - 1;
    }
    out[n++] = rest.substring(0, cut);
    rest = rest.substring(cut);
    rest.trim();
  }
  return n;
}

static const char* const DAYS[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
static const char* const MONTHS[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

static String when(const Game& g) {   // "Sun, Oct 11  1:00 PM"
  struct tm lt;
  time_t t = g.start;
  localtime_r(&t, &lt);
  char b[32];
  int h = lt.tm_hour % 12;
  snprintf(b, sizeof(b), "%s, %s %d  %d:%02d %s", DAYS[lt.tm_wday % 7], MONTHS[lt.tm_mon % 12], lt.tm_mday, h ? h : 12,
           lt.tm_min, lt.tm_hour < 12 ? "AM" : "PM");
  return b;
}

static String stateWord(const Game& g) {
  if (g.state == GS_LIVE) return playStatus(g);
  if (g.state == GS_POST) return "Final";
  return when(g);
}

static void topBar(const Game& g) {
  backButton();
  String t = String(g.away.abbr) + " at " + g.home.abbr;
  uiText(F_B16, t, 240, 16, C_WHITE, C_BG, middle_center);
  String w = stateWord(g);
  fit(w, F_S13, 150);
  uiText(F_S13, w, 474, 16, g.state == GS_LIVE ? C_RED : C_GREY, C_BG, middle_right);
}

static void notice(const char* l1, const char* l2) {
  uiText(F_B18, l1, 240, 150, C_WHITE, C_BG, middle_center);
  uiText(F_M15, l2, 240, 180, C_GREY, C_BG, middle_center);
}

// ----------------------------------------------------------------- teams
static const char* wordFor(const char* cat) {
  static const struct { const char* c; const char* w; } M[] = {{"PTS", "points"}, {"REB", "rebounds"}, {"AST", "assists"},
                                                               {"G", "goals"}, {"HR", "home runs"}, {"RBI", "RBIs"},
                                                               {"AVG", "batting average"}};
  for (auto& m : M) if (!strcmp(cat, m.c)) return m.w;
  return nullptr;
}

static void teamHeader(const TeamSide& t, const SideDetail& d, bool det, int x0, uint16_t col) {
  if (!logoDraw(t, x0 + 38, 68, 76, C_BG)) uiText(F_B24, t.abbr, x0 + 38, 68, C_WHITE, C_BG, middle_center);
  String n = t.name;
  fit(n, F_B16, 150);
  uiText(F_B16, n, x0 + 80, 56, C_WHITE, C_BG, middle_left);
  lcd.fillRect(x0 + 80, 71, 36, 3, col);
  String rec = t.rec;
  uiText(F_S13, rec[0] ? rec : String(""), x0 + 80, 84, C_GREY, C_BG, middle_left);
  if (det && (d.homeRec[0] || d.roadRec[0])) {
    String hr = String("H ") + d.homeRec + "   R " + d.roadRec;
    uiText(F_S13, hr, x0 + 80, 100, C_DIM, C_BG, middle_left);
  }
}

static void lineScore(const Game& g, int y0) {
  int cols = max(g.det.away.nLines, g.det.home.nLines);
  if (!cols) return;
  const int LX = 12, X0 = 66, XT = 462;
  int w = min(34, (XT - 40 - X0) / cols);
  uint16_t ca, ch;
  playTeamColours(g, ca, ch);
  uiTile(8, y0 - 2, 472, y0 + 56, C_TILE, C_EDGE, 8, 1);
  for (int i = 0; i < cols; i++) {
    int regulation = g.league == L_MLB ? 9 : g.league == L_NHL ? 3 : 4;
    String h = i < regulation ? String(i + 1) : String("OT");
    uiText(F_S13, h, X0 + i * w + w / 2, y0 + 8, C_DIM, C_TILE, middle_center);
  }
  uiText(F_S13, g.league == L_MLB ? "R" : "T", XT, y0 + 8, C_DIM, C_TILE, middle_right);
  const SideDetail* sd[2] = {&g.det.away, &g.det.home};
  const TeamSide* ts[2] = {&g.away, &g.home};
  uint16_t cc[2] = {ca, ch};
  for (int r = 0; r < 2; r++) {
    int y = y0 + 24 + r * 18;
    lcd.fillRect(LX, y - 5, 4, 11, cc[r]);
    uiText(F_B12, ts[r]->abbr, LX + 10, y, C_WHITE, C_TILE, middle_left);
    for (int i = 0; i < sd[r]->nLines; i++) uiText(F_S13, String((int)sd[r]->lines[i]), X0 + i * w + w / 2, y, C_WHITE, C_TILE, middle_center);
    uiText(F_B16, String((int)ts[r]->score), XT, y, C_WHITE, C_TILE, middle_right);
  }
}

static void leaders(const SideDetail& d, int x0, int y0) {
  for (int i = 0; i < 3; i++) {
    const Leader& l = d.lead[i];
    if (!l.cat[0]) continue;
    int y = y0 + i * 34;
    uiText(F_B12, l.cat, x0, y + 7, C_YELLOW, C_BG, middle_left);
    String nm = l.name;
    fit(nm, F_B16, 170);
    uiText(F_B16, nm, x0 + 46, y + 7, C_WHITE, C_BG, middle_left);
    String v = l.val;
    const char* word = wordFor(l.cat);
    if (word) v += String(" ") + word;
    fit(v, F_S13, 176);
    uiText(F_S13, v, x0 + 46, y + 25, C_GREY, C_BG, middle_left);
  }
}

static void drawTeams(const Game& g) {
  uint16_t ca, ch;
  playTeamColours(g, ca, ch);
  teamHeader(g.away, g.det.away, g.det.has, 8, ca);
  teamHeader(g.home, g.det.home, g.det.has, 244, ch);
  lcd.drawFastVLine(240, 112, 176, C_EDGE);
  if (!g.det.has) {
    if (millis() - openedAt > 9000) notice("No more details yet", "ESPN hasn't sent any for this game.");
    else notice("Getting the details...", "");
    return;
  }
  int y = 112;
  bool any = false;
  for (int i = 0; i < 3; i++) any = any || g.det.away.lead[i].cat[0] || g.det.home.lead[i].cat[0];
  bool starters = g.det.away.starter[0] || g.det.home.starter[0];
  if (g.state != GS_PRE && (g.det.away.nLines || g.det.home.nLines)) {
    lineScore(g, y + 2);
    y += 64;
  } else if (starters && g.state == GS_PRE) {
    uiText(F_B12, "STARTER", 12, y + 8, C_GREY, C_BG, middle_left);
    uiText(F_B16, g.det.away.starter, 12, y + 28, C_WHITE, C_BG, middle_left);
    uiText(F_B16, g.det.home.starter, 248, y + 28, C_WHITE, C_BG, middle_left);
    y += 44;
  }
  if (any) {
    uiText(F_B12, g.state == GS_PRE ? "SEASON LEADERS" : "LEADERS", 12, y + 8, C_GREY, C_BG, middle_left);
    y += 18;
    leaders(g.det.away, 12, y);
    leaders(g.det.home, 248, y);
  } else if (g.state == GS_LIVE) {
    uiText(F_M15, "Leaders show up once the game is done.", 240, y + 50, C_DIM, C_BG, middle_center);
  }
  // the stadium and TV
  String foot = g.det.venue;
  if (g.net[0]) foot += (foot.length() ? "   -   " : "") + String(g.net);
  if (g.state == GS_PRE) foot += (foot.length() ? "   -   " : "") + when(g);
  fit(foot, F_S13, 456);
  uiText(F_S13, foot, 240, 305, C_GREY, C_BG, middle_center);
}

// ------------------------------------------------------------------ play
static void drawPlay(const Game& g) {
  const FbSit& f = g.fb;
  uiText(F_B12, "LAST PLAY", 14, 50, C_GREY, C_BG, middle_left);
  String lines[6];
  int n = wrap(f.play, F_B18, 452, lines, 6);
  if (!n) { notice("No play yet", "The next one shows up here."); return; }
  for (int i = 0; i < n; i++) uiText(F_B18, lines[i], 14, 82 + i * 28, C_WHITE, C_BG, middle_left);
  // under it: where the game stands
  int y = 262;
  lcd.drawFastHLine(14, y - 14, 452, C_EDGE);
  String s = String(g.away.abbr) + " " + String((int)g.away.score) + "  -  " + String((int)g.home.score) + " " + g.home.abbr;
  uiText(F_B16, s, 14, y + 8, C_WHITE, C_BG, middle_left);
  String d = f.dd[0] ? String(f.dd) : String("");
  if (f.at[0]) d += String(d.length() ? "  at " : "at ") + f.at;
  uiText(F_B16, d, 466, y + 8, f.redzone ? C_RED : C_YELLOW, C_BG, middle_right);
  uiText(F_S13, playStatus(g) + (f.redzone ? "   RED ZONE" : ""), 14, y + 32, f.redzone ? C_RED : C_GREY, C_BG, middle_left);
}

// ------------------------------------------------------------- situation
static String yardText(const Game& g, int y) {   // ESPN's scale -> "BYU 35"
  if (y == 50) return "50";
  return y < 50 ? String(g.home.abbr) + " " + String(y) : String(g.away.abbr) + " " + String(100 - y);
}

static void drawSit(const Game& g) {
  const FbSit& f = g.fb;
  uint16_t ca, ch;
  playTeamColours(g, ca, ch);
  bool rz = f.redzone;
  uiText(F_B36, f.dd[0] ? f.dd : "Timeout", 240, 70, rz ? C_RED : C_YELLOW, C_BG, middle_center);
  if (rz) uiText(F_B12, "RED ZONE", 240, 98, C_RED, C_BG, middle_center);
  // three facts side by side
  struct Fact { const char* label; String value; };
  Fact facts[3];
  facts[0] = {"BALL ON", f.yardLine >= 0 ? yardText(g, f.yardLine) : String("-")};
  int toGoal = -1;
  if (f.possession && f.yardLine >= 0) toGoal = f.possession == 2 ? f.yardLine : 100 - f.yardLine;
  facts[1] = {"TO THE END ZONE", toGoal >= 0 ? String(toGoal) + (toGoal == 1 ? " yard" : " yards") : String("-")};
  facts[2] = {"BALL", f.possession == 2 ? String(g.home.abbr) : f.possession == 1 ? String(g.away.abbr) : String("-")};
  for (int i = 0; i < 3; i++) {
    int x0 = 12 + i * 154;
    uiTile(x0, 112, x0 + 148, 176, C_TILE, C_EDGE, 12, 1);
    uiText(F_B12, facts[i].label, x0 + 74, 126, C_GREY, C_TILE, middle_center);
    String v = facts[i].value;
    fit(v, F_B24, 136);
    uiText(F_B24, v, x0 + 74, 154, C_WHITE, C_TILE, middle_center);
  }
  // timeouts and the drive
  String drive = f.driveStart >= 0 ? String("Drive started at ") + yardText(g, f.driveStart) : String("");
  uiText(F_S13, drive, 14, 192, C_GREY, C_BG, middle_left);
  auto touts = [&](const char* ab, int left, int y, uint16_t col) {
    uiText(F_B12, String(ab) + " timeouts", 14, y, C_GREY, C_BG, middle_left);
    for (int k = 0; k < 3; k++) uiTile(120 + k * 20, y - 4, 134 + k * 20, y + 4, k < left ? C_YELLOW : C_DIM, k < left ? C_YELLOW : C_DIM, 2, 1);
    lcd.fillRect(100, y - 4, 4, 9, col);
  };
  if (f.toAway >= 0) touts(g.away.abbr, f.toAway, 216, ca);
  if (f.toHome >= 0) touts(g.home.abbr, f.toHome, 238, ch);
  // win chance, big
  if (f.winHome >= 0) {
    int away = 100 - f.winHome;
    const int x0 = 14, x1 = 466, y = 262, h = 28;
    int xm = x0 + (x1 - x0) * away / 100;
    lcd.fillRoundRect(x0, y, x1 - x0, h, 9, ch);
    lcd.fillRoundRect(x0, y, max(xm - x0 + 9, 18), h, 9, ca);
    lcd.fillRect(xm - 1, y, 3, h, C_WHITE);
    uiText(F_B16, String(g.away.abbr) + " " + String(away) + "%", x0 + 10, y + h / 2, C_WHITE, ca, middle_left);
    uiText(F_B16, String(f.winHome) + "% " + g.home.abbr, x1 - 10, y + h / 2, C_WHITE, ch, middle_right);
    uiText(F_B12, "WIN CHANCE", 240, y + h + 10, C_GREY, C_BG, middle_center);
  }
}

// ------------------------------------------------------------------ the card
void detailDraw(DetailKind k, int, const Game& g, bool known) {
  lcd.fillScreen(C_BG);
  if (!known || g.state == GS_NONE) {
    backButton();
    notice("No game to show", "");
    return;
  }
  topBar(g);
  if (k == DK_PLAY && g.fb.has) drawPlay(g);
  else if (k == DK_SIT && g.fb.has) drawSit(g);
  else drawTeams(g);
}
