// The screens for this stage: start-up, Wi-Fi setup, home tiles, and a
// placeholder team page. Layout and colours follow design/mock_mini.py.
#pragma once
#include <Arduino.h>

void uiSplash();
void uiSetup(const String& apName, bool cantJoin, const String& ssid);
void uiJoining(const String& ssid);           // call often: the dots move
void uiConnected(const String& ip);
void uiMessage(const char* title, const char* line1, const char* line2, uint16_t titleCol);
void uiUpdating(int pct, const char* version);
void uiBootHold(int secondsLeft);

// Home: six tiles a page. Up to 5 teams: the teams plus AUTO on one page.
// More: page 0 has the first 5 plus MORE, page 1 the rest plus AUTO and a
// back button. uiHomeHit: what a tap on that page is on (-1 none,
// 0.. index into settings.picks, or one of the HIT_ values).
static const int HIT_AUTO = 100, HIT_MORE = 101, HIT_BACK = 102;
int uiHomePages();                            // 1 or 2
void uiHome(int page);
void uiHomeClock(bool force);                 // redraws the clock when the minute changes
int uiHomeHit(int page, int x, int y);
void uiTileFlash(int page, int hit);          // brief outline when a tile is tapped

// Placeholder game page (team = index into TEAMS, or -1 for AUTO) with HOME
void uiTeam(int team);
bool uiHomeButtonHit(int x, int y);
