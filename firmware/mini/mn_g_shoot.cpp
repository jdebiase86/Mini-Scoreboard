// Penalty Kick and Free Throw: flick the ball up the screen. The speed of the
// flick is the power and its direction is the aim. A shot has to be neither too
// soft nor too hard. Each round is five shots; score enough to go up a level
// (the goalie gets sharper, the power window narrower).
#include "mn_games.h"
#include "mn_ui.h"
#include "mn_lcd.h"
#include <math.h>

namespace {
enum Phase { AIM, FLIGHT, RESULT, ROUND_END };

struct Shoot {
  bool ft = false;
  int level = 1, score = 0, shot = 0, made = 0, streak = 0, best = 0;
  Phase phase = AIM;
  uint32_t t = 0;
  int frame = 0;
  // the ball and where it ends
  float bx = 240, by = 262, tx = 240, ty = 110, sx = 240, sy = 262;
  int radius = 14;
  // the goalie
  float gx = 240, gFrom = 240, gTo = 240;
  bool dive = false;
  // what happened
  String msg;
  uint16_t msgCol = 0;
  int gained = 0;
  bool outcomeGoal = false;
  int shotKind = 0;   // 0 in the net / hoop, 1 saved / rim, 2 wide / off, 3 over / long, 4 short
  int oldBx = 240, oldBy = 262, oldGx = 240;
} S;

const int BALL_X = 240, BALL_Y = 262;
const int FRAMES = 14;

// ------------------------------------------------------------------ scenes
uint16_t grass() { return rgb(18, 70, 34); }
uint16_t floorCol() { return rgb(92, 56, 30); }

void drawKickScene() {
  lcd.fillRect(0, 36, 480, 284, grass());
  for (int i = 0; i < 6; i++) lcd.fillRect(0, 140 + i * 30, 480, 15, rgb(22, 80, 40));   // mown stripes
  // the goal: net, posts, bar
  lcd.fillRect(100, 52, 280, 78, rgb(10, 40, 24));
  for (int x = 100; x <= 380; x += 14) lcd.drawFastVLine(x, 52, 78, rgb(70, 110, 84));
  for (int y = 52; y <= 130; y += 13) lcd.drawFastHLine(100, y, 280, rgb(70, 110, 84));
  lcd.fillRect(94, 48, 292, 6, C_WHITE);
  lcd.fillRect(94, 48, 6, 86, C_WHITE);
  lcd.fillRect(380, 48, 6, 86, C_WHITE);
  lcd.drawFastHLine(60, 132, 360, C_WHITE);
  lcd.drawCircle(240, 262, 20, rgb(120, 180, 130));
  lcd.fillCircle(240, 262, 2, C_WHITE);
}

void drawFtScene() {
  lcd.fillRect(0, 36, 480, 284, floorCol());
  for (int i = 0; i < 8; i++) lcd.drawFastHLine(0, 150 + i * 22, 480, rgb(80, 48, 26));
  lcd.drawFastHLine(120, 300, 240, C_WHITE);
  lcd.drawCircle(240, 262, 60, rgb(220, 200, 170));
  // backboard, target square, rim, net
  lcd.fillRoundRect(190, 46, 100, 60, 4, rgb(235, 238, 245));
  lcd.drawRect(222, 74, 36, 26, rgb(230, 80, 40));
  lcd.fillRect(206, 108, 68, 5, rgb(255, 110, 30));
  for (int i = 0; i <= 5; i++) lcd.drawLine(210 + i * 12, 113, 218 + i * 8, 140, rgb(235, 235, 235));
  lcd.drawLine(218, 140, 262, 140, rgb(235, 235, 235));
}

void drawScene() { S.ft ? drawFtScene() : drawKickScene(); }

void drawGoalie(int gx) {
  if (S.ft) return;
  int lean = (int)((gx - 240) / 6);
  lcd.fillRect(gx - 12 + lean / 2, 100, 24, 30, rgb(255, 160, 30));         // body
  lcd.fillCircle(gx + lean / 3, 92, 8, rgb(240, 200, 160));                  // head
  lcd.drawLine(gx - 12, 104, gx - 26 + lean / 2, 92, rgb(255, 160, 30));     // arms up
  lcd.drawLine(gx + 12, 104, gx + 26 + lean / 2, 92, rgb(255, 160, 30));
  lcd.fillCircle(gx - 26 + lean / 2, 90, 4, C_WHITE);                        // gloves
  lcd.fillCircle(gx + 26 + lean / 2, 90, 4, C_WHITE);
  lcd.fillRect(gx - 10 + lean / 2, 128, 8, 8, rgb(30, 30, 40));              // boots
  lcd.fillRect(gx + 2 + lean / 2, 128, 8, 8, rgb(30, 30, 40));
}

void drawBall(int x, int y, int r) {
  if (S.ft) {
    lcd.fillCircle(x, y, r, rgb(238, 120, 30));
    lcd.drawCircle(x, y, r, rgb(60, 30, 10));
    lcd.drawFastHLine(x - r, y, r * 2, rgb(60, 30, 10));
    lcd.drawFastVLine(x, y - r, r * 2, rgb(60, 30, 10));
  } else {
    lcd.fillCircle(x, y, r, C_WHITE);
    lcd.drawCircle(x, y, r, rgb(40, 40, 50));
    lcd.fillCircle(x, y, r / 3, rgb(40, 40, 50));
    lcd.fillCircle(x - r / 2 - 1, y - r / 2, r / 5 + 1, rgb(40, 40, 50));
    lcd.fillCircle(x + r / 2 + 1, y - r / 2, r / 5 + 1, rgb(40, 40, 50));
  }
}

// repaint one small area of the scene (with the goalie and ball over it)
void repaint(int x, int y, int w, int h) {
  if (x < 0) { w += x; x = 0; }
  if (y < 36) { h -= 36 - y; y = 36; }
  if (x + w > 480) w = 480 - x;
  if (y + h > 320) h = 320 - y;
  if (w <= 0 || h <= 0) return;
  lcd.setClipRect(x, y, w, h);
  drawScene();
  drawGoalie((int)S.gx);
  drawBall((int)S.bx, (int)S.by, S.radius);
  lcd.clearClipRect();
}

void header() {
  String right = String("Score ") + String(S.score);
  gTopBar(S.ft ? "Free Throw" : "Penalty Kick", right);
  String sub = String("Level ") + String(S.level) + "   Shot " + String(min(S.shot + 1, 5)) + " of 5";
  uiText(F_S13, sub, 240, 41, C_WHITE, S.ft ? floorCol() : grass(), middle_center);
}

void fullDraw() {
  drawScene();
  drawGoalie((int)S.gx);
  drawBall((int)S.bx, (int)S.by, S.radius);
  header();
  if (S.phase == AIM && S.shot == 0 && S.score == 0) {
    uiText(F_B16, "Flick the ball up the screen", 240, 168, C_WHITE, S.ft ? floorCol() : grass(), middle_center);
    uiText(F_S13, "Aim = where you flick. Power = how fast.", 240, 190, C_WHITE, S.ft ? floorCol() : grass(), middle_center);
    uiText(F_S13, "Not too soft, not too hard.", 240, 208, C_WHITE, S.ft ? floorCol() : grass(), middle_center);
  }
}

// ------------------------------------------------------------------- rules
float loPower() { return S.ft ? 0.95f - (0.35f - 0.015f * S.level > 0.16f ? 0.35f - 0.015f * S.level : 0.16f) : 0.45f + 0.02f * (S.level - 1); }
float hiPower() { return S.ft ? 0.95f + (0.35f - 0.015f * S.level > 0.16f ? 0.35f - 0.015f * S.level : 0.16f) : fmaxf(0.95f, 1.5f - 0.04f * (S.level - 1)); }

void newShot() {
  S.phase = AIM;
  S.bx = BALL_X; S.by = BALL_Y; S.radius = 14;
  S.gx = 240; S.dive = false;
  S.frame = 0;
  fullDraw();
}

void startRound() {
  S.score = 0; S.shot = 0; S.made = 0; S.streak = 0;
  newShot();
}

int target() { return S.ft ? 2 + S.level / 2 : 4 + S.level; }   // free throws made / kick points to go up

// the shot decides: what happens and where the ball ends
void shoot(const TouchGesture& g) {
  float dx = (float)(g.x1 - g.x0), dy = (float)(g.y1 - g.y0);
  if (dy > -30 || g.y0 < 130) return;                        // up the screen, started in the lower part
  float speed = sqrtf(dx * dx + dy * dy) / (float)(g.ms > 0 ? g.ms : 1);
  float tan = dx / -dy;
  if (tan > 1.4f) tan = 1.4f;
  if (tan < -1.4f) tan = -1.4f;
  float lo = loPower(), hi = hiPower();
  S.sx = S.bx; S.sy = S.by;
  S.msg = ""; S.gained = 0; S.outcomeGoal = false;
  if (!S.ft) {
    S.tx = 240 + tan * 200;
    float k = (speed - lo) / (hi - lo);
    S.ty = 118 - fminf(fmaxf(k, 0), 1) * 58;
    float off = fabsf(S.tx - 240);
    S.gFrom = 240;
    if (speed < lo) {               // a soft roll: the keeper just picks it up
      S.shotKind = 4; S.tx = 240 + tan * 60; S.ty = 150; S.gTo = S.tx;
      S.msg = "TOO SOFT"; S.msgCol = C_RED;
    } else if (speed > hi) {        // blasted over the bar
      S.shotKind = 3; S.ty = 28; S.gTo = 240 + (gRand(2) ? 40 : -40);
      S.msg = "OVER THE BAR!"; S.msgCol = C_RED;
    } else if (off > 142) {         // outside the post
      S.shotKind = 2; S.gTo = 240 + (S.tx > 240 ? -40 : 40);
      S.msg = "WIDE!"; S.msgCol = C_RED;
    } else {
      float pGuess = fminf(0.65f, 0.2f + 0.06f * S.level);
      bool guessed = gRand(1000) < (uint32_t)(pGuess * 1000);
      float reach = (38 + 3 * S.level) * (S.ty < 80 ? 0.6f : 1.0f);
      if (guessed) S.gTo = S.tx + (float)((int)gRand(41) - 20);
      else { float side = (S.tx >= 240) ? -1.0f : 1.0f; if (gRand(3) == 0) side = -side; S.gTo = S.tx + side * (float)(70 + gRand(60)); }
      if (S.gTo < 130) S.gTo = 130;
      if (S.gTo > 350) S.gTo = 350;
      if (fabsf(S.gTo - S.tx) < reach) {
        S.shotKind = 1; S.msg = "SAVED!"; S.msgCol = C_RED;
      } else {
        S.shotKind = 0; S.outcomeGoal = true;
        S.gained = 1 + (off > 85 ? 1 : 0) + (S.ty < 92 ? 1 : 0);
        S.msg = S.gained == 3 ? "TOP CORNER! +3" : String("GOAL! +") + String(S.gained);
        S.msgCol = C_GREEN;
      }
    }
  } else {
    S.tx = 240 + tan * 160;
    float vmid = 0.95f;
    float half = (hi - lo) / 2;
    float tol = fmaxf(12.0f, 24.0f - 0.8f * S.level);
    bool powerOk = speed >= lo && speed <= hi;
    bool aimOk = fabsf(S.tx - 240) <= tol;
    S.ty = 108;
    if (!powerOk) {
      S.shotKind = speed < lo ? 4 : 3;
      S.msg = speed < lo ? "SHORT!" : "TOO HARD!"; S.msgCol = C_RED;
      S.ty = speed < lo ? 150 : 70;
    } else if (!aimOk) {
      S.shotKind = 2; S.msg = S.tx < 240 ? "LEFT OF THE RIM" : "RIGHT OF THE RIM"; S.msgCol = C_RED;
      S.tx = 240 + (S.tx < 240 ? -tol - 14 : tol + 14);
    } else {
      S.shotKind = 0; S.outcomeGoal = true;
      bool swish = fabsf(S.tx - 240) < tol * 0.4f && fabsf(speed - vmid) < half * 0.4f;
      S.streak++;
      S.gained = (swish ? 3 : 2) + (S.streak >= 3 ? 1 : 0);
      S.msg = String(swish ? "SWISH! +" : "BUCKET! +") + String(S.gained);
      S.msgCol = C_GREEN;
      S.tx = 240 + (S.tx - 240) * 0.3f;
    }
    if (!S.outcomeGoal) S.streak = 0;
  }
  S.phase = FLIGHT;
  S.frame = 0;
  S.t = millis();
  S.dive = !S.ft;
  S.oldBx = (int)S.bx; S.oldBy = (int)S.by; S.oldGx = (int)S.gx;
  if (S.shot == 0 && S.score == 0) fullDraw();   // clears the instructions
}

void endRound() {
  S.phase = ROUND_END;
  bool up = S.ft ? S.made >= target() : S.score >= target();
  int best = gGet(S.ft ? "ft_best" : "kick_best", 0);
  if (S.score > best) { best = S.score; gPut(S.ft ? "ft_best" : "kick_best", best); }
  if (up && S.level < 20) { S.level++; gPut(S.ft ? "ft_lv" : "kick_lv", S.level); }
  S.best = best;
  lcd.fillRect(40, 70, 400, 190, C_BG);
  uiTile(40, 70, 440, 260, C_TILE, C_EDGE, 16, 2);
  uiText(F_B24, "ROUND OVER", 240, 98, C_WHITE, C_TILE, middle_center);
  uiText(F_B36, String(S.score) + (S.ft ? "  pts" : " pts"), 240, 144, C_YELLOW, C_TILE, middle_center);
  uiText(F_S13, String("Best ever: ") + String(best), 240, 178, C_GREY, C_TILE, middle_center);
  if (up) uiText(F_B18, String("LEVEL UP!  Now level ") + String(S.level), 240, 206, C_GREEN, C_TILE, middle_center);
  else uiText(F_S13, S.ft ? String("Make ") + String(target()) + " baskets to go up a level" : String("Score ") + String(target()) + " to go up a level", 240, 206, C_GREY, C_TILE, middle_center);
  uiText(F_B16, "Tap to play again", 240, 238, C_WHITE, C_TILE, middle_center);
}

void finishShot() {
  S.shot++;
  if (S.outcomeGoal) { S.score += S.gained; S.made++; }
  header();
  if (S.shot >= 5) endRound();
  else newShot();
}

void open(bool ft) {
  S.ft = ft;
  S.level = gGet(ft ? "ft_lv" : "kick_lv", 1);
  S.best = gGet(ft ? "ft_best" : "kick_best", 0);
  lcd.fillScreen(C_BG);
  startRound();
}
void openKick() { open(false); }
void openFt() { open(true); }

void tap(int, int) {
  if (S.phase == ROUND_END) startRound();
}

void tick() {
  uint32_t now = millis();
  if (S.phase == FLIGHT) {
    if (now - S.t < 24) return;
    S.t = now;
    S.frame++;
    float k = (float)S.frame / FRAMES;
    float e = 1 - (1 - k) * (1 - k);                       // fast, then slowing
    int ox = (int)S.bx, oy = (int)S.by, og = (int)S.gx;
    S.bx = S.sx + (S.tx - S.sx) * e;
    float arc = S.ft ? 90.0f * 4 * k * (1 - k) : 0;
    S.by = S.sy + (S.ty - S.sy) * e - arc;
    S.radius = 14 - (int)(6 * e);
    if (S.dive) S.gx = S.gFrom + (S.gTo - S.gFrom) * (k < 0.15f ? 0 : (k - 0.15f) / 0.85f);
    repaint(ox - 20, oy - 20, 40, 40);
    repaint(og - 40, 84, 80, 56);
    repaint((int)S.bx - 20, (int)S.by - 20, 40, 40);
    repaint((int)S.gx - 40, 84, 80, 56);
    if (S.frame >= FRAMES) {
      S.phase = RESULT;
      S.t = now;
      gBanner(S.msg, S.msgCol, 188);
    }
  } else if (S.phase == RESULT && now - S.t > 1200) {
    finishShot();
  }
}

void gesture(const TouchGesture& g) {
  if (S.phase == AIM) shoot(g);
}

void iconKick(int cx, int cy) {
  lcd.fillRoundRect(cx - 34, cy - 20, 68, 38, 4, rgb(18, 70, 34));
  lcd.drawRect(cx - 30, cy - 18, 60, 22, C_WHITE);
  lcd.fillCircle(cx + 6, cy + 8, 9, C_WHITE);
  lcd.fillCircle(cx + 6, cy + 8, 3, rgb(40, 40, 50));
}
void iconFt(int cx, int cy) {
  lcd.fillRect(cx - 22, cy - 22, 44, 26, rgb(235, 238, 245));
  lcd.fillRect(cx - 14, cy + 2, 28, 3, rgb(255, 110, 30));
  lcd.fillCircle(cx + 14, cy + 18, 9, rgb(238, 120, 30));
  lcd.drawFastHLine(cx + 5, cy + 18, 18, rgb(60, 30, 10));
}
}  // namespace

const Minigame GAME_KICK = {"Penalty Kick", "Flick it past the goalie", "kick", iconKick, openKick, tap, nullptr, gesture, tick};
const Minigame GAME_FREETHROW = {"Free Throw", "Aim and power", "ft", iconFt, openFt, tap, nullptr, gesture, tick};
