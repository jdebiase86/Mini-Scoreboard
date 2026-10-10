#include "../mini/mn_settings.h"
#include "../mini/mn_g_2048.cpp"
#include <stdio.h>
Settings settings;
int main() {
  struct T { uint8_t in[4], out[4]; int pts; } tests[] = {
    {{1, 1, 1, 1}, {2, 2, 0, 0}, 8}, {{1, 1, 2, 0}, {2, 2, 0, 0}, 4 + 4}, {{0, 1, 0, 1}, {2, 0, 0, 0}, 4},
    {{2, 1, 1, 0}, {2, 2, 0, 0}, 4}, {{1, 2, 3, 4}, {1, 2, 3, 4}, 0}, {{3, 3, 3, 0}, {4, 3, 0, 0}, 16}};
  int bad = 0;
  for (auto& t : tests) {
    uint8_t l[4]; memcpy(l, t.in, 4);
    int p = slide(l);
    if (memcmp(l, t.out, 4) || p != t.pts) { bad++; printf("FAIL %d%d%d%d -> %d%d%d%d (%d pts), wanted %d%d%d%d (%d)\n", t.in[0], t.in[1], t.in[2], t.in[3], l[0], l[1], l[2], l[3], p, t.out[0], t.out[1], t.out[2], t.out[3], t.pts); }
  }
  // a full random game by random swipes always ends and never breaks the board
  srand(1);
  int games = 0;
  for (int g = 0; g < 200; g++) {
    memset(b, 0, 16); score = 0; spawn(); spawn();
    for (int n = 0; n < 5000 && canMove(); n++) { move(rand() % 4) ? spawn() : (void)0; }
    games++;
  }
  printf("2048: %d line tests failed; %d random games finished\n", bad, games);
}
