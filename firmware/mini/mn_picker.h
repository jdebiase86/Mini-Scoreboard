// Picking teams on the mini itself (EDIT on the home screen). The phone
// setup page stays the main way; this is the handy extra.
//   Leagues: one tile per league with how many are picked, and DONE.
//   College football: pick a conference first.
//   Teams: a list, 10 a page in two columns, up / down arrows on the right;
//   tap a row to tick or untick it.
// Changes are kept aside until DONE, which saves them (up to 10 teams).
#pragma once
#include <Arduino.h>

void pickerStart();              // takes a copy of the current teams, draws the league tiles
bool pickerTap(int x, int y);    // true once DONE was tapped (saved)
void pickerDraw();               // redraw the current picker screen
void pickerLoop();               // call often: puts the count back after "10 is the most"
