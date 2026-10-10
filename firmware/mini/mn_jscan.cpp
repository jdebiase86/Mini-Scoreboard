#include "mn_jscan.h"

static const char LAT1[] = "AAAAAAACEEEEIIIIDNOOOOOxOUUUUYTsaaaaaaaceeeeiiiidnooooo/ouuuuyty";   // U+00C0 - U+00FF
static const char LATA[] = "AaAaAaCcCcCcCcDdDdEeEeEeEeEeGgGgGgGgHhHhIiIiIiIiIi??JjKkkLlLlLl??LlNnNnNn?NnOoOoOoOoRrRrRrSsSsSsSsTtTtTtUuUuUuUuUuUuWwYyYZzZzZzs";   // U+0100 - U+017F

// one character as ASCII ("" = leave it out)
static const char* foldCp(uint32_t cp, char (&tmp)[4]) {
  tmp[1] = tmp[2] = tmp[3] = 0;
  if (cp < 0x80) { tmp[0] = (char)cp; return tmp; }
  if (cp >= 0xC0 && cp <= 0xFF) { tmp[0] = LAT1[cp - 0xC0]; return tmp; }
  if (cp >= 0x100 && cp <= 0x17F) { tmp[0] = LATA[cp - 0x100]; return tmp; }
  switch (cp) {
    case 0x2018: case 0x2019: tmp[0] = '\''; return tmp;
    case 0x201C: case 0x201D: tmp[0] = '"'; return tmp;
    case 0x2010: case 0x2011: case 0x2012: case 0x2013: case 0x2014: tmp[0] = '-'; return tmp;
    case 0xA0: tmp[0] = ' '; return tmp;
    case 0x2026: return "...";
    default: tmp[0] = '?'; return tmp;
  }
}

void foldUtf8(const char* in, char* out, size_t cap) {
  size_t n = 0;
  for (const uint8_t* p = (const uint8_t*)in; *p && n + 1 < cap;) {
    uint32_t cp = *p++;
    if (cp >= 0xC0) {   // the rest of a UTF-8 sequence
      int more = cp >= 0xF0 ? 3 : cp >= 0xE0 ? 2 : 1;
      cp &= cp >= 0xF0 ? 0x07 : cp >= 0xE0 ? 0x0F : 0x1F;
      while (more-- && (*p & 0xC0) == 0x80) cp = (cp << 6) | (*p++ & 0x3F);
    } else if (cp >= 0x80) {
      cp = '?';
    }
    char tmp[4];
    for (const char* s = foldCp(cp, tmp); *s && n + 1 < cap; s++) out[n++] = *s;
  }
  out[n] = 0;
}

namespace {
struct Scan {
  ByteSource& src;
  JsonScan::Enter enter;
  JsonScan::Leaf leaf;
  void* ctx;
  int pk = -2;
  bool ok = true;
  char path[200];
  int plen = 0;
  char val[260];

  int get() { if (pk != -2) { int c = pk; pk = -2; return c; } return src.read(); }
  int peek() { if (pk == -2) pk = src.read(); return pk; }
  int ws() { int c; while ((c = peek()) == ' ' || c == '\n' || c == '\r' || c == '\t') get(); return c; }

  // a string, after its opening quote: into val (as ASCII), or just skipped
  void str(bool keep) {
    size_t n = 0;
    char tmp[4];
    for (;;) {
      int c = get();
      if (c < 0) { ok = false; return; }
      if (c == '"') break;
      uint32_t cp = c;
      if (c == '\\') {
        int e = get();
        switch (e) {
          case 'n': case 'r': case 't': cp = ' '; break;
          case 'b': case 'f': cp = ' '; break;
          case 'u': {
            cp = 0;
            for (int i = 0; i < 4; i++) { int h = get(); cp = cp * 16 + (h >= 'a' ? h - 'a' + 10 : h >= 'A' ? h - 'A' + 10 : h - '0'); }
            break;
          }
          default: cp = e; break;
        }
      } else if (c >= 0xC0) {   // UTF-8
        int more = c >= 0xF0 ? 3 : c >= 0xE0 ? 2 : 1;
        cp = c & (c >= 0xF0 ? 0x07 : c >= 0xE0 ? 0x0F : 0x1F);
        while (more-- > 0 && (peek() & 0xC0) == 0x80) cp = (cp << 6) | (get() & 0x3F);
      } else if (c >= 0x80) {
        cp = '?';
      }
      if (keep) for (const char* s = foldCp(cp, tmp); *s && n + 1 < sizeof(val); s++) val[n++] = *s;
    }
    if (keep) val[n] = 0;
  }

  void skip() {
    int c = ws();
    if (c == '"') { get(); str(false); return; }
    if (c == '{' || c == '[') {
      int depth = 0;
      for (;;) {
        c = get();
        if (c < 0) { ok = false; return; }
        if (c == '"') { str(false); continue; }
        if (c == '{' || c == '[') depth++;
        else if (c == '}' || c == ']') { if (--depth == 0) return; }
      }
    }
    while ((c = peek()) >= 0 && c != ',' && c != '}' && c != ']') get();
  }

  void value() {
    int c = ws();
    if (c == '{') {
      get();
      if (ws() == '}') { get(); return; }
      for (;;) {
        if (ws() != '"') { ok = false; return; }
        get();
        str(true);
        int save = plen;
        int add = snprintf(path + plen, sizeof(path) - plen, plen ? ".%s" : "%s", val);
        plen = add >= (int)(sizeof(path) - plen) ? (int)sizeof(path) - 1 : plen + add;
        if (ws() != ':') { ok = false; return; }
        get();
        if (!enter || enter(ctx, path)) value(); else skip();
        plen = save; path[plen] = 0;
        if (!ok) return;
        c = ws();
        get();
        if (c == '}') return;
        if (c != ',') { ok = false; return; }
      }
    } else if (c == '[') {
      get();
      if (ws() == ']') { get(); return; }
      for (int i = 0;; i++) {
        int save = plen;
        int add = snprintf(path + plen, sizeof(path) - plen, "[%d]", i);
        plen = add >= (int)(sizeof(path) - plen) ? (int)sizeof(path) - 1 : plen + add;
        if (!enter || enter(ctx, path)) value(); else skip();
        plen = save; path[plen] = 0;
        if (!ok) return;
        c = ws();
        get();
        if (c == ']') return;
        if (c != ',') { ok = false; return; }
      }
    } else if (c == '"') {
      get();
      str(true);
      if (ok && leaf) leaf(ctx, path, val);
    } else {   // a number, true, false, null
      size_t n = 0;
      while ((c = peek()) >= 0 && c != ',' && c != '}' && c != ']' && c != ' ' && c != '\n' && c != '\r') {
        get();
        if (n + 1 < sizeof(val)) val[n++] = (char)c;
      }
      val[n] = 0;
      if (n && strcmp(val, "null") && leaf) leaf(ctx, path, val);
    }
  }
};
}  // namespace

bool JsonScan::run(ByteSource& src, Enter enter, Leaf leaf, void* ctx) {
  Scan s{src, enter, leaf, ctx};
  s.path[0] = 0;
  s.value();
  return s.ok;
}
