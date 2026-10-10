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
Also in 0.7 (after Joe's screen stayed on old numbers for 10+ minutes while ESPN
had moved on): the download guard wanted a 70 KB free block but the biggest
block sits at 65-71 KB once logos have fragmented the memory, so score
downloads could stop for good - now 48 KB, with a log line when it waits; an
amber "OLD 2 min" tag in the top bar of a live game that hasn't heard from ESPN
for 45 s; a watchdog (no update on a live game for 3 min: reconnect Wi-Fi; 7
min: restart, which also clears the memory); football's last play comes from
the game's own play list (the scoreboard feed's lastPlay lags during stoppages).

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
- Wi-Fi list paging: on the board the down arrow works but the up arrow does
  not (the host picture program pages down and up fine with the same code), so
  it is probably touch accuracy in the bottom-right corner: make both arrows
  bigger and higher, and widen their hit zones. Swiping works as well.
- Games page (idea, Oct 10): a GAMES tile as the last page after the favourites.
  Small games that draw straight to the screen (no big buffers; the heap is
  about 160 KB with a 70 KB biggest block): memory match with the team logos
  already stored, snake, 2048, connect four against the board, sports trivia,
  a penalty-kick or free-throw flick, a reaction test. With the stylus: a
  drawing pad, minesweeper, sudoku, battleship. High scores kept in the board's
  settings. Sounds when the speaker is in.

## Games page plan (Oct 10, Joe picked from the ideas; build when he says go)
Wanted soon (family-friendly, works with no Wi-Fi, finger first and stylus
optional): a GAMES tile after the favourites pages.
- Penalty kick / free throw: flick to shoot; the flick's speed is the power and
  its direction the aim. Kick: a power window, corners score more than the
  middle, the goalie dives at random and gets better each level, a miss is wide
  or over. Free throw: the arc must match the power; swish bonus; streak.
- Memory match with the team logos already stored; levels get harder (2x3, 3x4,
  4x4, 4x5, then a flip limit and a timer); leagues unlock as you go.
- Sudoku: three difficulty levels, a number pad, mistakes marked, hints, notes,
  and the game is kept if you leave. Puzzles made on the board.
- More: snake, 2048, Simon with team colours (sounds with the speaker),
  connect four against the board, sports trivia, reaction test, coin toss.
- High scores and progress (levels, stars, unlocks) kept per game in the
  board's settings. Needs a touch drag/flick reading (speed and angle) added to
  mn_touch. Everything draws straight to the screen; no big buffers.

## v0.8 hotfix (Oct 10): 0.7 stopped fetching scores on the board
The 0.7 log showed one good download, then "biggest block 39 KB" and "waiting
for memory" forever (the guard wanted 48 KB). The compressed downloads (their
44 KB set aside before each connection) had fragmented the memory the first time they ran
on a real board. 0.8: compressed downloads off (USE_GZIP in mn_net.cpp; they switch
themselves off if the answer won't unpack), the guard is 30 KB, and the
watchdog restarts the mini after 90 s of waiting for memory. The game page is
read plain, every 30 s, only while a card is open. Compressed downloads can come back
once they are checked on the board with a log.

## v0.9: the GAMES page (Oct 10, not yet tried on the board)
GAMES button in the home header, and one more page after the last favourite when
swiping through the teams. Eleven games (firmware/mini/mn_g_*.cpp, menu in
mn_games.cpp), all drawing straight to the screen:
- Penalty Kick and Free Throw: flick up; speed = power, direction = aim; five
  shots a round; score enough to go up a level (keeper sharper / power window
  narrower).
- Logo Match: team logos from the board's storage (your teams and their
  opponents; symbols if not enough); 8 levels (more cards, then a move limit,
  then a timer).
- Sudoku: easy / medium / hard (medium and hard unlock), numbers pad, notes,
  hints, wrong numbers in red, kept if you leave. Puzzles are made on the board
  (always one answer).
- Snake (swipe or tap to steer, walls wrap), 2048 (swipe, undo), Cheer Simon,
  Connect Four against the mini (easy / medium / hard, unlock by winning),
  Sports Trivia (56 questions, 10 a round, 15 s each, 7 right to go up),
  Reaction, Coin and Dice.
- Scores and levels are kept in the board's settings (namespace "games").
- Dimming: settings page "When nobody touches the screen": dim unless charging
  (default), always, or never; a plugged-in board (charging, or 4.15 V or more)
  doesn't dim in the default mode.
- Tested on a computer: the sudoku maker (120 puzzles, all with exactly one
  answer), connect four (the mini beats a random player), 2048 sliding, and
  random taps / flicks / swipes in every game with a memory checker
  (hosttest/test_games.sh, fuzz_games.sh); pictures in design/mini_stage5.png
  (render_games2.sh). Not tried on the board.
- Sound: when the speaker is in (Simon and the shots first).

## v0.10 (Oct 10/11, not yet tried on the board)
Fixes (from Joe's logs of 0.9):
- Scores froze for six minutes while two big logos were saved over and over:
  making room for one 112 px logo deleted every other 112 px logo (including
  the one just saved), so each fetch undid the other. Now the six logos saved
  most recently and any logo waiting to be fetched are never deleted; other
  big logos go first, then any other unprotected one; with no room it gives
  up for 10 minutes instead of looping. The net task also does at most two
  logo fetches in a row before looking at scores.
- Sudoku restarted the board while "Making a puzzle": the search recursed
  deeper than the 8 KB loop stack. The board no longer makes puzzles: it mixes
  one of 24 ready-made puzzles per level (mn_sudoku_bank.h, made by
  hosttest/make_sudoku_bank.sh, each with exactly one answer) by renaming the
  digits and shuffling rows, columns, bands and stacks (and turning the grid):
  billions of different-looking games, ~13 KB of flash, nothing heavy at run
  time. test_games.sh sudoku checks 300 mixed puzzles per level (all unique
  answers).
- Snake best score is saved the moment it is beaten (it was only saved at game
  over).
- False charging bolt: a 40 mV jump up now has to hold for a minute (and the
  rise has to stay) before it counts as charging; the 5 minute trend rules are
  8 mV up / 4 mV down; the shown percentage moves one point at a time (every
  15 s), so a wrong guess never swings it 16 points.
- Wi-Fi list: the previous / next page arrows have a wider touch area.
New:
- Upcoming game screen: cleaner (design/mini_stage6.png): the logos moved down,
  name and record under each with a bar in the team colour, the date, a big
  time and one small line (a countdown within 12 hours, else the stadium); no
  starters / leaders on the screen. A GAME DETAILS button (bottom left) opens
  the details card, as tapping the screen already did.
- Bye week / no game: each favourite's logo, colour and name from its last game
  are kept in flash (namespace "teams", written only when they change). A team
  with no game shows its own logo with BYE WEEK (football) or NO GAME (other
  sports) on the home tile and the game screen.

Found on the board right after 0.10 (Joe, Oct 10; to fix next, nothing coded yet):
- A team on a bye week / out of season shows its letters, not its logo, until it has had a game since 0.10
  (the logo is only remembered from a game seen). Fix: when a favourite has no remembered logo and no game,
  read ESPN's own team page once (site.api.espn.com/apis/site/v2/sports/<sport>/<league>/teams/<abbr>, 20-36 KB,
  works with the abbreviation; gives id, colour and logo paths), keep the logo path / colour / name in flash.
  The out-of-season tile should show just the logo (no "none scheduled"): maybe "Off season" in grey.
- GAME DETAILS button does nothing: taps below y 246 count as "no hit" on the game screen; make the button's
  area (and the bottom of an upcoming game screen) open the details card.
- More-teams page: the MY TEAMS back button overhangs the clock and clips it; narrow the button or move the clock.
- One favourite's logo (it had a game on Sunday) was still letters: ask for the log (look for "logo ... no room" lines).

Still to do: battery calibration (needs Joe's overnight and plug-in logs),
animations (touchdown burst etc., design/mini_mockups.png no. 5), more games
(Breakout, Whack-a-mole, football drill ...), sound.

## v0.11 (Oct 11, not yet tried on the board)
From Joe's first day with 0.10 and the plan for his event day:
- Bye week / out of season: a favourite with no game and no remembered logo reads ESPN's own page for the team once
  (site.api.espn.com/apis/site/v2/sports/<sport>/<league>/teams/<abbr>, works with the short code) and keeps the logo path,
  colour and name in flash. Football in September to January: BYE WEEK (logo, big yellow words); otherwise "Off season"
  in grey under the logo. (hosttest/test_team.cpp checks the page reader with real pages: feeds/team_*.json.)
- GAME DETAILS button works (taps below y 246 on an upcoming game open the card); the MY TEAMS back button on the
  second home page is narrower and the clock moved over, so it no longer clips the time.
- Data saver (setup page: Automatic / Always / Never): automatic on an iPhone hotspot (gateway 172.20.10.x) or the usual
  Android one (192.168.43.x). Live games every 8 s instead of 5, last ten minutes before a start every 60 s, a game that
  just ended every 30 min, at most eight logo downloads per ten minutes.
- No Wi-Fi: "Waiting for Wi-Fi..." where the clock is; with one saved network it asks again every minute.
- Low battery: a message once at 15 % (again only after a charge).
- About page (tap the battery at the top right of the home or a game screen): version and run time, why it last
  restarted (power, crash, stuck / watchdog, or on purpose with the reason) and how long that run lasted, Wi-Fi name and
  strength, free memory and biggest block, battery and the data saver, then the battery's last 24 hours as a graph. BEFORE
  RESTART shows the last ten log lines of the run before. The reason and lines live in the chip's RTC memory
  (RTC_NOINIT_ATTR, mn_diag.*): they survive crashes and watchdog restarts, not a power cut.
- Battery history: voltage, percent and the charging guess every five minutes for 24 hours (288 x 8 bytes), in RTC memory and
  saved to flash (namespace "diag") once an hour; http://mini.local/battery gives it as a table (CSV) to copy.
- Games: Basket Toss (page 2 of the games): slide the bases under the flyer; 3 lives, a spare life every 10 catches, level up
  every 5 catches (faster and wider tosses, narrower arms, wind from level 4, no landing ring after level 3).
  Pictures: design/mini_basket.png. It is the plain cheer look only (no school logo or colours; nothing personal in the repo).
- Pictures of the new screens: design/mini_stage6.png (0.10) and design/mini_stage7.png (0.11).
Next: score alerts, idle clock, what's new screen, next five games, friends mode, the two-player games (Connect Four for two,
dots and boxes, checkers, air hockey, tap duels, with "Who's playing?" names and a tally per pair); battery calibration once
the history has a full charge and a plug-in.

## Animations and always-on (Oct 11, Joe: build them; mock-ups design/mini_anims.png, mock_anim.py)
Joe wants all of them: the nine play banners (design/mini_plays.png), the red zone pop-ups (mini_redzone_opp.png: ours red,
theirs caution tape), and for big moments sunbursts that are not overdone. Added in the mock-up: the field goal (kick
lined up, ball in the air, through the uprights with IT'S GOOD!, NO GOOD wide right with caution tape), touchdown, their
touchdown (caution tape), hockey goal (red goal lights), home run, final win (confetti). Rule: good for us = bright team
colours and rays; bad (they score, we turn it over, a missed kick) = black caution tape. Sound comes later (the speaker is
in the mail): touchdown horn, goal horn, crowd, kick thud.
Dim on battery: stays fully bright while a game is live on the screen (setting: do not dim during a live game on
battery), otherwise dims after a minute; the dim level goes from level/12 to about level/25 (a fifth as bright).
Detecting moments: ESPN's last-play text, score changes and down / possession changes; a moment can be missed if two
happen between checks (live checks every 5 s, 8 s in data saver).

## v0.12: animations, dimmer screen (Oct 11, not yet tried on the board)
Ported from the big LED board's rules (Scoreboard repo, sb_events.cpp): the mini compares each new look at a favourite's game
with the look before (only one under 3.5 minutes old counts, so old points never set one off), and queues what changed (up to
4, dropped if older than 45 s when the screen gets to them). mn_events.cpp (rules; hosttest/test_events.cpp checks 33 cases),
mn_fx.cpp (the pictures). A tap on the screen ends an animation. Good news = team colours and rays; bad news = caution tape.
- Ours: TOUCHDOWN (6+ points), FIELD GOAL (3: the kick, the ball through the uprights, IT'S GOOD!), GOAL, HOME RUN / GRAND SLAM / RUN
  (baseball: ESPN's last play text, now also read for MLB), THREE, WIN ("GIANTS WIN", "FLORIDA WINS", confetti), KICKOFF,
  HALFTIME / END OF A QUARTER / INTERMISSION card, FIRST DOWN, PICKED OFF!, FUMBLE!, SACKED!, STOPPED!, STONEWALLED!, WENT FOR IT!,
  PUNT-ASTIC!, NO PUNT INTENDED, FLAG (the foul from the play text).
- Theirs, in caution tape: their touchdown / field goal / goal / home run, TURNOVER (we throw it away), NO GOOD (our kick misses;
  when theirs misses it is a bright banner).
- The setup page (mini.local) has "Try the animations": a button for each one.
- Data: a live game that nobody is watching is checked every 15 s (30 s in data saver) instead of 5 s (8 s); the screen tells the
  net task what it shows (home and ticker: every favourite; a game screen or card: that one), and a dimmed screen watches nothing.
- Screen: dims to level / 25 (at least 3) instead of level / 12; it does not dim while a live game is being watched (a live
  favourite on the home or ticker screen, or the live game on screen), on the battery; "Always dim when idle" still dims. A
  moment wakes the screen.
- Pictures: design/mini_fx.png (frames of every animation, drawn by the real code: hosttest/render_fx.sh).
- Sound later: the hooks would go where fxStart is called (touchdown horn, goal horn, crowd, kick thud).

## v0.13: the big animations redone in the LED board's style (Oct 11, not yet tried on the board)
Joe: the 0.12 home run and field goal looked jagged. Reworked after Scoreboard's sb_fx.cpp: a dark flat stage, fireworks (sparks with
trails, gravity, fading, three speeds), shock-wave rings, white flashes, words that blink between white and the team colour, and for
the home run a night ballpark (lights, crowd, wall, batter, ball with a fading trail), for the three a court with the ball into the
hoop, for the field goal a side view with the ball arcing to big yellow posts (flash between them, then fireworks). Touchdown, goal,
win use the same pieces. Sparks never draw over the logo and words (a protected box), and rubbing out never touches it. Pictures:
design/mini_fx_big.png. The banners (rays, stripes, tape, bricks) are unchanged.

## Feedback on 0.13 (Joe, Oct 11; to fix in the next update, nothing coded yet)
Joe's photos of the real screen (Try the animations: field goal, three, win):
- The stage that should be black / dark navy shows as bright blue, and the plates behind the words as a darker blue; the court looks
  washed-out blue-grey. Check how colours are given to fillScreen / fillRect (uint16_t raw vs rgb565_t) and the colour mode; ask Joe
  whether the red goal strobe and the touchdown look right.
- The logo did not draw (letters NYG): the 112 px logo is not stored until a game screen has asked for it. Use a stored size, or fetch
  it ahead.
- Confetti leaves ghost shapes behind (pieces not fully rubbed out, the pattern hangs round the logo). Erase with the exact colour or
  draw confetti like the sparks.
- Three: the ball does not go through the basket (it ends beside / below the net). Aim at the middle of the rim, drop through, and draw
  the net over the ball.
- Home run: make it look like the LED board's baseball diamond (sb_fx.cpp drawPark): seen from behind home plate, sky with light
  towers, crowd, wall with a yellow line and foul poles, striped grass, the dirt diamond with bases and mound, chalk foul lines, the
  batter at the plate, the ball flying up over the wall.

Another from Joe (Oct 11): the win chance bar on a live football game screen: the "45%" / "55%" labels at the ends and "WIN CHANCE" in the
middle show as blocks taller than the 14 px bar (the text's background box is taller than the bar), so the bar looks lumpy. Fix: make the
bar tall enough for the text (about 22 px) or draw the labels without a background box, so the bar is one even strip.
Joe will run 0.13 through Sunday's NFL games and send more; nothing is being coded until he says.

From Joe watching a college game (Oct 11): when the other team punted, the animation showed "ARK" letters instead of a logo and used the
favourite team's colours (a mix-up). Possible causes to check: (1) both teams in a game are favourites, so the same play fires twice,
once from each side (PUNT-ASTIC for one, NO PUNT INTENDED for the other); fix by playing one animation per play (key: game id + play id),
(2) the logo only shows once its 112 px file is stored (letters until then), (3) wording: for "they punt" the banner shows OUR logo with
"<them> HAVE TO PUNT". Ask Joe which words were on screen and whether the other team is also a favourite.
Memory: the biggest free piece fell from 63 KB (0.10) to 57 KB at rest (dips to 37 KB while fetching): give some back (trim static
buffers) in the next update.

More from Joe (Oct 11): (1) the blacks in the animations are not all the same black: the stage colours I used (4,6,12), (6,6,10),
(8,14,40) are slightly blue next to the pure black of the word plates, so the plates show as squares. Fix: one black for the stage and
the plates (or draw plates in the stage colour). (2) The punt banner showed the other team's letters ("ARK") in the favourite's colours:
Arkansas is not a favourite, so its logo is not stored; fetch the opposing team's logo (76 px) once its game goes live, and use letters
only until it arrives. (3) Memory: give back some without losing anything (log ring 60 x 120 B = 7 KB, the spark / queue / about
buffers, the games' statics; target the biggest free block back to 63 KB).

More from Joe (Oct 11): (1) name entry when a game's high score is set (top 5 per game, last name prefilled, clear-scores button); picture first.
(2) Penalty Kick is too easy: the keeper dives away from the shot side 2 times in 3 even on a centre shot (mn_g_shoot.cpp ~line 182),
so shooting down the middle nearly always scores. Fix: when the keeper has not "guessed", pick a side at random (and sometimes stay
central), scale the guess chance up with level, and make the keeper reach wider. (3) Rename "Cheer Simon" to "Simon" (title bar,
game list, GAME_SIMON name). (4) Basket Toss icon: the flyer pokes out above the tile; lower or shrink it.

(5) Heads and tails: make the coin look like a real quarter (silver rim with reeding, an eagle-style head side and a tails side, shine, spinning edge-on as it flips); picture first.

(6) Logo Match: bigger boards after level 8 (8, 10, 12 pairs; 24 cards = 6 x 4, ~70 px tiles); mix plain symbols in with the logos so the
board fills faster; picture of the 24-card board first.
(7) Alerts while playing a game: today animations only play on HOME/TEAM/DETAIL/TICKER/ABOUT (mini.ino ~line 479), and netWatch is not
called in the games, so live games are polled slowly and events go stale (45 s). Idea: a small banner slides in at the top of the game
screen for ~4 s (team logo, short words like "TOUCHDOWN - Team"), game keeps running, tap to dismiss; the big animation is skipped
inside games; watch live favourites while a game is open (pause-safe: games with a timer should not be punished). Picture first.

(8) Logo Match is too hard at level 9 (daughter): levels 8 and up all use the last row, 6 pairs in 11 moves and 35 s (best possible is 6
moves), so it is nearly impossible. Fix: ease the limits (e.g. 14-16 moves, 50-60 s at 6 pairs), spread the squeeze over more levels,
and then add the bigger boards as a ramp (more pairs with matching moves/seconds). Possible "skipped level": no bug found in the
level counter (it adds 1 per win); level 1 -> 2 -> 3 change the board shape (6, 8, 12 cards) and level 4 adds a move limit, which can
look like a jump. Ask which levels she saw; show a "Level N" banner before each deal to make it obvious.

## Older notes (Oct 10 night; the first two items and the Wi-Fi arrows are done in 0.10)
- Upcoming-game screen: two big logos and the start time, one small line under
  them; the stats (starters, leaders, records, stadium) move behind a tap
  (a small DETAILS button, or tap the logos). The same on the home tile of an
  upcoming game: logos and time, not a crowded list.
- Bye week / no game: a home tile with the logo and "BYE WEEK" (football) or
  "NO GAME TODAY" (other sports), and a full screen with the big logo, the team
  name and its record when tapped. The mini keeps each favourite's logo path,
  colour and name (in the board's settings) from the last time a game was in
  the feed, so a team with no game still has its logo.
- Battery: after the overnight charge, the flat top reading is "full"; correct
  the reading with it. The charger lifts the reading (about 80 mV) and running
  the screen and Wi-Fi sags it (about 50 mV); work both out from the plug-in /
  unplug logs. Never let the percent rise while on battery; smooth it. There is
  no ready-made library for load compensation (only fuel-gauge chips, which
  this board lacks); the ESP32 reader is also known to be a few percent off per
  chip, so a multimeter check of the battery plug is worth doing once.
- Updates: the mini already checks at 4 AM and 3 minutes after start, plugged
  in or not, when the Wi-Fi is up.
- Wi-Fi paging arrows: bigger and higher.
