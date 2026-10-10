// What the unit remembers about itself, for finding out why something went wrong:
//  - why it last restarted (power, a crash, a stuck program, or on purpose) and the last log lines before that
//    (kept in the chip's RTC memory, which survives a restart but not a power cut)
//  - the battery voltage every five minutes for a day (also saved to flash once an hour)
// Shown on the About page (tap the battery on the home screen) and at http://mini.local/battery.
#pragma once
#include <Arduino.h>

struct BatSample { uint32_t minute; uint16_t mv; uint8_t pct; uint8_t flags; };   // minute: seconds since 1970 / 60 (0 = clock not set); flags: 1 = charging
static const int DIAG_BAT_N = 288;                 // a day of five-minute readings
static const int DIAG_LINES = 10, DIAG_LEN = 100;

void diagBegin();                                  // first thing in setup()
void diagTick();                                   // every pass of the loop
void diagNote(const char* why);                    // just before a restart on purpose
void diagLogLine(const char* line);                // from mnLog

// the last restart
String diagRestartWhy();                           // "Crash", "Stuck (the watchdog restarted it)", "Power on" ...
bool diagRestartBad();                             // a crash, a stuck program or a power dip
uint32_t diagPrevRunSecs();                        // how long the run before it lasted (0 = unknown)
uint32_t diagBoots();
int diagPrevLineCount();
const char* diagPrevLine(int i);                   // oldest first

// battery history
void diagBatSample(int mv, int pct, bool charging);
int diagBatCount();
BatSample diagBatAt(int i);                        // oldest first
String diagBatCsv();                               // for /battery
