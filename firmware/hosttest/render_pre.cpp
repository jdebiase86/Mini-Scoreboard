// Draws the 0.10 screens (cleaner upcoming game, bye week) with the real firmware code. Pictures go to out/p_*.ppm.
// (was: the screens added in 0.5 with the real firmware code: battery icon,
// Wi-Fi button, the Wi-Fi list and keyboard, and the details cards (from real
// ESPN feeds with the extras). Pictures go to out/n_*.ppm.
//   ./render_new.sh   -> design/mini_stage3.png
#include "../mini/mn_lcd.h"
#include "../mini/mn_ui.h"
#include "../mini/mn_play.h"
#include "../mini/mn_net.h"
#include "../mini/mn_logo.h"
#include "../mini/mn_espn.h"
#include "../mini/mn_settings.h"
#include "../mini/mn_detail.h"
#include "../mini/mn_keyboard.h"
#include "../mini/mn_wifi.h"
#include "../mini/mn_battery.h"
#include "../mini/mn_live.h"
#include <WiFi.h>
#include <stdio.h>

Settings settings;
time_t mnNow;
static uint32_t fakeMs = 1000;
uint32_t millis() { return fakeMs; }
void delay(uint32_t) {}
void mnLog(const char*, ...) {}
FakeWiFi WiFi;
FakeEsp ESP;
static const char* const LK[L_COUNT] = {"NFL", "CFB", "MLB", "NHL", "NBA"};
String teamKey(int i) { return String(LK[TEAMS[i].league]) + ":" + TEAMS[i].abbr; }

static Game fakeG[MAX_PICKS];
static bool fakeK[MAX_PICKS];
static TeamSide fakeSide[MAX_PICKS];
bool netTeamSide(int p, TeamSide& out) { if (p < 0 || p >= MAX_PICKS || !fakeSide[p].logo[0]) return false; out = fakeSide[p]; return true; }
bool netGame(int p, Game& out) { if (p < 0 || p >= MAX_PICKS || !fakeK[p]) return false; out = fakeG[p]; return true; }
uint32_t netVersion() { return 1; }
void netWantDetails(int) {}
static bool online = true;
bool mnWifiUp() { return online; }
// the battery
static int bPct = 78; static bool bChg = false, bHas = true;
bool batPresent() { return bHas; }
int batPercent() { return bPct; }
bool batCharging() { return bChg; }
int batMilliVolts() { return 3900; }

struct FileSource : ByteSource {
  FILE* f;
  FileSource(const char* path) { f = fopen(path, "rb"); }
  ~FileSource() { if (f) fclose(f); }
  int read() override { return f ? fgetc(f) : -1; }
  size_t readBytes(char* b, size_t n) override { return f ? fread(b, 1, n, f) : 0; }
};

static bool lookup(int team, Game& g) {
  const League lg = TEAMS[team].league;
  char path[80];
  for (int d = 0; d < 8; d++) {
    if (lg == L_NFL) snprintf(path, sizeof(path), "feeds/nfl_now.json");
    else if (lg == L_CFB) snprintf(path, sizeof(path), "feeds/cfb%d_now.json", TEAMS[team].group ? TEAMS[team].group : 8);
    else snprintf(path, sizeof(path), "feeds/%s_202610%02d.json", lg == L_MLB ? "mlb" : lg == L_NHL ? "nhl" : "nba", 10 + d);
    FileSource src(path);
    JsonDocument doc;
    if (src.f && espnLoad(src, doc, true) && espnFind(doc, team, mnNow, g)) return true;
    if (lg == L_NFL || lg == L_CFB) break;
  }
  g = Game();
  return false;
}

static void save(const char* name) {
  char path[256];
  snprintf(path, sizeof(path), "out/p_%s.ppm", name);
  FILE* f = fopen(path, "wb");
  fprintf(f, "P6\n%d %d\n255\n", SCREEN_W, SCREEN_H);
  static lgfx::rgb888_t line[SCREEN_W];
  for (int y = 0; y < SCREEN_H; y++) {
    lcd.readRect(0, y, SCREEN_W, 1, line);
    for (int x = 0; x < SCREEN_W; x++) { fputc(line[x].r, f); fputc(line[x].g, f); fputc(line[x].b, f); }
  }
  fclose(f);
}

static int idx(const char* key) {
  for (int i = 0; i < settings.npicks; i++) if (teamKey(settings.picks[i]) == key) return i;
  fprintf(stderr, "no favourite %s\n", key);
  return 0;
}

// what the game's own page told us (real pages, trimmed: feeds/summary_*.json)
static LiveInfo liveFor(int pick, const char* league) {
  static LiveInfo li;
  char path[80];
  snprintf(path, sizeof(path), "feeds/summary_%s.json", league);
  FileSource src(path);
  li = LiveInfo();
  if (src.f) liveParse(src, fakeG[pick], li);
  return li;
}

static void showDetail_unused() {}

int main() {
  setenv("TZ", "EST5EDT,M3.2.0,M11.1.0", 1);
  tzset();
  setenv("MN_LOGOS", "logos", 1);
  mnNow = espnParseTime("2026-10-10T16:00Z");
  lcd.setColorDepth(16);
  lcd.createSprite(SCREEN_W, SCREEN_H);
  settings.setPicksFromString("NFL:DAL,NFL:JAX,CFB:IOWA,MLB:CLE,NHL:DET,NBA:DAL");
  for (int i = 0; i < settings.npicks; i++) { fakeK[i] = true; lookup(settings.picks[i], fakeG[i]); }
  settings.nets[0] = {"Home Wi-Fi", "x"}; settings.nnets = 1; settings.ssid = "Home Wi-Fi";
  int dal = idx("NFL:DAL"), jax = idx("NFL:JAX"), iowa = idx("CFB:IOWA"), cle = idx("MLB:CLE"), det = idx("NHL:DET"), mav = idx("NBA:DAL");
  bPct = 72;
  for (int i = 0; i < settings.npicks; i++) fakeSide[i] = fakeG[i].mine();
  // upcoming games, as they are today
  playGame(settings.picks[jax], fakeG[jax], true, -1, PG_OPEN); save("p01_upcoming_football");
  playGame(settings.picks[cle], fakeG[cle], true, 11, PG_OPEN); save("p02_upcoming_baseball_auto");
  Game soon = fakeG[jax]; soon.start = mnNow + 2 * 3600 + 15 * 60; soon.det.has = true;
  playGame(settings.picks[jax], soon, true, -1, PG_OPEN); save("p03_upcoming_soon");
  Game mid = soon; mnNow += 30 * 60; playGame(settings.picks[jax], mid, true, -1, PG_DYN); save("p04_upcoming_soon_30min_later"); mnNow -= 30 * 60;
  Game cfb = fakeG[iowa]; playGame(settings.picks[iowa], cfb, true, -1, PG_OPEN); save("p05_upcoming_college");
  // bye week: no game found
  Game none = Game(); fakeG[dal] = none; fakeG[mav] = none;
  playGame(settings.picks[dal], none, true, -1, PG_OPEN); save("p06_bye_week_football");
  playGame(settings.picks[mav], none, true, -1, PG_OPEN); save("p07_no_game_basketball");
  uiHome(0); save("p08_home_with_bye_tiles");
  return 0;
}
