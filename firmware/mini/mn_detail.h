// Details cards: tap a game screen for more.
//   teams: records, score by period, leaders (or the starters), stadium, TV
//   play (football): the last play in full
//   situation (football): down and distance, where the ball is, win chance
#pragma once
#include <Arduino.h>
#include "mn_game.h"

enum DetailKind { DK_TEAMS = 0, DK_PLAY, DK_SIT };

void detailOpen(DetailKind k);                           // remembers when it was opened
void detailDraw(DetailKind k, int team, const Game& g, bool known);
bool detailTap(int x, int y);                            // true = BACK
uint32_t detailSig(const Game& g, bool known);           // changes when the card should be redrawn
bool detailExpired();                                    // untouched long enough: close it
void detailTouched();
