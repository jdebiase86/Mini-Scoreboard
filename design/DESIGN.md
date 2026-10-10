# Mini Scoreboard - design (settled Oct 6, 2026)

A desk-sized mini scoreboard on a 4.0" ESP32 touch screen. Companion to the
64x64 LED board in [Scoreboard](https://github.com/jdebiase86/Scoreboard).
Joe said go Oct 7, 2026 (first board due Fri Oct 9): firmware is being
built in the stages below.

## Hardware (ordered, not here yet - Amazon shows one board due Fri Oct 9 again, the other two Oct 20 to Nov 5, 2026; battery and speaker sooner)
- Hosyond 4.0" ESP32 display (Amazon B0FGJJ24S1; the one bought is the identical iPistBit listing): ESP32-32E (ESP32-D0WD-V3,
  dual core 240 MHz), 520 KB RAM, no PSRAM, 4 MB flash.
- 480x320 TN TFT, ST7796S over 4-line SPI. Resistive touch (firm press), so
  every button is big.
- USB-C power and programming, BAT plug with built-in lithium charging,
  micro SD, speaker plug, RGB LED.

- Oct 9, first try: on a USB-C to USB-C cable from a Mac the board stayed
  dark and the Mac saw no port (the same cable and Mac work with the big
  board). Photos show 5.1k ("512") resistors by the USB-C socket, so the
  parts are there; the cause is likely how they're wired (cheap boards often
  share one resistor between both CC pins, which Macs won't power). Not
  software: nothing lit up at all. Also dark on a car's USB-C port with a
  C-to-C cable, so it's the board: always power it from USB-A. Use a USB-A to USB-C
  cable (and a USB-C to USB-A adapter on the Mac); for everyday power a
  USB-A charger brick. Confirmed Oct 9: lights right up from a car USB charger. Factory demo (LVGL
  widgets) shows normal colours, upright with the USB-C plug on the right.
  Touch: a normal fingertip works fine, even through the factory protective
  film, and swipes work (the demo's tabs follow a swipe).

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
   2.0 mm plug that won't fit; not the 4 ohm version). Ordered Oct 9. Plugs into the board's speaker socket (IO26 DAC, IO4 amp enable
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

14. Swipes (Oct 9, Joe: all four), as shortcuts - every one also has a
   button: home left / right between the two pages; team / game screen left
   / right to the next / previous favourite; team picker list up / down for
   the next / previous page; pop-up cards (last play) swiped away, when
   they're built.

## Build stages (Oct 7)
1. Screen, touch and Wi-Fi setup (v0.1, firmware/mini): touch setup on first
   start, setup screen with a code to scan (joins the mini's own Wi-Fi, the
   setup page pops up), home screen tiles (up to 5 teams + AUTO) with
   placeholder team pages and a HOME button, settings at mini.local,
   screen dims after a minute, BOOT held 5 s forgets the Wi-Fi, updates
   over Wi-Fi from this repo's releases. Screens: design/mini_stage1.png
   (drawn by the real code, firmware/hosttest). Hardware test:
   firmware/mini_hw_test. Both flash from the browser (flash/README.md).
2. Scores (v0.3, written Oct 10, not yet tried on the board): ESPN per
   favourite team (one league feed at a time, filtered while streaming), logos
   from ESPN's own picture server kept in flash, live / final / upcoming
   tiles with live games first, a basic game screen for every sport (logos,
   score, clock, status), AUTO rotation every 20 s. Football field strip,
   win chance bar and last-play card: v0.4 (stage 3, football first). Pictures drawn from real
   ESPN feeds: design/mini_stage2.png.
3. Game screens: football field strip with the drive, win chance bar, last
   play pop-up (v0.4); then the other sports.
   v0.5 (Oct 10, not yet tried on the board), decided with Joe:
   - Checking: live game every 5 s; last 10 min before a start every 30 s;
     a game that just ended every 10 min for 5 h; otherwise hourly (early
     enough to be watching 10 min before a start). ESPN is asked for gzip
     (about 12x smaller; its data is cached 5 s anyway), so a 5 s poll is
     about 20 KB for a big football feed.
   - Wi-Fi button on the home screen: list of networks in range, tap to
     join, on-screen keyboard for the password (the iPhone can't run a
     hotspot and join another Wi-Fi at once, so the phone page can't do
     it), up to 5 remembered networks (the strongest in range is joined at
     start and when carried somewhere else), "Other network..." for a hidden
     one. Open networks with a sign-in page can't work (no browser).
   - Battery meter: icon and percent top right, bolt when charging (a guess
     from the voltage rising; the board has no charge wire), amber at 20%,
     red at 10%. No battery connected: nothing shown. Saver mode and the
     battery page are later.
   - Taps on the game screen: the last-play card (full play, holds the card
     8 s more when you go back), the teams (records, score by period,
     leaders or starters, stadium, TV), the field / down and distance / win
     bar (situation). The extras are only read from ESPN while a details
     card is open (they cost memory). No betting lines. ESPN sends leaders
     for NFL only before a game.
   v0.6 (Oct 10, not yet tried on the board), after Joe's first try of 0.5:
   - Wi-Fi: a scan that fails to start is retried (up to 3 times); the network
     you're on shows JOINED and SAVED together; forgetting it says you stay on
     it until you join another.
   - Battery: charging from a 40 mV jump in 30 s (plug in / out), a slow
     6 mV / 5 min climb or fall, or 4.17 V and up; 80 mV (less near full) is
     taken off while charging because the charger lifts the reading (90% became
     81% on unplug). Raw millivolts are in mini.local/log every minute.
   - The game's own page (ESPN summary, 40-90 KB compressed) is read as it
     streams (mn_jscan / mn_live, about 800 bytes kept) while a details card is
     open: team stats (football, basketball, hockey; none for baseball), leaders
     so far, and the last play for every sport.
   - Game screen: LAST PLAY button (live games); under the scores, where the
     field is for live football: score by period (live, final) or starters and
     leaders (before the game); tap it for the team stats card. Teams card and
     stats card switch with a button.
4. Red zone and play animations (touchdown, field goal, goal, home run and
   win first), then the rest.
5. Sound.
6. Saver mode and the battery level icon.
7. Remote for the big LED board (last).

## Carried over from the first design chat (Oct 6-7, added Oct 9)
Board and parts
- The board bought is the iPistBit 4.0" ESP32-32E listing ($19.99 for the
  4.0 inch option; the $17.99 option is the 3.5 inch). Same board as the
  Hosyond. LCDwiki calls it E32R40T (spec PDF:
  lcdwiki.com/res/E32R40T/E32R40T_E32N40T_Specification_V1.0.pdf).
- Speakers: two CQRobot pairs (three boards plus a spare). Passed over: a
  4 ohm 3 W 25 x 35 (ACEIRMC and similar) and a bare 28 mm 8 ohm (uxcell).
  Speaker polarity can't damage anything, so no plug check for it.

Case and stand
- Speaker pocket still to redo in deepen.py: 35 x 25 mm (plus the 1 mm foam
  lip and the wire tab) instead of 20 x 30 at x=21, face-down over the hole
  grille, battery about 15 mm toward the far end, same 24 mm closed case.
  Check it clears the RESET / BOOT flex tabs in the floor.
- Colours: case in Giants blue, stand in Giants red (case/scene.py
  CASE=0.07,0.17,0.50 STAND=0.70,0.10,0.20).
- Stand: not designed. Screen leans back about 17 degrees (Joe corrected a
  render that tipped forward). Needs the MakerWorld stand file or a new
  stand sized for the 24 mm case.
- The MakerWorld base, lid and buttons files are Joe's downloads and stay
  out of the repo on purpose; re-attach them to rebuild the case.
- Don't print the deep base until the speaker pocket is redone and the
  board fit is checked. Printing the designer's original buttons version
  (or just the lid) to test fit is fine.
- Open: in the original case the USB-C opening is on the left seen from the
  front (lid side), while the factory demo reads upright with USB-C on the
  right. Check on the real board and case; the firmware can flip the screen.

Battery saver (agreed, to build in its stage)
- A small battery button in the home top bar, next to BOARD, toggles Normal
  and Saver; the back "B" tab can toggle it too.
- Saver: dims to about a third, checks ESPN every 30 s with Wi-Fi resting in
  between, screen off after a couple of minutes untouched, tap to wake. Big
  moments still wake it, then it dims again.
- Offered, not drawn: a mock-up of the battery icon and Saver button.

Sound
- Short clips only (touchdown horn, goal horn, crowd roar, maybe a bit of a
  fight song), volume setting, mute, quiet hours, top volume just under max.

Ideas offered, not picked (don't build unless Joe asks)
- Countdown to the next game, standings, schedule (next five games), night
  mode / clock overnight, upset alert for ranked college teams, the RGB
  light glowing team colours (needs a clear window in the case), a
  rivalry-week look, a trash-talk button between two minis (needs an online
  relay), big-board sync of animations.
- Mock-ups offered, not drawn: baseball diamond (runners, balls, strikes,
  outs), hockey and basketball screens.

Small notes
- The TN screen is fine in shade but hard to read in direct sun.

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


## v0.7 (Oct 10, not yet tried on the board)
Done from the ideas below: dimmer dim (a third less); small HOME button in the top
bar of the game screen (next to the TV network and battery), the bottom is
free, so LAST PLAY moves left and the last-play card spans the screen; AUTO
changes game every 15 s; ticker mode (tap the AUTO tag on a game screen, pick
all my teams or one league; live games of that set take turns); compressed
downloads fixed (the 44 KB is set aside before the connection, with a
fallback, and the game page is never read uncompressed). Still open: the
bigger layout change (everything shifted down, ticker on top) as pictures, and
the battery calibration.

## Ideas for next (Oct 10, from Joe's first day with 0.5 / 0.6)
- Dim screen: a little dimmer than now (it is still quite readable).
- HOME button: smaller, like EDIT, and moved up into the top bar of the game
  screen (next to the TV network / battery), freeing the bottom of the screen.
- Game screen layout idea: shift everything down; the last-play ticker comes in
  at the top of the screen; logos, score and quarter in the middle; the field
  (or the score-by-period area) at the bottom. Or keep the layout and give the
  last-play card more room. Draw both as pictures first and let Joe pick.
- Ticker mode: pick a sport, and the screen rotates through that sport's live
  games (like AUTO but for one sport).
- AUTO mode: change game every 15 s instead of 20 s.
- Battery: sitting at 92% on the charger for a long time may just be full (the
  ADC reads a bit low near 4.2 V); check the minute-by-minute millivolts in
  mini.local/log from 0.6 on, and if needed add a small calibration offset
  (measure the battery plug with a multimeter once).
- Memory: after a score download the biggest free block is only about 71 KB,
  but compressed downloads were only asked for with more than 110 KB, so they
  never switched on. Reserve the 44 KB before connecting (and retry plain if
  the connection then fails), and never read the big game page uncompressed.
