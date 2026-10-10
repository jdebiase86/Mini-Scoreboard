// Draws the 0.12 animations (cleaner upcoming game, bye week) with the real firmware code. Pictures go to out/p_*.ppm.
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
#include "../mini/mn_version.h"
#include "../mini/mn_fx.h"
#include "../mini/mn_about.h"
#include "../mini/mn_diag.h"
#include <WiFi.h>
#include <stdio.h>
#include <math.h>

Settings settings;
time_t mnNow;
static uint32_t fakeMs = 1000;
static void adv(uint32_t ms) { fakeMs += ms; }
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
  snprintf(path, sizeof(path), "out/x_%s.ppm", name);
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



static void frameTimes(const char* name, FxSpec f, std::initializer_list<int> shots) {
  fxStart(f);
  uint32_t start = fakeMs;
  int idx = 0;
  while (fxActive() && fakeMs - start < fxLength(f.kind) + 200) {
    adv(70);
    fxStep();
    for (int s : shots) {
      if ((int)(fakeMs - start) >= s && (int)(fakeMs - start) < s + 70) {
        char n[64]; snprintf(n, sizeof(n), "%s_%d", name, idx++); save(n);
      }
    }
  }
}

static FxSpec spec(FxKind k, const Game& g, const char* word, const char* sub, bool tape = false) {
  FxSpec f;
  f.kind = k; f.league = g.league; f.mine = g.mine(); f.them = g.them(); f.tape = tape; f.theirs = tape;
  snprintf(f.word, sizeof(f.word), "%s", word); snprintf(f.sub, sizeof(f.sub), "%s", sub);
  return f;
}

int main() {
  setenv("TZ", "EST5EDT,M3.2.0,M11.1.0", 1);
  tzset();
  setenv("MN_LOGOS", "logos", 1);
  mnNow = espnParseTime("2026-10-10T16:00Z");
  srand(5);
  lcd.setColorDepth(16);
  lcd.createSprite(SCREEN_W, SCREEN_H);
  settings.setPicksFromString("NFL:DAL,NFL:JAX,CFB:IOWA,MLB:CLE,NHL:DET,NBA:DAL");
  for (int i = 0; i < settings.npicks; i++) { fakeK[i] = true; lookup(settings.picks[i], fakeG[i]); fakeSide[i] = fakeG[i].mine(); }
  int dal = idx("NFL:DAL"), jax = idx("NFL:JAX"), cle = idx("MLB:CLE"), det = idx("NHL:DET"), mav = idx("NBA:DAL");
  Game fb = fakeG[jax], bb = fakeG[cle], hk = fakeG[det], nba = fakeG[mav];
  fb.mine().score; 
  frameTimes("td", spec(FX_TOUCHDOWN, fb, "TOUCHDOWN", "JAX 28   PHI 17"), {500, 2500});
  frameTimes("fg", spec(FX_FIELDGOAL, fb, "IT'S GOOD!", "JAX 20   PHI 17"), {1200, 2300, 3300, 5000});
  frameTimes("nogood", spec(FX_NOGOOD, fb, "NO GOOD", "JAX KEEP 17", true), {2300, 4300});
  frameTimes("theirs", spec(FX_THEIRSCORE, fb, "TOUCHDOWN", "JAX 21   PHI 24", true), {1000});
  frameTimes("goal", spec(FX_GOAL, hk, "GOAL!", "DET 3   PHI 2"), {1000, 2500});
  frameTimes("hr", spec(FX_HOMERUN, bb, "HOME RUN!", "CLE 4   CHW 2"), {1000, 3000});
  frameTimes("three", spec(FX_THREE, nba, "THREE!", "DAL 88   HOU 85"), {1000});
  frameTimes("win", spec(FX_WIN, fb, "JAGUARS WIN", "FINAL  31 - 24"), {900, 3000});
  frameTimes("kickoff", spec(FX_KICKOFF, fb, "KICKOFF", "PHI  AT  JAX"), {1000});
  frameTimes("quarter", spec(FX_QUARTER, fb, "HALFTIME", "JAX 14   PHI 10"), {1000});
  frameTimes("flag", spec(FX_FLAG, fb, "FLAG", "DEFENSIVE HOLDING, 10 YARDS"), {1000});
  frameTimes("first", spec(FX_FIRSTDOWN, fb, "FIRST DOWN!", "1ST & 10"), {900});
  frameTimes("picked", spec(FX_PICKED, fb, "PICKED OFF!", "INTERCEPTION - JAX BALL"), {1000});
  frameTimes("fumble", spec(FX_FUMBLE, fb, "FUMBLE!", "JAX BALL - RECOVERED"), {1000});
  frameTimes("sack", spec(FX_SACK, fb, "SACKED!", "PHI SACKED"), {1000});
  frameTimes("stopped", spec(FX_STOPPED, fb, "STOPPED!", "PHI FACE 4TH & 3"), {1000});
  frameTimes("stonewall", spec(FX_STONEWALL, fb, "STONEWALLED!", "TURNOVER ON DOWNS - JAX BALL"), {1000});
  frameTimes("punt", spec(FX_PUNT, fb, "PUNT-ASTIC!", "PHI HAVE TO PUNT"), {1000});
  frameTimes("went", spec(FX_WENTFORIT, fb, "WENT FOR IT!", "AND MADE IT - FIRST DOWN"), {1000});
  frameTimes("nopunt", spec(FX_NOPUNT, fb, "NO PUNT INTENDED", "JAX PUNT"), {1000});
  frameTimes("turnover", spec(FX_TURNOVER, fb, "TURNOVER", "PICKED OFF - PHI BALL", true), {1000});
  return 0;
}
