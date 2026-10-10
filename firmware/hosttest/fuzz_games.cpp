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
int main() {
  setenv("MN_LOGOS", "logos", 1);
  mnNow = espnParseTime("2026-10-10T16:00Z");
  srand(5);
  lcd.setColorDepth(16);
  lcd.createSprite(SCREEN_W, SCREEN_H);
  settings.setPicksFromString("NFL:DAL,NFL:JAX,CFB:IOWA,MLB:CLE,NHL:DET,NBA:DAL");
  for (int i = 0; i < settings.npicks; i++) { fakeK[i] = true; lookup(settings.picks[i], fakeG[i]); }
  long taps = 0;
  for (int page = 0; page < 2; page++) {
    for (int slot = 0; slot < 6; slot++) {
      gamesMenu();
      if (page) gamesMenuSwipe(T_SWIPE_LEFT);
      int col = slot % 3, row = slot / 3;
      if (page == 1 && slot > 4) break;
      gamesMenuTap(6 + col * 158 + 70, 40 + row * 140 + 60);
      for (int i = 0; i < 6000; i++) {
        int k = rand() % 10;
        int x = rand() % 480, y = 40 + rand() % 280;   // (not the BACK button)
        if (k < 5) { gamesTap(x, y); taps++; }
        else if (k < 7) { TouchGesture g; g.x0 = rand() % 480; g.y0 = rand() % 320; g.x1 = rand() % 480; g.y1 = rand() % 320; g.ms = 1 + rand() % 400; gamesGesture(g); }
        else if (k < 8) gamesSwipe((TouchEvent)(2 + rand() % 4));
        else frames(1 + rand() % 6, 10 + rand() % 400);
      }
      gamesTap(10, 10);
    }
  }
  printf("fuzz finished: %ld taps, no crash\n", taps);
  return 0;
}
