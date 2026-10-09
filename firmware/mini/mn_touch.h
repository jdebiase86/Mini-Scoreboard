// Taps and swipes on the resistive touch screen. Both count when the finger
// lifts. A tap is reported at the spot it first pressed (resistive screens
// wobble while pressed); a finger that moved far enough is a swipe.
#pragma once
#include <Arduino.h>

void touchBegin();                  // applies the saved touch setup, if any
// What a finger just did. SWIPE_LEFT = the finger moved right to left (next
// page), SWIPE_UP = bottom to top (further down a list). T_NONE is 0, so
// "if (touchPoll(x, y))" means "something happened".
enum TouchEvent { T_NONE = 0, T_TAP, T_SWIPE_LEFT, T_SWIPE_RIGHT, T_SWIPE_UP, T_SWIPE_DOWN };
TouchEvent touchPoll(int& x, int& y);   // x, y: where the finger first pressed; call often
bool touchDown();                   // a finger is on the screen right now
uint32_t touchLastActivity();       // millis() of the last press

// The first-start touch setup: four arrows to tap, then a check. Saves it.
void touchCalibrate();
