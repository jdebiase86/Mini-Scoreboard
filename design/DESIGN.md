# Mini Scoreboard - design (settled Oct 6, 2026)

A desk-sized mini scoreboard on a 4.0" ESP32 touch screen. Companion to the
64x64 LED board in [Scoreboard](https://github.com/jdebiase86/Scoreboard).
Design only so far: no firmware is written until the board arrives and Joe
says go.

## Hardware (ordered, not here yet)
- Hosyond 4.0" ESP32 display (Amazon B0FGJJ24S1): ESP32-32E (ESP32-D0WD-V3,
  dual core 240 MHz), 520 KB RAM, no PSRAM, 4 MB flash.
- 480x320 TN TFT, ST7796S over 4-line SPI. Resistive touch (firm press), so
  every button is big.
- USB-C power and programming, BAT plug with built-in lithium charging,
  micro SD, speaker plug, RGB LED.

## Decisions
1. Wide layout (standing on a desk), dark look, rounded tiles, big buttons.
2. Home screen: a tile per favourite team plus an AUTO tile. Live games get a
   red outline and LIVE tag and sort to the top. Finals read "Final 31-24"
   with "Win vs LSU" (green) underneath. Upcoming games show day and time.
3. Tap a team: its game goes full screen and stays. HOME and AUTO buttons
   along the bottom. AUTO rotates through the favourites about every 20 s
   (live first, else each team's final or next game); "next in 14s" shows
   while it runs.
4. Live football keeps the field strip: end zones in team colours (away
   left, home right), ball spot, yellow line to gain, red zone tint, last
   play underneath. Timeouts under the scores, ball under the team with it.
5. Red zone:
   - Our team inside the 20: red RED ZONE banner with our logo for ~3 s.
   - Their team inside our 20: "DEFENSE!" banner for ~3 s - black with
     yellow caution tape, warning sign, small opponent logo (option A).
   - While in the red zone: red RED ZONE tag top right, red quarter, clock
     and down, bright red end of the field strip with a red outline, and a
     red "RED ZONE 1st & Goal" strip on that team's home tile.
6. Remote for the big LED board: a small BOARD button top right of the home
   screen, shown only when the big board is found on the same Wi-Fi (and a
   setting to turn it off). The page has the team logos plus AUTO, ALL NFL,
   ALL COLLEGE and FULL GAME; the board's current mode is outlined and the
   bottom line says what it is showing. Gift units never show it. The big
   board needs a small update to accept these commands. Built last.
7. Play animations (Oct 7, Joe loves them all; mock-ups design/mini_plays.png):
   our interception "PICKED OFF!", our fumble recovery "FUMBLE!" (team ball),
   their third-down stop "STOPPED!" (stop sign), turnover on downs
   "STONEWALLED!" (brick wall), sack "SACKED!" (burst), they punt
   "PUNT-ASTIC!", we convert on 4th "WENT FOR IT!", we punt "NO PUNT
   INTENDED" (dull grey), we turn it over "TURNOVER" (caution tape, like
   DEFENSE!). Bright team colours = good news, yellow tape = trouble.
   Detected from ESPN's last-play text and down/possession changes; a play
   can be missed if two happen between checks. Could go on the LED board too.
8. Animations redrawn smooth with the sharp logos (not the LED pixel ones).
   Start with touchdown, field goal, goal, home run and win.
9. Logos: only the favourites' logos (plus a few opponent logos, fetched
   when needed) are stored on the mini, downloaded from ESPN and shrunk.
   Same files as Scoreboard/assets/logos.
10. Power and case:
   - Board: the 4.0" ESP32-32E display (iPistBit listing, ST7796U screen chip,
     same board as the Hosyond with ST7796S; support both). Joe has three
     coming (his, a gift unit, a spare).
   - Battery lives inside the display's case, not in the stand, so the
     screen lifts off and runs anywhere (garage, patio).
   - Must last a whole football game with the screen bright. Battery: JLJLUP
     LP103665, 3.7 V 3000 mAh (likely ~2500 real), about 67 x 36 x 10 mm,
     protected, JST 1.25 mm 2-pin plug (matches the board's BAT plug, to be
     confirmed with a ruler photo). 7 to 9 hours. Ordered; full details and
     the polarity check in design/BATTERY.md. Closed case about 24 mm.
   - Small on/off slide switch on the side edge (the board has none).
     Charges through the board's USB-C while running.
   - Case: the MakerWorld "ESP32-32E 4.0 CYD" enclosure Joe found, buttons
     version (flex tabs press RESET and BOOT), with the base made 10 mm
     deeper (case/deepen.py, posts grow to match); lid unchanged, M3x6 screws. It must still sit
     on a desk stand (match that model's orange stand or a clip-on one).
   - Screen dims after about a minute when idle and wakes on a tap.
   - Before buying the battery: close-up photo of the board's BAT plug
     (plug sizes and wire order vary; reversed wires can damage the board).
     Draw the case for Joe to OK before printing.
11. Its own repo (this one) so its update releases never get in the way of
    the big board, which installs Scoreboard's latest release.

## Technical notes for later
- No PSRAM: fetch one favourite team's ESPN scoreboard at a time and filter
  the JSON while streaming. No all-FBS ticker or ranked feed. HTTPS needs
  roughly 40-50 KB of heap per connection.
- Logos stored as raw RGB565 (a 96 px logo is about 18 KB).
- Flash: two OTA app slots of about 1.9 MB plus a small filesystem.
- Reuse from Scoreboard: team list (sb_teams.h), Joe's logo picks
  (sb_logofix.h), ESPN parsing, setup page, OTA from GitHub releases.

## Mock-ups
- mini_mockups.png: home, live game, final (Auto on), upcoming game,
  touchdown animation, big-board remote.
- mini_redzone.png: home tile, pop-up and game screen with our team in the
  red zone.
- mini_redzone_opp.png: our RED ZONE pop-up next to the two options for
  theirs (A "DEFENSE!" was picked).
- mini_plays.png: the nine play animations (defense, turnovers, punts).
- mock_mini.py draws them all; see the top of the file.

## Device renders
design/device_renders.png (Oct 7): the deeper buttons case with the mock-up
screens on it - on a desk stand (orange stand is a placeholder, not designed
yet) and lying flat. Drawn by case/scene.py.
