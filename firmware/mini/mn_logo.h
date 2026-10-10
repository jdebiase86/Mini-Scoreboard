// Team logos: ESPN's own pictures, shrunk by ESPN to the size wanted, kept in
// the board's little filesystem (a few KB each) and drawn from there.
#pragma once
#include <Arduino.h>
#include "mn_game.h"

// Draws the team's logo centred on (cx, cy) over a plain background colour.
// size: 76 (home tiles) or 112 (game screens). true = drawn. false = it's not
// on the board yet; it has been asked for, and the caller shows the team's
// letters until logoVersion() changes.
bool logoDraw(const TeamSide& t, int cx, int cy, int size, uint16_t bg);
uint32_t logoVersion();

#ifndef MN_HOST
void logoBegin();        // mounts the filesystem
bool logoFetchOne();     // network task: downloads one wanted logo. true = it tried one
#endif
