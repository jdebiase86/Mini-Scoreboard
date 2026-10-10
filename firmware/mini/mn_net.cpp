#include "mn_net.h"
#include "mn_espn.h"
#include "mn_logo.h"
#include "mn_settings.h"
#include "mn_tls.h"
#include "mn_gzip.h"
#include "mn_live.h"
#include "mn_log.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

// Reads the HTTPS answer in 2 KB gulps (byte by byte through TLS is far slower)
struct NetReader : ByteSource {
  WiFiClient* c;
  long remaining;
  uint8_t buf[2048];
  int pos = 0, len = 0;
  NetReader(WiFiClient* cl, long size) : c(cl), remaining(size) {}
  bool fill() {
    if (remaining == 0) return false;
    uint32_t t0 = millis();
    for (;;) {
      int a = c->available();
      if (a > 0) {
        int want = a < (int)sizeof(buf) ? a : sizeof(buf);
        if (remaining > 0 && want > remaining) want = remaining;
        int n = c->read(buf, want);
        if (n > 0) {
          pos = 0; len = n;
          if (remaining > 0) remaining -= n;
          return true;
        }
      }
      if (!c->connected() && c->available() <= 0) return false;
      if (millis() - t0 > 15000) return false;
      delay(2);
    }
  }
  int read() override {
    if (pos >= len && !fill()) return -1;
    return buf[pos++];
  }
  size_t readBytes(char* b, size_t n) override {
    size_t k = 0;
    while (k < n) {
      if (pos >= len && !fill()) break;
      size_t m = len - pos;
      if (m > n - k) m = n - k;
      memcpy(b + k, buf + pos, m);
      pos += m; k += m;
    }
    return k;
  }
};

// ----------------------------------------------------------------- state
// A download needs about 45 KB in a few pieces; the biggest free block is only 65 to 70 KB after a
// while (logos fragment the memory), so asking for more than this left the scores frozen.
static const size_t MIN_BLOCK = 48000;

struct Slot {
  int team = -1;             // TEAMS index this slot is for
  Game g;
  bool known = false;
  time_t dayAt = 0;          // feed day as local noon (0 = today / this week); later while the next game is further off
  uint32_t nextAt = 0;
  uint32_t okAt = 0;         // when this slot last got a good answer
  uint32_t wantUntil = 0;    // a details card is open (or was just): ask for the extras until then
  int fails = 0;
};
static Slot slots[MAX_PICKS];
static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
static volatile uint32_t version = 1;
static volatile bool picksDirty = true;

bool mnWifiUp() { return WiFi.status() == WL_CONNECTED; }
void netPicksChanged() { picksDirty = true; }
uint32_t netVersion() { return version + logoVersion(); }

// seconds since favourite `pick` last heard from ESPN (65535 = never)
uint32_t netAgeSecs(int pick) {
  if (pick < 0 || pick >= MAX_PICKS) return 65535;
  portENTER_CRITICAL(&mux);
  uint32_t at = slots[pick].okAt;
  portEXIT_CRITICAL(&mux);
  return at ? (millis() - at) / 1000 : 65535;
}

// The details cards need extras that cost memory to read, so they're only
// asked for while a card is open: the first call fetches them right away.
void netWantDetails(int pick) {
  if (pick < 0 || pick >= MAX_PICKS) return;
  portENTER_CRITICAL(&mux);
  Slot& s = slots[pick];
  if (!(s.known && s.g.det.has)) s.nextAt = millis();
  s.wantUntil = millis() + 90000;
  portEXIT_CRITICAL(&mux);
}

bool netGame(int pick, Game& out) {
  if (pick < 0 || pick >= MAX_PICKS) return false;
  portENTER_CRITICAL(&mux);
  bool k = slots[pick].known;
  if (k) out = slots[pick].g;
  portEXIT_CRITICAL(&mux);
  return k;
}

static bool daily(League l) { return l == L_MLB || l == L_NHL || l == L_NBA; }

static void dayString(time_t t, char (&out)[9]) {
  struct tm lt;
  localtime_r(&t, &lt);
  snprintf(out, sizeof(out), "%04d%02d%02d", lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday);
}

static time_t noonToday(time_t now) {
  struct tm lt;
  localtime_r(&now, &lt);
  lt.tm_hour = 12; lt.tm_min = 0; lt.tm_sec = 0;
  return mktime(&lt);
}

static void rebuild() {
  portENTER_CRITICAL(&mux);
  for (int i = 0; i < MAX_PICKS; i++) {
    int t = i < settings.npicks ? settings.picks[i] : -1;
    if (slots[i].team != t) { slots[i] = Slot(); slots[i].team = t; }
  }
  version++;
  portEXIT_CRITICAL(&mux);
}

// Downloads `url` and hands the bytes (unpacked if ESPN sent them compressed) to read().
// The compressed way needs about 44 KB, which is set aside *before* the connection
// takes its share (the biggest free block is only about 70 KB). needGz: for the
// game page (up to 1 MB), which is never read uncompressed.
typedef bool (*Reader)(ByteSource& src, void* ctx);
static uint32_t gzOffUntil = 0;
static int gzMisses = 0;

static bool fetchStream(const String& url, Reader read, void* ctx, bool needGz = false) {
  if (!mnTlsTake(20000)) return false;
  bool ok = false;
  for (int attempt = 0; attempt < 2 && !ok; attempt++) {
    // attempt 0: memory set aside first. attempt 1: (only if the first couldn't connect) plain, or
    // for the game page, the compressed memory taken after the connection is up
    const bool gzAllowed = (int32_t)(millis() - gzOffUntil) >= 0;
    GzSource* gz = attempt == 0 && gzAllowed ? new GzSource() : nullptr;
    if (gz && !gz->ok()) { delete gz; gz = nullptr; }
    if (attempt == 0 && !gz && needGz) continue;   // no room set aside: try the late way
    if (attempt == 1 && needGz && !gzAllowed) break;
    const bool askGz = gz || (attempt == 1 && needGz);
    bool reached = false;
    {
      WiFiClientSecure client;
      client.setInsecure();
      HTTPClient http;
      http.useHTTP10(true);
      http.setReuse(false);
      http.setConnectTimeout(10000);
      http.setTimeout(15000);
      if (http.begin(client, url)) {
        http.setUserAgent("Mozilla/5.0 (Mini Scoreboard)");
        if (askGz) http.addHeader("Accept-Encoding", "gzip");
        const char* keep[] = {"Content-Encoding"};
        http.collectHeaders(keep, 1);
        int code = http.GET();
        reached = code > 0;
        if (code == 200) {
          NetReader r(http.getStreamPtr(), http.getSize());
          bool packed = askGz && http.header("Content-Encoding").indexOf("gzip") >= 0;
          if (packed && !gz) gz = new GzSource();
          if (packed && gz && gz->ok() && gz->begin(r)) {
            ok = read(*gz, ctx) && !gz->failed();
            if (!ok) mnLog("scores: gzip answer didn't read");
          } else if (!packed && !needGz) {
            ok = read(r, ctx);
          } else {
            mnLog("scores: couldn't unpack the answer");
          }
        } else if (code > 0) {
          mnLog("scores: ESPN answered %d", code);
        }
        http.end();
      }
    }
    bool usedGz = gz != nullptr;
    delete gz;
    if (attempt == 0 && usedGz && !reached) {   // the memory set aside left too little to connect
      if (++gzMisses >= 3) { gzOffUntil = millis() + 600000; gzMisses = 0; mnLog("scores: compressed downloads off for 10 minutes"); }
    } else if (ok) {
      gzMisses = 0;
    }
    if (reached) break;
  }
  mnTlsGive();
  return ok;
}

struct FeedCtx { JsonDocument* doc; bool rich; };
static bool readFeed(ByteSource& src, void* c) {
  FeedCtx* f = (FeedCtx*)c;
  return espnLoad(src, *f->doc, f->rich);
}
static bool fetchFeed(const String& url, JsonDocument& doc, bool rich) {
  FeedCtx c{&doc, rich};
  return fetchStream(url, readFeed, &c);
}

// ---------------------------------------------------- the live page of one game
// While a details card is open the mini also reads the game's own page for the
// team stats, leaders so far and the last play (mn_live). One game at a time.
static LiveInfo liveBuf;
static int livePick = -1;
static uint32_t liveUntil = 0, liveNextAt = 0;
static char liveId[12] = "";

void netWantLive(int pick) {
  if (pick < 0 || pick >= MAX_PICKS) return;
  portENTER_CRITICAL(&mux);
  if (livePick != pick) { livePick = pick; liveBuf = LiveInfo(); liveId[0] = 0; liveNextAt = millis(); }
  else if ((int32_t)(millis() - liveUntil) >= 0) liveNextAt = millis();   // it had lapsed
  liveUntil = millis() + 60000;
  portEXIT_CRITICAL(&mux);
}

bool netLive(int pick, LiveInfo& out) {
  portENTER_CRITICAL(&mux);
  bool ok = livePick == pick && liveBuf.has;
  if (ok) out = liveBuf;
  portEXIT_CRITICAL(&mux);
  return ok;
}

struct LiveCtx { const Game* g; LiveInfo* out; };
static bool readLive(ByteSource& src, void* c) {
  LiveCtx* l = (LiveCtx*)c;
  return liveParse(src, *l->g, *l->out);
}

// true when it fetched (or tried to)
static bool liveStep() {
  uint32_t ms = millis();
  if (livePick < 0 || (int32_t)(ms - liveUntil) >= 0 || (int32_t)(ms - liveNextAt) < 0) return false;
  Game g;
  if (!netGame(livePick, g) || !g.id[0] || g.state == GS_NONE || g.state == GS_PRE) { liveNextAt = ms + 5000; return false; }
  if (ESP.getMaxAllocHeap() < MIN_BLOCK) { liveNextAt = ms + 3000; return false; }
  static LiveInfo fresh;   // (not on the task's small stack)
  LiveCtx c{&g, &fresh};
  uint32_t t0 = millis();
  bool ok = fetchStream(espnSummaryUrl(g.league, g.id), readLive, &c, true);
  portENTER_CRITICAL(&mux);
  if (ok && livePick >= 0) {
    strncpy(fresh.id, g.id, sizeof(fresh.id) - 1);
    bool changed = !liveBuf.has || liveBuf.sig != fresh.sig;
    liveBuf = fresh;
    if (changed) version++;
    liveNextAt = millis() + (g.state == GS_LIVE ? 12000 : 300000);   // a final doesn't change
  } else {
    liveNextAt = millis() + 8000;
  }
  portEXIT_CRITICAL(&mux);
  mnLog("live: %s %s in %lu ms, heap %u KB", g.id, ok ? "ok" : "failed", (unsigned long)(millis() - t0),
        (unsigned)(ESP.getFreeHeap() / 1024));
  return true;
}

// how soon to look again
//   live game: every 5 seconds (ESPN itself only refreshes about that often)
//   the last 10 minutes before the start: every 30 seconds
//   a game that just ended: every 10 minutes for a few hours (a late correction)
//   otherwise: every hour, and early enough to be watching 10 minutes before a start
static uint32_t interval(const Game& g, time_t now) {
  const uint32_t HOUR = 3600000UL;
  if (g.state == GS_LIVE) return 5000;
  if (g.state == GS_PRE) {
    long toStart = (long)(g.start - now);
    if (toStart <= 600) return 30000;   // includes a start that ESPN hasn't called "live" yet
    uint32_t wait = (uint32_t)(toStart - 600) * 1000UL;
    return wait < HOUR ? wait : HOUR;
  }
  if (g.state == GS_POST) return (now - g.start) < 5 * 3600 ? 10 * 60000UL : HOUR;
  return HOUR;
}

static void netTask(void*) {
  for (;;) {
    delay(300);
    if (WiFi.status() != WL_CONNECTED || time(nullptr) < 1700000000) continue;
    if (picksDirty) { picksDirty = false; rebuild(); }
    if (logoFetchOne()) { delay(200); continue; }
    if (liveStep()) { delay(100); continue; }

    // the favourite that's most overdue
    uint32_t ms = millis();
    int pick = -1;
    for (int i = 0; i < MAX_PICKS; i++)
      if (slots[i].team >= 0 && (int32_t)(ms - slots[i].nextAt) >= 0 &&
          (pick < 0 || (int32_t)(slots[i].nextAt - slots[pick].nextAt) < 0))
        pick = i;
    if (pick < 0) continue;
    if (ESP.getMaxAllocHeap() < MIN_BLOCK) {   // not enough room for a download: wait (restarting would clear it)
      static uint32_t loggedAt = 0;
      if (millis() - loggedAt > 30000) { loggedAt = millis(); mnLog("scores: waiting for memory (biggest block %u KB)", (unsigned)(ESP.getMaxAllocHeap() / 1024)); }
      delay(1000);
      continue;
    }

    const League lg = TEAMS[slots[pick].team].league;
    const int group = TEAMS[slots[pick].team].group;
    const time_t dayAt = slots[pick].dayAt;
    char day[9] = "";
    if (dayAt) dayString(dayAt, day);
    uint32_t t0 = millis();
    bool ok;
    {
      // extras for the details cards, if one is open for anyone on this feed
      bool rich = false;
      for (int i = 0; i < MAX_PICKS; i++) {
        const Slot& s = slots[i];
        if (s.team >= 0 && TEAMS[s.team].league == lg && TEAMS[s.team].group == group && s.dayAt == dayAt &&
            (int32_t)(s.wantUntil - ms) > 0)
          rich = true;
      }
      JsonDocument doc;
      ok = fetchFeed(espnUrl(lg, group, dayAt ? day : nullptr), doc, rich);
      time_t now = time(nullptr);
      if (ok) {
        // everyone who watches this same feed gets their game from it
        for (int i = 0; i < MAX_PICKS; i++) {
          Slot& s = slots[i];
          if (s.team < 0 || TEAMS[s.team].league != lg || TEAMS[s.team].group != group || s.dayAt != dayAt) continue;
          Game g;
          uint32_t next = 3600000UL;
          time_t newDay = s.dayAt;
          if (espnFind(doc, s.team, now, g)) {
            next = interval(g, now);
            // a final long past, found on a day we looked ahead to: back to today's feed
            if (g.state == GS_POST && s.dayAt && now - g.start > 12 * 3600) { newDay = 0; next = 1000; }
          } else if (daily(lg)) {
            // no game that day: look at the next one, up to a week ahead
            time_t cand = (s.dayAt ? s.dayAt : noonToday(now)) + 86400;
            if (cand - now <= 7 * 86400) { newDay = cand; next = 1500; }
            else newDay = 0;
          }
          portENTER_CRITICAL(&mux);
          bool changed = !s.known || !s.g.same(g);
          s.g = g;
          s.known = true;
          s.fails = 0;
          s.dayAt = newDay;
          s.nextAt = millis() + next;
          s.okAt = millis() | 1;
          if (changed) version++;
          portEXIT_CRITICAL(&mux);
        }
      }
    }
    if (!ok) {
      Slot& s = slots[pick];
      s.fails = min(s.fails + 1, 5);
      s.nextAt = millis() + (10000UL << (s.fails - 1));   // 10 s, 20 s ... 160 s
      mnLog("scores: %s failed (%d in a row)", TEAMS[s.team].abbr, s.fails);
    } else {
      mnLog("scores: %s ok in %lu ms, heap %u KB (biggest block %u KB)", TEAMS[slots[pick].team].abbr,
            (unsigned long)(millis() - t0), (unsigned)(ESP.getFreeHeap() / 1024),
            (unsigned)(ESP.getMaxAllocHeap() / 1024));
    }
  }
}

void netStart() {
  static bool started = false;
  if (started) return;
  started = true;
  logoBegin();
  xTaskCreatePinnedToCore(netTask, "net", 12288, nullptr, 1, nullptr, 0);
}
