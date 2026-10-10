#include "mn_logo.h"
#include "mn_lcd.h"
#include <PNGdec.h>
#include <new>

static volatile uint32_t version = 1;
uint32_t logoVersion() { return version; }

static const size_t MAX_PNG = 24 * 1024;

// "/i/teamlogos/nfl/500-dark/scoreboard/kc.png" -> "nfl_kc_76"
static bool keyOf(const TeamSide& t, int size, char (&key)[40]) {
  const char* p = strstr(t.logo, "/teamlogos/");
  if (!p) return false;
  p += 11;
  const char* slash = strchr(p, '/');
  const char* last = strrchr(p, '/');
  const char* dot = strrchr(p, '.');
  if (!slash || !last || !dot || dot <= last) return false;
  snprintf(key, sizeof(key), "%.*s_%.*s_%d", (int)(slash - p), p, (int)(dot - last - 1), last + 1, size);
  return true;
}

// ------------------------------------------------------------ the files
#ifdef MN_HOST
#include <stdio.h>
#include <stdlib.h>
static uint8_t* loadFile(const char* key, size_t& n) {
  const char* dir = getenv("MN_LOGOS");
  char path[300];
  snprintf(path, sizeof(path), "%s/%s.png", dir ? dir : "hosttest/logos", key);
  FILE* f = fopen(path, "rb");
  if (!f) return nullptr;
  uint8_t* b = (uint8_t*)malloc(MAX_PNG);
  n = fread(b, 1, MAX_PNG, f);
  fclose(f);
  return b;
}
static void want(const char*, const TeamSide&, int) {}
#else
#include <LittleFS.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "mn_tls.h"
#include "mn_log.h"
#include "mn_net.h"

struct Want { char key[40]; char path[64]; uint8_t size; };
static const int NWANT = 8;
static Want wants[NWANT];
static int nwant = 0;
static struct { char key[40]; uint32_t at; } failed[8];
static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
static bool fsOk = false;

void logoBegin() {
  fsOk = LittleFS.begin(true, "/littlefs", 10, "spiffs");
  mnLog("logo storage %s: %u KB used of %u KB", fsOk ? "ready" : "FAILED", fsOk ? (unsigned)(LittleFS.usedBytes() / 1024) : 0,
        fsOk ? (unsigned)(LittleFS.totalBytes() / 1024) : 0);
}

static uint8_t* loadFile(const char* key, size_t& n) {
  if (!fsOk) return nullptr;
  char path[48];
  snprintf(path, sizeof(path), "/%s.png", key);
  File f = LittleFS.open(path, "r");
  if (!f) return nullptr;
  n = f.size();
  if (n < 60 || n > MAX_PNG) { f.close(); return nullptr; }
  uint8_t* b = (uint8_t*)heap_caps_malloc(n, MALLOC_CAP_8BIT);
  if (b && f.read(b, n) != n) { free(b); b = nullptr; }
  f.close();
  return b;
}

static void pushWant(const Want& w) {
  portENTER_CRITICAL(&mux);
  if (nwant < NWANT) wants[nwant++] = w;
  portEXIT_CRITICAL(&mux);
}

static void want(const char* key, const TeamSide& t, int size) {
  uint32_t now = millis();
  portENTER_CRITICAL(&mux);
  bool skip = false;
  for (auto& f : failed) if (!strcmp(f.key, key) && now - f.at < 10 * 60000UL) skip = true;
  for (int i = 0; i < nwant; i++) if (!strcmp(wants[i].key, key)) skip = true;
  if (!skip && nwant < NWANT) {
    strncpy(wants[nwant].key, key, 39); wants[nwant].key[39] = 0;
    strncpy(wants[nwant].path, t.logo, 63); wants[nwant].path[63] = 0;
    wants[nwant].size = size;
    nwant++;
  }
  portEXIT_CRITICAL(&mux);
}

// the logos saved most recently, and the ones about to be fetched, are never thrown out to make room
// (two big logos used to throw each other out, again and again)
static char recent[6][40];
static int recentAt = 0;
static void markRecent(const char* key) { strncpy(recent[recentAt++ % 6], key, 39); }
static bool protectedKey(const String& nm, const char* saving) {
  String k = nm.startsWith("/") ? nm.substring(1) : nm;
  if (k.endsWith(".png")) k = k.substring(0, k.length() - 4);
  if (k == saving) return true;
  for (auto& r : recent) if (r[0] && k == r) return true;
  bool hit = false;
  portENTER_CRITICAL(&mux);
  for (int i = 0; i < nwant; i++) if (k == wants[i].key) hit = true;
  portEXIT_CRITICAL(&mux);
  return hit;
}

// room for one more: first let go of game-screen sizes nobody needs right now, then any other logo that isn't
// protected (they come back when drawn again), then give up
static bool makeRoom(size_t need, const char* saving) {
  auto enough = [&]() { return LittleFS.totalBytes() - LittleFS.usedBytes() > need + 8192; };
  if (enough()) return true;
  for (int pass = 0; pass < 2 && !enough(); pass++) {
    String doomed[16];
    int n = 0;
    File root = LittleFS.open("/");
    for (File f = root.openNextFile(); f && n < 16; f = root.openNextFile()) {
      String nm = f.name();
      if (!nm.endsWith(".png") || protectedKey(nm, saving)) continue;
      if (pass == 0 && !nm.endsWith("_112.png")) continue;
      doomed[n++] = nm;
    }
    for (int i = 0; i < n; i++) LittleFS.remove(doomed[i].startsWith("/") ? doomed[i] : "/" + doomed[i]);
  }
  return enough();
}

static bool download(const String& path, int size, uint8_t* buf, size_t& n, int& code) {
  String url = String("https://a.espncdn.com/combiner/i?img=") + path + "&w=" + size + "&h=" + size;
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.useHTTP10(true);
  http.setReuse(false);
  http.setConnectTimeout(10000);
  http.setTimeout(15000);
  n = 0;
  if (!http.begin(client, url)) { code = -1; return false; }
  http.setUserAgent("Mozilla/5.0 (Mini Scoreboard)");
  code = http.GET();
  if (code != 200) { http.end(); return false; }
  WiFiClient* s = http.getStreamPtr();
  uint32_t last = millis();
  while ((http.connected() || s->available()) && n < MAX_PNG && millis() - last < 10000) {
    int a = s->available();
    if (a <= 0) { delay(5); continue; }
    int k = s->read(buf + n, min((size_t)a, MAX_PNG - n));
    if (k > 0) { n += k; last = millis(); }
  }
  http.end();
  return n > 60 && buf[0] == 0x89 && buf[1] == 'P' && buf[2] == 'N' && buf[3] == 'G';
}

bool logoFetchOne() {
  if (!fsOk) return false;
  Want w;
  portENTER_CRITICAL(&mux);
  if (!nwant) { portEXIT_CRITICAL(&mux); return false; }
  w = wants[0];
  memmove(&wants[0], &wants[1], sizeof(Want) * (nwant - 1));
  nwant--;
  portEXIT_CRITICAL(&mux);

  char fn[48];
  snprintf(fn, sizeof(fn), "/%s.png", w.key);
  if (LittleFS.exists(fn)) return false;
  if (netSaverOn()) {   // saving data: at most eight logos in ten minutes, whatever the screens ask for
    static uint32_t winAt = 0;
    static int fetched = 0;
    if (!winAt || millis() - winAt > 600000UL) { winAt = millis() | 1; fetched = 0; }
    if (fetched >= 8) { pushWant(w); return false; }
    fetched++;
  }
  if (heap_caps_get_largest_free_block(MALLOC_CAP_8BIT) < 60000) { pushWant(w); delay(500); return true; }   // short of memory: later
  uint8_t* buf = (uint8_t*)heap_caps_malloc(MAX_PNG, MALLOC_CAP_8BIT);
  if (!buf) return true;
  bool ok = false;
  size_t n = 0;
  int code = 0;
  if (mnTlsTake(15000)) {
    ok = download(w.path, w.size, buf, n, code);
    if (!ok && code == 404 && strstr(w.path, "/500-dark/")) {   // no dark version: the ordinary one
      String alt = w.path;
      alt.replace("/500-dark/", "/500/");
      ok = download(alt, w.size, buf, n, code);
    }
    mnTlsGive();
  }
  bool room = ok && makeRoom(n, w.key);
  if (ok && !room) { mnLog("logo %s: no room on the board", w.key); ok = false; code = -2; }
  if (room) {
    String tmp = String(fn) + ".tmp";
    File f = LittleFS.open(tmp, "w");
    if (f) {
      bool wrote = f.write(buf, n) == n;
      f.close();
      if (wrote) { LittleFS.remove(fn); LittleFS.rename(tmp, fn); markRecent(w.key); version++; mnLog("logo %s saved (%u bytes)", w.key, (unsigned)n); }
      else LittleFS.remove(tmp);
    }
  } else if (!ok) {
    if (code != -2) mnLog("logo %s failed (HTTP %d)", w.key, code);
    portENTER_CRITICAL(&mux);
    static int fi = 0;
    strncpy(failed[fi & 7].key, w.key, 39);
    failed[fi & 7].at = millis();
    fi++;
    portEXIT_CRITICAL(&mux);
  }
  free(buf);
  return true;
}
#endif

// ------------------------------------------------------------- drawing
struct Ctx { PNG* png; int x0, y0, size; uint8_t br, bg, bb; bool keyed; bool clr[128]; lgfx::rgb565_t line[128]; };

static inline uint8_t mix(uint8_t f, uint8_t b, uint8_t a) { return (f * a + b * (255 - a) + 127) / 255; }

static int drawLine(PNGDRAW* d) {
  Ctx* c = (Ctx*)d->pUser;
  int w = d->iWidth > 128 ? 128 : d->iWidth;
  const uint8_t* p = d->pPixels;
  bool plain = (d->iPixelType == PNG_PIXEL_TRUECOLOR_ALPHA || d->iPixelType == PNG_PIXEL_TRUECOLOR ||
                d->iPixelType == PNG_PIXEL_INDEXED || d->iPixelType == PNG_PIXEL_GRAY_ALPHA) && d->iBpp == 8;
  if (!plain) {   // any other kind of PNG: the decoder's own conversion, blended onto the tile colour
    uint16_t tmp[128];
    c->png->getLineAsRGB565(d, tmp, PNG_RGB565_LITTLE_ENDIAN, ((uint32_t)c->br << 16) | ((uint32_t)c->bg << 8) | c->bb);
    for (int x = 0; x < w; x++) c->line[x] = lgfx::rgb565_t((uint16_t)tmp[x]);
    lcd.pushImage(c->x0, c->y0 + d->y, w, 1, c->line);
    return 1;
  }
  for (int x = 0; x < w; x++) {
    uint8_t r = c->br, g = c->bg, b = c->bb;
    bool clear = false;
    if (d->iPixelType == PNG_PIXEL_TRUECOLOR_ALPHA && d->iBpp == 8) {
      uint8_t a = p[x * 4 + 3];
      clear = a < 6;
      r = mix(p[x * 4], r, a); g = mix(p[x * 4 + 1], g, a); b = mix(p[x * 4 + 2], b, a);
    } else if (d->iPixelType == PNG_PIXEL_TRUECOLOR && d->iBpp == 8) {
      r = p[x * 3]; g = p[x * 3 + 1]; b = p[x * 3 + 2];
    } else if (d->iPixelType == PNG_PIXEL_INDEXED && d->iBpp == 8) {
      uint8_t i = p[x];
      uint8_t a = d->iHasAlpha ? d->pPalette[768 + i] : 255;
      clear = a < 6;
      r = mix(d->pPalette[i * 3], r, a); g = mix(d->pPalette[i * 3 + 1], g, a); b = mix(d->pPalette[i * 3 + 2], b, a);
    } else if (d->iPixelType == PNG_PIXEL_GRAY_ALPHA && d->iBpp == 8) {
      uint8_t a = p[x * 2 + 1];
      clear = a < 6;
      r = mix(p[x * 2], r, a); g = mix(p[x * 2], g, a); b = mix(p[x * 2], b, a);
    }
    c->line[x] = lgfx::rgb565_t(r, g, b);
    c->clr[x] = clear;
  }
  if (c->keyed) {   // over a picture: only the parts of the row that are not clear
    int x = 0;
    while (x < w) {
      while (x < w && c->clr[x]) x++;
      int st = x;
      while (x < w && !c->clr[x]) x++;
      if (x > st) lcd.pushImage(c->x0 + st, c->y0 + d->y, x - st, 1, c->line + st);
    }
    return 1;
  }
  lcd.pushImage(c->x0, c->y0 + d->y, w, 1, c->line);
  return 1;
}

static bool logoDrawImpl(const TeamSide& t, int cx, int cy, int size, uint16_t bg, bool keyed) {
  char key[40];
  if (!keyOf(t, size, key)) return false;
  size_t n = 0;
  uint8_t* buf = loadFile(key, n);
  if (!buf) { want(key, t, size); return false; }
  PNG* png = (PNG*)malloc(sizeof(PNG));
  if (!png) { free(buf); return false; }   // short of memory: the letters show; asked again next redraw
  new (png) PNG();
  bool ok = false;
  Ctx* c = (Ctx*)malloc(sizeof(Ctx));
  if (c && png->openRAM(buf, n, drawLine) == PNG_SUCCESS && png->getWidth() <= 128) {
    c->png = png;
    c->size = size;
    c->keyed = keyed;
    c->x0 = cx - png->getWidth() / 2;
    c->y0 = cy - png->getHeight() / 2;
    // the tile colour as 8-bit red, green, blue
    c->br = ((bg >> 11) & 31) * 255 / 31; c->bg = ((bg >> 5) & 63) * 255 / 63; c->bb = (bg & 31) * 255 / 31;
    ok = png->decode(c, 0) == PNG_SUCCESS;
    png->close();
  }
  free(c);
  png->~PNG();
  free(png);
  free(buf);
  return ok;
}

bool logoDraw(const TeamSide& t, int cx, int cy, int size, uint16_t bg) { return logoDrawImpl(t, cx, cy, size, bg, false); }
// over a picture: the logo's clear parts leave what is already on the screen alone (no square round it); its soft edges blend with bg
bool logoDrawOver(const TeamSide& t, int cx, int cy, int size, uint16_t bg) { return logoDrawImpl(t, cx, cy, size, bg, true); }
