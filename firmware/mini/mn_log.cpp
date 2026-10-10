#include "mn_log.h"
#include "mn_diag.h"
#include <stdarg.h>

static const int LINES = 60, LEN = 120;   // ~7 KB: no PSRAM on this board
static char ring[LINES][LEN];
static int head = 0, count = 0;
static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

void mnLog(const char* fmt, ...) {
  char line[LEN];
  time_t now = time(nullptr);
  int n = 0;
  if (now > 1700000000) {
    struct tm lt;
    localtime_r(&now, &lt);
    n = strftime(line, sizeof(line), "%H:%M:%S ", &lt);
  } else {
    n = snprintf(line, sizeof(line), "+%lus ", (unsigned long)(millis() / 1000));
  }
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(line + n, sizeof(line) - n, fmt, ap);
  va_end(ap);
  Serial.println(line);
  portENTER_CRITICAL(&mux);
  memcpy(ring[head], line, LEN);
  diagLogLine(line);
  head = (head + 1) % LINES;
  if (count < LINES) count++;
  portEXIT_CRITICAL(&mux);
}

String mnLogText() {
  String out;
  out.reserve(count * 60);
  for (int i = 0; i < count; i++) {
    char line[LEN];
    portENTER_CRITICAL(&mux);
    memcpy(line, ring[(head - count + i + LINES) % LINES], LEN);
    portEXIT_CRITICAL(&mux);
    line[LEN - 1] = 0;
    out += line;
    out += "\n";
  }
  return out;
}
