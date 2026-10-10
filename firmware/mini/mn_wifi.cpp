#include "mn_wifi.h"
#include "mn_keyboard.h"
#include "mn_settings.h"
#include "mn_ui.h"
#include "mn_lcd.h"
#include "mn_log.h"
#include <WiFi.h>

struct Ap { String ssid; int rssi; bool open; };
static const int MAX_APS = 24;
static Ap aps[MAX_APS];
static int nAps = 0;
static bool scanning = false;
static uint32_t scanAt = 0;
static int page = 0;
static const int PER_PAGE = 6;

enum State { S_LIST, S_ACTION, S_KEY_NAME, S_KEY_PASS, S_JOINING, S_MSG };
static State state = S_LIST;
static String selSsid, selPass, prevSsid, prevPass;
static bool selOpen = false, selNew = false, fromKeyboard = false;
static uint32_t stateAt = 0;
static String msgTitle, msgLine;


static int entries() { return nAps + 1; }          // the networks, then "Other network..."
static int pages() { return (entries() + PER_PAGE - 1) / PER_PAGE; }

static String connectedSsid() { return WiFi.status() == WL_CONNECTED ? WiFi.SSID() : String(); }

static void startScan() {
  WiFi.scanDelete();
  WiFi.scanNetworks(true, false);   // in the background; the screen keeps working
  scanning = true;
  scanAt = millis();
}

static void gather() {
  int n = WiFi.scanComplete();
  nAps = 0;
  for (int i = 0; i < n; i++) {
    String s = WiFi.SSID(i);
    if (!s.length()) continue;      // hidden: the "Other network" row covers it
    int rssi = WiFi.RSSI(i);
    bool open = WiFi.encryptionType(i) == WIFI_AUTH_OPEN;
    int dup = -1;
    for (int k = 0; k < nAps; k++) if (aps[k].ssid == s) dup = k;
    if (dup >= 0) { if (rssi > aps[dup].rssi) { aps[dup].rssi = rssi; aps[dup].open = open; } continue; }
    if (nAps < MAX_APS) aps[nAps++] = {s, rssi, open};
  }
  WiFi.scanDelete();
  scanning = false;
  // strongest first
  for (int i = 1; i < nAps; i++)
    for (int j = i; j > 0 && aps[j].rssi > aps[j - 1].rssi; j--) { Ap t = aps[j]; aps[j] = aps[j - 1]; aps[j - 1] = t; }
  mnLog("wifi: %d networks in range", nAps);
}

int wifiBestSaved() {
  int n = WiFi.scanNetworks();   // waits for the answer: only used when there's nothing else to do
  int best = -1, bestRssi = -1000;
  for (int i = 0; i < n; i++) {
    int at = settings.findNet(WiFi.SSID(i));
    if (at >= 0 && WiFi.RSSI(i) > bestRssi) { best = at; bestRssi = WiFi.RSSI(i); }
  }
  WiFi.scanDelete();
  return best;
}

// ------------------------------------------------------------------ drawing
static void bars(int x, int y, int rssi, uint16_t col) {   // four little bars, x/y = bottom left
  int lit = rssi > -55 ? 4 : rssi > -65 ? 3 : rssi > -75 ? 2 : 1;
  for (int b = 0; b < 4; b++) lcd.fillRect(x + b * 6, y - 5 - b * 4, 4, 5 + b * 4, b < lit ? col : C_DIM);
}

static void lockIcon(int x, int y, uint16_t col, uint16_t bg) {   // x/y = top left of a 12 x 14 padlock
  lcd.fillArc(x + 6, y + 5, 6, 3, 180, 360, col);
  lcd.fillRoundRect(x, y + 5, 12, 9, 2, col);
  lcd.fillRect(x + 5, y + 8, 2, 3, bg);
}

static void pillText(int xr, int yc, const char* s, uint16_t bg, uint16_t fg) {   // right edge xr
  useFont(F_B12);
  int w = lcd.textWidth(s) + 14;
  uiTile(xr - w, yc - 9, xr, yc + 9, bg, bg, 9, 1);
  uiText(F_B12, s, xr - w / 2, yc, fg, bg, middle_center);
}

static void topBar(const char* title) {
  uiTile(6, 3, 96, 29, C_TILE, C_EDGE, 13, 1);
  lcd.fillTriangle(18, 16, 26, 9, 26, 23, C_WHITE);
  uiText(F_B12, "BACK", 60, 16, C_WHITE, C_TILE, middle_center);
  uiText(F_B16, title, 240, 16, C_WHITE, C_BG, middle_center);
}

static void rowRect(int k, int& x0, int& y0, int& x1, int& y1) {
  x0 = 6; x1 = 474;
  y0 = 62 + k * 38;
  y1 = y0 + 36;
}

void wifiDraw() {
  lcd.fillScreen(C_BG);
  topBar("Wi-Fi");
  uiTile(380, 3, 474, 29, C_TILE, C_EDGE, 13, 1);
  uiText(F_B12, scanning ? "LOOKING..." : "RESCAN", 427, 16, C_WHITE, C_TILE, middle_center);
  String cur = connectedSsid();
  if (cur.length()) {
    String l = "Connected to " + cur;
    useFont(F_S13);
    while (l.length() > 8 && lcd.textWidth(l.c_str()) > 460) l = l.substring(0, l.length() - 1);
    uiText(F_S13, l, 12, 46, C_GREEN, C_BG, middle_left);
  } else {
    uiText(F_S13, "Not connected", 12, 46, C_RED, C_BG, middle_left);
  }
  int total = entries();
  if (page >= pages()) page = pages() - 1;
  for (int k = 0; k < PER_PAGE; k++) {
    int i = page * PER_PAGE + k;
    if (i >= total) break;
    int x0, y0, x1, y1;
    rowRect(k, x0, y0, x1, y1);
    int cy = (y0 + y1) / 2;
    if (i == nAps) {   // the last row
      uiTile(x0, y0, x1, y1, C_TILE, C_EDGE, 10, 1);
      uiText(F_B16, "Other network...", 18, cy, C_GREY, C_TILE, middle_left);
      uiText(F_S13, "type the name", x1 - 14, cy, C_DIM, C_TILE, middle_right);
      continue;
    }
    const Ap& a = aps[i];
    bool on = a.ssid == cur;
    bool saved = settings.findNet(a.ssid) >= 0;
    uiTile(x0, y0, x1, y1, on ? C_TILE_HI : C_TILE, on ? C_GREEN : C_EDGE, 10, on ? 2 : 1);
    String s = a.ssid;
    useFont(F_B16);
    while (s.length() > 4 && lcd.textWidth(s.c_str()) > 250) s = s.substring(0, s.length() - 1);
    uiText(F_B16, s, 18, cy, C_WHITE, on ? C_TILE_HI : C_TILE, middle_left);
    int xr = x1 - 14;
    bars(xr - 22, cy + 9, a.rssi, C_WHITE);
    xr -= 34;
    if (!a.open) { lockIcon(xr - 12, cy - 7, C_GREY, on ? C_TILE_HI : C_TILE); xr -= 22; }
    if (on) pillText(xr, cy, "JOINED", C_GREEN, C_WHITE);
    else if (saved) pillText(xr, cy, "SAVED", rgb(60, 66, 82), C_WHITE);
  }
  if (!nAps && !scanning) uiText(F_M15, "No networks found. Tap RESCAN.", 240, 120, C_GREY, C_BG, middle_center);
  // pages
  if (pages() > 1) {
    uiText(F_S13, String("Page ") + String(page + 1) + " of " + String(pages()), 12, 306, C_GREY, C_BG, middle_left);
    for (int d = 0; d < 2; d++) {
      int x0 = 400 + d * 40;
      uiTile(x0, 292, x0 + 34, 316, C_TILE, C_EDGE, 8, 1);
      bool up = d == 0;   // first button = previous page
      int cx = x0 + 17, cy = 304;
      if (up) lcd.fillTriangle(cx - 7, cy + 4, cx + 7, cy + 4, cx, cy - 5, C_WHITE);
      else lcd.fillTriangle(cx - 7, cy - 4, cx + 7, cy - 4, cx, cy + 5, C_WHITE);
    }
  }
}

static void drawAction() {
  wifiDraw();
  lcd.fillRect(0, 36, 480, 284, C_BG);
  uiTile(50, 50, 430, 280, C_TILE, C_EDGE, 16, 1);
  String s = selSsid;
  useFont(F_B18);
  while (s.length() > 4 && lcd.textWidth(s.c_str()) > 330) s = s.substring(0, s.length() - 1);
  uiText(F_B18, s, 240, 76, C_WHITE, C_TILE, middle_center);
  bool on = selSsid == connectedSsid();
  uiText(F_S13, on ? "You're on this network" : "Remembered network", 240, 98, on ? C_GREEN : C_GREY, C_TILE, middle_center);
  uiTile(70, 114, 410, 156, on ? C_TILE_HI : C_GREEN, on ? C_EDGE : C_GREEN, 12, 1);
  uiText(F_B16, on ? "Already joined" : "JOIN", 240, 135, on ? C_DIM : C_WHITE, on ? C_TILE_HI : C_GREEN, middle_center);
  uiTile(70, 166, 410, 208, rgb(110, 36, 40), rgb(160, 60, 64), 12, 1);
  uiText(F_B16, "FORGET THIS NETWORK", 240, 187, C_WHITE, rgb(110, 36, 40), middle_center);
  uiTile(70, 218, 410, 260, C_TILE_HI, C_EDGE, 12, 1);
  uiText(F_B16, "CANCEL", 240, 239, C_WHITE, C_TILE_HI, middle_center);
}

// ------------------------------------------------------------------ joining
static void joinStart(const String& ssid, const String& pass) {
  selSsid = ssid;
  selPass = pass;
  prevSsid = connectedSsid();
  prevPass = "";
  int at = settings.findNet(prevSsid);
  if (at >= 0) prevPass = settings.nets[at].pass;
  WiFi.setAutoReconnect(false);
  WiFi.disconnect(false);
  delay(100);
  WiFi.begin(ssid.c_str(), pass.c_str());
  state = S_JOINING;
  stateAt = millis();
  mnLog("wifi: joining %s", ssid.c_str());
}

static void showMsg(const char* title, const String& line) {
  msgTitle = title;
  msgLine = line;
  state = S_MSG;
  stateAt = millis();
  uiMessage(title, line.c_str(), nullptr, C_RED);
}

bool wifiBusy() { return state == S_JOINING || state == S_MSG; }

void wifiStart() {
  page = 0;
  state = S_LIST;
  nAps = 0;
  startScan();
  wifiDraw();
}

static void backToList() {
  state = S_LIST;
  wifiDraw();
}

WifiResult wifiLoop() {
  if (state == S_LIST && scanning) {
    int n = WiFi.scanComplete();
    if (n >= 0 || (n == -2 && millis() - scanAt > 500) || millis() - scanAt > 15000) {
      gather();
      wifiDraw();
    }
  }
  if (state == S_JOINING) {
    uiJoining(selSsid);
    wl_status_t st = WiFi.status();
    uint32_t t = millis() - stateAt;
    if (st == WL_CONNECTED) {
      settings.addNet(selSsid, selPass);
      settings.save();
      mnLog("wifi: joined %s", selSsid.c_str());
      uiMessage("Joined", selSsid.substring(0, 28).c_str(), "Restarting...", C_GREEN);
      delay(1500);
      ESP.restart();
    } else if (t > 25000 || (t > 7000 && (st == WL_CONNECT_FAILED || st == WL_NO_SSID_AVAIL))) {
      mnLog("wifi: couldn't join %s (%d)", selSsid.c_str(), (int)st);
      WiFi.disconnect(false);
      if (prevSsid.length()) WiFi.begin(prevSsid.c_str(), prevPass.c_str());   // back to where it was
      WiFi.setAutoReconnect(true);
      showMsg("Couldn't join", selOpen ? "Is it a sign-in network?" : "Check the password and try again.");
    }
  }
  if (state == S_MSG && millis() - stateAt > 2800) {
    if (fromKeyboard) {
      state = S_KEY_PASS;
      kbOpen("Password: " + selSsid, "", true, 63);
      kbDraw();
    } else {
      backToList();
    }
  }
  return WR_STAY;
}

void wifiSwipe(bool up) {
  if (state != S_LIST) return;
  int np = pages();
  int old = page;
  page = constrain(page + (up ? 1 : -1), 0, np - 1);
  if (page != old) wifiDraw();
}

static void chooseEntry(int i) {
  if (i == nAps) {   // another network, typed in
    state = S_KEY_NAME;
    kbOpen("Network name", "", false, 32);
    kbDraw();
    return;
  }
  const Ap& a = aps[i];
  selSsid = a.ssid;
  selOpen = a.open;
  int at = settings.findNet(a.ssid);
  if (at >= 0 || a.ssid == connectedSsid()) {   // already known: join or forget
    state = S_ACTION;
    drawAction();
    return;
  }
  fromKeyboard = false;
  if (a.open) { joinStart(a.ssid, ""); return; }
  fromKeyboard = true;
  state = S_KEY_PASS;
  kbOpen("Password: " + a.ssid, "", true, 63);
  kbDraw();
}

WifiResult wifiTap(int x, int y) {
  switch (state) {
    case S_LIST: {
      if (x < 104 && y < 34) return WR_BACK;
      if (x >= 376 && y < 34) { if (!scanning) { startScan(); wifiDraw(); } return WR_STAY; }
      if (y >= 288 && pages() > 1) {
        if (x >= 396 && x < 436) wifiSwipe(false);
        else if (x >= 436) wifiSwipe(true);
        return WR_STAY;
      }
      for (int k = 0; k < PER_PAGE; k++) {
        int i = page * PER_PAGE + k;
        if (i >= entries()) break;
        int x0, y0, x1, y1;
        rowRect(k, x0, y0, x1, y1);
        if (y >= y0 - 1 && y < y1 + 1) { chooseEntry(i); break; }
      }
      return WR_STAY;
    }
    case S_ACTION: {
      bool on = selSsid == connectedSsid();
      if (y >= 108 && y < 160 && x >= 60 && x < 420 && !on) {
        int at = settings.findNet(selSsid);
        fromKeyboard = false;
        selOpen = false;
        joinStart(selSsid, at >= 0 ? settings.nets[at].pass : String());
      } else if (y >= 162 && y < 212 && x >= 60 && x < 420) {
        int at = settings.findNet(selSsid);
        if (at >= 0) { settings.forgetNet(at); settings.save(); }
        backToList();
      } else if (y >= 214 && y < 266) {
        backToList();
      }
      return WR_STAY;
    }
    case S_KEY_NAME:
    case S_KEY_PASS: {
      KbResult r = kbTap(x, y);
      if (r == KB_CANCEL) { backToList(); }
      else if (r == KB_OK) {
        if (state == S_KEY_NAME) {
          selSsid = kbText();
          selOpen = false;
          fromKeyboard = true;
          state = S_KEY_PASS;
          kbOpen("Password: " + selSsid, "", true, 63);
          kbDraw();
        } else {
          fromKeyboard = true;
          selOpen = false;
          joinStart(selSsid, kbText());
        }
      }
      return WR_STAY;
    }
    default:
      return WR_STAY;
  }
}
