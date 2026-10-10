// The screens for this stage: start-up, Wi-Fi setup, home tiles, and a
// placeholder team page. Layout and colours follow design/mock_mini.py.
#pragma once
#include <Arduino.h>
#include "mn_lcd.h"

void uiSplash();
void uiSetup(const String& apName, bool cantJoin, const String& ssid);
bool uiSetupWifiHit(int x, int y);            // the "pick a network here" button
void uiJoining(const String& ssid);           // call often: the dots move
void uiConnected(const String& ip);
void uiMessage(const char* title, const char* line1, const char* line2, uint16_t titleCol);
void uiUpdating(int pct, const char* version);
void uiBootHold(int secondsLeft);

// Home: six tiles a page. Up to 5 teams: the teams plus AUTO on one page.
// More: page 0 has the first 5 plus MORE, page 1 the rest plus AUTO and a
// back button. uiHomeHit: what a tap on that page is on (-1 none,
// 0.. index into settings.picks, or one of the HIT_ values).
static const int HIT_AUTO = 100, HIT_MORE = 101, HIT_BACK = 102, HIT_EDIT = 103, HIT_WIFI = 104;   // EDIT, WIFI: page 0 only
int uiHomePages();                            // 1 or 2
void uiHome(int page);
void uiHomeRefresh(int page);                 // redraws just the tiles whose game changed
void uiHomeClock(bool force);                 // redraws the clock when the minute changes
void uiHomeWifiIcon(bool force);              // the Wi-Fi button: white when joined, red when not
int uiHomeHit(int page, int x, int y);
void uiTileFlash(int page, int hit);          // brief outline when a tile is tapped

// The HOME button on game screens (bottom left)
void uiHomeButton();
bool uiHomeButtonHit(int x, int y);
// Drawing helpers shared with the team picker (mn_picker.cpp)
void uiText(FontId f, const String& s, int x, int y, uint16_t col, uint16_t bg, textdatum_t datum);
void uiTile(int x0, int y0, int x1, int y1, uint16_t fill, uint16_t edge, int r = 14, int width = 1);

// The battery icon and percent, top right of the home and game screens. Draws
// nothing when no battery is connected. force: draw even if nothing changed.
void uiBattery(bool force);
