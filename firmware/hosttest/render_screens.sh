#!/bin/sh
# Builds render_screens.cpp against LovyanGFX (SDL platform, needs libsdl2-dev)
# and lays the screens out on one sheet: design/mini_stage1.png.
#   LGFX=<LovyanGFX folder> (default ~/Arduino/libraries/LovyanGFX)
set -e
cd "$(dirname "$0")"
LGFX=${LGFX:-$HOME/Arduino/libraries/LovyanGFX}
mkdir -p out
cat > out/mn_settings_host.cpp <<'X'
// the parts of mn_settings.cpp the screens need (no flash storage)
#include "../../mini/mn_settings.h"
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
X
g++ -std=gnu++17 -O1 -w -DMN_HOST -I shim -I "$LGFX/src" \
  render_screens.cpp ../mini/mn_ui.cpp ../mini/mn_picker.cpp ../mini/mn_lcd.cpp out/mn_settings_host.cpp \
  $(find "$LGFX/src/lgfx/v1" -maxdepth 1 -name '*.cpp') \
  $(find "$LGFX/src/lgfx/v1/misc" "$LGFX/src/lgfx/v1/panel" "$LGFX/src/lgfx/v1/platforms/sdl" -name '*.cpp') \
  $(find "$LGFX/src/lgfx/utility" "$LGFX/src/lgfx/Fonts" -name "*.c") \
  -lSDL2 -lpthread -o out/render_screens
out/render_screens
python3 sheet.py
python3 sheet.py picker
