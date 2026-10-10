#include "mn_keyboard.h"
#include "mn_ui.h"
#include "mn_lcd.h"

static String text, title;
static bool secret = true, reveal = false;
static int maxLen = 63;
static int layer = 0;   // 0 lower case, 1 UPPER CASE, 2 symbols, 3 more symbols

// special keys (anything below 32 is not a letter)
enum { K_NONE = 0, K_SHIFT = 1, K_BACK, K_MODE, K_SPACE, K_OK, K_CANCEL, K_EYE, K_MORE, K_FIELD };

static const int ROW_Y0 = 78, ROW_PITCH = 48, KEY_H = 44, PITCH = 47, KEY_W = 43, X0 = 5;

static const char* const ROWS[4][4] = {
    {"1234567890", "qwertyuiop", "asdfghjkl-", "zxcvbnm"},
    {"1234567890", "QWERTYUIOP", "ASDFGHJKL_", "ZXCVBNM"},
    {"1234567890", "!@#$%^&*()", "-_=+/?:;'\"", "<>~`|\\"},
    {"1234567890", "", "", "[]{}"}};

struct Key { int x0, y0, x1, y1; int code; String label; };

// every key of the current layer, in drawing order
static int layout(Key* k) {
  int n = 0;
  auto add = [&](int x0, int y0, int w, int code, const String& label) {
    k[n++] = {x0, y0, x0 + w, y0 + KEY_H, code, label};
  };
  for (int r = 0; r < 4; r++) {
    const char* s = ROWS[layer][r];
    int y = ROW_Y0 + r * ROW_PITCH;
    int x = r == 3 ? 75 : X0;
    for (int i = 0; s[i]; i++) add(x + i * PITCH, y, KEY_W, s[i], String(s[i]));
    if (r == 3) {
      if (layer <= 1) add(X0, y, 66, K_SHIFT, layer == 1 ? "ABC" : "abc");
      else add(X0, y, 66, K_MORE, layer == 2 ? "more" : "back");
      add(409, y, 66, K_BACK, "DEL");
    }
  }
  int y = ROW_Y0 + 4 * ROW_PITCH;
  add(X0, y, 70, K_MODE, layer >= 2 ? "abc" : "123/#");
  add(79, y, KEY_W, ',', ",");
  add(126, y, 200, K_SPACE, "space");
  add(330, y, KEY_W, '.', ".");
  add(377, y, 98, K_OK, "JOIN");
  return n;
}

static void drawKey(const Key& k, bool pressed) {
  uint16_t fill = pressed ? C_AUTO_EDGE : C_TILE_HI, ink = C_WHITE;
  if (k.code == K_OK) { fill = pressed ? C_WHITE : C_GREEN; ink = pressed ? C_BG : C_WHITE; }
  else if (k.code < 32 && k.code != K_SPACE) fill = pressed ? C_AUTO_EDGE : C_TILE;
  uiTile(k.x0, k.y0, k.x1, k.y1, fill, C_EDGE, 7, 1);
  bool small = k.label.length() > 1;
  uiText(small ? F_B12 : F_B18, k.label, (k.x0 + k.x1) / 2, (k.y0 + k.y1) / 2, ink, fill, middle_center);
}

static const int FX0 = 8, FY0 = 36, FX1 = 424, FY1 = 70;

static void drawField() {
  uiTile(FX0, FY0, FX1, FY1, C_TILE, C_EDGE, 9, 1);
  String shown;
  if (secret && !reveal) { for (unsigned i = 0; i < text.length(); i++) shown += '*'; }
  else shown = text;
  useFont(F_B18);
  while (shown.length() > 1 && lcd.textWidth((shown + "_").c_str()) > FX1 - FX0 - 24) shown = shown.substring(1);
  uiText(F_B18, shown + "_", FX0 + 12, (FY0 + FY1) / 2, C_WHITE, C_TILE, middle_left);
  uiTile(430, FY0, 474, FY1, C_TILE, C_EDGE, 9, 1);
  uiText(F_B12, secret ? (reveal ? "HIDE" : "SHOW") : "", 452, (FY0 + FY1) / 2, C_GREY, C_TILE, middle_center);
}

static void drawKeys() {
  lcd.fillRect(0, ROW_Y0 - 3, SCREEN_W, SCREEN_H - ROW_Y0 + 3, C_BG);
  Key k[48];
  int n = layout(k);
  for (int i = 0; i < n; i++) drawKey(k[i], false);
}

void kbOpen(const String& t, const String& initial, bool sec, int maxL) {
  title = t;
  text = initial;
  secret = sec;
  reveal = !sec;
  maxLen = maxL;
  layer = 0;
}

void kbDraw() {
  lcd.fillScreen(C_BG);
  uiTile(6, 3, 96, 29, C_TILE, C_EDGE, 13, 1);
  uiText(F_B12, "CANCEL", 51, 16, C_WHITE, C_TILE, middle_center);
  String t = title;
  useFont(F_B16);
  while (t.length() > 3 && lcd.textWidth(t.c_str()) > 360) t = t.substring(0, t.length() - 1);
  uiText(F_B16, t, 290, 16, C_WHITE, C_BG, middle_center);
  drawField();
  drawKeys();
}

const String& kbText() { return text; }

KbResult kbTap(int x, int y) {
  if (x < 104 && y < 34) return KB_CANCEL;
  if (y >= FY0 - 4 && y < FY1 + 4 && x >= 426 && secret) {
    reveal = !reveal;
    drawField();
    return KB_NONE;
  }
  Key k[48];
  int n = layout(k);
  // taps on a resistive screen land a little off: take the nearest key within 12 dots
  int best = -1, bestD = 13 * 13;
  for (int i = 0; i < n; i++) {
    int dx = x < k[i].x0 ? k[i].x0 - x : x > k[i].x1 ? x - k[i].x1 : 0;
    int dy = y < k[i].y0 ? k[i].y0 - y : y > k[i].y1 ? y - k[i].y1 : 0;
    int d = dx * dx + dy * dy;
    if (d < bestD) { bestD = d; best = i; }
  }
  if (best < 0) return KB_NONE;
  const Key& key = k[best];
  drawKey(key, true);
  delay(60);
  switch (key.code) {
    case K_OK:
      return text.length() ? KB_OK : (drawKey(key, false), KB_NONE);
    case K_SHIFT: layer = layer == 0 ? 1 : 0; drawKeys(); return KB_NONE;
    case K_MORE: layer = layer == 2 ? 3 : 2; drawKeys(); return KB_NONE;
    case K_MODE: layer = layer >= 2 ? 0 : 2; drawKeys(); return KB_NONE;
    case K_BACK:
      if (text.length()) text.remove(text.length() - 1);
      drawKey(key, false);
      drawField();
      return KB_CHANGED;
    default: {
      char c = key.code == K_SPACE ? ' ' : (char)key.code;
      if ((int)text.length() < maxLen) text += c;
      drawKey(key, false);
      drawField();
      if (layer == 1) { layer = 0; drawKeys(); }   // one capital letter, then back to small ones
      return KB_CHANGED;
    }
  }
}
