#include "mn_diag.h"
#include <esp_system.h>
#include <Preferences.h>
#include <time.h>

namespace {
const uint32_t MAGIC = 0x4D4E4431;   // "MND1"

struct Rtc {
  uint32_t magic;
  uint32_t boots;
  uint32_t aliveSecs;
  char note[48];
  uint8_t lhead, lcount;
  char lines[DIAG_LINES][DIAG_LEN];
  uint16_t bhead, bcount;
  BatSample bat[DIAG_BAT_N];
};
RTC_NOINIT_ATTR Rtc rtc;
bool ready = false;
bool batDirty = false;

// the run before this one
esp_reset_reason_t prevReason = ESP_RST_UNKNOWN;
char prevNote[48] = "";
uint32_t prevRun = 0;
char prevLines[DIAG_LINES][DIAG_LEN];
int prevCount = 0;

void loadHistory() {   // after a power cut: what was saved to flash
  Preferences p;
  if (!p.begin("diag", true)) return;
  size_t n = p.getBytesLength("bat");
  if (n == sizeof(rtc.bat) + 4) {
    uint8_t* buf = (uint8_t*)malloc(n);
    if (buf && p.getBytes("bat", buf, n) == n) {
      uint16_t head, count;
      memcpy(&head, buf, 2); memcpy(&count, buf + 2, 2);
      if (head < DIAG_BAT_N && count <= DIAG_BAT_N) { rtc.bhead = head; rtc.bcount = count; memcpy(rtc.bat, buf + 4, sizeof(rtc.bat)); }
    }
    free(buf);
  }
  p.end();
}

void saveHistory() {
  uint8_t* buf = (uint8_t*)malloc(sizeof(rtc.bat) + 4);
  if (!buf) return;
  memcpy(buf, &rtc.bhead, 2); memcpy(buf + 2, &rtc.bcount, 2); memcpy(buf + 4, rtc.bat, sizeof(rtc.bat));
  Preferences p;
  if (p.begin("diag", false)) { p.putBytes("bat", buf, sizeof(rtc.bat) + 4); p.end(); }
  free(buf);
}

// the last ten restarts, kept in flash (written at every start, so a bad moment is never lost to the next restart)
struct Rec { uint32_t n; uint8_t reason; uint8_t bad; uint16_t ranMins; char note[28]; };
const int HIST_N = 10;
Rec hist[HIST_N];
int histCount = 0;                      // newest last
struct Crash { uint32_t n; uint8_t count; char lines[DIAG_LINES][DIAG_LEN]; };

String reasonText(uint8_t r, const char* note) {
  switch ((esp_reset_reason_t)r) {
    case ESP_RST_POWERON: return "Power on";
    case ESP_RST_SW: return note[0] ? String("On purpose - ") + note : String("Software restart");
    case ESP_RST_PANIC: return "CRASH";
    case ESP_RST_INT_WDT: case ESP_RST_TASK_WDT: case ESP_RST_WDT: return "STUCK (watchdog)";
    case ESP_RST_BROWNOUT: return "Power dip";
    default: return "Restart (unknown)";
  }
}

void loadRestarts() {
  Preferences p;
  if (!p.begin("diag", true)) return;
  size_t n = p.getBytesLength("rst");
  if (n && n % sizeof(Rec) == 0 && n <= sizeof(hist)) { p.getBytes("rst", hist, n); histCount = n / sizeof(Rec); }
  p.end();
}

void saveRestarts(const Crash* crash) {   // the crash record is built and kept in flash only; it is not held in memory
  Preferences p;
  if (!p.begin("diag", false)) return;
  p.putBytes("rst", hist, histCount * sizeof(Rec));
  if (crash) p.putBytes("crash", crash, sizeof(Crash));
  p.end();
}
}  // namespace

void diagBegin() {
  esp_reset_reason_t r = esp_reset_reason();
  esp_reset_reason_t r_ = r;
  bool fresh = r == ESP_RST_POWERON || rtc.magic != MAGIC || rtc.lhead >= DIAG_LINES || rtc.lcount > DIAG_LINES ||
               rtc.bhead >= DIAG_BAT_N || rtc.bcount > DIAG_BAT_N;
  prevReason = r;
  if (!fresh) {
    prevRun = rtc.aliveSecs;
    strncpy(prevNote, rtc.note, sizeof(prevNote) - 1);
    prevCount = rtc.lcount;
    for (int i = 0; i < prevCount; i++) {
      memcpy(prevLines[i], rtc.lines[(rtc.lhead - prevCount + i + DIAG_LINES) % DIAG_LINES], DIAG_LEN);
      prevLines[i][DIAG_LEN - 1] = 0;
    }
  } else {
    memset(&rtc, 0, sizeof(rtc));
    rtc.magic = MAGIC;
    loadHistory();
  }
  loadRestarts();
  {   // add the run that just ended to the flash history
    uint32_t n = (histCount ? hist[histCount - 1].n : 0) + 1;
    Rec r = {};
    r.n = n; r.reason = (uint8_t)r_; r.ranMins = (uint16_t)(prevRun / 60 > 65535 ? 65535 : prevRun / 60);
    strncpy(r.note, fresh ? "" : prevNote, sizeof(r.note) - 1);
    r.bad = diagRestartBad();
    if (histCount == HIST_N) { memmove(hist, hist + 1, sizeof(Rec) * (HIST_N - 1)); histCount--; }
    hist[histCount++] = r;
    bool bad = r.bad && !fresh && prevCount > 0;
    Crash* cr = bad ? (Crash*)calloc(1, sizeof(Crash)) : nullptr;
    if (cr) {
      cr->n = n; cr->count = (uint8_t)prevCount;
      for (int i = 0; i < prevCount; i++) memcpy(cr->lines[i], prevLines[i], DIAG_LEN);
    }
    saveRestarts(cr);
    free(cr);
  }
  rtc.boots++;
  rtc.aliveSecs = 0;
  rtc.note[0] = 0;
  rtc.lhead = 0; rtc.lcount = 0;
  ready = true;
}

void diagLogLine(const char* line) {
  if (!ready) return;
  strncpy(rtc.lines[rtc.lhead], line, DIAG_LEN - 1);
  rtc.lines[rtc.lhead][DIAG_LEN - 1] = 0;
  rtc.lhead = (rtc.lhead + 1) % DIAG_LINES;
  if (rtc.lcount < DIAG_LINES) rtc.lcount++;
}

void diagNote(const char* why) {
  if (!ready) return;
  strncpy(rtc.note, why, sizeof(rtc.note) - 1);
  rtc.note[sizeof(rtc.note) - 1] = 0;
}

void diagTick() {
  static uint32_t aliveAt = 0, savedAt = 0;
  if (!ready) return;
  uint32_t ms = millis();
  if (ms - aliveAt >= 20000) { aliveAt = ms; rtc.aliveSecs = ms / 1000; }
  if (batDirty && ms - savedAt >= 3600000UL) { savedAt = ms; batDirty = false; saveHistory(); }
}

String diagRestartWhy() {
  switch (prevReason) {
    case ESP_RST_POWERON: return "Power on (it had lost power)";
    case ESP_RST_SW: return prevNote[0] ? String("On purpose - ") + prevNote : String("Restarted by the software");
    case ESP_RST_PANIC: return "Crash (a program error)";
    case ESP_RST_INT_WDT: case ESP_RST_TASK_WDT: case ESP_RST_WDT: return "Stuck (watchdog restart)";
    case ESP_RST_BROWNOUT: return "Power dip";
    case ESP_RST_DEEPSLEEP: return "Woke from sleep";
    default: return "Restarted (reason unknown)";
  }
}

bool diagRestartBad() {
  return prevReason == ESP_RST_PANIC || prevReason == ESP_RST_INT_WDT || prevReason == ESP_RST_TASK_WDT ||
         prevReason == ESP_RST_WDT || prevReason == ESP_RST_BROWNOUT;
}

uint32_t diagPrevRunSecs() { return prevRun; }
uint32_t diagBoots() { return ready ? rtc.boots : 0; }
int diagPrevLineCount() { return prevCount; }
const char* diagPrevLine(int i) { return i >= 0 && i < prevCount ? prevLines[i] : ""; }

void diagBatSample(int mv, int pct, bool charging) {
  if (!ready) return;
  time_t now = time(nullptr);
  BatSample s;
  s.minute = now > 1700000000 ? (uint32_t)(now / 60) : 0;
  s.mv = (uint16_t)mv;
  s.pct = (uint8_t)(pct < 0 ? 0 : pct > 100 ? 100 : pct);
  s.flags = charging ? 1 : 0;
  rtc.bat[rtc.bhead] = s;
  rtc.bhead = (rtc.bhead + 1) % DIAG_BAT_N;
  if (rtc.bcount < DIAG_BAT_N) rtc.bcount++;
  batDirty = true;
}

int diagBatCount() { return ready ? rtc.bcount : 0; }
BatSample diagBatAt(int i) {
  if (!ready || i < 0 || i >= rtc.bcount) return BatSample{0, 0, 0, 0};
  return rtc.bat[(rtc.bhead - rtc.bcount + i + DIAG_BAT_N) % DIAG_BAT_N];
}

String diagBatCsv() {
  String out = "time,millivolts,percent,charging_guess\n";
  out.reserve(rtc.bcount * 32 + 60);
  for (int i = 0; i < diagBatCount(); i++) {
    BatSample s = diagBatAt(i);
    char line[48];
    if (s.minute) {
      time_t t = (time_t)s.minute * 60;
      struct tm lt;
      localtime_r(&t, &lt);
      snprintf(line, sizeof(line), "%04d-%02d-%02d %02d:%02d,%u,%u,%u\n", lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday, lt.tm_hour,
               lt.tm_min, s.mv, s.pct, s.flags & 1);
    } else {
      snprintf(line, sizeof(line), "?,%u,%u,%u\n", s.mv, s.pct, s.flags & 1);
    }
    out += line;
  }
  return out;
}

int diagRestartCount() { return histCount; }
String diagRestartLine(int i) {   // 0 = the newest
  if (i < 0 || i >= histCount) return "";
  const Rec& r = hist[histCount - 1 - i];
  String s = String("#") + String(r.n) + "  " + reasonText(r.reason, r.note);
  if (r.ranMins) s += String("  (ran ") + (r.ranMins >= 60 ? String(r.ranMins / 60) + "h " + String(r.ranMins % 60) + "m" : String(r.ranMins) + "m") + ")";
  return s;
}
bool diagRestartBadAt(int i) { return i >= 0 && i < histCount && hist[histCount - 1 - i].bad; }
String diagCrashText() {   // the log lines before the last crash or stuck restart (read back from flash when asked for)
  Preferences p;
  if (!p.begin("diag", true)) return "";
  String t;
  if (p.getBytesLength("crash") == sizeof(Crash)) {
    Crash* cr = (Crash*)malloc(sizeof(Crash));
    if (cr && p.getBytes("crash", cr, sizeof(Crash)) == sizeof(Crash)) {
      t = String("LOG BEFORE THE LAST BAD RESTART (#") + String(cr->n) + ")\n";
      for (int i = 0; i < cr->count && i < DIAG_LINES; i++) { cr->lines[i][DIAG_LEN - 1] = 0; t += String(cr->lines[i]) + "\n"; }
    }
    free(cr);
  }
  p.end();
  return t;
}
