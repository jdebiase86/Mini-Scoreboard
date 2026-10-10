// Host-only puzzle maker: makes the puzzle bank (mini/mn_sudoku_bank.h) with make_sudoku_bank.sh. The board itself
// never makes puzzles (the search needs more stack than the board has); it mixes up puzzles from the bank.
#pragma once
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
uint32_t gRand(uint32_t n);
namespace sdk {
struct Solver {
  uint8_t g[81];
  uint16_t row[9], col[9], box[9];
  long nodes, nodeLimit;
  int found, limit;
  bool shuffle;
  uint8_t order[9];

  void load(const uint8_t* src) {
    for (int i = 0; i < 9; i++) row[i] = col[i] = box[i] = 0;
    for (int i = 0; i < 81; i++) {
      g[i] = src[i];
      if (g[i]) { uint16_t b = 1 << g[i]; row[i / 9] |= b; col[i % 9] |= b; box[(i / 27) * 3 + (i % 9) / 3] |= b; }
    }
  }
  // fills / counts: stops at `limit` solutions, or when it has tried too long
  bool run() {
    int best = -1, bestN = 10;
    uint16_t bestMask = 0;
    for (int i = 0; i < 81; i++) {
      if (g[i]) continue;
      uint16_t used = row[i / 9] | col[i % 9] | box[(i / 27) * 3 + (i % 9) / 3];
      uint16_t free = ~used & 0x3FE;
      int n = __builtin_popcount(free);
      if (n < bestN) { best = i; bestN = n; bestMask = free; if (n <= 1) break; }
    }
    if (best < 0) { found++; return found >= limit; }   // full
    if (bestN == 0) return false;
    if (++nodes > nodeLimit) return true;               // gave up
    int digits[9], nd = 0;
    for (int d = 1; d <= 9; d++) if (bestMask & (1 << d)) digits[nd++] = d;
    if (shuffle) for (int i = nd - 1; i > 0; i--) { int j = gRand(i + 1); int t = digits[i]; digits[i] = digits[j]; digits[j] = t; }
    for (int k = 0; k < nd; k++) {
      int d = digits[k];
      uint16_t b = 1 << d;
      int r = best / 9, c = best % 9, bx = (best / 27) * 3 + c / 3;
      g[best] = d; row[r] |= b; col[c] |= b; box[bx] |= b;
      bool stop = run();
      if (shuffle && found >= 1) return true;           // one full grid is enough
      g[best] = 0; row[r] &= ~b; col[c] &= ~b; box[bx] &= ~b;
      if (stop) return true;
    }
    return false;
  }
};

// how many answers (up to 2) a puzzle has; 2 also means "too hard to tell"
int countSolutions(const uint8_t* puz) {
  static Solver s;
  s.load(puz);
  s.nodes = 0; s.nodeLimit = 6000; s.found = 0; s.limit = 2; s.shuffle = false;
  s.run();
  if (s.nodes > s.nodeLimit) return 2;
  return s.found;
}

bool makeSolution(uint8_t* out) {
  static Solver s;
  uint8_t empty[81] = {0};
  s.load(empty);
  s.nodes = 0; s.nodeLimit = 200000; s.found = 0; s.limit = 1; s.shuffle = true;
  // run() leaves the filled grid in s.g when it finds one
  s.run();
  if (!s.found) return false;
  memcpy(out, s.g, 81);
  return true;
}

// givens wanted: easy 38, medium 32, hard 27
void makeFresh(int level, uint8_t* sol, uint8_t* puz) {
  for (int tries = 0; tries < 5 && !makeSolution(sol); tries++) {}
  memcpy(puz, sol, 81);
  int want = level == 0 ? 38 : level == 1 ? 32 : 27;
  int order[81];
  for (int i = 0; i < 81; i++) order[i] = i;
  for (int i = 80; i > 0; i--) { int j = gRand(i + 1); int t = order[i]; order[i] = order[j]; order[j] = t; }
  int givens = 81;
  for (int k = 0; k < 81 && givens > want; k++) {
    int i = order[k];
    uint8_t keep = puz[i];
    puz[i] = 0;
    if (countSolutions(puz) != 1) puz[i] = keep; else givens--;
  }
}

}
