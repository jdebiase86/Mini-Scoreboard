// An on-screen keyboard for typing a Wi-Fi password on the mini itself
// (letters, a number row, symbols). Tap keys with a fingertip or the stylus.
#pragma once
#include <Arduino.h>

enum KbResult { KB_NONE = 0, KB_CHANGED, KB_OK, KB_CANCEL };

// secret: shows stars instead of the letters, with a SHOW button to check them
void kbOpen(const String& title, const String& initial, bool secret, int maxLen);
void kbDraw();                       // the whole keyboard
KbResult kbTap(int x, int y);        // draws its own changes
const String& kbText();
