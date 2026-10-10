// Checks what the mini makes of two looks at a game (mn_events.cpp): scores, turnovers, flags and so on.
#include "../mini/mn_fx.h"
#include <stdio.h>
static int bad = 0;
static Game base(League l = L_NFL) {
  Game g; g.state = GS_LIVE; g.league = l; g.mineHome = true; strcpy(g.id, "401");
  strcpy(g.home.abbr, "NYG"); strcpy(g.away.abbr, "PHI"); strcpy(g.home.name, "Giants"); strcpy(g.away.name, "Eagles");
  g.home.hasScore = g.away.hasScore = true; g.home.score = 14; g.away.score = 10; g.period = 2;
  g.fb.has = l == L_NFL; g.fb.possession = 2; g.fb.down = 2; g.fb.distance = 6; strcpy(g.fb.playId, "1"); strcpy(g.fb.play, "start");
  return g;
}
static void play(Game& g, const char* id, const char* text) { strcpy(g.fb.playId, id); strcpy(g.fb.play, text); }
static void expect(const char* name, const Game& a, const Game& b, FxKind want, bool tape = false) {
  FxSpec f;
  FxKind k = detectEvent(a, b, f);
  bool ok = k == want && (want == FX_NONE || f.tape == tape);
  printf("%-28s %s%s\n", name, ok ? "ok" : "WRONG: got kind ", ok ? "" : (k == FX_NONE ? "none" : f.word));
  if (!ok) bad++;
}
int main() {
  Game a = base(), b;
  b = a; b.home.score = 21; expect("our touchdown", a, b, FX_TOUCHDOWN);
  b = a; b.home.score = 17; expect("our field goal", a, b, FX_FIELDGOAL);
  b = a; b.away.score = 17; expect("their touchdown (tape)", a, b, FX_THEIRSCORE, true);
  b = a; b.away.score = 13; expect("their field goal (tape)", a, b, FX_THEIRSCORE, true);
  b = a; b.home.score = 16; expect("a safety / 2 points: nothing", a, b, FX_NONE);
  b = a; expect("nothing happened", a, b, FX_NONE);
  b = a; strcpy(b.id, "402"); b.home.score = 21; expect("another game: nothing", a, b, FX_NONE);
  b = a; b.period = 3; expect("end of the half", a, b, FX_QUARTER);
  Game pre = a; pre.state = GS_PRE; b = a; expect("kickoff", pre, b, FX_KICKOFF);
  b = a; play(b, "2", "PENALTY on PHI-J.Smith, Defensive Holding, 10 yards"); expect("flag", a, b, FX_FLAG);
  b = a; play(b, "2", "J.Smith pass intercepted by D.Jones at the PHI 30"); b.fb.possession = 1; expect("we intercept? (they had it): wrong side first", a, b, FX_TURNOVER, true);
  Game a2 = a; a2.fb.possession = 1; b = a2; play(b, "2", "J.Hurts pass intercepted by D.Jones"); b.fb.possession = 2; expect("picked off (ours)", a2, b, FX_PICKED);
  b = a; play(b, "2", "D.Jones fumbles, recovered by PHI"); b.fb.possession = 1; expect("we fumble (tape)", a, b, FX_TURNOVER, true);
  b = a2; play(b, "2", "J.Hurts fumbles, recovered by NYG"); b.fb.possession = 2; expect("their fumble (ours)", a2, b, FX_FUMBLE);
  b = a2; play(b, "2", "J.Hurts sacked at the PHI 20 for -8 yards"); expect("sack (ours)", a2, b, FX_SACK);
  b = a; play(b, "2", "D.Jones sacked at the NYG 20 for -8 yards"); expect("we are sacked: nothing", a, b, FX_NONE);
  b = a2; play(b, "2", "P.Punter punts 46 yards"); b.fb.possession = 2; expect("they punt", a2, b, FX_PUNT);
  b = a; play(b, "2", "G.Punter punts 46 yards"); b.fb.possession = 1; expect("we punt", a, b, FX_NOPUNT);
  b = a; b.fb.down = 1; play(b, "2", "D.Jones rush for 8 yards"); Game c = a; c.fb.down = 3; expect("our first down", c, b, FX_FIRSTDOWN);
  c = a; c.fb.down = 4; b = a; b.fb.down = 1; play(b, "2", "D.Jones rush for 2 yards"); expect("went for it on 4th", c, b, FX_WENTFORIT);
  c = a2; c.fb.down = 4; b = a2; b.fb.possession = 2; b.fb.down = 1; play(b, "2", "J.Hurts pass incomplete"); expect("turnover on downs", c, b, FX_STONEWALL);
  c = a2; c.fb.down = 3; b = a2; b.fb.down = 4; b.fb.distance = 3; play(b, "2", "J.Hurts pass incomplete"); expect("third down stop", c, b, FX_STOPPED);
  b = a; play(b, "2", "G.Kicker 48 yard field goal is No Good"); expect("our kick misses (tape)", a, b, FX_NOGOOD, true);
  b = a2; play(b, "2", "P.Kicker 48 yard field goal is No Good"); expect("their kick misses", a2, b, FX_NOGOOD, false);
  // other sports
  Game h = base(L_NHL), h2 = h; h2.home.score = 15; expect("hockey goal", h, h2, FX_GOAL);
  h2 = h; h2.away.score = 11; expect("their goal (tape)", h, h2, FX_THEIRSCORE, true);
  Game m = base(L_MLB), m2 = m; m2.home.score = 15; play(m2, "9", "Judge homers to left"); expect("home run", m, m2, FX_HOMERUN);
  m2 = m; m2.home.score = 15; play(m2, "9", "Soto singles, scoring Judge"); expect("a run", m, m2, FX_RUN);
  m2 = m; m2.away.score = 11; play(m2, "9", "Harper homers to right"); expect("their home run (tape)", m, m2, FX_THEIRSCORE, true);
  Game n = base(L_NBA), n2 = n; n2.home.score = 17; expect("a three", n, n2, FX_THREE);
  n2 = n; n2.home.score = 16; expect("a two: nothing", n, n2, FX_NONE);
  // the win
  Game e = a; e.state = GS_POST; e.home.score = 31; e.away.score = 24; FxSpec w;
  printf("%-28s %s  [%s / %s]\n", "win", detectWin(a, e, w) ? "ok" : "WRONG", w.word, w.sub);
  if (strcmp(w.word, "GIANTS WIN")) bad++;
  e.home.score = 20; e.away.score = 24; if (detectWin(a, e, w)) { printf("a loss counted as a win\n"); bad++; }
  Game col = a; strcpy(col.home.name, "Florida"); col.state = GS_POST; col.home.score = 30; col.away.score = 3; detectWin(a, col, w);
  printf("college win text: %s\n", w.word); if (strcmp(w.word, "FLORIDA WINS")) bad++;
  printf("%d problems\n", bad);
  return bad;
}
