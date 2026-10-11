// A feed whose live game is at halftime (feeds/cfb4_now.json with the status changed): the game must stay "live" for the
// polling, but carry no field, red zone or last play.
#include "../mini/mn_espn.h"
#include "../mini/mn_teams.h"
#include <stdio.h>
struct FileSource : ByteSource {
  FILE* f;
  FileSource(const char* path) { f = fopen(path, "rb"); }
  ~FileSource() { if (f) fclose(f); }
  int read() override { return f ? fgetc(f) : -1; }
  size_t readBytes(char* b, size_t n) override { return f ? fread(b, 1, n, f) : 0; }
};
static int check(const char* path, bool wantHalf) {
  int found = 0, bad = 0;
  for (int t = 0; t < NTEAMS; t++) {
    FileSource s(path);
    JsonDocument doc;
    if (!espnLoad(s, doc)) continue;
    Game g;
    if (!espnFind(doc, t, time(nullptr), g) || g.state != GS_LIVE) continue;
    found++;
    bool half = !strncmp(g.detail, "Half", 4);
    printf("%s team %d: detail=%s fb.has=%d redzone=%d play=[%.20s]\n", path, t, g.detail, g.fb.has, g.fb.redzone, g.fb.play);
    if (half != wantHalf) bad++;
    if (wantHalf && (g.fb.has || g.fb.redzone || g.fb.play[0])) bad++;
    if (!wantHalf && !g.fb.has) bad++;
    if (found >= 2) break;
  }
  if (!found) { printf("%s: no live game found for any team\n", path); bad++; }
  return bad;
}
int main() {
  int bad = check("out/half.json", true) + check("feeds/cfb4_now.json", false);
  printf("%d problems\n", bad);
  return bad;
}
