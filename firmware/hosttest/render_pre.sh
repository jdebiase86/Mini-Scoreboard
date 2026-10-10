#!/bin/sh
# Builds render_pre.cpp (the 0.10 screens, drawn by the real firmware code) and
# lays the pictures out on design/mini_stage6.png. Needs libsdl2-dev,
# LovyanGFX, ArduinoJson and PNGdec in ~/Arduino/libraries.
set -e
cd "$(dirname "$0")"
LIB=${LIB:-$HOME/Arduino/libraries}
LGFX=$LIB/LovyanGFX
P=$LIB/PNGdec/src
mkdir -p out
for c in adler32 crc32 infback inffast inflate inftrees zutil; do
  [ -f out/z_$c.o ] || gcc -D__LINUX__ -O2 -I "$P" -c "$P/$c.c" -o out/z_$c.o
done
g++ -std=gnu++17 -O1 -w -DMN_HOST -D__LINUX__ -I shim -I "$LGFX/src" -I "$LIB/ArduinoJson/src" -I "$P" \
  render_pre.cpp ../mini/mn_ui.cpp ../mini/mn_play.cpp ../mini/mn_logo.cpp ../mini/mn_espn.cpp ../mini/mn_jscan.cpp ../mini/mn_live.cpp ../mini/mn_lcd.cpp \
  ../mini/mn_detail.cpp ../mini/mn_keyboard.cpp ../mini/mn_wifi.cpp ../mini/mn_battery.cpp ../mini/mn_about.cpp \
  settings_host.cpp "$P/PNGdec.cpp" out/z_*.o \
  $(find "$LGFX/src/lgfx/v1" -maxdepth 1 -name '*.cpp') \
  $(find "$LGFX/src/lgfx/v1/misc" "$LGFX/src/lgfx/v1/panel" "$LGFX/src/lgfx/v1/platforms/sdl" -name '*.cpp') \
  $(find "$LGFX/src/lgfx/utility" "$LGFX/src/lgfx/Fonts" -name "*.c") \
  -lSDL2 -lpthread -o out/render_pre
rm -f out/p_*.ppm
out/render_pre
python3 sheet_pre.py
