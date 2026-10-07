# Mini Scoreboard firmware

- mini/: the Mini Scoreboard (Arduino, esp32 core 2.0.9, LovyanGFX 1.2.7,
  ArduinoJson 7.4.2). Board: ESP32 Dev Module, 4 MB flash, partition scheme
  "Minimal SPIFFS" (two 1.9 MB program slots for updates over Wi-Fi).
- mini_hw_test/: the hardware test (one file, LovyanGFX only).
- hosttest/: draws the real screens on a computer (render_screens.sh ->
  design/mini_stage1.png). Needs libsdl2-dev and LovyanGFX.

Build:

    arduino-cli compile --fqbn esp32:esp32:esp32:PartitionScheme=min_spiffs \
      --build-property "compiler.cpp.extra_flags=-std=gnu++17" --output-dir build firmware/mini
    tools/make_full_bin.sh build mini flash/mini-X.Y-full.bin    # one file for the browser flasher

Fonts: tools/make_fonts.py turns Inter (the mock-ups' font) into
firmware/mini/mn_fonts.h.

Releases: bump FW_VERSION in mini/mn_version.h; when that lands on main,
.github/workflows/release.yml publishes vX.Y with mini-X.Y.bin (installed
over Wi-Fi) and mini-X.Y-full.bin (first install, flash/README.md). These
releases live in this repo only, never in Scoreboard.

Layout (mini/):
- mini.ino: modes (setup, joining, can't join, connected, home, team page),
  taps, screen dimming, BOOT held 5 s = forget Wi-Fi.
- mn_lcd: screen + touch driver (ST7796S/U, XPT2046), colour modes, fonts.
- mn_touch: taps, first-start touch setup.
- mn_ui: the screens. mn_portal: setup page / mini.local (plus /log and
  /screen, a picture of the screen). mn_ota: updates from GitHub releases.
- mn_settings, mn_teams (copied from Scoreboard's sb_teams.h), mn_dns
  (Scoreboard's captive-page name server), mn_log.
