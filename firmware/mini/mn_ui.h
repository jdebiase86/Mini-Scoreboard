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

// Home: a tile per favourite plus AUTO. uiHomeHit: which tile a tap is on
// (-1 none, 0.. team tile, HIT_AUTO).
static const int HIT_AUTO = 100;
void uiHome();
void uiHomeClock(bool force);                 // redraws the clock when the minute changes
int uiHomeHit(int x, int y);
void uiTileFlash(int hit);                    // brief outline when a tile is tapped

// Placeholder game page (team = index into TEAMS, or -1 for AUTO) with HOME
void uiTeam(int team);
bool uiHomeButtonHit(int x, int y);
