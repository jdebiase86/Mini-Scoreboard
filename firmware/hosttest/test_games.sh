#!/bin/sh
# Builds and runs the games' logic tests on a computer (sudoku maker, connect four, 2048, shots).
set -e
cd "$(dirname "$0")"
LIB=${LIB:-$HOME/Arduino/libraries}
LGFX=$LIB/LovyanGFX
mkdir -p out
COMMON="-std=gnu++17 -O2 -w -DMN_HOST -I shim -I $LGFX/src -I $LIB/ArduinoJson/src"
OBJS="../mini/mn_ui.cpp ../mini/mn_lcd.cpp ../mini/mn_games.cpp games_stubs.cpp settings_host.cpp"
LG="$(find $LGFX/src/lgfx/v1 -maxdepth 1 -name '*.cpp') $(find $LGFX/src/lgfx/v1/misc $LGFX/src/lgfx/v1/panel $LGFX/src/lgfx/v1/platforms/sdl -name '*.cpp') $(find $LGFX/src/lgfx/utility $LGFX/src/lgfx/Fonts -name '*.c')"
for t in "$@"; do
  g++ $COMMON -DMN_GAMES_HELPERS_ONLY test_$t.cpp $OBJS $LG -lSDL2 -lpthread -o out/test_$t 2>&1 | head -20
  out/test_$t
done
