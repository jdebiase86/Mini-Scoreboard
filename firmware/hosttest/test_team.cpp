// Reads ESPN's team pages (real, trimmed: feeds/team_*.json) and checks the logo, colour and name come out.
#include "../mini/mn_espn.h"
#include <stdio.h>
struct FileSource : ByteSource {
  FILE* f;
  FileSource(const char* path) { f = fopen(path, "rb"); }
  ~FileSource() { if (f) fclose(f); }
  int read() override { return f ? fgetc(f) : -1; }
  size_t readBytes(char* b, size_t n) override { return f ? fread(b, 1, n, f) : 0; }
};
int main() {
  int bad = 0;
  const char* files[] = {"feeds/team_iowa.json", "feeds/team_cle.json", "feeds/team_det.json"};
  const char* want[] = {"/i/teamlogos/ncaa/500-dark/2294.png", "/i/teamlogos/mlb/500-dark/cle.png", "/i/teamlogos/nhl/500-dark/det.png"};
  for (int i = 0; i < 3; i++) {
    FileSource s(files[i]);
    TeamSide t;
    bool ok = espnLoadTeam(s, t);
    printf("%s: %s logo=%s color=%06x name=%s abbr=%s id=%s\n", files[i], ok ? "ok" : "FAILED", t.logo, (unsigned)t.color, t.name, t.abbr, t.id);
    if (!ok || strcmp(t.logo, want[i])) bad++;
  }
  printf("%d problems\n", bad);
  return bad;
}
