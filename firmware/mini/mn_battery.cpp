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
static const int RING = 12;              // two minutes of readings, one every 10 seconds
static int ring[RING];
static int nring = 0, smooth = 0;
static bool present = false, charging = false;
static uint32_t lastAt = 0;

static int readMv() {
  uint32_t mv = 0;
  for (int k = 0; k < 16; k++) mv += analogReadMilliVolts(PIN_BAT);
  return (int)(mv / 16) * 2;           // the board halves the battery voltage
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
  if (!present) { nring = 0; smooth = 0; charging = false; return; }
  if (nring < RING) ring[nring++] = mv;
  else { memmove(ring, ring + 1, sizeof(int) * (RING - 1)); ring[RING - 1] = mv; }
  smooth = smooth ? (smooth * 3 + mv) / 4 : mv;
  // rising by 10 mV or more over the last minutes: charging; falling: not
  if (nring >= 6) {
    int a = (ring[0] + ring[1] + ring[2]) / 3, b = (ring[nring - 1] + ring[nring - 2] + ring[nring - 3]) / 3;
    if (b - a >= 10) charging = true;
    else if (a - b >= 10) charging = false;
  }
  if (smooth >= 4170) charging = true;   // full and still on the charger
}

bool batPresent() { return present; }
int batPercent() { return present && smooth ? batPercentFor(smooth) : -1; }
bool batCharging() { return present && charging; }
int batMilliVolts() { return smooth; }
#endif
