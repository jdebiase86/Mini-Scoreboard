// Makes puzzles with mn_g_sudoku.cpp's maker and checks each has exactly one answer.
#include "../mini/mn_settings.h"
#include "../mini/mn_g_sudoku.cpp"
#include <stdio.h>
#include <chrono>
Settings settings;
bool valid(const uint8_t* g) {
  for (int i = 0; i < 9; i++) {
    int r = 0, c = 0, b = 0;
    for (int k = 0; k < 9; k++) {
      r |= 1 << g[i * 9 + k]; c |= 1 << g[k * 9 + i];
      b |= 1 << g[((i / 3) * 3 + k / 3) * 9 + (i % 3) * 3 + k % 3];
    }
    if (r != 0x3FE || c != 0x3FE || b != 0x3FE) return false;
  }
  return true;
}
int main() {
  srand(7);
  for (int level = 0; level < 3; level++) {
    int bad = 0, total = 0, givensSum = 0;
    double worst = 0, sum = 0;
    for (int n = 0; n < 40; n++) {
      uint8_t sol[81], puz[81];
      auto t0 = std::chrono::steady_clock::now();
      sdk::makePuzzle(level, sol, puz);
      double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
      worst = ms > worst ? ms : worst; sum += ms;
      int givens = 0;
      for (int i = 0; i < 81; i++) { if (puz[i]) { givens++; if (puz[i] != sol[i]) bad++; } }
      if (!valid(sol)) bad++;
      if (sdk::countSolutions(puz) != 1) bad++;
      givensSum += givens; total++;
    }
    printf("level %d: %d puzzles, %d problems, avg givens %.1f, avg %.1f ms, worst %.0f ms\\n", level, total, bad, givensSum / (double)total, sum / total, worst);
  }
}
