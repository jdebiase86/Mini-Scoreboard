// A pretend Wi-Fi radio, so the Wi-Fi screen (mn_wifi.cpp) can be drawn on a computer.
#pragma once
#include "Arduino.h"
#include <vector>
enum wl_status_t { WL_IDLE_STATUS = 0, WL_NO_SSID_AVAIL = 1, WL_CONNECTED = 3, WL_CONNECT_FAILED = 4, WL_DISCONNECTED = 6 };
enum { WIFI_AUTH_OPEN = 0, WIFI_AUTH_WPA2_PSK = 3 };
struct FakeNet { std::string ssid; int rssi; int auth; };
struct FakeWiFi {
  std::vector<FakeNet> found;
  std::string joined;
  wl_status_t st = WL_DISCONNECTED;
  wl_status_t status() { return st; }
  String SSID() { return joined; }
  String SSID(int i) { return found[i].ssid; }
  int scanNetworks(bool = false, bool = false) { return found.size(); }
  int scanComplete() { return found.size(); }
  void scanDelete() {}
  int RSSI(int i) { return found[i].rssi; }
  int encryptionType(int i) { return found[i].auth; }
  void disconnect(bool = false) {}
  void begin(const char*, const char*) {}
  void setAutoReconnect(bool) {}
};
extern FakeWiFi WiFi;
struct FakeEsp { void restart() {} };
extern FakeEsp ESP;
