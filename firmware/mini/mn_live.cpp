#include "mn_live.h"
#include "mn_jscan.h"
#include <strings.h>
#include <stdlib.h>

namespace {
struct StatDef { const char* key; const char* label; };
const StatDef FB_STATS[] = {{"totalYards", "Total yards"}, {"firstDowns", "1st downs"}, {"thirdDownEff", "3rd down"},
                            {"turnovers", "Turnovers"}, {"possessionTime", "Possession"}, {"totalPenaltiesYards", "Penalties"}};
const StatDef BB_STATS[] = {{"fieldGoalPct", "FG %"}, {"threePointFieldGoalPct", "3PT %"}, {"totalRebounds", "Rebounds"},
                              {"assists", "Assists"}, {"turnovers", "Turnovers"}, {"fouls", "Fouls"}, {"pointsInPaint", "In the paint"}};
const StatDef HK_STATS[] = {{"shotsTotal", "Shots"}, {"hits", "Hits"}, {"blockedShots", "Blocks"}, {"takeaways", "Takeaways"},
                          {"faceoffPercent", "Faceoff %"}, {"penaltyMinutes", "Penalty min"}};

struct Ctx {
  const Game* g;
  LiveInfo* out;
  const StatDef* defs;
  int nDefs;
  int boxSide[2];        // boxscore.teams[i] -> 0 away, 1 home
  int leadSide[2];       // leaders[i] -> 0 away, 1 home
  char statName[40];
  char ppGoals[2][6], ppOpp[2][6];
  int ppSeen;
  uint32_t sig;
  // the play being read, and the newest one so far (by wall-clock time)
  char candKey[56], candText[200], candWhen[20], candWall[24];
  bool haveCand;
  char bestWall[24];
};

void scopy(char* dst, size_t n, const char* s) { strncpy(dst, s, n - 1); dst[n - 1] = 0; }

int sideOfAbbr(const Ctx& c, const char* ab) {
  if (!strcasecmp(ab, c.g->away.abbr)) return 0;
  if (!strcasecmp(ab, c.g->home.abbr)) return 1;
  return -1;
}

float numberOf(const char* s) {   // "465", "3-9", "28:19", "55"
  float v = atof(s);
  const char* colon = strchr(s, ':');
  if (colon) v = v + atof(colon + 1) / 60.0f;
  return v;
}

// the play lists: top-level "plays[N]" (basketball, hockey, baseball) or football's
// "drives.current.plays[N]" / "drives.previous[J].plays[N]"
bool playsPath(const char* p) {
  const char* last = strrchr(p, '.');
  last = last ? last + 1 : p;
  if (strstr(p, "plays[")) {   // inside a play: only the parts we read
    const char* d = strrchr(p, ']');
    d = d && d[1] == '.' ? d + 2 : nullptr;
    if (!d) return true;       // the play itself
    return !strcmp(d, "text") || !strcmp(d, "wallclock") || !strcmp(d, "period") || !strncmp(d, "period.", 7) ||
           !strcmp(d, "clock") || !strncmp(d, "clock.", 6);
  }
  if (!strcmp(last, "plays") || !strncmp(last, "plays[", 6)) return true;
  if (!strncmp(p, "drives", 6)) {   // on the way down to a drive's plays
    if (!strcmp(p, "drives") || !strcmp(p, "drives.current") || !strcmp(p, "drives.previous")) return true;
    return !strncmp(p, "drives.previous[", 16) && !strchr(p + 16, '.');
  }
  return false;
}

bool enter(void* vc, const char* p) {
  // only the parts we read
  if (!strncmp(p, "boxscore", 8)) return p[8] == 0 || !strncmp(p, "boxscore.teams", 14);
  if (!strncmp(p, "leaders", 7)) return p[7] == 0 || p[7] == '[' || p[7] == '.';
  (void)vc;
  if (!strncmp(p, "plays", 5) && (p[5] == 0 || p[5] == '[')) return playsPath(p);
  if (!strncmp(p, "drives", 6)) return playsPath(p);
  return false;
}

void scopy(char* dst, size_t n, const char* s);

void commitPlay(Ctx& c) {
  if (!c.haveCand) return;
  c.haveCand = false;
  if (!c.candText[0]) return;
  if (c.bestWall[0] && c.candWall[0] && strcmp(c.candWall, c.bestWall) < 0) return;   // an older one
  LiveInfo& o = *c.out;
  scopy(o.play, sizeof(o.play), c.candText);
  scopy(o.playWhen, sizeof(o.playWhen), c.candWhen);
  o.hasPlay = true;
  scopy(c.bestWall, sizeof(c.bestWall), c.candWall);
}

void leaf(void* vc, const char* p, const char* v) {
  Ctx& c = *(Ctx*)vc;
  LiveInfo& o = *c.out;
  int i = -1, j = -1, k = -1;
  char tail[60];
  // which side each team entry is
  if (sscanf(p, "boxscore.teams[%d].team.abbreviation%n", &i, &k) == 1 && k > 0 && p[k] == 0 && i >= 0 && i < 2) {
    int s = sideOfAbbr(c, v);
    c.boxSide[i] = s >= 0 ? s : i;
    return;
  }
  k = -1;
  if (sscanf(p, "leaders[%d].team.abbreviation%n", &i, &k) == 1 && k > 0 && p[k] == 0 && i >= 0 && i < 2) {
    int s = sideOfAbbr(c, v);
    c.leadSide[i] = s >= 0 ? s : i;
    return;
  }
  // team stats: a stat's name comes before its number
  if (sscanf(p, "boxscore.teams[%d].statistics[%d].%59s", &i, &j, tail) == 3 && i >= 0 && i < 2) {
    if (!strcmp(tail, "name")) { scopy(c.statName, sizeof(c.statName), v); return; }
    if (strcmp(tail, "displayValue")) return;
    int side = c.boxSide[i];
    if (side < 0) side = i;
    if (!strcmp(c.statName, "powerPlayGoals")) { scopy(c.ppGoals[side], 6, v); c.ppSeen |= 1 << side; return; }
    if (!strcmp(c.statName, "powerPlayOpportunities")) { scopy(c.ppOpp[side], 6, v); c.ppSeen |= 4 << side; return; }
    for (int d = 0; d < c.nDefs; d++) {
      if (strcmp(c.statName, c.defs[d].key)) continue;
      LiveStat& s = o.st[d];
      scopy(s.label, sizeof(s.label), c.defs[d].label);
      scopy(side ? s.h : s.a, sizeof(s.h), v);
      if (d >= o.nStats) o.nStats = d + 1;
      break;
    }
    return;
  }
  // leaders so far: the top three categories for each team
  if (sscanf(p, "leaders[%d].leaders[%d].%59s", &i, &j, tail) == 3 && i >= 0 && i < 2 && j >= 0 && j < 3) {
    int side = c.leadSide[i] >= 0 ? c.leadSide[i] : i;
    Leader& l = o.lead[side][j];
    if (!strcmp(tail, "name")) {
      static const struct { const char* k; const char* w; } M[] = {
          {"passingYards", "PASS"}, {"passingLeader", "PASS"}, {"rushingYards", "RUSH"}, {"rushingLeader", "RUSH"},
          {"receivingYards", "REC"}, {"receivingLeader", "REC"}, {"points", "PTS"}, {"rebounds", "REB"},
          {"assists", "AST"}, {"goals", "G"}, {"avg", "AVG"}, {"homeRuns", "HR"}, {"RBIs", "RBI"}};
      scopy(l.cat, sizeof(l.cat), "");
      for (auto& m : M) if (!strcmp(v, m.k)) scopy(l.cat, sizeof(l.cat), m.w);
    } else if (!strcmp(tail, "leaders[0].displayValue")) {
      scopy(l.val, sizeof(l.val), v);
    } else if (!strcmp(tail, "leaders[0].athlete.shortName")) {
      scopy(l.name, sizeof(l.name), v);
      if (l.cat[0]) o.hasLead = true;
    }
    return;
  }
  // plays: keep the newest one (by its wall-clock time; the lists run oldest first)
  const char* pl = strstr(p, "plays[");
  if (pl) {
    const char* close = strchr(pl, ']');
    if (!close) return;
    size_t keyLen = close - p + 1;
    if (keyLen >= sizeof(c.candKey)) return;
    if (c.haveCand && (strlen(c.candKey) != keyLen || strncmp(c.candKey, p, keyLen))) commitPlay(c);
    if (!c.haveCand) { memcpy(c.candKey, p, keyLen); c.candKey[keyLen] = 0; c.haveCand = true; c.candText[0] = c.candWhen[0] = c.candWall[0] = 0; }
    const char* t = close + 2;
    if (!strcmp(t, "text")) scopy(c.candText, sizeof(c.candText), v);
    else if (!strcmp(t, "wallclock")) scopy(c.candWall, sizeof(c.candWall), v);
    else if (!strcmp(t, "period.displayValue")) scopy(c.candWhen, sizeof(c.candWhen), v);
    else if (!strcmp(t, "period.number") && !c.candWhen[0]) snprintf(c.candWhen, sizeof(c.candWhen), "Q%s", v);
    else if (!strcmp(t, "clock.displayValue")) {
      size_t n = strlen(c.candWhen);
      if (n + 2 < sizeof(c.candWhen)) snprintf(c.candWhen + n, sizeof(c.candWhen) - n, " %s", v);
    }
  }
}

uint32_t hash(uint32_t h, const char* s) {
  for (; *s; s++) { h ^= (uint8_t)*s; h *= 16777619u; }
  return (h ^ 0xff) * 16777619u;
}
}  // namespace

bool liveParse(ByteSource& src, const Game& g, LiveInfo& out) {
  out = LiveInfo();
  Ctx c{};
  c.g = &g;
  c.out = &out;
  switch (g.league) {
    case L_NFL: case L_CFB: c.defs = FB_STATS; c.nDefs = sizeof(FB_STATS) / sizeof(FB_STATS[0]); break;
    case L_NBA: c.defs = BB_STATS; c.nDefs = sizeof(BB_STATS) / sizeof(BB_STATS[0]); break;
    case L_NHL: c.defs = HK_STATS; c.nDefs = sizeof(HK_STATS) / sizeof(HK_STATS[0]); break;
    default: c.defs = nullptr; c.nDefs = 0;
  }
  c.boxSide[0] = c.boxSide[1] = c.leadSide[0] = c.leadSide[1] = -1;
  bool ok = JsonScan::run(src, enter, leaf, &c);
  commitPlay(c);
  if (!ok) return false;
  if (c.ppSeen == 15) {   // hockey's power play: goals / chances
    LiveStat& s = out.st[out.nStats < 8 ? out.nStats : 7];
    scopy(s.label, sizeof(s.label), "Power play");
    snprintf(s.a, sizeof(s.a), "%s/%s", c.ppGoals[0], c.ppOpp[0]);
    snprintf(s.h, sizeof(s.h), "%s/%s", c.ppGoals[1], c.ppOpp[1]);
    if (out.nStats < 8) out.nStats++;
  }
  // squeeze out the stats the page didn't have, and work out each bar
  int n = 0;
  for (int i = 0; i < out.nStats; i++) {
    if (!out.st[i].label[0]) continue;
    LiveStat s = out.st[i];
    float a = numberOf(s.a), h = numberOf(s.h);
    s.shareA = a + h > 0 ? (uint8_t)(100.0f * a / (a + h) + 0.5f) : 50;
    out.st[n++] = s;
  }
  for (int i = n; i < 8; i++) out.st[i] = LiveStat();
  out.nStats = n;
  uint32_t h = hash(2166136261u, out.play);
  for (int i = 0; i < n; i++) { h = hash(h, out.st[i].a); h = hash(h, out.st[i].h); }
  for (int s = 0; s < 2; s++) for (auto& l : out.lead[s]) { h = hash(h, l.name); h = hash(h, l.val); }
  out.sig = h;
  out.has = true;
  return true;
}
