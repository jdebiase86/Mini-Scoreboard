#!/bin/sh
# Builds render_screens.cpp against LovyanGFX (SDL platform, needs libsdl2-dev)
# and lays the screens out on one sheet: design/mini_stage1.png.
#   LGFX=<LovyanGFX folder> (default ~/Arduino/libraries/LovyanGFX)
set -e
cd "$(dirname "$0")"
LGFX=${LGFX:-$HOME/Arduino/libraries/LovyanGFX}
mkdir -p out
P=$HOME/Arduino/libraries/PNGdec/src
for c in adler32 crc32 infback inffast inflate inftrees zutil; do
  [ -f out/z_$c.o ] || gcc -D__LINUX__ -O2 -I "$P" -c "$P/$c.c" -o out/z_$c.o
done
g++ -std=gnu++17 -O1 -w -DMN_HOST -D__LINUX__ -I shim -I "$LGFX/src" -I "$HOME/Arduino/libraries/ArduinoJson/src" -I "$P" \
  render_screens.cpp ../mini/mn_ui.cpp ../mini/mn_picker.cpp ../mini/mn_play.cpp ../mini/mn_logo.cpp ../mini/mn_espn.cpp \
  ../mini/mn_lcd.cpp settings_host.cpp "$P/PNGdec.cpp" out/z_*.o \
  $(find "$LGFX/src/lgfx/v1" -maxdepth 1 -name '*.cpp') \
  $(find "$LGFX/src/lgfx/v1/misc" "$LGFX/src/lgfx/v1/panel" "$LGFX/src/lgfx/v1/platforms/sdl" -name '*.cpp') \
  $(find "$LGFX/src/lgfx/utility" "$LGFX/src/lgfx/Fonts" -name "*.c") \
  -lSDL2 -lpthread -o out/render_screens
out/render_screens
python3 sheet.py
python3 sheet.py picker
