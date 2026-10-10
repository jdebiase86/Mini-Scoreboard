// The Wi-Fi screen: networks the mini can see, tap one to join it. Secured
// ones ask for the password on the on-screen keyboard. The mini remembers up
// to five networks and joins whichever it can find. Tap a remembered one to
// forget it. (The phone setup page stays the way to do the very first setup.)
#pragma once
#include <Arduino.h>

enum WifiResult { WR_STAY = 0, WR_BACK };

void wifiStart();                    // opens the screen and starts looking for networks
void wifiDraw();                     // everything again (after a wake-up or a swipe)
WifiResult wifiTap(int x, int y);
void wifiSwipe(bool up);             // list: swipe up = next page
WifiResult wifiLoop();               // call often: shows the networks when found, runs a join
bool wifiBusy();                     // a join is under way (don't dim the screen)

// The best remembered network that's in range right now (strongest first), or
// -1: used at start-up and when the mini has been carried somewhere else.
int wifiBestSaved();
