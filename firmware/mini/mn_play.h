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
void playGame(int team, const Game& g, bool known, int autoSecs);
void playAutoTag(int autoSecs);       // just the "AUTO next game in 14s" tag
uint32_t playGameSig(const Game& g, bool known);

// "Q3 4:12", "2nd 8:31", "Top 5th": one line for a live game
String playStatus(const Game& g);
