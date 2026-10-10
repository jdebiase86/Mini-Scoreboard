#include "../mini/mn_settings.h"
#include "../mini/mn_g_connect4.cpp"
#include <stdio.h>
#include <chrono>
Settings settings;
// a hard mini plays (as 2) against a random player (as 1): it should nearly always win, and never lose
int main() {
  srand(3);
  int wins = 0, losses = 0, draws = 0;
  double worst = 0, sum = 0; int moves = 0;
  for (int d = 0; d < 3; d++) {
    wins = losses = draws = 0;
    diff = d;
    for (int g = 0; g < 40; g++) {
      memset(bd, 0, sizeof(bd));
      int who = 1;
      for (int t = 0; t < 42; t++) {
        int c;
        if (who == 1) { do { c = rand() % COLS; } while (height(c) >= ROWS); }
        else {
          auto t0 = std::chrono::steady_clock::now();
          c = aiMove();
          double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
          if (d == 2) { worst = ms > worst ? ms : worst; sum += ms; moves++; }
        }
        bd[c][height(c)] = who;
        int w[4][2];
        if (::wins(who, w)) { if (who == 2) wins++; else losses++; goto done; }
        if (full()) { draws++; goto done; }
        who = 3 - who;
      }
      done:;
    }
    printf("level %d: mini won %d, lost %d, drawn %d of 40 vs a random player\n", d, wins, losses, draws);
  }
  printf("hard move time on this computer: avg %.1f ms, worst %.0f ms\n", sum / moves, worst);
}
