// The scores: a background task that keeps each favourite's game up to date
// from ESPN, and fetches the logos the screens ask for.
#pragma once
#include <Arduino.h>
#include "mn_game.h"
#include "mn_live.h"

void netStart();                        // once Wi-Fi is up (and the clock set)
void netPicksChanged();                 // the favourites were edited: start over
uint32_t netVersion();                  // changes whenever a game or a logo changed
// Favourite number `pick` (index into settings.picks). false = nothing heard
// yet; otherwise out.state == GS_NONE means "no game found".
bool netGame(int pick, Game& out);
// A details card for favourite `pick` is open: also read the extras (leaders,
// score by period, starters, stadium) for a while. Call it when the card opens.
void netWantDetails(int pick);
bool mnWifiUp();                        // joined to a network right now
// The live page of favourite `pick` (team stats, leaders so far, last play):
// call netWantLive while a card that needs it is open; netLive gives what has
// arrived (false = nothing yet).
void netWantLive(int pick);
bool netLive(int pick, LiveInfo& out);
uint32_t netAgeSecs(int pick);        // seconds since favourite `pick` last got a good answer (65535 = never)
