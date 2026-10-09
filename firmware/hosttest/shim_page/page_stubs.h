// Stand-ins for the ESP32 Wi-Fi and web server, just enough to build the
// real setup page (mn_portal.cpp) on a computer: page_host.cpp.
#pragma once
#include <Arduino.h>
#include <functional>
#include <vector>
#include <stdio.h>
#define PROGMEM
#define FPSTR(x) (x)
#define HTTP_GET 1
#define HTTP_POST 2
template <class T, class A, class B> T constrain(T v, A a, B b) { return v < (T)a ? (T)a : v > (T)b ? (T)b : v; }
struct CaptiveDns { void begin(uint32_t) {} void stop() {} void process() {} };
struct IPAddress { String toString() const { return "192.168.4.1"; } operator uint32_t() const { return 0; } };
struct WiFiClient {
  void print(const String&) {}
  void write(const uint8_t*, size_t) {}
};
struct WiFiUDP {
  bool begin(int) { return true; }
  void stop() {}
  int parsePacket() { return 0; }
  void flush() {}
  int read(uint8_t*, int) { return 0; }
  IPAddress remoteIP() { return {}; }
  int remotePort() { return 0; }
  void beginPacket(IPAddress, int) {}
  void write(const uint8_t*, size_t) {}
  void endPacket() {}
};
#define WIFI_AP_STA 3
#define WIFI_STA 1
struct WiFiClass {
  void mode(int) {}
  int scanNetworks() { return 3; }
  String SSID(int i) { const char* n[] = {"Home Wi-Fi", "Home Wi-Fi 5G", "Neighbour"}; return n[i]; }
  void scanDelete() {}
  void softAP(const char*) {}
  void softAPdisconnect(bool) {}
  IPAddress softAPIP() { return {}; }
};
extern WiFiClass WiFi;
struct WebServer {
  String body;
  WebServer(int) {}
  void on(const char*, int, std::function<void()>) {}
  void onNotFound(std::function<void()>) {}
  void begin() {}
  void handleClient() {}
  void send(int, const char*, const String& b) { body = b; }
  void sendHeader(const char*, const String&, bool = false) {}
  int args() { return 0; }
  String argName(int) { return ""; }
  String arg(const char*) { return ""; }
  String arg(int) { return ""; }
  bool hasArg(const char*) { return false; }
  WiFiClient client() { return {}; }
};
struct MDNSClass { bool begin(const char*) { return true; } void addService(const char*, const char*, int) {} };
extern MDNSClass MDNS;
