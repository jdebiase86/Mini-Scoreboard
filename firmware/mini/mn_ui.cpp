#include "mn_ui.h"
#include "mn_lcd.h"
#include "mn_settings.h"
#include "mn_version.h"
#include "mn_net.h"
#include "mn_play.h"
#include "mn_game.h"
#include "mn_battery.h"
#include <time.h>

// ------------------------------------------------------------------ helpers
static void text(FontId f, const String& s, int x, int y, uint16_t col, uint16_t bg, textdatum_t datum) {
  useFont(f);
  lcd.setTextColor(col, bg);
  lcd.setTextDatum(datum);
  lcd.drawString(s.c_str(), x, y);
}

static void tile(int x0, int y0, int x1, int y1, uint16_t fill, uint16_t edge, int r = 14, int width = 1) {
  lcd.fillRoundRect(x0, y0, x1 - x0, y1 - y0, r, fill);
  for (int i = 0; i < width; i++) lcd.drawRoundRect(x0 + i, y0 + i, x1 - x0 - 2 * i, y1 - y0 - 2 * i, r - i, edge);
}

static void homeIcon(int cx, int cy, int r, uint16_t col, uint16_t bg) {
  lcd.fillTriangle(cx - r, cy - 1, cx, cy - r, cx + r, cy - 1, col);
  lcd.fillRect(cx - r * 65 / 100, cy - 2, r * 130 / 100 + 1, r * 80 / 100 + 3, col);
  lcd.fillRect(cx - 3, cy + 3, 7, r * 80 / 100 - 2, bg);
}

// two curved arrows chasing each other (the mock-up's AUTO icon)
static void autoIcon(int cx, int cy, int r, uint16_t col, int w) {
  lcd.fillArc(cx, cy, r + w / 2, r - w / 2, 200, 340, col);
  lcd.fillArc(cx, cy, r + w / 2, r - w / 2, 20, 160, col);
  for (int ang : {340, 160}) {
    float a = ang * DEG_TO_RAD, t = (ang + 90) * DEG_TO_RAD;
    float px = cx + r * cosf(a), py = cy + r * sinf(a);
    float s = w * 2.0f;
    lcd.fillTriangle(px + s * cosf(t), py + s * sinf(t), px + s * cosf(a), py + s * sinf(a), px - s * cosf(a),
                     py - s * sinf(a), col);
  }
}

void uiText(FontId f, const String& s, int x, int y, uint16_t col, uint16_t bg, textdatum_t datum) {
  text(f, s, x, y, col, bg, datum);
}
void uiTile(int x0, int y0, int x1, int y1, uint16_t fill, uint16_t edge, int r, int width) {
  tile(x0, y0, x1, y1, fill, edge, r, width);
}

static void centre(FontId f, const String& s, int y, uint16_t col) {
  text(f, s, SCREEN_W / 2, y, col, C_BG, middle_center);
}

// ------------------------------------------------------------------ screens
void uiSplash() {
  lcd.fillScreen(C_BG);
  centre(F_B36, "MINI SCOREBOARD", 140, C_WHITE);
  centre(F_M15, "Version " FW_VERSION, 185, C_GREY);
}

void uiSetup(const String& apName, bool cantJoin, const String& ssid) {
  lcd.fillScreen(C_BG);
  int y = 34;
  if (cantJoin) {
    text(F_B18, "Can't join " + ssid.substring(0, 18), 20, y, C_RED, C_BG, middle_left);
    text(F_M12, "Still trying. Or set it up again:", 20, y + 24, C_GREY, C_BG, middle_left);
    y += 60;
  } else {
    text(F_B24, "Let's set up", 20, y, C_WHITE, C_BG, middle_left);
    text(F_B24, "your scoreboard", 20, y + 32, C_WHITE, C_BG, middle_left);
    y += 76;
  }
  // steps on the left
  struct { const char* n; const char* a; const char* b; } steps[] = {
      {"1", "Point your phone's camera", "at the code. Tap Join."},
      {"2", "A setup page opens. Pick", "your Wi-Fi and your teams."},
  };
  for (auto& s : steps) {
    lcd.fillCircle(34, y + 12, 13, C_AUTO_BG);
    lcd.drawCircle(34, y + 12, 13, C_AUTO_EDGE);
    text(F_B16, s.n, 34, y + 13, C_WHITE, C_AUTO_BG, middle_center);
    text(F_M15, s.a, 58, y + 2, C_WHITE, C_BG, middle_left);
    text(F_M15, s.b, 58, y + 22, C_WHITE, C_BG, middle_left);
    y += 54;
  }
  // no camera: the long way
  tile(14, 234, 300, 312, C_TILE, C_EDGE, 12);
  text(F_M12, "No camera? Join the Wi-Fi network", 26, 250, C_GREY, C_TILE, middle_left);
  text(F_B18, apName, 26, 273, C_YELLOW, C_TILE, middle_left);
  text(F_M12, "then open 192.168.4.1 in the browser", 26, 296, C_GREY, C_TILE, middle_left);
  // the code: joins the open setup network
  tile(312, 70, 468, 226, C_WHITE, C_WHITE, 12);
  String q = "WIFI:T:nopass;S:" + apName + ";;";
  lcd.qrcode(q.c_str(), 322, 80, 136, 3);
  text(F_S13, "Scan me", 390, 244, C_GREY, C_BG, middle_center);
  // or pick the network right here on the mini
  tile(312, 262, 468, 304, C_TILE, C_EDGE, 12);
  text(F_B12, "WI-FI LIST", 390, 275, C_WHITE, C_TILE, middle_center);
  text(F_S13, "pick it on this screen", 390, 292, C_GREY, C_TILE, middle_center);
}

bool uiSetupWifiHit(int x, int y) { return x >= 304 && y >= 254; }

void uiJoining(const String& ssid) {
  static uint32_t lastCall = 0;
  if (!lastCall || millis() - lastCall > 500) {   // just arrived on this screen
    lcd.fillScreen(C_BG);
    centre(F_B24, "Joining Wi-Fi", 130, C_WHITE);
    centre(F_B18, ssid.substring(0, 28), 170, C_YELLOW);
  }
  lastCall = millis();
  int n = (millis() / 400) % 4;
  for (int i = 0; i < 3; i++) lcd.fillCircle(222 + i * 18, 215, 5, i < n ? C_WHITE : C_TILE);
}

void uiConnected(const String& ip) {
  lcd.fillScreen(C_BG);
  centre(F_B36, "Connected", 110, C_GREEN);
  centre(F_M15, "Change teams and settings from any", 165, C_GREY);
  centre(F_M15, "phone or computer on your Wi-Fi:", 188, C_GREY);
  centre(F_B24, "mini.local", 228, C_YELLOW);
  centre(F_M12, "or " + ip, 262, C_GREY);
}

void uiMessage(const char* title, const char* line1, const char* line2, uint16_t titleCol) {
  lcd.fillScreen(C_BG);
  centre(F_B36, title, 120, titleCol);
  if (line1) centre(F_M15, line1, 175, C_GREY);
  if (line2) centre(F_M15, line2, 200, C_GREY);
}

void uiUpdating(int pct, const char* version) {
  static int shown = -2;
  if (pct < shown || shown < 0) {
    lcd.fillScreen(C_BG);
    centre(F_B36, "Updating", 110, C_YELLOW);
    centre(F_M15, String("to version ") + version + ". Don't unplug.", 160, C_GREY);
    tile(60, 200, 420, 224, C_TILE, C_EDGE, 12);
  }
  if (pct != shown) {
    int w = 356 * pct / 100;
    if (w > 0) lcd.fillRoundRect(62, 202, w, 20, 10, C_GREEN);
    shown = pct;
  }
}

void uiBootHold(int secondsLeft) {
  lcd.fillScreen(C_BG);
  centre(F_B24, "Keep holding to reset Wi-Fi", 120, C_WHITE);
  centre(F_B36, String(secondsLeft), 180, C_YELLOW);
}

// --------------------------------------------------------------------- home
static void tileRect(int i, int& x0, int& y0, int& x1, int& y1) {
  int col = i % 3, row = i / 3;
  x0 = 6 + col * 158;
  y0 = 34 + row * 143;
  x1 = x0 + 152;
  y1 = y0 + 137;
}

static int lastMinute = -1;

void uiHomeClock(bool force) {
  // no Wi-Fi: say so in plain words where the time is (the scores below are the last ones heard)
  static bool offShown = false;
  if (!mnWifiUp()) {
    if (!offShown || force) {
      lcd.fillRect(104, 4, 92, 26, C_BG);
      text(F_M12, "Waiting for", 150, 11, rgb(255, 176, 32), C_BG, middle_center);
      text(F_M12, "Wi-Fi...", 150, 24, rgb(255, 176, 32), C_BG, middle_center);
    }
    offShown = true;
    return;
  }
  if (offShown) { offShown = false; force = true; }
  time_t now = mnTime();
  if (now < 1700000000) return;
  struct tm lt;
  localtime_r(&now, &lt);
  if (!force && lt.tm_min == lastMinute) return;
  lastMinute = lt.tm_min;
  // "Fri 7:42 PM" (the board's strftime has no "%-I" for an hour without
  // its leading zero, so the hour is put in by hand)
  static const char* const DAYS[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  char s[24];
  int h = lt.tm_hour % 12;
  snprintf(s, sizeof(s), "%s %d:%02d %s", DAYS[lt.tm_wday % 7], h ? h : 12, lt.tm_min, lt.tm_hour < 12 ? "AM" : "PM");
  lcd.fillRect(104, 4, 92, 26, C_BG);   // clear of the MY TEAMS button on the second page
  text(F_S13, s, 150, 16, C_GREY, C_BG, middle_center);
}

// What sits in each of a page's six slots: an index into settings.picks,
// HIT_AUTO or HIT_MORE. Live games come first.
int uiHomePages() { return settings.npicks > 5 ? 2 : 1; }

static int order[MAX_PICKS];   // slot -> favourite, live ones first

static void computeOrder() {
  int n = 0;
  for (int i = 0; i < settings.npicks; i++) {
    Game g;
    if (netGame(i, g) && g.state == GS_LIVE) order[n++] = i;
  }
  for (int i = 0; i < settings.npicks; i++) {
    Game g;
    if (!(netGame(i, g) && g.state == GS_LIVE)) order[n++] = i;
  }
}

static int pageSlots(int page, int* slot) {
  int n = 0;
  if (uiHomePages() == 1) {
    for (int i = 0; i < settings.npicks; i++) slot[n++] = order[i];
    slot[n++] = HIT_AUTO;
  } else if (page == 0) {
    for (int i = 0; i < 5; i++) slot[n++] = order[i];
    slot[n++] = HIT_MORE;
  } else {
    for (int i = 5; i < settings.npicks; i++) slot[n++] = order[i];
    slot[n++] = HIT_AUTO;
  }
  return n;
}

static uint32_t tileSig[6];
static int tilePick[6];

static void arrowIcon(int cx, int cy, uint16_t col) {   // a fat right arrow
  lcd.fillRoundRect(cx - 24, cy - 5, 34, 11, 4, col);
  lcd.fillTriangle(cx + 4, cy - 17, cx + 4, cy + 17, cx + 24, cy, col);
}

// back button where "My Teams" sits on the first page
static const int BK_X0 = 6, BK_Y0 = 3, BK_X1 = 100, BK_Y1 = 29;
// EDIT (the team picker) and Wi-Fi in the first page's top bar, left of the battery
static const int ED_X0 = 198, ED_Y0 = 3, ED_X1 = 254, ED_Y1 = 29;
static const int GM_X0 = 258, GM_Y0 = 3, GM_X1 = 340, GM_Y1 = 29;
static const int WF_X0 = 344, WF_Y0 = 3, WF_X1 = 390, WF_Y1 = 29;

static void editButton() {
  tile(ED_X0, ED_Y0, ED_X1, ED_Y1, C_TILE, C_EDGE, 13);
  lcd.drawWideLine(ED_X0 + 13, 22, ED_X0 + 21, 10, 1.6f, C_WHITE);   // a pencil
  lcd.fillTriangle(ED_X0 + 10, 25, ED_X0 + 11, 20, ED_X0 + 14, 23, C_WHITE);
  text(F_B12, "EDIT", ED_X0 + 36, 16, C_WHITE, C_TILE, middle_center);
}

static void gamesButton() {
  tile(GM_X0, GM_Y0, GM_X1, GM_Y1, C_AUTO_BG, C_AUTO_EDGE, 13);
  // a little game pad
  lcd.fillRoundRect(GM_X0 + 8, 10, 18, 11, 4, C_WHITE);
  lcd.fillRect(GM_X0 + 11, 14, 5, 2, C_AUTO_BG);
  lcd.fillRect(GM_X0 + 13, 12, 2, 6, C_AUTO_BG);
  lcd.fillCircle(GM_X0 + 21, 14, 1, C_AUTO_BG);
  lcd.fillCircle(GM_X0 + 23, 17, 1, C_AUTO_BG);
  text(F_B12, "GAMES", GM_X0 + 54, 16, C_WHITE, C_AUTO_BG, middle_center);
}

static bool wifiShown = true;

void uiHomeWifiIcon(bool force) {
  bool up = mnWifiUp();
  if (!force && up == wifiShown) return;
  wifiShown = up;
  uint16_t col = up ? C_WHITE : C_RED;
  tile(WF_X0, WF_Y0, WF_X1, WF_Y1, C_TILE, up ? C_EDGE : C_RED, 13);
  int cx = (WF_X0 + WF_X1) / 2, cy = 21;
  for (int r = 1; r <= 3; r++) lcd.fillArc(cx, cy, r * 5 + 2, r * 5 - 1, 232, 308, col);
  lcd.fillCircle(cx, cy - 1, 2, col);
  if (!up) lcd.drawWideLine(cx - 9, 8, cx + 9, 24, 1.6f, C_RED);
}

void uiHome(int page) {
  if (page >= uiHomePages()) page = 0;
  computeOrder();
  for (int k = 0; k < 6; k++) tilePick[k] = -1;
  lcd.fillScreen(C_BG);
  uiHomeClock(true);
  uiBattery(true);
  if (page == 0) {
    text(F_B16, "My Teams", 12, 16, C_WHITE, C_BG, middle_left);
    editButton();
    gamesButton();
    uiHomeWifiIcon(true);
  } else {
    tile(BK_X0, BK_Y0, BK_X1, BK_Y1, C_TILE, C_EDGE, 13);
    lcd.fillTriangle(BK_X0 + 12, 16, BK_X0 + 20, 9, BK_X0 + 20, 23, C_WHITE);
    text(F_B12, "MY TEAMS", BK_X0 + 58, 16, C_WHITE, C_TILE, middle_center);
    text(F_S13, "More teams", 384, 16, C_GREY, C_BG, middle_right);
  }
  if (!settings.npicks) {
    centre(F_B24, "Pick your teams", 130, C_WHITE);
    centre(F_M15, "Tap EDIT up top, or on a phone or", 172, C_GREY);
    centre(F_M15, "computer on your Wi-Fi go to", 196, C_GREY);
    centre(F_B24, "mini.local", 232, C_YELLOW);
    return;
  }
  int slot[6];
  int n = pageSlots(page, slot);
  for (int k = 0; k < n; k++) {
    int x0, y0, x1, y1;
    tileRect(k, x0, y0, x1, y1);
    int cx = (x0 + x1) / 2;
    if (slot[k] == HIT_AUTO) {
      tile(x0, y0, x1, y1, C_AUTO_BG, C_AUTO_EDGE, 14, 2);
      autoIcon(cx, y0 + 50, 26, C_WHITE, 5);
      text(F_B24, "AUTO", cx, y0 + 98, C_WHITE, C_AUTO_BG, middle_center);
      text(F_M12, "rotate my teams", cx, y0 + 120, C_AUTO_INK, C_AUTO_BG, middle_center);
    } else if (slot[k] == HIT_MORE) {
      tile(x0, y0, x1, y1, C_TILE_HI, C_EDGE, 14, 2);
      arrowIcon(cx, y0 + 50, C_WHITE);
      text(F_B24, "MORE", cx, y0 + 98, C_WHITE, C_TILE_HI, middle_center);
      int extra = settings.npicks - 5;
      String sub = String(extra) + (extra == 1 ? " more team + AUTO" : " more teams + AUTO");
      text(F_M12, sub, cx, y0 + 120, C_GREY, C_TILE_HI, middle_center);
    } else {
      int t = settings.picks[slot[k]];
      Game g;
      bool known = netGame(slot[k], g);
      playTile(x0, y0, x1, y1, t, g, known);
      tileSig[k] = playTileSig(t, g, known);
      tilePick[k] = slot[k];
    }
  }
}

// Redraw only the tiles whose game changed (or that moved: a game going live
// jumps to the front).
void uiHomeRefresh(int page) {
  if (page >= uiHomePages()) page = 0;
  if (!settings.npicks) return;
  computeOrder();
  int slot[6];
  int n = pageSlots(page, slot);
  for (int k = 0; k < n; k++) {
    if (slot[k] >= settings.npicks) continue;   // AUTO and MORE don't change
    int t = settings.picks[slot[k]];
    Game g;
    bool known = netGame(slot[k], g);
    uint32_t sig = playTileSig(t, g, known);
    if (tilePick[k] == slot[k] && tileSig[k] == sig) continue;
    int x0, y0, x1, y1;
    tileRect(k, x0, y0, x1, y1);
    playTile(x0, y0, x1, y1, t, g, known);
    tileSig[k] = sig;
    tilePick[k] = slot[k];
  }
}

int uiHomeHit(int page, int x, int y) {
  if (page > 0 && x < BK_X1 + 16 && y < BK_Y1 + 6) return HIT_BACK;
  if (page == 0 && x >= ED_X0 - 6 && x < ED_X1 + 3 && y < ED_Y1 + 4) return HIT_EDIT;
  if (page == 0 && x >= WF_X0 - 3 && x < WF_X1 + 6 && y < WF_Y1 + 4) return HIT_WIFI;
  if (page == 0 && x >= GM_X0 - 3 && x < GM_X1 + 3 && y < GM_Y1 + 4) return HIT_GAMES;
  int slot[6];
  int n = pageSlots(page, slot);
  for (int k = 0; k < n; k++) {
    int x0, y0, x1, y1;
    tileRect(k, x0, y0, x1, y1);
    if (x >= x0 && x < x1 && y >= y0 && y < y1) return slot[k];
  }
  return -1;
}

void uiTileFlash(int page, int hit) {
  if (hit == HIT_BACK || hit == HIT_EDIT || hit == HIT_WIFI || hit == HIT_GAMES) {
    if (hit == HIT_BACK) lcd.drawRoundRect(BK_X0, BK_Y0, BK_X1 - BK_X0, BK_Y1 - BK_Y0, 13, C_WHITE);
    else if (hit == HIT_GAMES) lcd.drawRoundRect(GM_X0, GM_Y0, GM_X1 - GM_X0, GM_Y1 - GM_Y0, 13, C_WHITE);
    else if (hit == HIT_WIFI) lcd.drawRoundRect(WF_X0, WF_Y0, WF_X1 - WF_X0, WF_Y1 - WF_Y0, 13, C_WHITE);
    else lcd.drawRoundRect(ED_X0, ED_Y0, ED_X1 - ED_X0, ED_Y1 - ED_Y0, 13, C_WHITE);
    delay(90);
    return;
  }
  int slot[6];
  int n = pageSlots(page, slot);
  for (int k = 0; k < n; k++) {
    if (slot[k] != hit) continue;
    int x0, y0, x1, y1;
    tileRect(k, x0, y0, x1, y1);
    for (int w = 0; w < 2; w++) lcd.drawRoundRect(x0 + w, y0 + w, x1 - x0 - 2 * w, y1 - y0 - 2 * w, 14 - w, C_WHITE);
  }
  delay(90);
}

// ----------------------------------------------------------------- battery
static int battShown = -2;
static bool battChg = false;

static void batteryIcon(int x, int y, int pct, bool chg, uint16_t col) {   // 30 x 15, x/y top left
  uint16_t edge = chg ? C_GREEN : C_WHITE;
  lcd.drawRoundRect(x, y, 31, 15, 3, edge);
  lcd.drawRoundRect(x + 1, y + 1, 29, 13, 2, edge);
  lcd.fillRect(x + 31, y + 4, 3, 7, edge);
  int w = max(2, 24 * pct / 100);
  lcd.fillRoundRect(x + 4, y + 4, w, 7, 1, col);
  if (chg) {   // a bolt
    lcd.fillTriangle(x + 17, y + 1, x + 10, y + 9, x + 16, y + 9, C_WHITE);
    lcd.fillTriangle(x + 14, y + 14, x + 21, y + 6, x + 15, y + 6, C_WHITE);
  }
}

void uiBattery(bool force) {
  int pct = batPresent() ? batPercent() : -1;
  bool chg = batCharging();
  if (!force && pct == battShown && chg == battChg) return;
  battShown = pct;
  battChg = chg;
  lcd.fillRect(392, 3, 86, 26, C_BG);
  if (pct < 0) return;   // no battery: nothing to show
  uint16_t amber = rgb(255, 176, 32);
  uint16_t col = chg || pct > 20 ? C_GREEN : pct > 10 ? amber : C_RED;
  batteryIcon(440, 8, pct, chg, col);
  text(F_B12, String(pct) + "%", 434, 16, pct <= 20 && !chg ? col : C_WHITE, C_BG, middle_right);
}

// ------------------------------------------------------------ HOME button
// small, in the top bar of the game screen (like EDIT on the home screen)
static const int HB_X0 = 314, HB_Y0 = 3, HB_X1 = 386, HB_Y1 = 29;

void uiHomeButton() {
  tile(HB_X0, HB_Y0, HB_X1, HB_Y1, C_TILE, C_EDGE, 13);
  homeIcon(HB_X0 + 17, 16, 8, C_WHITE, C_TILE);
  text(F_B12, "HOME", HB_X0 + 48, 16, C_WHITE, C_TILE, middle_center);
}

bool uiHomeButtonHit(int x, int y) {
  // a bit of slack round the button: resistive taps land a few dots off
  return x >= HB_X0 - 8 && x < HB_X1 + 4 && y < HB_Y1 + 8;
}

// ------------------------------------------------------------ ticker chooser
// Rows: "All my teams", then each league that has a favourite. cur: the row ticked now.
static int tickRows = 0;
void uiTicker(const char* const* names, const int* total, const int* live, int n, int cur) {
  tickRows = n;
  lcd.fillScreen(C_BG);
  tile(6, 3, 96, 29, C_TILE, C_EDGE, 13);
  lcd.fillTriangle(18, 16, 26, 9, 26, 23, C_WHITE);
  text(F_B12, "BACK", 60, 16, C_WHITE, C_TILE, middle_center);
  text(F_B16, "Ticker: rotate through", 270, 16, C_WHITE, C_BG, middle_center);
  for (int i = 0; i < n && i < 6; i++) {
    int y0 = 40 + i * 46;
    bool on = i == cur;
    tile(10, y0, 470, y0 + 40, on ? C_AUTO_BG : C_TILE, on ? C_AUTO_EDGE : C_EDGE, 12, on ? 2 : 1);
    text(F_B18, names[i], 28, y0 + 20, C_WHITE, on ? C_AUTO_BG : C_TILE, middle_left);
    String sub = String(total[i]) + (total[i] == 1 ? " team" : " teams");
    text(F_S13, sub, 340, y0 + 20, C_GREY, on ? C_AUTO_BG : C_TILE, middle_right);
    if (live[i] > 0) {
      String lv = String(live[i]) + " live";
      useFont(F_B12);
      int w = lcd.textWidth(lv.c_str()) + 16;
      tile(452 - w, y0 + 10, 452, y0 + 30, C_RED, C_RED, 10);
      text(F_B12, lv, 452 - w / 2, y0 + 20, C_WHITE, C_RED, middle_center);
    }
  }
}

// -2 = BACK, -1 = nothing, 0.. = the row
int uiTickerHit(int x, int y) {
  if (x < 104 && y < 34) return -2;
  for (int i = 0; i < tickRows && i < 6; i++) {
    int y0 = 40 + i * 46;
    if (y >= y0 - 2 && y < y0 + 42) return i;
  }
  return -1;
}
