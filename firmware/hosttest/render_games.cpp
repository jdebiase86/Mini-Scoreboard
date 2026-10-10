// Draws the mini's score screens with the real firmware code, from real ESPN
// feeds (feeds/) and logos (logos/) saved by get_feeds.py, into out/g_*.ppm.
// The live games are made by changing a real game's state: ESPN has no game
// on at the moment this was written (it says so under each picture).
//   ./render_games.sh   -> design/mini_stage2.png
#include "../mini/mn_lcd.h"
#include "../mini/mn_ui.h"
#include "../mini/mn_play.h"
#include "../mini/mn_net.h"
#include "../mini/mn_battery.h"
#include "../mini/mn_logo.h"
#include "../mini/mn_espn.h"
#include "../mini/mn_settings.h"
#include <stdio.h>
#include <string>

Settings settings;
time_t mnNow;
uint32_t millis() { return 1000; }
void delay(uint32_t) {}
void mnLog(const char*, ...) {}
static const char* const LK[L_COUNT] = {"NFL", "CFB", "MLB", "NHL", "NBA"};
String teamKey(int i) { return String(LK[TEAMS[i].league]) + ":" + TEAMS[i].abbr; }

// ---- the board's snapshot, filled by hand
static Game fakeG[MAX_PICKS];
static bool fakeK[MAX_PICKS];
bool netGame(int p, Game& out) { if (p < 0 || p >= MAX_PICKS || !fakeK[p]) return false; out = fakeG[p]; return true; }
uint32_t netVersion() { return 1; }
bool mnWifiUp() { return true; }
bool batPresent() { return true; }
int batPercent() { return 78; }
bool batCharging() { return false; }

struct FileSource : ByteSource {
  FILE* f;
  FileSource(const char* path) { f = fopen(path, "rb"); }
  ~FileSource() { if (f) fclose(f); }
  int read() override { return f ? fgetc(f) : -1; }
  size_t readBytes(char* b, size_t n) override { return f ? fread(b, 1, n, f) : 0; }
};

// what the board does for a favourite: today's feed, then the next days
static bool lookup(int team, Game& g) {
  const League lg = TEAMS[team].league;
  char path[80];
  for (int d = 0; d < 8; d++) {
    if (lg == L_NFL) snprintf(path, sizeof(path), "feeds/nfl_now.json");
    else if (lg == L_CFB) snprintf(path, sizeof(path), "feeds/cfb%d_now.json", TEAMS[team].group ? TEAMS[team].group : 8);
    else {
      
      snprintf(path, sizeof(path), "feeds/%s_202610%02d.json", lg == L_MLB ? "mlb" : lg == L_NHL ? "nhl" : "nba", 10 + d);
    }
    FileSource src(path);
    JsonDocument doc;
    if (src.f && espnLoad(src, doc) && espnFind(doc, team, mnNow, g)) return true;
    if (lg == L_NFL || lg == L_CFB) break;
  }
  g = Game();
  return false;
}

static void save(const char* name) {
  char path[256];
  snprintf(path, sizeof(path), "out/%s.ppm", name);
  FILE* f = fopen(path, "wb");
  fprintf(f, "P6\n%d %d\n255\n", SCREEN_W, SCREEN_H);
  static lgfx::rgb888_t line[SCREEN_W];
  for (int y = 0; y < SCREEN_H; y++) {
    lcd.readRect(0, y, SCREEN_W, 1, line);
    for (int x = 0; x < SCREEN_W; x++) { fputc(line[x].r, f); fputc(line[x].g, f); fputc(line[x].b, f); }
  }
  fclose(f);
}

static void makeLive(Game& g, int mine, int theirs, int period, const char* clock, const char* detail) {
  g.state = GS_LIVE;
  TeamSide& m = g.mineHome ? g.home : g.away;
  TeamSide& t = g.mineHome ? g.away : g.home;
  m.score = mine; m.hasScore = true;
  t.score = theirs; t.hasScore = true;
  g.period = period;
  snprintf(g.clock, sizeof(g.clock), "%s", clock);
  snprintf(g.detail, sizeof(g.detail), "%s", detail);
}

static int pickOf(const char* key) { return findTeam(key); }

int main() {
  setenv("TZ", "EST5EDT,M3.2.0,M11.1.0", 1);
  tzset();
  setenv("MN_LOGOS", "logos", 1);
  // Sat Oct 10 2026, 12:00 PM Eastern
  mnNow = espnParseTime("2026-10-10T16:00Z");
  lcd.setColorDepth(16);
  lcd.createSprite(SCREEN_W, SCREEN_H);

  const char* keys[] = {"NFL:NYG", "NHL:NYR", "CFB:LSU", "MLB:NYY", "NFL:DAL", "NBA:NY", "CFB:FLA", "CFB:BYU"};
  String all;
  for (auto k : keys) { if (all.length()) all += ","; all += k; }
  settings.setPicksFromString(all);
  for (int i = 0; i < settings.npicks; i++) {
    fakeK[i] = true;
    lookup(settings.picks[i], fakeG[i]);
  }
  int nyg = -1, nyr = -1, lsu = -1, nyy = -1, dal = -1, ny = -1, fla = -1, byu = -1;
  for (int i = 0; i < settings.npicks; i++) {
    String k = teamKey(settings.picks[i]);
    if (k == "NFL:NYG") nyg = i; if (k == "NHL:NYR") nyr = i; if (k == "CFB:LSU") lsu = i; if (k == "MLB:NYY") nyy = i;
    if (k == "NFL:DAL") dal = i; if (k == "NBA:NY") ny = i; if (k == "CFB:FLA") fla = i; if (k == "CFB:BYU") byu = i;
  }

  // 1. nothing heard yet
  for (int i = 0; i < settings.npicks; i++) fakeK[i] = false;
  uiHome(0); save("g01_loading");
  for (int i = 0; i < settings.npicks; i++) fakeK[i] = true;
  // 2. as ESPN has it right now: finals, upcoming, and a team with no game
  uiHome(0); save("g02_home_real");
  // 3. the Giants' game on: it jumps to the front
  Game real = fakeG[nyg];
  makeLive(fakeG[nyg], 21, 17, 3, "4:12", "");
  uiHome(0); save("g03_home_live");
  uiHome(1); save("g04_home_page2");

  // game screens
  playGame(settings.picks[nyg], fakeG[nyg], true, -1, PG_OPEN); save("g05_football_live");
  fakeG[nyg] = real;
  playGame(settings.picks[nyg], fakeG[nyg], true, -1, PG_OPEN); save("g06_football_upcoming");
  playGame(settings.picks[dal], fakeG[dal], true, -1, PG_OPEN); save("g07_football_final");
  playGame(settings.picks[lsu], fakeG[lsu], true, 14, PG_OPEN); save("g08_college_today_auto");
  Game h = fakeG[nyr];
  makeLive(fakeG[nyr], 2, 1, 2, "8:31", "");
  playGame(settings.picks[nyr], fakeG[nyr], true, -1, PG_OPEN); save("g09_hockey_live");
  fakeG[nyr] = h;
  playGame(settings.picks[nyr], fakeG[nyr], true, -1, PG_OPEN); save("g10_hockey_upcoming");
  Game b = fakeG[ny];
  makeLive(fakeG[ny], 108, 102, 4, "1:05", "");
  playGame(settings.picks[ny], fakeG[ny], true, -1, PG_OPEN); save("g11_basketball_3digits");
  fakeG[ny] = b;
  // baseball: the Yankees have no game this week, so borrow the Mets' look with a made-up one
  playGame(settings.picks[nyy], fakeG[nyy], true, -1, PG_OPEN); save("g12_no_game");
  Game base = fakeG[nyg];
  base.league = L_MLB;
  makeLive(base, 4, 2, 5, "", "Top 5th");
  playGame(settings.picks[nyg], base, true, -1, PG_OPEN); save("g13_baseball_live_made_up");
  // football, from the real game ESPN has on right now (BYU at home against Iowa State)
  Game live = fakeG[byu];
  fprintf(stderr, "LIVE fb has=%d poss=%d yard=%d dist=%d dd=%s at=%s win=%d drive=%d to=%d/%d id=%s/%s\n", live.fb.has, live.fb.possession, live.fb.yardLine, live.fb.distance, live.fb.dd, live.fb.at, live.fb.winHome, live.fb.driveStart, live.fb.toAway, live.fb.toHome, live.away.id, live.home.id);
  playGame(settings.picks[byu], live, true, -1, PG_OPEN); save("g15_football_live_real");
  // the next play: the ball moves and the last-play card pops up (redraw of just the moving parts)
  Game next = live;
  next.fb.yardLine = 14; next.fb.down = 1; next.fb.distance = 10; next.fb.driveStart = 5;
  snprintf(next.fb.dd, sizeof(next.fb.dd), "1st & 10"); snprintf(next.fb.at, sizeof(next.fb.at), "BYU 14");
  snprintf(next.fb.playId, sizeof(next.fb.playId), "401856826111");
  snprintf(next.fb.play, sizeof(next.fb.play), "Shotgun #20 J.Tonga rush middle for 9 yards gain to the BYU14 for a 1ST down (#97 M.Baloun, #4 J.Smith)");
  next.fb.winHome = 86; next.away.score = 0; next.home.score = 7; snprintf(next.clock, sizeof(next.clock), "4:01");
  playGame(settings.picks[byu], next, true, -1, PG_DYN); save("g16_football_next_play_card");
  // the same game drawn from scratch, to check the partial redraw looks identical
  playGame(settings.picks[byu], next, true, -1, PG_FULL); save("g16b_football_full_redraw");
  // red zone: BYU attacking Iowa State's end
  Game rz = next;
  rz.fb.redzone = true; rz.fb.yardLine = 88; rz.fb.down = 1; rz.fb.distance = 10; rz.fb.driveStart = 55;
  snprintf(rz.fb.dd, sizeof(rz.fb.dd), "1st & 10"); snprintf(rz.fb.at, sizeof(rz.fb.at), "ISU 12");
  snprintf(rz.fb.playId, sizeof(rz.fb.playId), "401856826120");
  snprintf(rz.fb.play, sizeof(rz.fb.play), "Pass complete to #8 C.Hall for 22 yards to the ISU12");
  rz.home.score = 7; rz.away.score = 3; rz.period = 2; snprintf(rz.clock, sizeof(rz.clock), "9:44"); rz.fb.winHome = 71;
  playGame(settings.picks[byu], rz, true, -1, PG_FULL); save("g17_football_red_zone");
  fakeG[byu] = rz;
  uiHome(0); save("g18_home_red_zone_tile");
  fakeG[byu] = live;
  // logos not here yet: the letters
  setenv("MN_LOGOS", "nowhere", 1);
  playGame(settings.picks[nyg], real, true, -1, PG_OPEN); save("g14_logos_missing");
  return 0;
}
