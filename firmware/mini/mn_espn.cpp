#include "mn_espn.h"
#include <strings.h>

static const char* const PATHS[L_COUNT] = {"football/nfl", "football/college-football", "baseball/mlb", "hockey/nhl",
                                           "basketball/nba"};

String espnUrl(League l, int group, const char* day) {
  String u = String("https://site.api.espn.com/apis/site/v2/sports/") + PATHS[l] + "/scoreboard";
  String q;
  if (l == L_CFB) q += String("groups=") + (group ? group : 80) + "&limit=300";
  if (day) { if (q.length()) q += "&"; q += String("dates=") + day; }
  if (q.length()) u += "?" + q;
  return u;
}

bool Game::same(const Game& o) const {
  return state == o.state && league == o.league && mineHome == o.mineHome && start == o.start && period == o.period &&
         !strcmp(clock, o.clock) && !strcmp(detail, o.detail) && !strcmp(net, o.net) &&
         !strcmp(away.abbr, o.away.abbr) && !strcmp(home.abbr, o.home.abbr) && away.score == o.away.score &&
         home.score == o.home.score && away.hasScore == o.away.hasScore && !strcmp(away.rec, o.away.rec) &&
         !strcmp(home.rec, o.home.rec) && !strcmp(away.logo, o.away.logo) && !strcmp(home.logo, o.home.logo);
}

template <size_t N> static void scopy(char (&dst)[N], const char* src) {
  strncpy(dst, src ? src : "", N - 1);
  dst[N - 1] = 0;
}

bool espnLoad(ByteSource& src, JsonDocument& doc) {
  // (written out in full: ArduinoJson only builds a filter's nested parts when
  // they're assigned through the document itself)
  JsonDocument filter;
  JsonObject ev = filter["events"][0].to<JsonObject>();
  ev["date"] = true;
  ev["status"]["period"] = true;
  ev["status"]["displayClock"] = true;
  ev["status"]["type"]["state"] = true;
  ev["status"]["type"]["name"] = true;
  ev["status"]["type"]["shortDetail"] = true;
  ev["competitions"][0]["broadcasts"][0]["names"][0] = true;
  ev["competitions"][0]["competitors"][0]["homeAway"] = true;
  ev["competitions"][0]["competitors"][0]["score"] = true;
  ev["competitions"][0]["competitors"][0]["records"][0]["summary"] = true;
  ev["competitions"][0]["competitors"][0]["team"]["id"] = true;
  ev["competitions"][0]["competitors"][0]["team"]["abbreviation"] = true;
  ev["competitions"][0]["competitors"][0]["team"]["displayName"] = true;
  ev["competitions"][0]["competitors"][0]["team"]["shortDisplayName"] = true;
  ev["competitions"][0]["competitors"][0]["team"]["logo"] = true;
  ev["competitions"][0]["competitors"][0]["team"]["logoDark"] = true;
  DeserializationError e = deserializeJson(doc, src, DeserializationOption::Filter(filter),
                                           DeserializationOption::NestingLimit(20));
  return !e;
}

time_t espnParseTime(const char* iso) {
  int y, mo, d, h = 0, mi = 0;
  if (!iso || sscanf(iso, "%4d-%2d-%2dT%2d:%2d", &y, &mo, &d, &h, &mi) < 3) return 0;
  // days since 1970-01-01 (civil calendar, no time zone: the feed is UTC)
  y -= mo <= 2;
  long era = (y >= 0 ? y : y - 399) / 400;
  unsigned yoe = y - era * 400;
  unsigned doy = (153 * (mo + (mo > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  long days = era * 146097 + (long)doe - 719468;
  return (time_t)(days * 86400L + h * 3600L + mi * 60L);
}

// ESPN's own logo files for a team, the dark-background version when it has one
static void logoPath(char (&dst)[64], JsonObjectConst team) {
  const char* u = team["logoDark"] | (const char*)(team["logo"] | "");
  const char* p = strstr(u, "/i/teamlogos/");
  scopy(dst, p ? p : "");
}

static void fillSide(TeamSide& s, JsonObjectConst c) {
  JsonObjectConst t = c["team"];
  scopy(s.abbr, t["abbreviation"] | "");
  scopy(s.name, t["shortDisplayName"] | (const char*)(t["displayName"] | ""));
  scopy(s.rec, c["records"][0]["summary"] | "");
  logoPath(s.logo, t);
  const char* sc = c["score"] | "";
  s.hasScore = sc[0] != 0;
  s.score = atoi(sc);
}

static bool isTeam(JsonObjectConst c, int team) {
  JsonObjectConst t = c["team"];
  const char* ab = t["abbreviation"] | "";
  if (!strcasecmp(ab, TEAMS[team].abbr)) return true;
  // ESPN changed an abbreviation? the name is the fallback
  const char* dn = t["displayName"] | "";
  size_t n = strlen(TEAMS[team].name);
  return !strncasecmp(dn, TEAMS[team].name, n) && (dn[n] == 0 || dn[n] == ' ');
}

bool espnFind(const JsonDocument& doc, int team, time_t now, Game& out) {
  out = Game();
  long bestRank = -1000000;
  bool found = false;
  for (JsonObjectConst e : doc["events"].as<JsonArrayConst>()) {
    JsonArrayConst cps = e["competitions"][0]["competitors"].as<JsonArrayConst>();
    if (cps.size() != 2) continue;
    int mine = -1;
    for (int i = 0; i < 2; i++) if (isTeam(cps[i], team)) mine = i;
    if (mine < 0) continue;
    Game g;
    g.league = TEAMS[team].league;
    const char* st = e["status"]["type"]["state"] | "";
    g.state = !strcmp(st, "in") ? GS_LIVE : !strcmp(st, "post") ? GS_POST : GS_PRE;
    g.start = espnParseTime(e["date"] | "");
    g.period = e["status"]["period"] | 0;
    scopy(g.clock, e["status"]["displayClock"] | "");
    scopy(g.detail, e["status"]["type"]["shortDetail"] | "");
    scopy(g.net, e["competitions"][0]["broadcasts"][0]["names"][0] | "");
    for (int i = 0; i < 2; i++) {
      bool home = !strcmp(cps[i]["homeAway"] | "", "home");
      fillSide(home ? g.home : g.away, cps[i]);
      if (i == mine) g.mineHome = home;
    }
    // live beats everything; then a final from the last day and a half;
    // then the next one to start; then an older final
    long hours = (long)((g.start - now) / 3600);
    long rank = g.state == GS_LIVE ? 100000
              : g.state == GS_POST ? (hours > -36 ? 50000 + hours : 1000 + hours)
                                   : 20000 - (hours > 0 ? hours : 0);
    if (rank > bestRank) { bestRank = rank; out = g; found = true; }
  }
  return found;
}
