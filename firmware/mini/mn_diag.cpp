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
}  // namespace

void diagBegin() {
  esp_reset_reason_t r = esp_reset_reason();
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
