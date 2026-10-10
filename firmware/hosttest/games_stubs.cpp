// What the screens need from the rest of the firmware, for the games' tests on a computer.
#include "../mini/mn_ui.h"
#include "../mini/mn_game.h"
#include "../mini/mn_play.h"
#include "../mini/mn_net.h"
#include "../mini/mn_battery.h"
#include "../mini/mn_settings.h"
#include "../mini/mn_logo.h"
#include <stdio.h>
time_t mnNow;
static uint32_t fakeNow = 1000;
uint32_t millis() { return fakeNow; }
void advanceMillis(uint32_t ms) { fakeNow += ms; }
void delay(uint32_t) {}
void mnLog(const char*, ...) {}
bool netGame(int, Game&) { return false; }
uint32_t netVersion() { return 1; }
bool mnWifiUp() { return true; }
bool batPresent() { return false; }
int batPercent() { return 80; }
bool batCharging() { return false; }
void playTile(int, int, int, int, int, const Game&, bool) {}
uint32_t playTileSig(int, const Game&, bool) { return 0; }
bool logoDraw(const TeamSide&, int, int, int, uint16_t) { return false; }
uint32_t logoVersion() { return 0; }
const char* const DUMMY = "stubs";

bool touchHeldAt(int& x, int& y) { return false; }
