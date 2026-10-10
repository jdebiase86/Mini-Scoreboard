// Draws the screens added in 0.5 with the real firmware code: battery icon,
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
  snprintf(path, sizeof(path), "out/n_%s.ppm", name);
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

static void showDetail(DetailKind k, int pick, const char* name, const char* liveLeague = nullptr) {
  Game g = fakeG[pick];
  detailOpen(k);
  static LiveInfo li;
  if (liveLeague) li = liveFor(pick, liveLeague);
  detailDraw(k, settings.picks[pick], g, fakeK[pick], liveLeague ? &li : nullptr);
  save(name);
}

static void makeLive(Game& g, int away, int home, int period, const char* clock, const char* detail = "") {
  g.state = GS_LIVE; g.period = period;
  snprintf(g.clock, sizeof(g.clock), "%s", clock); snprintf(g.detail, sizeof(g.detail), "%s", detail);
  g.away.score = away; g.home.score = home; g.away.hasScore = g.home.hasScore = true;
}

int main() {
  setenv("TZ", "EST5EDT,M3.2.0,M11.1.0", 1);
  tzset();
  setenv("MN_LOGOS", "logos", 1);
  mnNow = espnParseTime("2026-10-10T16:00Z");
  lcd.setColorDepth(16);
  lcd.createSprite(SCREEN_W, SCREEN_H);

  settings.setPicksFromString("NFL:DAL,NFL:JAX,CFB:IOWA,MLB:CLE,NHL:DET,NBA:DAL");
  for (int i = 0; i < settings.npicks; i++) { fakeK[i] = true; lookup(settings.picks[i], fakeG[i]); }
  settings.nets[0] = {"Home Wi-Fi", "x"}; settings.nets[1] = {"Phone hotspot", "x"}; settings.nnets = 2;
  settings.ssid = "Home Wi-Fi";
  int dal = idx("NFL:DAL"), jax = idx("NFL:JAX"), iowa = idx("CFB:IOWA"), cle = idx("MLB:CLE"), det = idx("NHL:DET"), mav = idx("NBA:DAL");

  // 1. home: battery, the Wi-Fi button
  uiHome(0); save("d01_home");
  bPct = 17; online = false; uiHome(0); save("d02_home_offline_low");
  bPct = 64; bChg = true; online = true;
  // a live football game (made up from the real upcoming one)
  Game live = fakeG[jax];
  live.state = GS_LIVE; live.period = 3; snprintf(live.clock, sizeof(live.clock), "4:12");
  live.home.score = 17; live.away.score = 21; live.home.hasScore = live.away.hasScore = true;
  live.fb.has = true; live.fb.down = 2; live.fb.distance = 7; live.fb.yardLine = 31; live.fb.possession = 1;
  live.fb.toAway = 2; live.fb.toHome = 3; live.fb.winHome = 41; live.fb.driveStart = 12;
  snprintf(live.fb.dd, sizeof(live.fb.dd), "2nd & 7"); snprintf(live.fb.at, sizeof(live.fb.at), "JAX 31");
  snprintf(live.fb.playId, sizeof(live.fb.playId), "1");
  snprintf(live.fb.play, sizeof(live.fb.play), "J.Hurts pass short right to D.Wicks for 11 yards to the JAX31, tackled by A.Walker and D.Lloyd. Penalty on JAX, defensive holding, declined.");
  live.det.has = true;
  fakeG[jax] = live;
  playGame(settings.picks[jax], live, true, -1, PG_OPEN); save("d03_game_live_battery");
  bChg = false;
  // 2. details: teams (before the game, with leaders)
  Game pre = fakeG[dal];
  showDetail(DK_TEAMS, jax, "d04_teams_live_nfl");
  fakeG[jax] = pre; fakeG[jax] = Game(); lookup(settings.picks[jax], fakeG[jax]);
  showDetail(DK_TEAMS, jax, "d05_teams_pre_nfl");
  showDetail(DK_TEAMS, dal, "d06_teams_final_nfl");
  showDetail(DK_TEAMS, mav, "d07_teams_final_nba");
  showDetail(DK_TEAMS, cle, "d08_teams_pre_mlb");
  showDetail(DK_TEAMS, det, "d09_teams_final_nhl");
  showDetail(DK_TEAMS, iowa, "d10_teams_cfb");
  // 3. details: play and situation (the live game made up above)
  fakeG[jax] = live;
  showDetail(DK_PLAY, jax, "d11_play");
  showDetail(DK_SIT, jax, "d12_situation");
  Game rz = live; rz.fb.redzone = true; rz.fb.yardLine = 8; rz.fb.possession = 2; rz.fb.distance = 8; rz.fb.down = 1;
  snprintf(rz.fb.dd, sizeof(rz.fb.dd), "1st & Goal"); snprintf(rz.fb.at, sizeof(rz.fb.at), "JAX 8");
  fakeG[jax] = rz; showDetail(DK_SIT, jax, "d13_situation_red_zone");
  // loading the extras
  Game noDet = live; noDet.det = Details();
  fakeG[jax] = noDet; showDetail(DK_TEAMS, jax, "d14_teams_loading");

  // 3b. the new game-screen bottoms, the LAST PLAY button, live stats and leaders (0.6)
  settings.setPicksFromString("NFL:DAL,NFL:JAX,CFB:IOWA,MLB:CLE,NHL:DET,NBA:DAL,NHL:SEA,NBA:HOU");
  for (int i = 0; i < settings.npicks; i++) { fakeK[i] = true; lookup(settings.picks[i], fakeG[i]); }
  int jax2 = idx("NFL:JAX"), dal2 = idx("NFL:DAL"), iowa2 = idx("CFB:IOWA"), cle2 = idx("MLB:CLE"), det2 = idx("NHL:DET"), mav2 = idx("NBA:DAL");
  bPct = 72; bChg = false;
  // (the pictures' made-up live games get made-up scores by period, as ESPN sends them once a game is on)
  auto fakeLines = [](Game& g, std::initializer_list<int> a, std::initializer_list<int> h) {
    g.det.has = true; g.det.away.nLines = g.det.home.nLines = 0;
    for (int v : a) g.det.away.lines[g.det.away.nLines++] = v;
    for (int v : h) g.det.home.lines[g.det.home.nLines++] = v;
    g.det.away.starter[0] = g.det.home.starter[0] = 0;
    for (auto& l : g.det.away.lead) l = Leader(); for (auto& l : g.det.home.lead) l = Leader();
  };
  Game bb = fakeG[mav2]; makeLive(fakeG[mav2], 88, 92, 3, "4:21"); fakeG[mav2].det = bb.det; fakeLines(fakeG[mav2], {27, 38, 23}, {30, 31, 31});
  playGame(settings.picks[mav2], fakeG[mav2], true, -1, PG_OPEN); save("e01_basketball_live");
  Game hk = fakeG[det2]; makeLive(fakeG[det2], 2, 1, 2, "8:31"); fakeG[det2].det = hk.det; fakeLines(fakeG[det2], {1, 1}, {0, 1});
  playGame(settings.picks[det2], fakeG[det2], true, -1, PG_OPEN); save("e02_hockey_live");
  playGame(settings.picks[cle2], fakeG[cle2], true, -1, PG_OPEN); save("e03_baseball_before");
  playGame(settings.picks[jax2], fakeG[jax2], true, -1, PG_OPEN); save("e04_football_before");
  playGame(settings.picks[dal2], fakeG[dal2], true, -1, PG_OPEN); save("e05_football_final");
  Game fb = fakeG[jax2]; fakeG[jax2] = live; fakeG[jax2].det = fb.det;
  playGame(settings.picks[jax2], fakeG[jax2], true, -1, PG_OPEN); save("e06_football_live_button");
  // the saved game pages are HOU at DAL and SEA at DET: draw those two with the same teams
  auto asGame = [&](int pick, const char* away, const char* home) {
    snprintf(fakeG[pick].away.abbr, 8, "%s", away); snprintf(fakeG[pick].home.abbr, 8, "%s", home);
  };
  asGame(mav2, "HOU", "DAL"); asGame(det2, "SEA", "DET");
  showDetail(DK_STATS, mav2, "e07_stats_basketball", "nba");
  showDetail(DK_STATS, det2, "e08_stats_hockey", "nhl");
  Game cf = fakeG[iowa2];
  showDetail(DK_STATS, iowa2, "e09_stats_college", "cfb");
  showDetail(DK_TEAMS, iowa2, "e10_teams_live_leaders", "cfb");
  showDetail(DK_PLAY, mav2, "e11_last_play_basketball", "nba");
  showDetail(DK_PLAY, det2, "e12_last_play_hockey", "nhl");
  showDetail(DK_STATS, cle2, "e13_stats_baseball_none", "mlb");

  // 3c. 0.7: HOME up in the top bar, a wider last-play card, the ticker chooser
  fakeG[jax2] = live; fakeG[jax2].det = fb.det;
  playGame(settings.picks[jax2], fakeG[jax2], true, -1, PG_OPEN); save("f01_home_in_top_bar");
  Game nx = live; snprintf(nx.fb.playId, sizeof(nx.fb.playId), "2");
  snprintf(nx.fb.play, sizeof(nx.fb.play), "J.Hurts pass short right to D.Wicks for 11 yards to the JAX31, tackled by A.Walker and D.Lloyd. Penalty on JAX, defensive holding, declined.");
  playGame(settings.picks[jax2], nx, true, 12, PG_DYN); save("f02_play_card_wider");
  playAutoLabel("NFL");
  playGame(settings.picks[jax2], live, true, 9, PG_OPEN); save("f03_ticker_tag");
  playAutoLabel("");
  playGame(settings.picks[jax2], live, true, -1, PG_OPEN); playStale(135, live); save("f05_old_tag");
  { const char* names[] = {"All my teams", "NFL", "College football", "MLB", "NHL", "NBA"}; int total[] = {8, 2, 1, 1, 2, 2}, lv[] = {1, 1, 0, 0, 1, 0};
    uiTicker(names, total, lv, 6, 1); save("f04_ticker_chooser"); }

  // 4. Wi-Fi list, the action sheet, the keyboard
  WiFi.found = {{"Home Wi-Fi", -48, 3}, {"Phone hotspot", -60, 3}, {"Coffee Shop Guest", -66, 0}, {"Neighbour 2G", -78, 3},
                {"Airport Free WiFi", -82, 0}, {"Office", -85, 3}, {"Printer-Direct", -90, 3}, {"Guest", -91, 3}};
  WiFi.joined = "Home Wi-Fi"; WiFi.st = WL_CONNECTED;
  wifiStart(); wifiLoop(); wifiDraw(); save("w01_wifi_list");
  wifiTap(240, 62 + 38 + 18);   // Phone hotspot: known -> the sheet
  save("w02_wifi_saved_sheet");
  wifiTap(240, 262);            // cancel
  wifiTap(240, 62 + 76 + 18);   // Coffee shop: open -> would join at once; (not drawn)
  wifiStart(); wifiLoop();
  wifiTap(240, 62 + 3 * 38 + 18);   // Neighbour: secured -> keyboard
  save("w03_keyboard_empty");
  for (char c : String("Sunshine")) { int dummy = c; (void)dummy; }
  kbOpen("Password: Neighbour 2G", "My-Hotspot_2026", true, 63); kbDraw(); save("w04_keyboard_typed");
  kbTap(30, 168);   // 123/#? (no: row 3 left) -> shift key
  kbOpen("Password: Neighbour 2G", "Hunter2!", true, 63); kbDraw();
  kbTap(40, 78 + 4 * 48 + 20);  // 123/#
  save("w05_keyboard_symbols");
  uiSetup("Mini-Scoreboard-4F2A", false, ""); save("w06_setup_screen");
  fakeMs = 1000;
  WiFi.st = WL_DISCONNECTED;
  uiJoining("Phone hotspot"); save("w07_joining");
  // paging: down, then up (Joe saw the up arrow not working)
  WiFi.joined = "Home Wi-Fi";
  wifiStart(); wifiLoop(); wifiTap(455, 302); save("w09_page2"); wifiTap(417, 302); save("w10_back_to_page1");
  // joined and saved at once, and the note after forgetting the one you're on
  settings.nets[0] = {"Home Wi-Fi", "x"}; settings.nets[1] = {"Phone hotspot", "x"}; settings.nnets = 2;
  WiFi.joined = "Phone hotspot"; WiFi.st = WL_CONNECTED;
  wifiStart(); wifiLoop(); wifiTap(240, 62 + 38 + 18); wifiTap(240, 187); save("w08_forgot_current");
  return 0;
}
