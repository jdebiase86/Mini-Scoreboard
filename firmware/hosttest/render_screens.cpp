// Draws the mini's real screens (firmware/mini/mn_ui.cpp, same code as the
// board) into pictures on a computer, so they can be checked before flashing.
//   ./render_screens.sh   -> design/mini_stage1.png
#include "../mini/mn_lcd.h"
#include "../mini/mn_ui.h"
#include "../mini/mn_settings.h"
#include <chrono>
#include <thread>
#include <stdio.h>

Settings settings;
static auto t0 = std::chrono::steady_clock::now();
uint32_t millis() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count() + 100000;
}
void delay(uint32_t) {}

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
  return 0;
}
