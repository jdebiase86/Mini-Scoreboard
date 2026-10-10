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

bool enter(void* vc, const char* p) {
  // only the parts we read
  if (!strncmp(p, "boxscore", 8)) return p[8] == 0 || !strncmp(p, "boxscore.teams", 14);
  if (!strncmp(p, "leaders", 7)) return p[7] == 0 || p[7] == '[' || p[7] == '.';
  if (!strncmp(p, "plays", 5) && (p[5] == 0 || p[5] == '[')) {
    // plays[N] and its text, scores, period and clock only
    const char* dot = strchr(p, '.');
    if (!dot) return true;
    dot++;
    return !strcmp(dot, "text") || !strcmp(dot, "scoringPlay") || !strcmp(dot, "period") || !strncmp(dot, "period.", 7) ||
           !strcmp(dot, "clock") || !strncmp(dot, "clock.", 6);
  }
  (void)vc;
  return false;
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
  // the last play (the list runs oldest first, so each one replaces the one before)
  if (sscanf(p, "plays[%d].%59s", &i, tail) == 2) {
    if (!strcmp(tail, "text")) { scopy(o.play, sizeof(o.play), v); o.hasPlay = o.play[0] != 0; o.playWhen[0] = 0; }
    else if (!strcmp(tail, "period.displayValue")) { scopy(o.playWhen, sizeof(o.playWhen), v); }
    else if (!strcmp(tail, "clock.displayValue")) {
      size_t n = strlen(o.playWhen);
      if (n + 2 < sizeof(o.playWhen)) snprintf(o.playWhen + n, sizeof(o.playWhen) - n, " %s", v);
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
