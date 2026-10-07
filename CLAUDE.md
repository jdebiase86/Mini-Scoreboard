# Mini Scoreboard - notes for Claude

## Working with Joe
- Reply in plain text only: no markdown, bullets, bold or code blocks. Joe often reads on his phone and copies text into iMessage. Numbered lines like "1." are fine.
- Talk a design through before coding. For anything visual, render a mock-up picture first (design/mock_mini.py) and wait for Joe to pick. Don't code until he says go.
- Joe isn't a programmer. Explain in plain words, keep steps short, and say exactly what to tap.
- Never ask for or accept Joe's GitHub (or any) password.
- Gift units (for example his father-in-law: Cowboys and LSU, Central time, Windows PC) must stay dead simple to set up.

## Status
Design settled Oct 6, 2026 (design/DESIGN.md). First board arrived Oct 7 and Joe said go: building in the stages listed in design/DESIGN.md (stage 1 = screen, touch, Wi-Fi setup, v0.1). Firmware notes in firmware/README.md. Before the battery's first plug-in, do the BAT plug photo and polarity check in design/BATTERY.md with Joe.
- Firmware is not tried on the board until Joe flashes it. Pins come from the LCDwiki E32R40T table; the hardware test (flash/mini-hw-test-full.bin) confirms them, and its colour mode number goes into the default in firmware/mini/mn_settings.cpp if it isn't 1.
- Visual changes: render with firmware/hosttest/render_screens.sh (real firmware code) and show Joe the picture before pushing.

## Related repo
jdebiase86/Scoreboard holds the LED board firmware, the team list, ESPN parsing and the real ESPN logos (assets/logos) to reuse. Its boards install Scoreboard's latest release, so never publish mini releases there.
