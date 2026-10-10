// Draws the games (0.8): the real game code, driven with taps, swipes and flicks.
// (old note: Draws the screens added in 0.5 with the real firmware code: battery icon,
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
#include "../mini/mn_games.h"
#include <WiFi.h>
#include <stdio.h>

Settings settings;
time_t mnNow;
static uint32_t fakeMs = 1000;
uint32_t millis() { return fakeMs; }
static void adv(uint32_t ms) { fakeMs += ms; }
void delay(uint32_t) {}
void mnLog(const char*, ...) {}
FakeWiFi WiFi;
FakeEsp ESP;
static const char* const LK[L_COUNT] = {"NFL", "CFB", "MLB", "NHL", "NBA"};
String teamKey(int i) { return String(LK[TEAMS[i].league]) + ":" + TEAMS[i].abbr; }

static Game fakeG[MAX_PICKS];
static bool fakeK[MAX_PICKS];
bool netTeamSide(int, TeamSide&) { return false; }
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


static void frames(int n, int ms = 25) { for (int i = 0; i < n; i++) { adv(ms); gamesLoop(); } }
static void openGame(int slot) {   // slot on the menu page (0-5)
  int col = slot % 3, row = slot / 3;
  gamesMenuTap(6 + col * 158 + 70, 40 + row * 140 + 60);
}
static void flick(int x0, int y0, int x1, int y1, int ms) { TouchGesture g; g.x0 = x0; g.y0 = y0; g.x1 = x1; g.y1 = y1; g.ms = ms; gamesGesture(g); }

int main() {
  setenv("TZ", "EST5EDT,M3.2.0,M11.1.0", 1);
  tzset();
  setenv("MN_LOGOS", "logos", 1);
  mnNow = espnParseTime("2026-10-10T16:00Z");
  srand(11);
  lcd.setColorDepth(16);
  lcd.createSprite(SCREEN_W, SCREEN_H);
  settings.setPicksFromString("NFL:DAL,NFL:JAX,CFB:IOWA,MLB:CLE,NHL:DET,NBA:DAL");
  for (int i = 0; i < settings.npicks; i++) { fakeK[i] = true; lookup(settings.picks[i], fakeG[i]); }
  bPct = 72; bHas = true;
  uiHome(0); save("h01_home_with_games_button");
  gamesMenu(); save("g01_menu_page1");
  gamesMenuSwipe(T_SWIPE_LEFT); save("g02_menu_page2");
  gamesMenuSwipe(T_SWIPE_RIGHT);
  // penalty kick
  openGame(0); save("g03_kick_aim");
  flick(240, 275, 262, 175, 110); frames(7); save("g04_kick_flight");
  frames(10); save("g05_kick_result");
  // a few more shots, to see a round end
  for (int i = 0; i < 4; i++) { frames(60); flick(240, 275, 240 + (i % 2 ? 80 : -70), 180, 100); frames(40); }
  frames(80); save("g06_kick_round_over");
  gamesTap(10, 10);   // BACK to the menu
  openGame(1); save("g07_free_throw_aim");
  flick(240, 275, 242, 172, 120); frames(8); save("g08_free_throw_flight"); frames(12); save("g09_free_throw_result");
  gamesTap(10, 10);
  openGame(2); save("g10_memory_level1");
  // flip two cards
  gamesTap(80, 140); gamesTap(200, 140); save("g11_memory_two_flipped");
  gamesTap(10, 10);
  openGame(3); save("g12_sudoku_menu");
  gamesTap(240, 110);   // EASY
  gamesTap(30, 80); gamesTap(380, 70); frames(2); gamesTap(100, 130); gamesTap(320, 94); gamesTap(30, 120); gamesTap(440, 94);
  save("g13_sudoku_play");
  gamesTap(10, 10);
  openGame(4); save("g14_snake_start");
  gamesTap(240, 190); for (int i = 0; i < 40; i++) { adv(180); gamesLoop(); if (i == 10) gamesSwipe(T_SWIPE_DOWN); if (i == 18) gamesSwipe(T_SWIPE_LEFT); if (i == 24) gamesSwipe(T_SWIPE_UP); }
  save("g15_snake_playing");
  gamesTap(10, 10);
  openGame(5); for (int i = 0; i < 12; i++) gamesSwipe(i % 4 == 0 ? T_SWIPE_LEFT : i % 4 == 1 ? T_SWIPE_UP : i % 4 == 2 ? T_SWIPE_RIGHT : T_SWIPE_DOWN);
  save("g16_2048");
  gamesTap(10, 10);
  gamesMenuSwipe(T_SWIPE_LEFT);
  openGame(0); gamesTap(240, 190); frames(3, 400); frames(2, 300); save("g17_simon");   // slot 0 on page 2 = Cheer Simon
  gamesTap(10, 10);
  openGame(1);   // Connect Four
  gamesTap(100, 150); frames(12, 50); frames(12, 100); gamesTap(150, 150); frames(12, 50); frames(14, 100); gamesTap(100, 150); frames(12, 50); frames(14, 100);
  save("g18_connect4");
  gamesTap(10, 10);
  openGame(2);   // Trivia
  save("g19_trivia_question");
  gamesTap(110, 180); save("g20_trivia_answered");
  gamesTap(10, 10);
  openGame(3);   // Reaction
  gamesTap(240, 160); frames(1, 5000); save("g21_reaction_go"); adv(268); gamesTap(240, 160); save("g22_reaction_result");
  gamesTap(10, 10);
  openGame(4);   // Coin and dice
  gamesTap(340, 285); frames(20, 80); save("g23_die");
  gamesTap(120, 285); frames(30, 60); save("g24_coin");
  return 0;
}
