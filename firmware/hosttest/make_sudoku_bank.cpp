// Makes ../mini/mn_sudoku_bank.h: ready-made puzzles with exactly one answer (the board mixes them up).
// Run with make_sudoku_bank.sh
#include "sudoku_maker.h"
#include <stdio.h>
#include <set>
#include <string>
uint32_t gRand(uint32_t n) { return rand() % n; }
int main() {
  srand(20261010);
  const int N = 24;
  FILE* f = fopen("../mini/mn_sudoku_bank.h", "w");
  fprintf(f, "// Made by firmware/hosttest/make_sudoku_bank.sh. %d puzzles per level (easy, medium, hard), each as a puzzle string\n"
             "// and its answer string (0 = empty). Every puzzle has exactly one answer.\n#pragma once\n"
             "#define SDK_BANK_N %d\nstatic const char* const SDK_BANK[3][SDK_BANK_N][2] = {\n", N, N);
  std::set<std::string> seen;
  for (int level = 0; level < 3; level++) {
    fprintf(f, "  {\n");
    for (int n = 0; n < N;) {
      uint8_t sol[81], puz[81];
      sdk::makeFresh(level, sol, puz);
      if (sdk::countSolutions(puz) != 1) continue;
      std::string p, s;
      for (int i = 0; i < 81; i++) { p += char('0' + puz[i]); s += char('0' + sol[i]); }
      if (!seen.insert(p).second) continue;
      fprintf(f, "    {\"%s\",\n     \"%s\"},\n", p.c_str(), s.c_str());
      n++;
    }
    fprintf(f, "  },\n");
  }
  fprintf(f, "};\n");
  fclose(f);
}
