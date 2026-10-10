// Score animations: what just happened in a favourite's game, worked out by comparing two looks at it (the
// same rules as the big LED board's sb_events.cpp), and the full-screen pictures that play for it. Good news for
// your team is bright (team colours, rays); bad news is black and yellow caution tape.
#pragma once
#include <Arduino.h>
#include "mn_game.h"

enum FxKind : uint8_t {
  FX_NONE = 0,
  FX_TOUCHDOWN, FX_FIELDGOAL, FX_NOGOOD, FX_GOAL, FX_RUN, FX_HOMERUN, FX_THREE, FX_WIN, FX_KICKOFF, FX_QUARTER,
  FX_FLAG, FX_FIRSTDOWN, FX_PICKED, FX_FUMBLE, FX_SACK, FX_STOPPED, FX_STONEWALL, FX_PUNT, FX_WENTFORIT, FX_NOPUNT,
  FX_TURNOVER, FX_THEIRSCORE
};

struct FxSpec {
  FxKind kind = FX_NONE;
  bool tape = false;         // bad news: caution tape
  bool theirs = false;       // the other team's moment (their logo, small)
  uint8_t n = 0;             // runs, ...
  League league = L_NFL;
  TeamSide mine, them;
  char word[20] = "";        // the big word: "TOUCHDOWN"
  char sub[48] = "";         // the line in the black pill under it
};

// Fills f and returns its kind, or FX_NONE. prev is an earlier look at the same game (not too old).
FxKind detectEvent(const Game& prev, const Game& now, FxSpec& f);
// Your team's game just ended and it won: the win card
bool detectWin(const Game& prev, const Game& now, FxSpec& f);

// Playing
void fxStart(const FxSpec& f);
bool fxStep();               // call every pass of the loop: draws the next frame; false once it is over
void fxStop();
bool fxActive();
uint32_t fxLength(FxKind k); // milliseconds
