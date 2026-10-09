// Updates over Wi-Fi from this repo's GitHub releases (mini-X.Y.bin). Runs in
// the background: a few minutes after start-up, nightly around 4 am, and
// when "Check for updates now" is tapped on the settings page.
#pragma once
#include <Arduino.h>

extern volatile int otaPercent;       // -1 = not updating, else 0-100
extern char otaNewVersion[16];        // what it's installing
void otaStart();                      // starts the background task
void otaRequest();                    // "Check for updates now"
String otaStatus();                   // one line for the settings page
int versionCompare(const char* a, const char* b);
bool versionFromAsset(const char* name, char (&out)[16]);   // "mini-0.2.bin" -> "0.2"
