#include "mn_battery.h"
#include "mn_hw.h"

int batPercentFor(int mv) {
  // resting voltage -> percent for a one-cell LiPo
  static const struct { int mv, pct; } T[] = {{4200, 100}, {4150, 95}, {4110, 90}, {4080, 85}, {4020, 80}, {3980, 75},
                                              {3950, 70}, {3910, 65}, {3870, 60}, {3850, 55}, {3840, 50}, {3820, 45},
                                              {3800, 40}, {3790, 35}, {3770, 30}, {3750, 25}, {3730, 20}, {3710, 15},
                                              {3690, 10}, {3610, 5}, {3270, 0}};
  const int n = sizeof(T) / sizeof(T[0]);
  if (mv >= T[0].mv) return 100;
  for (int i = 1; i < n; i++)
    if (mv >= T[i].mv) return T[i].pct + (T[i - 1].pct - T[i].pct) * (mv - T[i].mv) / (T[i - 1].mv - T[i].mv);
  return 0;
}

#ifndef MN_HOST
#include "mn_log.h"
static const int RING = 30;              // five minutes of readings, one every 10 seconds
static int ring[RING];
static int nring = 0, smooth = 0;
static bool present = false, charging = false;
static uint32_t lastAt = 0, loggedAt = 0, pendingAt = 0;
static bool pendingUp = false;
static int pendingBase = 0;

static int readMv() {
  uint32_t mv = 0;
  for (int k = 0; k < 16; k++) mv += analogReadMilliVolts(PIN_BAT);
  return (int)(mv / 16) * 2;           // the board halves the battery voltage
}

static int avg(int from, int n) {
  int t = 0;
  for (int i = 0; i < n; i++) t += ring[from + i];
  return t / n;
}

void batBegin() {
  analogSetPinAttenuation(PIN_BAT, ADC_11db);
  lastAt = 0;
}

void batPoll() {
  uint32_t ms = millis();
  if (lastAt && ms - lastAt < (nring < 3 ? 1000 : 10000)) return;   // quick at first, so the meter shows soon
  lastAt = ms ? ms : 1;
  int mv = readMv();
  present = mv > 2500;                 // nothing plugged in: the pin floats low
  if (!present) { nring = 0; smooth = 0; charging = false; pendingUp = false; return; }
  if (nring < RING) ring[nring++] = mv;
  else { memmove(ring, ring + 1, sizeof(int) * (RING - 1)); ring[RING - 1] = mv; }
  smooth = smooth ? (smooth * 3 + mv) / 4 : mv;
  // Charging, from how the voltage moves (the board has no charge-status wire):
  //   plug in / unplug: a jump of 40 mV or more within half a minute
  //   a long slow climb (a charge) or fall (running on the battery): 6 mV over five minutes
  //   at the top (4.17 V or more): still on the charger
  //   (a jump up has to hold for a minute before it counts: Wi-Fi and games make the voltage sag and
  //   spring back, which used to show the bolt on an unplugged unit)
  if (nring >= 10) {
    int jump = ring[nring - 1] - ring[nring - 4];
    int held = avg(nring - 5, 5) - avg(nring - 10, 3);          // the last 50 s against the readings before the step
    if (jump <= -40) charging = false;
    if (jump >= 40 && !pendingUp) { pendingUp = true; pendingAt = ms; pendingBase = ring[nring - 4]; }
    if (pendingUp && ms - pendingAt >= 60000) {
      pendingUp = false;
      if (ring[nring - 1] - pendingBase >= 45 && held >= 30) charging = true;
    }
  } else if (nring >= 4) {
    if (ring[nring - 1] - ring[nring - 4] <= -40) charging = false;
  }
  if (nring >= RING) {
    int slope = avg(RING - 5, 5) - avg(0, 5);
    if (slope >= 8) charging = true;
    else if (slope <= -4 && smooth < 4150) charging = false;
  }
  if (smooth >= 4170) charging = true;
  if (ms - loggedAt > 60000) {         // for tuning on a real board: see mini.local/log
    loggedAt = ms;
    mnLog("battery %d mV (%d%%, %s)", smooth, batPercent(), charging ? "charging" : "not charging");
  }
}

bool batPresent() { return present; }
// While charging the charger lifts the reading (80 mV or so, less near full), so take that off
static int shown = -1;
static uint32_t shownAt = 0;
int batPercent() {
  if (!present || !smooth) { shown = -1; return -1; }
  int mv = smooth;
  if (charging) mv -= mv < 4120 ? 80 : (4200 - mv > 0 ? 4200 - mv : 0);
  int p = batPercentFor(mv);
  // the shown percentage moves one point at a time (every 15 s at most), so a wrong guess about charging never
  // makes it jump
  uint32_t now = millis();
  if (shown < 0) { shown = p; shownAt = now; }
  else if (p != shown && now - shownAt >= 15000) { shown += p > shown ? 1 : -1; shownAt = now; }
  return shown;
}
bool batCharging() { return present && charging; }
int batMilliVolts() { return smooth; }
#endif
