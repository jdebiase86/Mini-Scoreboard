// Reads a saved ESPN game page (summary JSON) with mn_live.cpp and prints what it kept:
//   ./live_test.sh summary.json LEAGUE AWAY HOME     e.g. summary.json NBA HOU DAL
#include "../mini/mn_live.h"
#include "../mini/mn_espn.h"
#include <stdio.h>
time_t mnNow;
struct F : ByteSource { FILE* f; F(const char* p) { f = fopen(p, "rb"); } int read() override { return fgetc(f); } size_t readBytes(char* b, size_t n) override { return fread(b, 1, n, f); } };
int main(int, char** a) {
  F f(a[1]);
  Game g;
  const char* lg = a[2];
  g.league = !strcmp(lg, "NFL") ? L_NFL : !strcmp(lg, "CFB") ? L_CFB : !strcmp(lg, "MLB") ? L_MLB : !strcmp(lg, "NHL") ? L_NHL : L_NBA;
  strncpy(g.away.abbr, a[3], 7); strncpy(g.home.abbr, a[4], 7);
  static LiveInfo li;
  bool ok = liveParse(f, g, li);
  printf("ok=%d has=%d stats=%d play=[%s] when=[%s] sizeof(LiveInfo)=%zu\n", ok, li.has, li.nStats, li.play, li.playWhen, sizeof(LiveInfo));
  for (int i = 0; i < li.nStats; i++) printf("  %-14s %-8s | %-8s  share %d\n", li.st[i].label, li.st[i].a, li.st[i].h, li.st[i].shareA);
  for (int s = 0; s < 2; s++) for (auto& l : li.lead[s]) if (l.cat[0]) printf("  %s lead %-5s %-16s %s\n", s ? "home" : "away", l.cat, l.name, l.val);
}
