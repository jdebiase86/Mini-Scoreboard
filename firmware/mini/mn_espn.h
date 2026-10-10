// ESPN's scoreboard feed: download-and-filter, and picking a favourite's game.
#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include "mn_game.h"

// Anything the JSON can be read from (the network on the board, a file on a
// computer). ArduinoJson wants read() and readBytes().
struct ByteSource {
  virtual int read() = 0;
  virtual size_t readBytes(char* buf, size_t n) = 0;
  virtual ~ByteSource() {}
};

// The feed's address. group: college football conference (0 = all FBS).
// day: "YYYYMMDD" for another day (MLB, NHL, NBA), or nullptr for today / this week.
String espnUrl(League l, int group, const char* day);

// A team's own page (logo, colour, name): for a favourite with no game to take them from
String espnTeamUrl(League l, const char* abbr);
bool espnLoadTeam(ByteSource& src, TeamSide& out);

// The per-game page (team stats, leaders so far, plays): big, so read it with mn_live
String espnSummaryUrl(League l, const char* id);

// Reads a feed into doc, keeping only what the mini needs (about 1/20th).
// rich: also keep the extras for the details cards (leaders, score by
// period, starters, home / road records, stadium).
bool espnLoad(ByteSource& src, JsonDocument& doc, bool rich = false);

// The game to show for a favourite (TEAMS index): live first, else a recent
// final, else the next one to start. false = none in this feed.
bool espnFind(const JsonDocument& doc, int team, time_t now, Game& out);

time_t espnParseTime(const char* iso);   // "2026-10-11T13:30Z"
