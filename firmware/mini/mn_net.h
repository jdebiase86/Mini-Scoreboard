// The scores: a background task that keeps each favourite's game up to date
// from ESPN, and fetches the logos the screens ask for.
#pragma once
#include <Arduino.h>
#include "mn_game.h"

void netStart();                        // once Wi-Fi is up (and the clock set)
void netPicksChanged();                 // the favourites were edited: start over
uint32_t netVersion();                  // changes whenever a game or a logo changed
// Favourite number `pick` (index into settings.picks). false = nothing heard
// yet; otherwise out.state == GS_NONE means "no game found".
bool netGame(int pick, Game& out);
