// One game as the mini shows it (from ESPN's scoreboard feed), seen from one
// of the favourite teams.
#pragma once
#include <Arduino.h>
#include <time.h>
#include "mn_teams.h"

// The time of day. On a computer (the picture-drawing programs) the clock is
// a pretend one, so the pictures come out the same every time.
#ifdef MN_HOST
extern time_t mnNow;
static inline time_t mnTime() { return mnNow; }
#else
static inline time_t mnTime() { return time(nullptr); }
#endif

struct TeamSide {
  char abbr[8] = "";
  char name[22] = "";    // short name: "Giants"
  char rec[10] = "";     // season record: "3-1"
  char logo[64] = "";    // ESPN logo path (dark-background version)
  int16_t score = 0;
  bool hasScore = false;
};

enum GState : uint8_t { GS_NONE = 0, GS_PRE, GS_LIVE, GS_POST };

struct Game {
  GState state = GS_NONE;
  League league = L_NFL;
  bool mineHome = false;     // the favourite is the home side
  time_t start = 0;
  uint8_t period = 0;
  char clock[8] = "";        // "4:12"
  char detail[28] = "";      // ESPN's short status: "Top 5th", "End of 1st", "Final/OT"
  char net[14] = "";         // TV network
  TeamSide away, home;
  const TeamSide& mine() const { return mineHome ? home : away; }
  const TeamSide& them() const { return mineHome ? away : home; }
  bool same(const Game& o) const;   // nothing on screen would change
};
