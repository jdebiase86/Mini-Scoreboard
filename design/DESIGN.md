# Mini Scoreboard - design (settled Oct 6, 2026)

A desk-sized mini scoreboard on a 4.0" ESP32 touch screen. Companion to the
64x64 LED board in [Scoreboard](https://github.com/jdebiase86/Scoreboard).
Joe said go Oct 7, 2026 (first board due Fri Oct 9): firmware is being
built in the stages below.

## Hardware (ordered, not here yet - Amazon shows one board due Fri Oct 9 again, the other two Oct 20 to Nov 5, 2026; battery and speaker sooner)
- Hosyond 4.0" ESP32 display (Amazon B0FGJJ24S1): ESP32-32E (ESP32-D0WD-V3,
  dual core 240 MHz), 520 KB RAM, no PSRAM, 4 MB flash.
- 480x320 TN TFT, ST7796S over 4-line SPI. Resistive touch (firm press), so
  every button is big.
- USB-C power and programming, BAT plug with built-in lithium charging,
  micro SD, speaker plug, RGB LED.

- Oct 9, first try: on a USB-C to USB-C cable from a Mac the board stayed
  dark and the Mac saw no port (the same cable and Mac work with the big
  board). Likely the board's USB-C socket lacks the resistors that ask a
  C-to-C cable for power, a common gap on these boards. Use a USB-A to USB-C
  cable (and a USB-C to USB-A adapter on the Mac); for everyday power a
  USB-A charger brick. Confirmed Oct 9: lights right up from a car USB charger. Factory demo (LVGL
  widgets) shows normal colours, upright with the USB-C plug on the right.

## Decisions
1. Wide layout (standing on a desk), dark look, rounded tiles, big buttons.
2. Home screen: a tile per favourite team plus an AUTO tile. Live games get a
   red outline and LIVE tag and sort to the top. Finals read "Final 31-24"
   with "Win vs LSU" (green) underneath. Upcoming games show day and time.
3. Tap a team: its game goes full screen and stays. Game screens have only
   a smaller HOME button bottom left (Oct 7); AUTO is started from the AUTO
   tile on the home screen, and a small "AUTO next game in 14s" tag shows
   while it rotates (design/mini_game_v2.png). Tapping HOME ends Auto. AUTO rotates through the favourites about every 20 s
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
8. Game screen extras (Oct 7, mock-ups design/mini_game_extras.png):
   win chance bar under the field (Joe loves it). Picked: the drive drawn
   on the field as a light band from where it started to the ball (no
   drive text), and each new play pops up as a "LAST PLAY" card beside the
   HOME button for a few seconds, then hides (design/mini_game_v2.png); close game alert (late, within one score);
   tap the score for stats (Joe is fine giving that tap up if needed).
   Other sports: everything the LED board does (hockey goal and
   intermission, baseball run/home run/grand slam, basketball quarter card
   and three, kickoff, quarter/halftime, field goal, flag, first down, win)
   plus more: hockey power play, penalty, empty net, overtime/shootout;
   baseball strikeout, walk-off, double play, bases loaded, inning card;
   basketball runs, buzzer beater, dunk, and-one.
   Sound (Joe wants it, Oct 7): CQRobot enclosed (cavity) speaker, 8 ohm,
   2 W RMS (3 W max), front-firing, 35 x 25 x 6.8 mm plus a 1 mm lip and a
   small wire tab, JST 1.25 mm 2-pin plug on an 8 cm lead (red +), sold as
   a pair. Amazon B0CMQCQQV4 (not B0F2MXKNBW, the same speaker with a
   2.0 mm plug that won't fit; not the 4 ohm version). Not ordered yet (Oct 9). Plugs into the board's speaker socket (IO26 DAC, IO4 amp enable
   per LCDwiki). Sits face-down at the USB-C end over a patch of sound holes
   in the back (case/case_speaker_option.png shows the earlier 20 x 30
   pocket; to redo at 25 x 35, battery slides about 15 mm toward the far
   end). Volume capped just under max; mute and quiet hours.
9. Animations redrawn smooth with the sharp logos (not the LED pixel ones).
   Start with touchdown, field goal, goal, home run and win.
10. Logos: only the favourites' logos (plus a few opponent logos, fetched
   when needed) are stored on the mini, downloaded from ESPN and shrunk.
   Same files as Scoreboard/assets/logos.
11. Power and case:
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
   - No on/off switch (Oct 7, Joe: "if it dies it dies, plug it back in").
     The battery's protection circuit cuts off before it's harmed. Charges
     through the board's USB-C while running. Instead: dim / battery saver
     mode (screen dims, slower checks, screen sleeps, tap to wake), set from
     the screen and maybe the back "B" tab (BOOT button, IO0, readable once
     running). Saver is only ever turned on and off by Joe (Oct 7) - never
     automatically on unplugging. Even in Saver the screen wakes for big
     moments (touchdown, turnover, red zone). A battery level icon if the
     board can measure the battery.
   - Case: the MakerWorld "ESP32-32E 4.0 CYD" enclosure Joe found, buttons
     version (flex tabs press RESET and BOOT), with the base made 10 mm
     deeper (case/deepen.py, posts grow to match); lid unchanged, M3x6 screws. It must still sit
     on a desk stand (match that model's orange stand or a clip-on one).
   - Screen dims after about a minute when idle and wakes on a tap.
   - Before the battery's first plug-in: close-up photo of the board's BAT
     plug next to a ruler and the polarity check in design/BATTERY.md.
     Draw the final case for Joe to OK before printing.
12. Its own repo (this one) so its update releases never get in the way of
    the big board, which installs Scoreboard's latest release.

13. Team picker on the mini itself (Oct 8, mock-ups design/mini_picker.png):
   a small EDIT button in the home screen's top bar opens it. One tile per
   league with how many are picked, plus a green DONE tile (save, go home).
   College football picks a conference first (SEC, Big Ten, ACC, Big 12,
   Others). Teams: option A picked - a list, 10 a page in two columns, big
   up / down arrows on the right, tap a row to tick it. The phone setup
   page stays the main way; this is the handy extra. Up to 10 teams: more
   than 5 go on a second home page behind a MORE tile (Oct 7).

## Build stages (Oct 7)
1. Screen, touch and Wi-Fi setup (v0.1, firmware/mini): touch setup on first
   start, setup screen with a code to scan (joins the mini's own Wi-Fi, the
   setup page pops up), home screen tiles (up to 5 teams + AUTO) with
   placeholder team pages and a HOME button, settings at mini.local,
   screen dims after a minute, BOOT held 5 s forgets the Wi-Fi, updates
   over Wi-Fi from this repo's releases. Screens: design/mini_stage1.png
   (drawn by the real code, firmware/hosttest). Hardware test:
   firmware/mini_hw_test. Both flash from the browser (flash/README.md).
2. Scores: ESPN per favourite team, logos, live / final / upcoming tiles,
   AUTO rotation.
3. Game screens: football field strip with the drive, win chance bar, last
   play pop-up; then the other sports.
4. Red zone and play animations (touchdown, field goal, goal, home run and
   win first), then the rest.
5. Sound.
6. Saver mode and the battery level icon.
7. Remote for the big LED board (last).

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
- mini_game_extras.png: win chance bar, drive tracker options A/B/C, close
  game alert, stats page.
- mini_picker.png: the team picker on the mini (option A list picked).
- mini_game_v2.png: the picked game screen (drive on the field, play
  pop-up, small HOME, no AUTO button), final with Auto running, upcoming.
- mock_mini.py draws them all; see the top of the file.

## Device renders
design/device_renders.png (Oct 7): the deeper buttons case with the mock-up
screens on it - on a desk stand (orange stand is a placeholder, not designed
yet) and lying flat. Drawn by case/scene.py.
