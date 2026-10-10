/*
  Mini Scoreboard - 4.0" ESP32-32E touch screen (480x320).
  Stage 1: screen, touch and Wi-Fi setup, plus updates over Wi-Fi.

  First start: touch setup (four arrows), then the screen shows a code to
  scan; the phone joins the mini's own Wi-Fi and the setup page pops up.
  After that: home screen with a tile per team plus AUTO; settings at
  http://mini.local. Hold BOOT for 5 seconds to forget the Wi-Fi.

  Build: see firmware/README.md.
*/
#include <WiFi.h>
#include <Preferences.h>
#include "mn_hw.h"
#include "mn_lcd.h"
#include "mn_touch.h"
#include "mn_settings.h"
#include "mn_portal.h"
#include "mn_ota.h"
#include "mn_ui.h"
#include "mn_picker.h"
#include "mn_net.h"
#include "mn_play.h"
#include "mn_log.h"
#include "mn_version.h"

enum Mode { M_SETUP, M_CONNECTING, M_FALLBACK, M_CONNECTED, M_HOME, M_TEAM, M_PICK };
static Mode mode = M_SETUP;
static uint32_t modeAt = 0, lastTry = 0;
static bool dirty = true;
static int shownPick = 0;    // M_TEAM: which favourite (index into settings.picks)
static bool autoOn = false;  // M_TEAM: AUTO is rotating through the favourites
static uint32_t autoAt = 0;  // when AUTO moves on
static uint32_t seenVer = 0, shownSig = 0, shownShape = 0;
static const uint32_t AUTO_MS = 20000;
static int homePage = 0;     // M_HOME: 0, or 1 for the teams past the fifth
static String apName;
static bool dimmed = false;
static const uint32_t DIM_AFTER_MS = 60000;

static int autoPick(int from);
static void setMode(Mode m) { mode = m; modeAt = millis(); dirty = true; }

static uint8_t level() { return BRIGHTS[settings.bright].level; }

static void startStation() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.setHostname("mini-scoreboard");
  WiFi.begin(settings.ssid.c_str(), settings.pass.c_str());
  lastTry = millis();
  setMode(M_CONNECTING);
}

static void onConnected() {
  mnLog("Wi-Fi up on %s: %s", settings.ssid.c_str(), WiFi.localIP().toString().c_str());
  if (portalAPRunning()) portalStopAP();
  WiFi.setAutoReconnect(true);
  configTzTime(TZS[settings.tz].posix, "pool.ntp.org", "time.nist.gov", "time.google.com");
  portalStartHome();
  otaStart();
  netStart();
  setMode(M_CONNECTED);
}

// Hold BOOT for 5 seconds: forget the Wi-Fi and go back to setup.
// true while it's being held (the countdown owns the screen)
static bool checkBootButton() {
  static uint32_t downAt = 0;
  static int shownLeft = -1;
  if (digitalRead(PIN_BOOT) == LOW) {
    if (!downAt) downAt = millis();
    uint32_t held = millis() - downAt;
    if (held > 800) {
      int left = 5 - (int)(held / 1000);
      if (left <= 0) {
        uiMessage("Wi-Fi forgotten", "Restarting...", nullptr, C_YELLOW);
        settings.forgetWifi();
        delay(1500);
        ESP.restart();
      }
      if (left != shownLeft) { lcdBrightness(level()); uiBootHold(left); shownLeft = left; }
      return true;
    }
    return false;
  }
  if (downAt) {
    if (shownLeft >= 0) dirty = true;   // let go early: put the screen back
    downAt = 0;
    shownLeft = -1;
  }
  return false;
}

// What happened at the last update (shown once after the restart)
static void updateNotice() {
  Preferences p;
  p.begin("mini", false);
  String v = p.getString("updated", "");
  if (v.length()) {
    p.remove("updated");
    if (v == FW_VERSION) {
      p.remove("bad");
      uiMessage("Updated", ("to version " + v).c_str(), nullptr, C_GREEN);
      mnLog("updated to %s", v.c_str());
    } else {
      // the new version didn't make it and the chip went back to this one:
      // remember, so it isn't installed again every night
      p.putString("bad", v);
      String l1 = "Version " + v + " didn't work.";
      String l2 = "Back on version " FW_VERSION ".";
      uiMessage("Update failed", l1.c_str(), l2.c_str(), C_RED);
      mnLog("update to %s failed, back on %s", v.c_str(), FW_VERSION);
    }
    delay(3000);
  }
  p.end();
}

void setup() {
  Serial.begin(115200);
  delay(200);
  mnLog("Mini Scoreboard " FW_VERSION " starting");
  pinMode(PIN_BOOT, INPUT_PULLUP);
  for (int pin : {PIN_LED_R, PIN_LED_G, PIN_LED_B}) { pinMode(pin, OUTPUT); digitalWrite(pin, HIGH); }   // LED off
  pinMode(PIN_AMP, OUTPUT);
  digitalWrite(PIN_AMP, HIGH);   // speaker amplifier off until sound arrives
  settings.load();
  setenv("TZ", TZS[settings.tz].posix, 1);
  tzset();
  lcdBegin(settings.colour, settings.flip);
  lcdBrightness(level());
  uiSplash();
  delay(1500);
  updateNotice();

  touchBegin();
  if (!settings.hasCal) touchCalibrate();

  uint8_t mac[6];
  WiFi.macAddress(mac);
  char nm[28];
  snprintf(nm, sizeof(nm), "Mini-Scoreboard-%02X%02X", mac[4], mac[5]);
  apName = nm;

  if (!settings.hasWifi()) {
    portalStartAP(apName);
    setMode(M_SETUP);
  } else {
    startStation();
  }
}

static void handleTap(int x, int y) {
  switch (mode) {
    case M_HOME: {
      int hit = uiHomeHit(homePage, x, y);
      if (hit < 0) return;
      uiTileFlash(homePage, hit);
      if (hit == HIT_EDIT) {
        setMode(M_PICK);
        pickerStart();
        dirty = false;
        return;
      }
      if (hit == HIT_MORE || hit == HIT_BACK) {
        homePage = hit == HIT_MORE ? 1 : 0;
        dirty = true;
        return;
      }
      autoOn = hit == HIT_AUTO;
      shownPick = autoOn ? autoPick(-1) : hit;
      autoAt = millis() + AUTO_MS;
      setMode(M_TEAM);
      break;
    }
    case M_TEAM:
      if (uiHomeButtonHit(x, y)) { homePage = 0; setMode(M_HOME); }
      break;
    case M_CONNECTED:
      setMode(M_HOME);   // a tap skips the message
      break;
    case M_PICK:
      if (pickerTap(x, y)) { netPicksChanged(); homePage = 0; setMode(M_HOME); }
      break;
    default:
      break;
  }
}

// AUTO's next favourite after `from`: the live games take turns; with none
// live, every favourite does (their final, or next game)
static int autoPick(int from) {
  int n = settings.npicks;
  if (n < 1) return 0;
  bool anyLive = false;
  for (int i = 0; i < n; i++) {
    Game g;
    if (netGame(i, g) && g.state == GS_LIVE) anyLive = true;
  }
  for (int k = 1; k <= n; k++) {
    int i = (from + k + n) % n;
    Game g;
    bool live = netGame(i, g) && g.state == GS_LIVE;
    if (!anyLive || live) return i;
  }
  return (from + 1 + n) % n;
}

// Swipes are shortcuts; every one of them has a button that does the same.
//   home: left / right between the two pages (more than 5 teams)
//   team page: left / right to the next / previous team
//   team picker list: up / down for the next / previous page
//   (later: a pop-up card swiped away)
static void handleSwipe(TouchEvent ev, int sx, int sy) {
  switch (mode) {
    case M_HOME:
      if (ev == T_SWIPE_LEFT && homePage == 0 && uiHomePages() > 1) { homePage = 1; dirty = true; }
      if (ev == T_SWIPE_RIGHT && homePage == 1) { homePage = 0; dirty = true; }
      break;
    case M_TEAM: {
      if (playCardVisible() && sy >= 240) { playCardHide(); break; }   // swipe the last-play card away
      if (settings.npicks < 2 || (ev != T_SWIPE_LEFT && ev != T_SWIPE_RIGHT)) break;
      shownPick = (shownPick + (ev == T_SWIPE_LEFT ? 1 : settings.npicks - 1)) % settings.npicks;
      autoAt = millis() + AUTO_MS;   // a swipe is a pick of your own: AUTO waits a full turn
      dirty = true;
      break;
    }
    case M_PICK:
      if (ev == T_SWIPE_UP || ev == T_SWIPE_DOWN) pickerSwipe(ev == T_SWIPE_UP);
      break;
    default:
      break;
  }
}

void loop() {
  portalLoop();
  if (checkBootButton()) { delay(20); return; }

  // the setup page saved something that needs a restart
  if (portalRestart && millis() - portalSavedAt > 2500) {
    lcdBrightness(level());
    uiMessage("Saved", "Restarting...", nullptr, C_GREEN);
    delay(800);
    ESP.restart();
  }
  if (portalChanged) {
    portalChanged = false;
    netPicksChanged();
    lcdBrightness(level());
    dimmed = false;
    if (mode == M_HOME || mode == M_TEAM) { homePage = 0; setMode(M_HOME); }
  }

  // installing an update: the progress owns the screen
  if (otaPercent >= 0) {
    if (dimmed) { lcdBrightness(level()); dimmed = false; }
    uiUpdating(otaPercent, otaNewVersion);
    delay(50);
    return;
  }

  // taps; the screen dims after a minute untouched and the first tap wakes it
  int tx, ty;
  TouchEvent ev = touchPoll(tx, ty);
  if (ev) {
    if (dimmed) { lcdBrightness(level()); dimmed = false; }
    else if (ev == T_TAP) handleTap(tx, ty);
    else handleSwipe(ev, tx, ty);
  } else if (touchDown() && dimmed) {
    lcdBrightness(level());   // wake as soon as the finger lands; the tap itself is swallowed
  }
  bool idleScreen = mode == M_HOME || mode == M_TEAM || mode == M_PICK;
  if (idleScreen && !dimmed && millis() - touchLastActivity() > DIM_AFTER_MS && millis() - modeAt > DIM_AFTER_MS) {
    lcdBrightness(max(10, level() / 8));
    dimmed = true;
  }

  switch (mode) {
    case M_SETUP:
      if (dirty) { uiSetup(apName, false, ""); dirty = false; }
      break;

    case M_CONNECTING:
      if (WiFi.status() == WL_CONNECTED) { onConnected(); break; }
      uiJoining(settings.ssid);
      if (millis() - modeAt > 45000) {   // not happening: offer the setup page, keep trying
        WiFi.setAutoReconnect(false);
        portalStartAP(apName);
        setMode(M_FALLBACK);
      }
      break;

    case M_FALLBACK:
      if (WiFi.status() == WL_CONNECTED) { onConnected(); break; }
      if (dirty) { uiSetup(apName, true, settings.ssid); dirty = false; }
      // retry now and then (a router that was still booting), but not while
      // someone's using the setup page - retrying hops the radio's channel
      if (millis() - lastTry > 120000 && WiFi.softAPgetStationNum() == 0 &&
          (!portalUsedAt || millis() - portalUsedAt > 180000)) {
        mnLog("retrying %s", settings.ssid.c_str());
        WiFi.begin(settings.ssid.c_str(), settings.pass.c_str());
        lastTry = millis();
      }
      break;

    case M_CONNECTED:
      if (dirty) { uiConnected(WiFi.localIP().toString()); dirty = false; }
      if (millis() - modeAt > 8000) setMode(M_HOME);
      break;

    case M_HOME:
      if (dirty) { uiHome(homePage); dirty = false; seenVer = netVersion(); }
      else {
        uiHomeClock(false);
        if (seenVer != netVersion()) { seenVer = netVersion(); uiHomeRefresh(homePage); }
      }
      break;

    case M_TEAM: {
      if (shownPick >= settings.npicks) shownPick = 0;
      Game g;
      bool known = settings.npicks && netGame(shownPick, g);
      int left = autoOn ? max(0, (int)((int32_t)(autoAt - millis()) / 1000)) : -1;
      if (autoOn && (int32_t)(millis() - autoAt) >= 0) {
        shownPick = autoPick(shownPick);
        autoAt = millis() + AUTO_MS;
        dirty = true;
        known = settings.npicks && netGame(shownPick, g);
        left = (int)(AUTO_MS / 1000);
      }
      uint32_t sig = settings.npicks ? playGameSig(g, known) : 0;
      uint32_t shape = settings.npicks ? playGameShape(g, known) : 0;
      if (dirty || shape != shownShape) {
        // opening the screen, or the game itself changed: everything again
        if (settings.npicks) playGame(settings.picks[shownPick], g, known, left, dirty ? PG_OPEN : PG_FULL);
        shownSig = sig;
        shownShape = shape;
        dirty = false;
      } else if (sig != shownSig) {
        // the score, clock or ball moved: just those parts
        playGame(settings.picks[shownPick], g, known, left, PG_DYN);
        shownSig = sig;
      } else if (autoOn) {
        static int lastLeft = -2;
        if (left != lastLeft) { playAutoTag(left); lastLeft = left; }
      }
      playCardTick();
      break;
    }

    case M_PICK:
      if (dirty) { pickerDraw(); dirty = false; }
      pickerLoop();
      break;
  }
  delay(10);
}
