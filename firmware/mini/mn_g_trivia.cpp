// Sports trivia: ten questions a round, 15 seconds each, faster is worth more.
// Seven right moves you up a level (harder questions); a streak adds bonus.
#include "mn_games.h"
#include "mn_ui.h"
#include "mn_lcd.h"

namespace {
struct Q { const char* q; const char* a[4]; uint8_t right; uint8_t diff; };   // diff 1 easy .. 3 hard

const Q QS[] = {
  // football
  {"How many points is a touchdown worth before the extra point?", {"5", "6", "7", "8"}, 1, 1},
  {"How many players per team are on the field in football?", {"9", "10", "11", "12"}, 2, 1},
  {"How many downs does a team get to move the ball 10 yards?", {"3", "4", "5", "6"}, 1, 1},
  {"How many points is a field goal worth?", {"1", "2", "3", "4"}, 2, 1},
  {"How many yards is the field between the goal lines?", {"80", "90", "100", "110"}, 2, 1},
  {"What do you call it when a defender catches a pass meant for the other team?", {"Fumble", "Sack", "Interception", "Punt"}, 2, 1},
  {"How many points is a safety worth?", {"1", "2", "3", "6"}, 1, 2},
  {"Which team's helmet has a star on it?", {"Eagles", "Cowboys", "Giants", "Texans"}, 1, 1},
  {"The Vince Lombardi Trophy goes to the winner of what?", {"The Pro Bowl", "The Super Bowl", "The Draft", "The Hall of Fame"}, 1, 1},
  {"Which team won the very first Super Bowl?", {"Packers", "Chiefs", "Cowboys", "Colts"}, 0, 2},
  {"Lambeau Field is the home of which team?", {"Bears", "Lions", "Packers", "Vikings"}, 2, 2},
  {"Which two teams share MetLife Stadium?", {"Giants and Jets", "Giants and Eagles", "Jets and Bills", "Jets and Patriots"}, 0, 2},
  {"Which team has played in the most Super Bowls?", {"Cowboys", "Patriots", "Steelers", "49ers"}, 1, 3},
  // college football
  {"What is the Florida Gators' home stadium nicknamed?", {"The Swamp", "The Pit", "The Horseshoe", "The Big House"}, 0, 1},
  {"Which trophy goes to the best player in college football?", {"Heisman", "Lombardi", "Outland", "Rose"}, 0, 1},
  {"Which school's team is the Crimson Tide?", {"Auburn", "Alabama", "Georgia", "LSU"}, 1, 1},
  {"Which school is home to the Fighting Irish?", {"Notre Dame", "Boston College", "Georgetown", "Villanova"}, 0, 1},
  {"Which school's mascot is Bevo, a longhorn steer?", {"Texas", "Oklahoma", "Texas A&M", "Baylor"}, 0, 2},
  {"The Hawkeyes are the team of which school?", {"Iowa", "Iowa State", "Nebraska", "Kansas"}, 0, 1},
  {"Ohio State and Michigan play a rivalry game simply called what?", {"The Game", "The Showdown", "The Classic", "The Clash"}, 0, 2},
  {"Cy the Cardinal is the mascot of which school?", {"Iowa State", "Louisville", "Stanford", "Ball State"}, 0, 3},
  // baseball
  {"How many strikes make an out?", {"2", "3", "4", "5"}, 1, 1},
  {"How many innings are in a regular baseball game?", {"7", "8", "9", "10"}, 2, 1},
  {"What do you call a hit over the outfield fence?", {"Triple", "Bunt", "Home run", "Walk"}, 2, 1},
  {"How many players does a baseball team have on defense?", {"8", "9", "10", "11"}, 1, 1},
  {"Which team plays at Fenway Park?", {"Red Sox", "Yankees", "Orioles", "Rays"}, 0, 1},
  {"Which team plays at Wrigley Field?", {"Cubs", "White Sox", "Cardinals", "Brewers"}, 0, 2},
  {"Which New York team plays at Citi Field in Queens?", {"Yankees", "Mets", "Giants", "Dodgers"}, 1, 2},
  {"Which slugger was nicknamed the Sultan of Swat?", {"Babe Ruth", "Lou Gehrig", "Hank Aaron", "Ted Williams"}, 0, 2},
  // hockey
  {"What is the black disc in hockey called?", {"Ball", "Puck", "Disk", "Block"}, 1, 1},
  {"What do you call three goals by one player in a game?", {"Triple play", "Hat trick", "Three-peat", "Trifecta"}, 1, 1},
  {"How many periods are in an NHL game?", {"2", "3", "4", "5"}, 1, 1},
  {"What trophy does the NHL champion win?", {"Stanley Cup", "Grey Cup", "Calder Cup", "Cup of Ice"}, 0, 1},
  {"How many players per side are on the ice, goalie included?", {"5", "6", "7", "8"}, 1, 2},
  {"Which NHL team plays at Madison Square Garden?", {"Islanders", "Devils", "Rangers", "Sabres"}, 2, 2},
  {"Which number did Wayne Gretzky wear?", {"66", "77", "88", "99"}, 3, 2},
  // basketball
  {"How many points is a shot from behind the arc worth?", {"1", "2", "3", "4"}, 2, 1},
  {"How many players per team are on the court?", {"4", "5", "6", "7"}, 1, 1},
  {"Michael Jordan won six titles with which team?", {"Lakers", "Celtics", "Bulls", "Pistons"}, 2, 1},
  {"Which NBA team plays at Madison Square Garden?", {"Nets", "Knicks", "Celtics", "76ers"}, 1, 1},
  {"How many seconds is the NBA shot clock?", {"20", "24", "30", "35"}, 1, 2},
  {"What do you call 10 or more in three stats, like points, rebounds and assists?", {"Triple-double", "Hat trick", "Grand slam", "Full house"}, 0, 2},
  {"How many fouls foul a player out of an NBA game?", {"4", "5", "6", "7"}, 2, 3},
  {"What is the NBA champion's trophy called?", {"Larry O'Brien", "Naismith", "Wooden", "Lombardi"}, 0, 3},
  // all sorts
  {"How many rings are on the Olympic flag?", {"4", "5", "6", "7"}, 1, 1},
  {"How many players per team are on the field in soccer?", {"9", "10", "11", "12"}, 2, 1},
  {"What colour card sends a soccer player off the field?", {"Yellow", "Red", "Blue", "Green"}, 1, 1},
  {"In golf, one stroke under par on a hole is called a what?", {"Eagle", "Birdie", "Bogey", "Ace"}, 1, 2},
  {"Which sport uses a shuttlecock?", {"Tennis", "Squash", "Badminton", "Table tennis"}, 2, 2},
  {"How many holes are in a full round of golf?", {"9", "12", "18", "21"}, 2, 1},
  {"How long is a marathon?", {"13.1 miles", "20 miles", "26.2 miles", "30 miles"}, 2, 2},
  {"In tennis, what does 'love' mean?", {"Zero", "One", "Match point", "A tie"}, 0, 2},
  {"Which country hosted the 2016 Summer Olympics?", {"China", "Brazil", "Greece", "England"}, 1, 3},
  {"Knocking down all ten pins with the first ball is called a what?", {"Spare", "Split", "Strike", "Gutter"}, 2, 1},
  // cheer
  {"What are the fluffy things cheerleaders wave called?", {"Pom-poms", "Tassels", "Streamers", "Puffs"}, 0, 1},
  {"In cheerleading, who is lifted to the top of a stunt?", {"The base", "The flyer", "The spotter", "The coach"}, 1, 2},
  {"The people holding a stunt up from below are called what?", {"Bases", "Flyers", "Tumblers", "Captains"}, 0, 2},
  {"A jump with both legs out to the sides, hands touching the toes, is called a what?", {"Toe touch", "Herkie", "Pike", "Cartwheel"}, 0, 3},
};
const int NQ = sizeof(QS) / sizeof(QS[0]);

const int PER_ROUND = 10, SECONDS = 15;
int level = 1, round_ = 0, asked = 0, correct = 0, score = 0, streak = 0, best = 0;
int order[10], cur = -1;      // the ten questions of this round
int shuf[4];                  // answer positions on screen -> original index
int chosen = -1;
uint32_t t0 = 0, shownLeft = 99;
enum Phase { MENU, ASK, SHOWN, END } phase = MENU;

void pickRound() {
  int maxDiff = level <= 2 ? 1 : level <= 5 ? 2 : 3;
  int n = 0;
  bool used[NQ] = {false};
  int guard = 0;
  while (n < PER_ROUND && guard++ < 2000) {
    int i = gRand(NQ);
    if (used[i]) continue;
    int d = QS[i].diff;
    if (d > maxDiff) continue;
    // harder levels lean on the harder questions
    if (level >= 3 && d == 1 && gRand(2)) continue;
    used[i] = true;
    order[n++] = i;
  }
  while (n < PER_ROUND) { int i = gRand(NQ); if (!used[i]) { used[i] = true; order[n++] = i; } }
}

void header() { gTopBar((String("Trivia  L") + String(level)).c_str(), String("Score ") + String(score)); }

void drawTimer(int left) {
  lcd.fillRect(0, 34, 480, 8, C_BG);
  int w = 480 * left / SECONDS;
  lcd.fillRect(0, 34, w, 6, left > 5 ? C_GREEN : C_RED);
}

String wrapLine(const String& s, int from, int maxW, int& next) {
  useFont(F_B16);
  int cut = s.length();
  while (cut > from + 1 && lcd.textWidth(s.substring(from, cut).c_str()) > maxW) {
    int sp = s.lastIndexOf(' ', cut - 1);
    cut = sp > from ? sp : cut - 1;
  }
  next = cut;
  while (next < (int)s.length() && s[next] == ' ') next++;
  return s.substring(from, cut);
}

void drawQuestion(bool reveal) {
  const Q& q = QS[order[cur]];
  lcd.fillRect(0, 44, 480, 276, C_BG);
  int next = 0, y = 62;
  String text = q.q;
  for (int i = 0, from = 0; i < 3 && from < (int)text.length(); i++) {
    String line = wrapLine(text, from, 456, next);
    uiText(F_B16, line, 12, y, C_WHITE, C_BG, middle_left);
    y += 22;
    from = next;
  }
  for (int k = 0; k < 4; k++) {
    int x0 = 10 + (k % 2) * 238, y0 = 138 + (k / 2) * 86;
    uint16_t fill = C_TILE, edge = C_EDGE;
    if (reveal) {
      if (shuf[k] == q.right) { fill = rgb(24, 90, 50); edge = C_GREEN; }
      else if (k == chosen) { fill = rgb(110, 30, 34); edge = C_RED; }
    }
    uiTile(x0, y0, x0 + 232, y0 + 76, fill, edge, 12, reveal && edge != C_EDGE ? 2 : 1);
    String a = q.a[shuf[k]];
    useFont(F_B16);
    // two lines if it is long
    if (lcd.textWidth(a.c_str()) > 210) {
      int sp = a.lastIndexOf(' ', a.length() / 2 + 4);
      if (sp > 0) {
        uiText(F_B16, a.substring(0, sp), x0 + 116, y0 + 28, C_WHITE, fill, middle_center);
        uiText(F_B16, a.substring(sp + 1), x0 + 116, y0 + 50, C_WHITE, fill, middle_center);
        continue;
      }
    }
    uiText(F_B16, a, x0 + 116, y0 + 38, C_WHITE, fill, middle_center);
  }
}

void ask() {
  const Q& q = QS[order[cur]];
  (void)q;
  for (int i = 0; i < 4; i++) shuf[i] = i;
  for (int i = 3; i > 0; i--) { int j = gRand(i + 1); int t = shuf[i]; shuf[i] = shuf[j]; shuf[j] = t; }
  chosen = -1;
  phase = ASK;
  t0 = millis();
  shownLeft = 99;
  header();
  drawQuestion(false);
  drawTimer(SECONDS);
}

void startRound() {
  pickRound();
  cur = 0; asked = 0; correct = 0; score = 0; streak = 0;
  lcd.fillScreen(C_BG);
  ask();
}

void endRound() {
  phase = END;
  if (score > best) { best = score; gPut("trivia_best", best); }
  bool up = correct >= 7;
  if (up) { level++; gPut("trivia_lv", level); }
  lcd.fillScreen(C_BG);
  gTopBar("Trivia", "");
  uiTile(40, 56, 440, 270, C_TILE, C_EDGE, 16, 2);
  uiText(F_B24, "ROUND OVER", 240, 86, C_WHITE, C_TILE, middle_center);
  uiText(F_B36, String(correct) + " / " + String(PER_ROUND), 240, 132, C_YELLOW, C_TILE, middle_center);
  uiText(F_B18, String(score) + " points", 240, 170, C_WHITE, C_TILE, middle_center);
  uiText(F_S13, String("Best ") + String(best), 240, 194, C_GREY, C_TILE, middle_center);
  if (up) uiText(F_B18, String("LEVEL UP!  Now level ") + String(level), 240, 224, C_GREEN, C_TILE, middle_center);
  else uiText(F_S13, "Get 7 right to go up a level", 240, 224, C_GREY, C_TILE, middle_center);
  uiText(F_B16, "Tap to play again", 240, 252, C_WHITE, C_TILE, middle_center);
}

void answered(int k, bool timeout) {
  const Q& q = QS[order[cur]];
  chosen = k;
  int left = SECONDS - (int)((millis() - t0) / 1000);
  if (left < 0) left = 0;
  bool ok = !timeout && k >= 0 && shuf[k] == q.right;
  asked++;
  if (ok) { correct++; streak++; score += 100 + left * 5 + (streak >= 3 ? 25 : 0); }
  else streak = 0;
  header();
  drawQuestion(true);
  lcd.fillRect(0, 34, 480, 8, C_BG);
  gBanner(ok ? (streak >= 3 ? "STREAK!" : "CORRECT!") : (timeout ? "TIME'S UP" : "NOT QUITE"), ok ? C_GREEN : C_RED, 112);
  phase = SHOWN;
  t0 = millis();
}

void open() {
  level = gGet("trivia_lv", 1);
  best = gGet("trivia_best", 0);
  startRound();
}

void tap(int x, int y) {
  if (phase == END) { startRound(); return; }
  if (phase == SHOWN) { if (millis() - t0 > 350) { cur++; if (cur >= PER_ROUND) endRound(); else ask(); } return; }
  if (phase != ASK) return;
  for (int k = 0; k < 4; k++) {
    int x0 = 10 + (k % 2) * 238, y0 = 138 + (k / 2) * 86;
    if (gIn(x, y, x0, y0, x0 + 232, y0 + 76)) { answered(k, false); return; }
  }
}

void tick() {
  if (phase == ASK) {
    int left = SECONDS - (int)((millis() - t0) / 1000);
    if (left != (int)shownLeft) { shownLeft = left; if (left >= 0) drawTimer(left); }
    if (left < 0) answered(-1, true);
  } else if (phase == SHOWN && millis() - t0 > 2200) {
    cur++;
    if (cur >= PER_ROUND) endRound(); else ask();
  }
}

void icon(int cx, int cy) {
  uiText(F_B36, "?", cx, cy, C_YELLOW, C_TILE, middle_center);
  lcd.drawCircle(cx, cy, 26, C_YELLOW);
}
}  // namespace

const Minigame GAME_TRIVIA = {"Sports Trivia", "Ten questions", "trivia", icon, open, tap, nullptr, nullptr, tick};
