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
  char id[8] = "";       // ESPN's team number (possession is given by it)
  uint32_t color = 0;    // ESPN's team colour (0xRRGGBB, 0 = unknown)
  char abbr[8] = "";
  char name[22] = "";    // short name: "Giants"
  char rec[10] = "";     // season record: "3-1"
  char logo[64] = "";    // ESPN logo path (dark-background version)
  int16_t score = 0;
  bool hasScore = false;
};

enum GState : uint8_t { GS_NONE = 0, GS_PRE, GS_LIVE, GS_POST };

// What's going on in a football game right now (ESPN's "situation")
struct FbSit {
  bool has = false;
  int8_t down = 0;            // 1-4
  int16_t distance = 0;       // yards to go
  int16_t yardLine = -1;      // 0 = the home team's goal line, 100 = the away team's
  uint8_t possession = 0;     // 0 nobody known, 1 away, 2 home
  bool redzone = false;
  int8_t toAway = -1, toHome = -1;   // timeouts left
  int8_t winHome = -1;        // home team's chance to win, percent (-1 = unknown)
  int16_t driveStart = -1;    // where this drive started, same scale as yardLine
  char dd[16] = "";           // "4th & 10"
  char at[16] = "";           // "BYU 7"
  char playId[20] = "";       // changes with every play
  char play[120] = "";        // the last play, in words
  bool same(const FbSit& o) const {
    return has == o.has && down == o.down && distance == o.distance && yardLine == o.yardLine &&
           possession == o.possession && redzone == o.redzone && toAway == o.toAway && toHome == o.toHome &&
           winHome == o.winHome && driveStart == o.driveStart && !strcmp(dd, o.dd) && !strcmp(at, o.at) &&
           !strcmp(playId, o.playId);
  }
};

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
  FbSit fb;
  const TeamSide& mine() const { return mineHome ? home : away; }
  const TeamSide& them() const { return mineHome ? away : home; }
  bool same(const Game& o) const;   // nothing on screen would change
};
