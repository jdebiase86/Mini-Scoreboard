// Details cards: tap a game screen for more.
//   teams: records, score by period, leaders (or the starters), stadium, TV
//   play (football): the last play in full
//   situation (football): down and distance, where the ball is, win chance
#pragma once
#include <Arduino.h>
#include "mn_game.h"
#include "mn_live.h"

enum DetailKind { DK_TEAMS = 0, DK_PLAY, DK_SIT, DK_STATS };

void detailOpen(DetailKind k);                           // remembers when it was opened
// live: what the game's own page has told us so far (team stats, leaders, last play), or nullptr
void detailDraw(DetailKind k, int team, const Game& g, bool known, const LiveInfo* live);
enum DetailTap { DT_NONE = 0, DT_BACK, DT_STATS, DT_TEAMS };
DetailTap detailTap(int x, int y, DetailKind k);         // BACK, or a button that goes to another card
uint32_t detailSig(const Game& g, bool known, const LiveInfo* live);   // changes when the card should be redrawn
bool detailHasStats(const Game& g);                      // this sport has team stats to show
bool detailExpired();                                    // untouched long enough: close it
void detailTouched();

// The score by period as a small table (the game screen's bottom area and the teams card)
void detailLineScore(const Game& g, int y0);
