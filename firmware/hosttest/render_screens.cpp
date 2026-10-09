// Draws the mini's real screens (firmware/mini/mn_ui.cpp, same code as the
// board) into pictures on a computer, so they can be checked before flashing.
//   ./render_screens.sh   -> design/mini_stage1.png
#include "../mini/mn_lcd.h"
#include "../mini/mn_ui.h"
#include "../mini/mn_settings.h"
#include "../mini/mn_picker.h"
#include <string.h>
#include <chrono>
#include <thread>
#include <stdio.h>

Settings settings;
static auto t0 = std::chrono::steady_clock::now();
uint32_t millis() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count() + 100000;
}
void delay(uint32_t) {}
void Settings::save() {}
static const char* const LK[L_COUNT] = {"NFL", "CFB", "MLB", "NHL", "NBA"};
String teamKey(int i) { return String(LK[TEAMS[i].league]) + ":" + TEAMS[i].abbr; }
void mnLog(const char*, ...) {}


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

// the team picker, driven by taps like a finger would (sheet: design/mini_picker_real.png)
static void pickerShots() {
  settings.setPicksFromString("NFL:NYG,NHL:NYR,CFB:FLA,MLB:NYY,NBA:NY");
  uiHome(0); save("p1_home_edit");
  pickerStart(); save("p2_leagues");
  pickerTap(80, 100); save("p3_nfl_page1");                 // NFL
  pickerSwipe(true); pickerSwipe(true);                      // swipe up twice (same as the down arrow twice)
  pickerTap(100, 285); save("p4_nfl_page3_jets");            // tick the Jets
  pickerTap(40, 16);                                         // back to leagues
  pickerTap(240, 100); save("p5_college");                   // COLLEGE
  pickerTap(80, 100);                                        // SEC
  pickerTap(300, 117); save("p6_sec_lsu");                   // tick LSU
  pickerTap(40, 16); pickerTap(40, 16);                      // back, back
  save("p7_leagues_after");
  pickerTap(400, 250);                                       // DONE
  uiHome(0); save("p8_home_after");
  settings.setPicksFromString("NFL:NYG,NFL:NYJ,NFL:DAL,CFB:FLA,CFB:LSU,MLB:NYY,MLB:NYM,NHL:NYR,NHL:NJ,NBA:NY");
  pickerStart(); pickerTap(80, 100); pickerTap(100, 61); save("p9_full");   // 10 picked, tap Arizona
}

int main() {
  setenv("TZ", "EST5EDT,M3.2.0,M11.1.0", 1);
  tzset();
  lcd.setColorDepth(16);
  lcd.createSprite(SCREEN_W, SCREEN_H);
  settings.ssid = "Home Wi-Fi";
  settings.setPicksFromString("NFL:NYG,NHL:NYR,CFB:FLA,MLB:NYY,NBA:NY");

  uiSplash(); save("01_splash");
  uiSetup("Mini-Scoreboard-3F2A", false, ""); save("02_setup");
  uiJoining("Home Wi-Fi"); save("03_joining");
  uiSetup("Mini-Scoreboard-3F2A", true, "Home Wi-Fi"); save("04_cant_join");
  uiConnected("192.168.1.42"); save("05_connected");
  uiHome(0); save("06_home");
  uiTeam(settings.picks[0]); save("07_team");
  uiTeam(-1); save("08_auto");
  uiUpdating(40, "0.2"); save("09_updating");
  settings.setPicksFromString("NFL:DAL,CFB:LSU");
  uiHome(0); save("10_home_two");
  settings.setPicksFromString("NFL:NYG,NHL:NYR,CFB:FLA,MLB:NYY,NBA:NY,NFL:NYJ,MLB:NYM,NHL:NJ");
  uiHome(0); save("11_home_eight_p1");
  uiHome(1); save("12_home_eight_p2");
  settings.setPicksFromString("");
  uiHome(0); save("13_no_teams");
  uiBootHold(3); save("14_boot_hold");
  pickerShots();
  return 0;
}
