// Builds the mini's real setup page (mn_portal.cpp) on a computer and writes
// it to out/setup.html (first start) and out/settings.html (mini.local), to
// look at in a browser.   ./page_host.sh
#include "../mini/mn_portal.cpp"
#include "../mini/mn_settings.h"
WiFiClass WiFi;
MDNSClass MDNS;
Settings settings;
uint32_t millis() { return 1; }
void delay(uint32_t) {}
String otaStatus() { return "8:41 PM: up to date (" FW_VERSION ")"; }
void otaRequest() {}
void mnLog(const char*, ...) {}
String mnLogText() { return ""; }
void Settings::forgetCal() {}
static const char* const LK[L_COUNT] = {"NFL", "CFB", "MLB", "NHL", "NBA"};
String teamKey(int i) { return String(LK[TEAMS[i].league]) + ":" + TEAMS[i].abbr; }
void Settings::save() {}

static void write(const char* path) {
  FILE* f = fopen(path, "w");
  fputs(server.body.c_str(), f);
  fclose(f);
}

int main() {
  apOn = true;
  settings.setPicksFromString("NFL:DAL,CFB:LSU");   // as if two boxes were ticked
  scanNetworks();
  handleRoot();
  write("out/setup.html");
  apOn = false;
  settings.ssid = "Home Wi-Fi";
  settings.setPicksFromString("NFL:NYG,CFB:FLA,MLB:NYY");
  handleRoot();
  write("out/settings.html");
  return 0;
}
