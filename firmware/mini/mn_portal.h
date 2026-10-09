// The setup / settings web page. On first power-up the mini makes its own
// Wi-Fi network ("Mini-Scoreboard-XXXX"); joining it on a phone (scan the
// code on the screen) pops this page up. Once the mini is on the home Wi-Fi,
// the same page (minus the Wi-Fi part, unless asked) is at http://mini.local.
#pragma once
#include <Arduino.h>

void portalStartAP(const String& apName);   // setup network + captive page
void portalStopAP();
void portalStartHome();                      // settings page on the home network
void portalLoop();
bool portalAPRunning();
// set when the page saved something that needs a restart (new Wi-Fi details,
// screen turned over, colour mode, touch setup to redo)
extern volatile bool portalRestart;
extern volatile uint32_t portalSavedAt;
// set when the page saved teams / brightness / time zone (no restart needed)
extern volatile bool portalChanged;
// last time someone loaded or saved the page (0 = never)
extern volatile uint32_t portalUsedAt;
