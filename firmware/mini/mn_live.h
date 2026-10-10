// Live numbers for one game from ESPN's per-game page: the team stats, the
// leaders so far, and the last play. The page is big (up to 1 MB), so it is
// read as it streams by (mn_jscan) and only what's wanted is kept.
#pragma once
#include <Arduino.h>
#include "mn_game.h"

struct LiveStat {
  char label[14] = "";    // "Total yards"
  char a[10] = "";        // the away team's number
  char h[10] = "";
  uint8_t shareA = 50;    // the away team's share of the two numbers, for the bar (0-100)
};

struct LiveInfo {
  bool has = false;
  uint32_t sig = 0;       // changes when anything below does
  uint8_t nStats = 0;
  LiveStat st[8];
  Leader lead[2][3];      // [0] away, [1] home
  bool hasLead = false;
  char play[200] = "";    // the last play, in words
  char playWhen[20] = ""; // "Q3 4:12", "3rd 12:40"
  bool hasPlay = false;
  char id[12] = "";       // the game it belongs to
};

// reads the game page; false = it didn't read
struct ByteSource;
bool liveParse(ByteSource& src, const Game& g, LiveInfo& out);
