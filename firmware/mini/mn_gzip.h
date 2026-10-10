// Reads a gzip-compressed answer as plain bytes. ESPN sends its scoreboard
// about twelve times smaller this way, which matters on a phone hotspot.
// Needs about 44 KB while it runs (the decompressor and its 32 KB window).
#pragma once
#include "mn_espn.h"

struct GzSource : ByteSource {
  explicit GzSource(ByteSource& src);
  ~GzSource();
  bool ok() const { return good; }        // false: no memory or not gzip
  bool failed() const { return bad; }     // the compressed data was broken
  int read() override;
  size_t readBytes(char* buf, size_t n) override;

 private:
  bool refill();
  bool header();
  ByteSource& src;
  void* d = nullptr;                      // tinfl_decompressor
  uint8_t* dict = nullptr;
  uint8_t in[1024];
  size_t inPos = 0, inLen = 0, dictOfs = 0, outN = 0;
  const uint8_t* outP = nullptr;
  bool inEnd = false, done = false, good = false, bad = false;
};
