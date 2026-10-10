#include "mn_about.h"
#include "mn_ui.h"
#include "mn_lcd.h"
#include "mn_diag.h"

static int page = 0;   // 0 = status, 1 = the lines before the last restart

static String since(uint32_t s) {
  if (s < 90) return String(s) + " s";
  if (s < 5400) return String((s + 30) / 60) + " min";
  uint32_t h = s / 3600, m = s % 3600 / 60;
  if (h < 48) return String(h) + "h " + String(m) + "m";
  return String(h / 24) + " d " + String(h % 24) + "h";
}

static void topBar() {
  lcd.fillRect(0, 0, 480, 34, C_BG);
  uiTile(6, 3, 96, 29, C_TILE, C_EDGE, 13, 1);
  lcd.fillTriangle(18, 16, 26, 9, 26, 23, C_WHITE);
  uiText(F_B12, "BACK", 60, 16, C_WHITE, C_TILE, middle_center);
  uiText(F_B16, "About", 112, 16, C_WHITE, C_BG, middle_left);
  uiTile(316, 3, 474, 29, C_TILE, C_EDGE, 13, 1);
  uiText(F_B12, page == 0 ? "BEFORE RESTART" : "STATUS", 395, 16, C_WHITE, C_TILE, middle_center);
}

static void row(int y, const char* label, const String& value, uint16_t col = C_WHITE) {
  lcd.fillRect(0, y - 11, 480, 22, C_BG);
  uiText(F_B12, label, 12, y, C_GREY, C_BG, middle_left);
  String v = value;
  useFont(F_M15);
  while (v.length() > 6 && lcd.textWidth(v.c_str()) > 340) v = v.substring(0, v.length() - 1);
  uiText(F_M15, v, 124, y, col, C_BG, middle_left);
}

static const char* wifiWord(int rssi) { return rssi >= -60 ? "strong" : rssi >= -70 ? "good" : rssi >= -80 ? "weak" : "very weak"; }

static void numbers(const AboutData& d) {
  row(50, "VERSION", d.version + "   (running " + since(d.upSecs) + ")");
  String why = diagRestartWhy();
  if (diagPrevRunSecs() > 30) why += "   (ran " + since(diagPrevRunSecs()) + " first)";
  row(76, "LAST RESTART", why, diagRestartBad() ? C_RED : C_WHITE);
  String w = d.rssi ? d.ssid + "   " + wifiWord(d.rssi) + " (" + String(d.rssi) + " dBm)" : String("not connected");
  row(102, "WI-FI", w, d.rssi ? C_WHITE : C_YELLOW);
  row(128, "MEMORY", String(d.freeKb) + " KB free, biggest piece " + String(d.biggestKb) + " KB", d.biggestKb < 40 ? C_YELLOW : C_WHITE);
  String b = d.pct < 0 ? String("no battery") : String(d.mv) + " mV   " + String(d.pct) + "%   " + (d.charging ? "charging" : "on the battery");
  row(154, "BATTERY", b);
  row(180, "DATA", d.saver ? String("saving data (phone hotspot)") : String("normal"), C_WHITE);
}

// the battery's last day as a line: white on the battery, green while it thought it was charging
static void graph() {
  const int X0 = 56, X1 = 470, Y0 = 228, Y1 = 294;
  lcd.fillRect(0, 196, 480, 124, C_BG);   // (the rows above end at y 191)
  uiText(F_B12, "BATTERY, LAST 24 HOURS", 12, 207, C_GREY, C_BG, middle_left);
  const int lo = 3300, hi = 4200;
  for (int v = 3300; v <= 4200; v += 300) {
    int y = Y1 - (v - lo) * (Y1 - Y0) / (hi - lo);
    lcd.drawFastHLine(X0, y, X1 - X0, C_DIM);
    uiText(F_S13, String(v / 1000) + "." + String(v % 1000 / 100) + "V", X0 - 6, y, C_GREY, C_BG, middle_right);
  }
  int n = diagBatCount();
  if (n < 2) { uiText(F_S13, "collecting... (a point every 5 minutes)", 260, 255, C_GREY, C_BG, middle_center); return; }
  int px = -1, py = -1;
  uint32_t lastMin = 0;
  for (int i = n - 1; i >= 0 && !lastMin; i--) lastMin = diagBatAt(i).minute;
  for (int i = 0; i < n; i++) {
    BatSample s = diagBatAt(i);
    int x;
    if (lastMin && s.minute) {   // by the time of day
      int age = (int)(lastMin - s.minute);   // minutes
      x = X1 - age * (X1 - X0) / 1440;
    } else {
      x = X1 - (n - 1 - i) * (X1 - X0) / DIAG_BAT_N;
    }
    if (x < X0) { px = -1; continue; }
    int v = s.mv < lo ? lo : s.mv > hi ? hi : s.mv;
    int y = Y1 - (v - lo) * (Y1 - Y0) / (hi - lo);
    uint16_t c = (s.flags & 1) ? C_GREEN : C_WHITE;
    if (px >= 0 && x - px < 40) lcd.drawLine(px, py, x, y, c);
    px = x; py = y;
  }
  if (px >= 0) lcd.fillCircle(px, py, 3, C_YELLOW);
  uiText(F_S13, "24 h ago", X0, 308, C_GREY, C_BG, middle_left);
  uiText(F_S13, "now", X1, 308, C_GREY, C_BG, middle_right);
}

static void lines() {
  lcd.fillRect(0, 36, 480, 284, C_BG);
  uiText(F_B12, "THE LAST LOG LINES BEFORE THE RESTART", 12, 48, C_GREY, C_BG, middle_left);
  int n = diagPrevLineCount();
  if (!n) { uiText(F_M15, "Nothing kept (the power was cut, or this is the first start).", 12, 90, C_GREY, C_BG, middle_left); return; }
  for (int i = 0; i < n; i++) {
    String s = diagPrevLine(i);
    useFont(F_S13);
    while (s.length() > 6 && lcd.textWidth(s.c_str()) > 456) s = s.substring(0, s.length() - 1);
    uiText(F_S13, s, 12, 66 + i * 22, i == n - 1 ? C_WHITE : C_GREY, C_BG, middle_left);
  }
}

void aboutOpen(const AboutData& d) {
  page = 0;
  lcd.fillScreen(C_BG);
  topBar();
  numbers(d);
  graph();
}

void aboutRefresh(const AboutData& d) {
  if (page == 0) numbers(d);
}

bool aboutTap(int x, int y, const AboutData& d) {
  if (x < 104 && y < 38) return true;
  if (x >= 308 && y < 38) {
    page = 1 - page;
    lcd.fillScreen(C_BG);
    topBar();
    if (page == 0) { numbers(d); graph(); } else lines();
  }
  return false;
}
