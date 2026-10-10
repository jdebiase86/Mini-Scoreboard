#include "mn_gzip.h"
#include <rom/miniz.h>

static const size_t DICT = TINFL_LZ_DICT_SIZE;

GzSource::GzSource() {
  d = malloc(sizeof(tinfl_decompressor));
  dict = (uint8_t*)malloc(DICT);
  if (d) tinfl_init((tinfl_decompressor*)d);
}

bool GzSource::begin(ByteSource& s) {
  src = &s;
  good = ok() && header();
  return good;
}

GzSource::~GzSource() {
  free(d);
  free(dict);
}

// the gzip wrapper: 10 fixed bytes, then optional extra / name / comment / header check
bool GzSource::header() {
  uint8_t h[10];
  if (src->readBytes((char*)h, 10) != 10 || h[0] != 0x1f || h[1] != 0x8b || h[2] != 8) return false;
  uint8_t flg = h[3];
  if (flg & 4) {
    int a = src->read(), b = src->read();
    if (a < 0 || b < 0) return false;
    for (int n = a | (b << 8); n > 0; n--) if (src->read() < 0) return false;
  }
  for (uint8_t bit : {8, 16})
    if (flg & bit) for (int c; (c = src->read()) != 0;) if (c < 0) return false;
  if (flg & 2) { src->read(); src->read(); }
  return true;
}

bool GzSource::refill() {
  while (outN == 0 && !done) {
    if (inPos >= inLen && !inEnd) {
      inLen = src->readBytes((char*)in, sizeof(in));
      inPos = 0;
      if (inLen == 0) inEnd = true;
    }
    size_t ib = inLen - inPos, ob = DICT - dictOfs;
    tinfl_status st = tinfl_decompress((tinfl_decompressor*)d, in + inPos, &ib, dict, dict + dictOfs, &ob,
                                       inEnd ? 0 : TINFL_FLAG_HAS_MORE_INPUT);
    inPos += ib;
    if (ob) {
      outP = dict + dictOfs;
      outN = ob;
      dictOfs = (dictOfs + ob) & (DICT - 1);
    }
    if (st == TINFL_STATUS_DONE) done = true;
    else if (st < 0) { done = true; bad = true; }
    else if (st == TINFL_STATUS_NEEDS_MORE_INPUT && inEnd && ob == 0) done = true;   // cut short
  }
  return outN > 0;
}

int GzSource::read() {
  if (!good || (outN == 0 && !refill())) return -1;
  outN--;
  return *outP++;
}

size_t GzSource::readBytes(char* b, size_t n) {
  size_t k = 0;
  while (good && k < n) {
    if (outN == 0 && !refill()) break;
    size_t m = outN < n - k ? outN : n - k;
    memcpy(b + k, outP, m);
    outP += m; outN -= m; k += m;
  }
  return k;
}
