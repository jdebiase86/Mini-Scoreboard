// The board's ROM has miniz's inflate (<rom/miniz.h>). On a computer the same
// few calls are made with zlib, so mn_gzip.cpp can be tried on real data.
#pragma once
#include <zlib.h>
#include <stdlib.h>
#include <string.h>
typedef unsigned char mz_uint8;
typedef unsigned int mz_uint32;
enum { TINFL_FLAG_PARSE_ZLIB_HEADER = 1, TINFL_FLAG_HAS_MORE_INPUT = 2, TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF = 4 };
enum tinfl_status { TINFL_STATUS_BAD_PARAM = -3, TINFL_STATUS_ADLER32_MISMATCH = -2, TINFL_STATUS_FAILED = -1,
                    TINFL_STATUS_DONE = 0, TINFL_STATUS_NEEDS_MORE_INPUT = 1, TINFL_STATUS_HAS_MORE_OUTPUT = 2 };
#define TINFL_LZ_DICT_SIZE 32768
struct tinfl_decompressor { z_stream z; int started; };
#define tinfl_init(r) do { memset(&(r)->z, 0, sizeof((r)->z)); (r)->started = 0; } while (0)
static inline tinfl_status tinfl_decompress(tinfl_decompressor* r, const mz_uint8* in, size_t* inN, mz_uint8*, mz_uint8* out,
                                            size_t* outN, mz_uint32) {
  if (!r->started) { inflateInit2(&r->z, -15); r->started = 1; }
  r->z.next_in = (Bytef*)in; r->z.avail_in = *inN;
  r->z.next_out = out; r->z.avail_out = *outN;
  int e = inflate(&r->z, Z_NO_FLUSH);
  *inN -= r->z.avail_in; *outN -= r->z.avail_out;
  if (e == Z_STREAM_END) return TINFL_STATUS_DONE;
  if (e == Z_OK || e == Z_BUF_ERROR) return r->z.avail_out == 0 ? TINFL_STATUS_HAS_MORE_OUTPUT : TINFL_STATUS_NEEDS_MORE_INPUT;
  return TINFL_STATUS_FAILED;
}
