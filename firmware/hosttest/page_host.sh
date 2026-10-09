#!/bin/sh
# Builds page_host.cpp (the real setup page, on a computer) and takes phone-
# sized pictures of it with Chromium: out/setup.png, out/settings.png.
set -e
cd "$(dirname "$0")"
LGFX=${LGFX:-$HOME/Arduino/libraries/LovyanGFX}
mkdir -p out
[ -f out/mn_settings_host.cpp ] || ./render_screens.sh
g++ -std=gnu++17 -O0 -w -DMN_HOST -I shim_page -I shim -I "$LGFX/src" \
  page_host.cpp ../mini/mn_lcd.cpp out/mn_settings_host.cpp \
  $(find "$LGFX/src/lgfx/v1" -maxdepth 1 -name '*.cpp') \
  $(find "$LGFX/src/lgfx/v1/misc" "$LGFX/src/lgfx/v1/panel" "$LGFX/src/lgfx/v1/platforms/sdl" -name '*.cpp') \
  $(find "$LGFX/src/lgfx/utility" "$LGFX/src/lgfx/Fonts" -name "*.c") \
  -lSDL2 -lpthread -o out/page_host
out/page_host
