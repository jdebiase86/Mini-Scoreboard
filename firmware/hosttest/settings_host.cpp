// the parts of mn_settings.cpp the picture-drawing programs need (no flash storage)
#include "../mini/mn_settings.h"
static const char* const LKEY[L_COUNT] = {"NFL", "CFB", "MLB", "NHL", "NBA"};
int findTeam(const char* key) {
  const char* c = strchr(key, ':');
  if (!c) return -1;
  for (int i = 0; i < NTEAMS; i++) {
    const char* lk = LKEY[TEAMS[i].league];
    if (strlen(lk) == (size_t)(c - key) && !strncmp(lk, key, c - key) && !strcmp(TEAMS[i].abbr, c + 1)) return i;
  }
  return -1;
}
void Settings::setPicksFromString(const String& s) {
  npicks = 0;
  size_t start = 0;
  while (start <= s.size() && npicks < MAX_PICKS) {
    size_t comma = s.find(',', start);
    if (comma == std::string::npos) comma = s.size();
    int idx = findTeam(s.substr(start, comma - start).c_str());
    if (idx >= 0) picks[npicks++] = idx;
    start = comma + 1;
  }
  // same order as the board: football, baseball, hockey, basketball
  for (int i = 1; i < npicks; i++)
    for (int j = i; j > 0 && leagueSport(TEAMS[picks[j]].league) < leagueSport(TEAMS[picks[j - 1]].league); j--) {
      int t = picks[j]; picks[j] = picks[j - 1]; picks[j - 1] = t;
    }
}
int Settings::findNet(const String& s) const {
  for (int i = 0; i < nnets; i++) if (nets[i].ssid == s) return i;
  return -1;
}
void Settings::addNet(const String& s, const String& p) { nets[0].ssid = s; nets[0].pass = p; nnets = nnets ? nnets : 1; ssid = s; pass = p; }
void Settings::forgetNet(int i) { for (; i + 1 < nnets; i++) nets[i] = nets[i + 1]; if (nnets) nnets--; }
void Settings::save() {}
