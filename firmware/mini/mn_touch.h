// Taps on the resistive touch screen. A tap counts when the finger lifts,
// at the spot it first pressed (resistive screens wobble while pressed).
#pragma once
#include <Arduino.h>

void touchBegin();                  // applies the saved touch setup, if any
bool touchPoll(int& x, int& y);     // true once per tap; call often
bool touchDown();                   // a finger is on the screen right now
uint32_t touchLastActivity();       // millis() of the last press

// The first-start touch setup: four arrows to tap, then a check. Saves it.
void touchCalibrate();
