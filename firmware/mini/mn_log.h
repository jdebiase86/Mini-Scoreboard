// Log lines go to the USB serial port and into a small ring that the
// settings page shows at http://mini.local/log - so a problem can be seen
// from a phone, no cable needed.
#pragma once
#include <Arduino.h>
void mnLog(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
String mnLogText();
