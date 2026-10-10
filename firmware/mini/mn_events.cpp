#include "mn_fx.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>

static bool has(const char* hay, const char* needle) {
  size_t n = strlen(needle);
  for (const char* p = hay; *p; p++) {
    size_t i = 0;
    while (i < n && p[i] && tolower((unsigned char)p[i]) == tolower((unsigned char)needle[i])) i++;
    if (i == n) return true;
  }
  return false;
}

static void upper(char* dst, size_t n, const char* src) {
  size_t i = 0;
  for (; src[i] && i + 1 < n; i++) dst[i] = toupper((unsigned char)src[i]);
  dst[i] = 0;
}

static void begin(const Game& g, FxKind k, FxSpec& f) {
  f = FxSpec();
  f.kind = k;
  f.league = g.league;
  f.mine = g.mine();
  f.them = g.them();
}

static void words(FxSpec& f, const char* word, const char* sub) {
  strncpy(f.word, word, sizeof(f.word) - 1);
  strncpy(f.sub, sub, sizeof(f.sub) - 1);
}

// "NYG 28   PHI 17"
static void score(const Game& g, char (&out)[48]) {
  char a[8], b[8];
  upper(a, sizeof(a), g.mine().abbr);
  upper(b, sizeof(b), g.them().abbr);
  snprintf(out, sizeof(out), "%s %d   %s %d", a, (int)g.mine().score, b, (int)g.them().score);
}

static const char* ordinal(int p) { return p == 1 ? "1ST" : p == 2 ? "2ND" : p == 3 ? "3RD" : p == 4 ? "4TH" : "OT"; }

FxKind detectEvent(const Game& prev, const Game& now, FxSpec& f) {
  if (prev.state == GS_NONE || now.state == GS_NONE || strcmp(prev.id, now.id) || prev.mineHome != now.mineHome ||
      prev.league != now.league || !prev.id[0])
    return FX_NONE;
  const bool both = prev.mine().hasScore && now.mine().hasScore && prev.them().hasScore && now.them().hasScore;
  const int myDiff = both ? now.mine().score - prev.mine().score : 0;
  const int thDiff = both ? now.them().score - prev.them().score : 0;
  const bool live = prev.state == GS_LIVE && now.state == GS_LIVE;
  const bool fresh = now.fb.playId[0] && strcmp(now.fb.playId, prev.fb.playId);
  const char* play = fresh ? now.fb.play : "";
  const bool football = now.league == L_NFL || now.league == L_CFB;
  char sc[48], me[8], th[8];
  upper(me, sizeof(me), now.mine().abbr);
  upper(th, sizeof(th), now.them().abbr);
  score(now, sc);

  if (football) {
    if (live && myDiff >= 6) { begin(now, FX_TOUCHDOWN, f); words(f, "TOUCHDOWN", sc); return f.kind; }
    if (live && myDiff == 3) { begin(now, FX_FIELDGOAL, f); words(f, "IT'S GOOD!", sc); return f.kind; }
    if (live && thDiff >= 6) { begin(now, FX_THEIRSCORE, f); f.tape = f.theirs = true; words(f, "TOUCHDOWN", sc); return f.kind; }
    if (live && thDiff == 3) { begin(now, FX_THEIRSCORE, f); f.tape = f.theirs = true; words(f, "FIELD GOAL", sc); return f.kind; }
    if (prev.state == GS_PRE && now.state == GS_LIVE) {
      begin(now, FX_KICKOFF, f);
      char s[48]; snprintf(s, sizeof(s), "%s  AT  %s", now.away.abbr, now.home.abbr);
      upper(f.sub, sizeof(f.sub), s);
      strcpy(f.word, "KICKOFF");
      return f.kind;
    }
    if (live && prev.period && now.period > prev.period) {
      begin(now, FX_QUARTER, f);
      if (prev.period == 2) strcpy(f.word, "HALFTIME");
      else { snprintf(f.word, sizeof(f.word), "END %s", ordinal(prev.period)); }
      strcpy(f.sub, sc);
      return f.kind;
    }
    const int mineSide = now.mineHome ? 2 : 1, themSide = 3 - mineSide;
    const int pPos = prev.fb.possession, nPos = now.fb.possession;
    if (live && fresh) {
      const bool kick = has(play, "field goal");
      if (kick && (has(play, "no good") || has(play, "missed") || has(play, "blocked"))) {
        begin(now, FX_NOGOOD, f);
        f.tape = pPos == mineSide;
        f.theirs = !f.tape;
        char s[48]; snprintf(s, sizeof(s), f.tape ? "%s KEEP %d" : "%s MISS THE KICK", f.tape ? me : th, (int)now.mine().score);
        words(f, "NO GOOD", s);
        return f.kind;
      }
      if (has(play, "intercept") && pPos && nPos && pPos != nPos) {
        if (nPos == mineSide) { begin(now, FX_PICKED, f); char s[48]; snprintf(s, sizeof(s), "INTERCEPTION - %s BALL", me); words(f, "PICKED OFF!", s); }
        else { begin(now, FX_TURNOVER, f); f.tape = f.theirs = true; char s[48]; snprintf(s, sizeof(s), "PICKED OFF - %s BALL", th); words(f, "TURNOVER", s); }
        return f.kind;
      }
      if (has(play, "fumble") && pPos && nPos && pPos != nPos) {
        if (nPos == mineSide) { begin(now, FX_FUMBLE, f); char s[48]; snprintf(s, sizeof(s), "%s BALL - RECOVERED", me); words(f, "FUMBLE!", s); }
        else { begin(now, FX_TURNOVER, f); f.tape = f.theirs = true; char s[48]; snprintf(s, sizeof(s), "FUMBLE - %s BALL", th); words(f, "TURNOVER", s); }
        return f.kind;
      }
      if (has(play, "sacked") && pPos == themSide) {
        begin(now, FX_SACK, f);
        char s[48]; snprintf(s, sizeof(s), "%s SACKED", th); words(f, "SACKED!", s);
        return f.kind;
      }
      if (has(play, "penalty")) {
        begin(now, FX_FLAG, f);
        strcpy(f.word, "FLAG");
        // "PENALTY on DAL-J.Smith, Defensive Holding, 10 yards": the foul and the yards
        const char* c = strchr(play, ',');
        char s[48] = "";
        if (c) { upper(s, sizeof(s), c + 1); while (s[0] == ' ') memmove(s, s + 1, strlen(s)); }
        strcpy(f.sub, s[0] ? s : "PENALTY");
        return f.kind;
      }
      if (has(play, "punt") && !has(play, "blocked")) {
        if (pPos == themSide) { begin(now, FX_PUNT, f); char s[48]; snprintf(s, sizeof(s), "%s HAVE TO PUNT", th); words(f, "PUNT-ASTIC!", s); return f.kind; }
        if (pPos == mineSide) { begin(now, FX_NOPUNT, f); char s[48]; snprintf(s, sizeof(s), "%s PUNT", me); words(f, "NO PUNT INTENDED", s); return f.kind; }
      }
      const bool special = has(play, "punt") || has(play, "field goal") || has(play, "touchdown") || has(play, "intercept") ||
                           has(play, "fumble") || has(play, "kickoff");
      if (!special && prev.fb.down == 4 && pPos == mineSide && nPos == mineSide && now.fb.down == 1) {
        begin(now, FX_WENTFORIT, f);
        words(f, "WENT FOR IT!", "AND MADE IT - FIRST DOWN");
        return f.kind;
      }
      if (!special && prev.fb.down == 4 && pPos == themSide && nPos == mineSide) {
        begin(now, FX_STONEWALL, f);
        char s[48]; snprintf(s, sizeof(s), "TURNOVER ON DOWNS - %s BALL", me); words(f, "STONEWALLED!", s);
        return f.kind;
      }
      if (prev.fb.down == 3 && pPos == themSide && nPos == themSide && now.fb.down == 4) {
        begin(now, FX_STOPPED, f);
        char s[48]; snprintf(s, sizeof(s), "%s FACE 4TH & %d", th, (int)now.fb.distance); words(f, "STOPPED!", s);
        return f.kind;
      }
      if (pPos == mineSide && nPos == mineSide && now.fb.down == 1 && prev.fb.down > 1) {
        begin(now, FX_FIRSTDOWN, f);
        words(f, "FIRST DOWN!", now.fb.dd);
        return f.kind;
      }
    }
    return FX_NONE;
  }
  if (now.league == L_NHL) {
    if (live && myDiff > 0) { begin(now, FX_GOAL, f); words(f, "GOAL!", sc); return f.kind; }
    if (live && thDiff > 0) { begin(now, FX_THEIRSCORE, f); f.tape = f.theirs = true; words(f, "GOAL", sc); return f.kind; }
    if (live && now.detail[0] && has(now.detail, "end of") && !has(prev.detail, "end of")) {
      begin(now, FX_QUARTER, f);
      upper(f.word, sizeof(f.word), now.detail);
      if (!strcmp(f.word, "END OF 1ST") || !strcmp(f.word, "END OF 2ND") || !strcmp(f.word, "END OF 3RD")) strcpy(f.word, "INTERMISSION");
      strcpy(f.sub, sc);
      return f.kind;
    }
    return FX_NONE;
  }
  if (now.league == L_MLB) {
    if (live && myDiff > 0) {
      bool hr = fresh && (has(play, "home run") || has(play, "homers") || has(play, "grand slam"));
      if (hr) { begin(now, FX_HOMERUN, f); words(f, has(play, "grand slam") ? "GRAND SLAM!" : "HOME RUN!", sc); }
      else { begin(now, FX_RUN, f); f.n = myDiff; words(f, myDiff > 1 ? "RUNS SCORE!" : "RUN SCORES!", sc); }
      return f.kind;
    }
    if (live && thDiff > 0 && fresh && (has(play, "home run") || has(play, "homers") || has(play, "grand slam"))) {
      begin(now, FX_THEIRSCORE, f); f.tape = f.theirs = true; words(f, "HOME RUN", sc); return f.kind;
    }
    return FX_NONE;
  }
  if (now.league == L_NBA) {
    if (live && prev.period && now.period > prev.period) {
      begin(now, FX_QUARTER, f);
      if (prev.period == 2) strcpy(f.word, "HALFTIME"); else snprintf(f.word, sizeof(f.word), "END %s", ordinal(prev.period));
      strcpy(f.sub, sc);
      return f.kind;
    }
    if (live && myDiff == 3 && !has(play, "free throw")) { begin(now, FX_THREE, f); words(f, "THREE!", sc); return f.kind; }
    return FX_NONE;
  }
  return FX_NONE;
}

bool detectWin(const Game& prev, const Game& now, FxSpec& f) {
  if (prev.state != GS_LIVE || now.state != GS_POST || strcmp(prev.id, now.id) || prev.mineHome != now.mineHome || !prev.id[0]) return false;
  if (!now.mine().hasScore || !now.them().hasScore || now.mine().score <= now.them().score) return false;
  begin(now, FX_WIN, f);
  // "GIANTS WIN", "FLORIDA WINS"
  char w[22];
  upper(w, sizeof(w), now.mine().name[0] ? now.mine().name : now.mine().abbr);
  size_t n = strlen(w);
  bool plural = n > 0 && w[n - 1] == 'S';
  snprintf(f.word, sizeof(f.word), "%.12s %s", w, plural ? "WIN" : "WINS");
  snprintf(f.sub, sizeof(f.sub), "FINAL  %d - %d", (int)now.mine().score, (int)now.them().score);
  return true;
}
