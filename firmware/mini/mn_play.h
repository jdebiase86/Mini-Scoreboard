// Screens that show a game: the home tile of a favourite and the game screen.
#pragma once
#include <Arduino.h>
#include "mn_game.h"

// Draws one favourite's home tile (the rectangle given, 152 x 137) with the
// game it's in. known = false: nothing heard from ESPN yet.
void playTile(int x0, int y0, int x1, int y1, int team, const Game& g, bool known);
// Changes whenever the tile would look different (so it's only redrawn then)
uint32_t playTileSig(int team, const Game& g, bool known);

// The full game screen for a favourite. autoSecs >= 0: AUTO is rotating and
// that many seconds are left on this game.
//   PG_OPEN: the screen was just opened (what's on already isn't "new")
//   PG_FULL: everything again;  PG_DYN: only what moves (scores, clock, field)
enum { PG_OPEN = 0, PG_FULL, PG_DYN };
void playGame(int team, const Game& g, bool known, int autoSecs, int mode);
// The last-play card (football): it shows for a few seconds after each play
bool playCardVisible();
void playCardHide();
void playCardTick();                  // call often: takes the card away when its time is up
void playAutoLabel(const char* label);   // "NFL": the ticker is on one league; "" = all my teams
void playAutoTag(int autoSecs);       // just the "AUTO next game in 14s" tag
uint32_t playGameSig(const Game& g, bool known);   // everything on the screen
uint32_t playGameShape(const Game& g, bool known); // what only changes when the screen needs redrawing in full

// "Q3 4:12", "2nd 8:31", "Top 5th": one line for a live game
String playStatus(const Game& g);
// The two teams' colours as the game screen uses them
void playTeamColours(const Game& g, uint16_t& away, uint16_t& home);

// What a tap on the game screen is on (when the game's details can be shown)
enum PlayHit { PH_NONE = 0, PH_CARD, PH_TEAMS, PH_SIT, PH_STATS, PH_LASTPLAY, PH_AUTOTAG };
PlayHit playGameHit(int x, int y, const Game& g, bool known);
// Brings the last-play card back for `ms` (after its details were looked at)
void playCardHold(const Game& g, uint32_t ms);
