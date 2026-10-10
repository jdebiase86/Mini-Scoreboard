// What the mini remembers between power-ups (ESP32 "Preferences" flash):
// Wi-Fi, the teams, time zone, brightness, the screen's touch setup and
// which way up it sits.
#pragma once
#include <Arduino.h>
#include "mn_teams.h"

// The home screen has six tiles. Up to 5 teams: the teams plus AUTO. More:
// the first 5 plus a MORE tile, and the rest plus AUTO on a second page.
static const int MAX_PICKS = 10;

struct TzDef { const char* label; const char* posix; };
static const TzDef TZS[] = {
  {"Eastern", "EST5EDT,M3.2.0,M11.1.0"},
  {"Central", "CST6CDT,M3.2.0,M11.1.0"},
  {"Mountain", "MST7MDT,M3.2.0,M11.1.0"},
  {"Arizona", "MST7"},
  {"Pacific", "PST8PDT,M3.2.0,M11.1.0"},
};
static const int NTZ = sizeof(TZS) / sizeof(TZS[0]);

struct BrightDef { const char* label; uint8_t level; };
static const BrightDef BRIGHTS[] = {{"Low", 70}, {"Medium", 140}, {"High", 210}, {"Max", 255}};
static const int NBRIGHT = sizeof(BRIGHTS) / sizeof(BRIGHTS[0]);

// Wi-Fi networks the mini remembers (the newest first). ssid / pass below are
// always the newest one.
static const int MAX_NETS = 5;
struct SavedNet { String ssid, pass; };

struct Settings {
  String ssid, pass;
  SavedNet nets[MAX_NETS];
  int nnets = 0;
  int picks[MAX_PICKS];   // indexes into TEAMS, in priority order
  int npicks = 0;
  int tz = 0;
  int bright = 2;
  bool flip = false;      // screen turned upside down
  int colour = 0;         // screen colour mode 0-3 (see lcdBegin)
  uint16_t tcal[8];       // touch setup (LovyanGFX calibrateTouch numbers)
  bool hasCal = false;

  void load();
  void save();
  void saveCal(const uint16_t* cal);
  void forgetCal();
  void forgetWifi();                   // all of them
  void addNet(const String& ssid, const String& pass);   // new, or updates the password; becomes the newest
  void forgetNet(int i);
  int findNet(const String& ssid) const;                 // -1 = not saved
  bool hasWifi() const { return nnets > 0; }
  String picksString() const;          // "NFL:NYG,CFB:FLA"
  void setPicksFromString(const String& s);
  void sortPicks();                    // football, baseball, hockey, basketball
};

int findTeam(const char* key);          // "NFL:NYG" -> index, or -1
String teamKey(int idx);
extern Settings settings;
