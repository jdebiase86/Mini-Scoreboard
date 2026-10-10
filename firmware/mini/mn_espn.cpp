#include "mn_espn.h"
#include "mn_jscan.h"
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

// ESPN's own page for one team (20-36 KB): its id, colour, name and logo files. Works with the short code.
String espnTeamUrl(League l, const char* abbr) {
  return String("https://site.api.espn.com/apis/site/v2/sports/") + PATHS[l] + "/teams/" + abbr;
}

String espnSummaryUrl(League l, const char* id) {
  return String("https://site.api.espn.com/apis/site/v2/sports/") + PATHS[l] + "/summary?event=" + id;
}

bool Game::same(const Game& o) const {
  return state == o.state && league == o.league && mineHome == o.mineHome && start == o.start && period == o.period &&
         !strcmp(clock, o.clock) && !strcmp(detail, o.detail) && !strcmp(net, o.net) &&
         !strcmp(away.abbr, o.away.abbr) && !strcmp(home.abbr, o.home.abbr) && away.score == o.away.score &&
         home.score == o.home.score && away.hasScore == o.away.hasScore && !strcmp(away.rec, o.away.rec) &&
         !strcmp(home.rec, o.home.rec) && !strcmp(away.logo, o.away.logo) && !strcmp(home.logo, o.home.logo) &&
         away.color == o.away.color && home.color == o.home.color && fb.same(o.fb) && det.has == o.det.has &&
         det.sig == o.det.sig;
}

template <size_t N> static void scopy(char (&dst)[N], const char* src) {
  foldUtf8(src ? src : "", dst, N);   // the screen fonts only have plain letters: "Jokic", not "Jokić"
}

bool espnLoadTeam(ByteSource& src, TeamSide& out) {
  JsonDocument filter;
  filter["team"]["id"] = true;
  filter["team"]["abbreviation"] = true;
  filter["team"]["displayName"] = true;
  filter["team"]["shortDisplayName"] = true;
  filter["team"]["color"] = true;
  filter["team"]["logos"][0]["href"] = true;
  JsonDocument doc;
  DeserializationError e = deserializeJson(doc, src, DeserializationOption::Filter(filter), DeserializationOption::NestingLimit(12));
  if (e) return false;
  JsonObjectConst t = doc["team"];
  out = TeamSide();
  scopy(out.abbr, t["abbreviation"] | "");
  scopy(out.name, t["shortDisplayName"] | (const char*)(t["displayName"] | ""));
  scopy(out.id, t["id"] | "");
  out.color = (uint32_t)strtoul(t["color"] | "0", nullptr, 16);
  // the dark-background logo when there is one, else the first
  const char* pick = nullptr;
  for (JsonObjectConst l : t["logos"].as<JsonArrayConst>()) {
    const char* h = l["href"] | "";
    if (!strstr(h, "/i/teamlogos/")) continue;
    if (strstr(h, "/500-dark/") && !strstr(h, "/scoreboard/")) { pick = h; break; }
    if (!pick) pick = h;
  }
  const char* p = pick ? strstr(pick, "/i/teamlogos/") : nullptr;
  scopy(out.logo, p ? p : "");
  return out.logo[0] != 0;
}

bool espnLoad(ByteSource& src, JsonDocument& doc, bool rich) {
  // (written out in full: ArduinoJson only builds a filter's nested parts when
  // they're assigned through the document itself)
  JsonDocument filter;
  JsonObject ev = filter["events"][0].to<JsonObject>();
  ev["id"] = true;
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
  ev["competitions"][0]["competitors"][0]["team"]["color"] = true;
  JsonObject sit = ev["competitions"][0]["situation"].to<JsonObject>();
  for (const char* k : {"down", "distance", "yardLine", "downDistanceText", "shortDownDistanceText", "possessionText",
                        "isRedZone", "possession", "homeTimeouts", "awayTimeouts"})
    sit[k] = true;
  sit["lastPlay"]["id"] = true;
  sit["lastPlay"]["text"] = true;
  sit["lastPlay"]["probability"]["homeWinPercentage"] = true;
  sit["lastPlay"]["drive"]["start"]["text"] = true;
  if (rich) {
    ev["competitions"][0]["venue"]["fullName"] = true;
    ev["competitions"][0]["competitors"][0]["records"][0]["name"] = true;
    ev["competitions"][0]["competitors"][0]["linescores"][0]["value"] = true;
    ev["competitions"][0]["competitors"][0]["leaders"][0]["name"] = true;
    ev["competitions"][0]["competitors"][0]["leaders"][0]["leaders"][0]["displayValue"] = true;
    ev["competitions"][0]["competitors"][0]["leaders"][0]["leaders"][0]["athlete"]["shortName"] = true;
    ev["competitions"][0]["competitors"][0]["probables"][0]["athlete"]["shortName"] = true;
  }
  DeserializationError e = deserializeJson(doc, src, DeserializationOption::Filter(filter),
                                           DeserializationOption::NestingLimit(20));
  if (!e) doc["rich"] = rich;
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
  scopy(s.id, t["id"] | "");
  s.color = (uint32_t)strtoul(t["color"] | "0", nullptr, 16);
  const char* sc = c["score"] | "";
  s.hasScore = sc[0] != 0;
  s.score = atoi(sc);
}

// "BYU 7" or "50" -> the yard line on ESPN's scale (0 = home goal line)
static int16_t spot(const char* text, const Game& g) {
  if (!text || !*text) return -1;
  if (!strcmp(text, "50")) return 50;
  const char* sp = strrchr(text, ' ');
  if (!sp) return -1;
  int yd = atoi(sp + 1);
  size_t n = sp - text;
  if (n == strlen(g.home.abbr) && !strncasecmp(text, g.home.abbr, n)) return yd;
  if (n == strlen(g.away.abbr) && !strncasecmp(text, g.away.abbr, n)) return 100 - yd;
  return -1;
}

static void fillFootball(Game& g, JsonObjectConst s) {
  FbSit& f = g.fb;
  if (s.isNull()) return;
  f.has = true;
  f.down = s["down"] | 0;
  f.distance = s["distance"] | 0;
  f.yardLine = s["yardLine"] | -1;
  f.redzone = s["isRedZone"] | false;
  f.toHome = s["homeTimeouts"] | -1;
  f.toAway = s["awayTimeouts"] | -1;
  scopy(f.dd, s["shortDownDistanceText"] | "");
  scopy(f.at, s["possessionText"] | "");
  const char* p = s["possession"] | "";
  if (*p && !strcmp(p, g.home.id)) f.possession = 2;
  else if (*p && !strcmp(p, g.away.id)) f.possession = 1;
  JsonObjectConst lp = s["lastPlay"];
  scopy(f.playId, lp["id"] | "");
  JsonVariantConst wp = lp["probability"]["homeWinPercentage"];
  if (!wp.isNull()) f.winHome = (int8_t)lround(wp.as<double>() * 100);
  f.driveStart = spot(lp["drive"]["start"]["text"] | "", g);
  // the play in words, without its leading "(04:37) " clock
  const char* t = lp["text"] | "";
  if (*t == '(') { const char* c = strchr(t, ')'); if (c) { t = c + 1; while (*t == ' ') t++; } }
  scopy(f.play, t);
}

// the short word shown for each of ESPN's leader categories (0 = leave it out)
static const char* leaderWord(const char* name) {
  static const struct { const char* k; const char* w; } M[] = {
      {"passingLeader", "PASS"}, {"rushingLeader", "RUSH"}, {"receivingLeader", "REC"}, {"points", "PTS"},
      {"rebounds", "REB"}, {"assists", "AST"}, {"goals", "G"}, {"avg", "AVG"}, {"homeRuns", "HR"}, {"RBIs", "RBI"}};
  for (auto& m : M) if (!strcmp(name, m.k)) return m.w;
  return nullptr;
}

static uint32_t hashStr(uint32_t h, const char* s) {
  for (; *s; s++) { h ^= (uint8_t)*s; h *= 16777619u; }
  return (h ^ 0xff) * 16777619u;
}

static void fillSideDetail(SideDetail& d, JsonObjectConst c, uint32_t& sig) {
  for (JsonObjectConst r : c["records"].as<JsonArrayConst>()) {
    const char* n = r["name"] | "";
    const char* sm = r["summary"] | "";
    if (!strcasecmp(n, "Home")) scopy(d.homeRec, sm);
    else if (!strcasecmp(n, "Road") || !strcasecmp(n, "Away")) scopy(d.roadRec, sm);
  }
  JsonArrayConst ls = c["linescores"].as<JsonArrayConst>();
  for (JsonVariantConst v : ls) {
    if (d.nLines >= sizeof(d.lines)) break;
    d.lines[d.nLines++] = (int8_t)(int)(v["value"] | 0.0);
  }
  int n = 0;
  for (JsonObjectConst l : c["leaders"].as<JsonArrayConst>()) {
    const char* w = leaderWord(l["name"] | "");
    JsonObjectConst top = l["leaders"][0];
    if (!w || top.isNull() || n >= 3) continue;
    scopy(d.lead[n].cat, w);
    scopy(d.lead[n].name, top["athlete"]["shortName"] | "");
    scopy(d.lead[n].val, top["displayValue"] | "");
    n++;
  }
  scopy(d.starter, c["probables"][0]["athlete"]["shortName"] | "");
  for (int i = 0; i < d.nLines; i++) sig = (sig ^ (uint8_t)d.lines[i]) * 16777619u;
  sig = hashStr(sig, d.homeRec); sig = hashStr(sig, d.roadRec); sig = hashStr(sig, d.starter);
  for (auto& l : d.lead) { sig = hashStr(sig, l.name); sig = hashStr(sig, l.val); }
}

static void fillDetails(Game& g, JsonObjectConst comp, JsonArrayConst cps) {
  Details& d = g.det;
  d.has = true;
  scopy(d.venue, comp["venue"]["fullName"] | "");
  uint32_t sig = hashStr(2166136261u, d.venue);
  for (int i = 0; i < 2; i++) {
    bool home = !strcmp(cps[i]["homeAway"] | "", "home");
    fillSideDetail(home ? d.home : d.away, cps[i], sig);
  }
  d.sig = sig;
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
    scopy(g.id, e["id"] | "");
    g.period = e["status"]["period"] | 0;
    scopy(g.clock, e["status"]["displayClock"] | "");
    scopy(g.detail, e["status"]["type"]["shortDetail"] | "");
    scopy(g.net, e["competitions"][0]["broadcasts"][0]["names"][0] | "");
    for (int i = 0; i < 2; i++) {
      bool home = !strcmp(cps[i]["homeAway"] | "", "home");
      fillSide(home ? g.home : g.away, cps[i]);
      if (i == mine) g.mineHome = home;
    }
    if (g.state == GS_LIVE && (g.league == L_NFL || g.league == L_CFB)) fillFootball(g, e["competitions"][0]["situation"]);
    else if (g.state == GS_LIVE && g.league == L_MLB) {   // baseball: only the last play (home runs)
      JsonObjectConst lp = e["competitions"][0]["situation"]["lastPlay"];
      scopy(g.fb.playId, lp["id"] | "");
      const char* t = lp["text"] | "";
      scopy(g.fb.play, t);
    }
    if (doc["rich"] | false) fillDetails(g, e["competitions"][0], cps);
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
