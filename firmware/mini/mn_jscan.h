// Reads a JSON answer one piece at a time without building it in memory, so
// even a 1 MB game page from ESPN costs only a few hundred bytes. Every value
// is handed over with its path ("boxscore.teams[1].statistics[3].name").
// Text comes out as plain ASCII (accents folded: "Jokic").
#pragma once
#include "mn_espn.h"

struct JsonScan {
  // Called before each member / element is read, with its path: return false
  // to skip the whole value (fast).
  typedef bool (*Enter)(void* ctx, const char* path);
  // Called for each string, number, true and false.
  typedef void (*Leaf)(void* ctx, const char* path, const char* value);
  static bool run(ByteSource& src, Enter enter, Leaf leaf, void* ctx);
};

// UTF-8 text -> plain ASCII (the screen fonts only have those letters)
void foldUtf8(const char* in, char* out, size_t cap);
