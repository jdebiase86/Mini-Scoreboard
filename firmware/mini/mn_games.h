// The GAMES page: small games that draw straight to the screen (no big
// buffers), played with a fingertip (the stylus is a bonus). Scores and
// progress are kept in the board's settings.
#pragma once
#include <Arduino.h>
#include "mn_touch.h"

struct Minigame {
  const char* name;                       // "Penalty Kick"
  const char* blurb;                      // "Flick the ball"
  const char* key;                        // short name for the saved scores
  void (*icon)(int cx, int cy);           // drawn on the menu tile
  void (*open)();                         // starts the game (draws everything)
  void (*tap)(int x, int y);
  void (*swipe)(TouchEvent ev);
  void (*gesture)(const TouchGesture& g); // every lift of the finger, before tap / swipe (flick games)
  void (*loop)();                         // every pass of the main loop: animations and timers
};

// the games (each in its own file)
extern const Minigame GAME_KICK, GAME_FREETHROW, GAME_MEMORY, GAME_SUDOKU, GAME_SNAKE, GAME_2048, GAME_SIMON, GAME_CONNECT4,
    GAME_TRIVIA, GAME_REACTION, GAME_TOSS;

// the menu: games on pages of six
void gamesMenu();                        // draws it
enum GamesResult { GR_NONE = 0, GR_EXIT, GR_OPENED };
GamesResult gamesMenuTap(int x, int y);  // GR_EXIT = BACK to where it came from
void gamesMenuSwipe(TouchEvent ev);

// playing
bool gamesPlaying();
// true = leave the game (BACK was tapped): back to the menu
bool gamesTap(int x, int y);
void gamesSwipe(TouchEvent ev);
void gamesGesture(const TouchGesture& g);
void gamesLoop();

// ---- for the games themselves
uint32_t gRand(uint32_t n);                         // 0 .. n-1
int gGet(const char* key, int dflt);                // saved numbers
void gPut(const char* key, int value);
bool gGetBlob(const char* key, void* buf, size_t n);
void gPutBlob(const char* key, const void* buf, size_t n);
void gTopBar(const char* title, const String& right);   // BACK, the title, and a score on the right
bool gBackHit(int x, int y);
void gButton(int x0, int y0, int x1, int y1, const String& label, uint16_t fill, uint16_t ink, int font = 1);
bool gIn(int x, int y, int x0, int y0, int x1, int y1);
void gBanner(const String& text, uint16_t col, int y);  // big words across the screen
void gStars(int cx, int cy, int stars, int of = 3);      // little stars (progress)
