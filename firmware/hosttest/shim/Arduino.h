// Just enough of Arduino to draw the mini's screens on a computer
// (render_screens.cpp). Not used on the board.
#pragma once
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <string>
#include <stdlib.h>
#include <ctype.h>

#ifndef DEG_TO_RAD
#define DEG_TO_RAD 0.017453292519943295f
#endif

class String : public std::string {
 public:
  String() {}
  String(const char* s) : std::string(s ? s : "") {}
  String(const std::string& s) : std::string(s) {}
  String(int v) : std::string(std::to_string(v)) {}
  unsigned length() const { return size(); }
  String substring(int a, int b = -1) const {
    int n = size();
    if (a > n) a = n;
    if (b < 0 || b > n) b = n;
    return b > a ? String(std::string::substr(a, b - a)) : String();
  }
  int lastIndexOf(char c) const { auto p = rfind(c); return p == npos ? -1 : (int)p; }
  int lastIndexOf(char c, int from) const { if (from < 0) return -1; auto p = rfind(c, from); return p == npos ? -1 : (int)p; }
  int indexOf(char c, int from = 0) const { auto p = find(c, from); return p == npos ? -1 : (int)p; }
  int indexOf(const String& s) const { auto p = find(s); return p == npos ? -1 : (int)p; }
  void trim() { erase(0, find_first_not_of(" \t\r\n")); erase(find_last_not_of(" \t\r\n") + 1); }
  void toUpperCase() { for (auto& c : *this) c = toupper((unsigned char)c); }
  void toLowerCase() { for (auto& c : *this) c = tolower((unsigned char)c); }
  long toInt() const { return atol(c_str()); }
  String& operator+=(const String& o) { append(o); return *this; }
  String& operator+=(const char* o) { append(o); return *this; }
  String& operator+=(char c) { push_back(c); return *this; }
};
inline String operator+(const String& a, const char* b) { return String(std::string(a) + b); }
inline String operator+(const char* a, const String& b) { return String(a + std::string(b)); }
inline String operator+(const String& a, const String& b) { return String(std::string(a) + std::string(b)); }
inline String operator+(const String& a, char b) { return String(std::string(a) + b); }

uint32_t millis();
void delay(uint32_t ms);
