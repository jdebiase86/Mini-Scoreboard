#include "mn_settings.h"
#include <Preferences.h>

Settings settings;
static const char* const LKEY[L_COUNT] = {"NFL", "CFB", "MLB", "NHL", "NBA"};
static const char* NS = "mini";

String teamKey(int idx) {
  if (idx < 0 || idx >= NTEAMS) return "";
  return String(LKEY[TEAMS[idx].league]) + ":" + TEAMS[idx].abbr;
}

int findTeam(const char* key) {
  const char* c = strchr(key, ':');
  if (!c) return -1;
  for (int i = 0; i < NTEAMS; i++) {
    const char* lk = LKEY[TEAMS[i].league];
    if (strlen(lk) == (size_t)(c - key) && !strncmp(lk, key, c - key) && !strcmp(TEAMS[i].abbr, c + 1)) return i;
  }
  return -1;
}

String Settings::picksString() const {
  String s;
  for (int i = 0; i < npicks; i++) { if (i) s += ","; s += teamKey(picks[i]); }
  return s;
}

void Settings::setPicksFromString(const String& s) {
  npicks = 0;
  int start = 0;
  while (start <= (int)s.length() && npicks < MAX_PICKS) {
    int comma = s.indexOf(',', start);
    if (comma < 0) comma = s.length();
    String k = s.substring(start, comma);
    k.trim();
    int idx = findTeam(k.c_str());
    bool dup = false;
    for (int i = 0; i < npicks; i++) if (picks[i] == idx) dup = true;
    if (idx >= 0 && !dup) picks[npicks++] = idx;
    start = comma + 1;
  }
  sortPicks();
}

// Auto's order: football first, then baseball, hockey, basketball; within a
// sport, the order they were picked in
void Settings::sortPicks() {
  for (int i = 1; i < npicks; i++)
    for (int j = i; j > 0 && leagueSport(TEAMS[picks[j]].league) < leagueSport(TEAMS[picks[j - 1]].league); j--) {
      int t = picks[j]; picks[j] = picks[j - 1]; picks[j - 1] = t;
    }
}

void Settings::load() {
  Preferences p;
  p.begin(NS, true);
  ssid = p.getString("ssid", "");
  pass = p.getString("pass", "");
  setPicksFromString(p.getString("teams", ""));
  tz = p.getInt("tz", 0);
  if (tz < 0 || tz >= NTZ) tz = 0;
  bright = p.getInt("bright", 2);
  if (bright < 0 || bright >= NBRIGHT) bright = 2;
  flip = p.getBool("flip", false);
  colour = p.getInt("colour", 0) & 3;
  hasCal = p.getBytes("tcal", tcal, sizeof(tcal)) == sizeof(tcal);
  p.end();
}

void Settings::save() {
  Preferences p;
  p.begin(NS, false);
  p.putString("ssid", ssid);
  p.putString("pass", pass);
  p.putString("teams", picksString());
  p.putInt("tz", tz);
  p.putInt("bright", bright);
  p.putBool("flip", flip);
  p.putInt("colour", colour);
  p.end();
}

void Settings::saveCal(const uint16_t* cal) {
  memcpy(tcal, cal, sizeof(tcal));
  hasCal = true;
  Preferences p;
  p.begin(NS, false);
  p.putBytes("tcal", tcal, sizeof(tcal));
  p.end();
}

void Settings::forgetCal() {
  hasCal = false;
  Preferences p;
  p.begin(NS, false);
  p.remove("tcal");
  p.end();
}

void Settings::forgetWifi() {
  ssid = "";
  pass = "";
  save();
}
