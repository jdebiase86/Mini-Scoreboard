// The About page: version, why it last restarted (and the log lines just before), Wi-Fi, memory, battery and the
// battery's last day. Opened by tapping the battery at the top of the home screen or a game screen.
#pragma once
#include <Arduino.h>

struct AboutData {
  String version, ssid;
  int rssi = 0;                 // dBm, 0 = not connected
  uint32_t upSecs = 0;
  uint32_t freeKb = 0, biggestKb = 0;
  int mv = 0, pct = -1;         // pct -1 = no battery
  bool charging = false, saver = false;
};

void aboutOpen(const AboutData& d);     // draws the first page
void aboutRefresh(const AboutData& d);  // every few seconds: the numbers
bool aboutTap(int x, int y, const AboutData& d);   // true = BACK was tapped
