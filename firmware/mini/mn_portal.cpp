#include "mn_portal.h"
#include "mn_settings.h"
#include "mn_ota.h"
#include "mn_lcd.h"
#include "mn_log.h"
#include "mn_version.h"
#include "mn_dns.h"
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>

static WebServer server(80);
static CaptiveDns dns;
static bool apOn = false, started = false;
static String scanned;   // <option>s of nearby networks
volatile bool portalRestart = false;
volatile uint32_t portalSavedAt = 0;
volatile bool portalChanged = false;
volatile uint32_t portalUsedAt = 0;

static String esc(const String& s) {
  String o;
  for (unsigned i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '&') o += "&amp;"; else if (c == '<') o += "&lt;"; else if (c == '>') o += "&gt;";
    else if (c == '"') o += "&quot;"; else o += c;
  }
  return o;
}

static void scanNetworks() {
  scanned = "";
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n && i < 25; i++) {
    String s = WiFi.SSID(i);
    if (!s.length() || scanned.indexOf("\"" + esc(s) + "\"") >= 0) continue;
    scanned += "<option value=\"" + esc(s) + "\">";
  }
  WiFi.scanDelete();
}

static const char PAGE_HEAD[] PROGMEM = R"HTML(<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Mini Scoreboard</title><style>
:root{color-scheme:light dark;--bg:#f2f1ee;--card:#fff;--ink:#17181b;--soft:#5f636b;--line:#d8d4cc;--acc:#c0650f}
@media(prefers-color-scheme:dark){:root{--bg:#111316;--card:#1a1d22;--ink:#eceef1;--soft:#a2a7b0;--line:#2d3139;--acc:#eda040}}
main>form{margin-bottom:14px}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--ink);font:16px/1.45 -apple-system,system-ui,sans-serif}
main{max-width:560px;margin:0 auto;padding:20px 16px 48px}h1{font-size:26px;margin:4px 0 2px}
p.sub{color:var(--soft);margin:0 0 18px}
section{background:var(--card);border:1px solid var(--line);border-radius:12px;padding:16px;margin:0 0 14px}
h2{font-size:17px;margin:0 0 4px}.hint{color:var(--soft);font-size:14px;margin:0 0 12px}
label.f{display:block;font-size:14px;color:var(--soft);margin:10px 0 4px}
input[type=text],input[type=password],select{width:100%;font:inherit;padding:11px 12px;border:1px solid var(--line);border-radius:8px;background:var(--bg);color:var(--ink)}
details{border-top:1px solid var(--line);padding:10px 0}details:first-of-type{border-top:0}
summary{cursor:pointer;font-weight:600}summary .n{color:var(--acc);font-weight:600;margin-left:6px}
.teams{display:grid;grid-template-columns:1fr 1fr;gap:2px 10px;margin-top:8px}
.teams label{display:flex;gap:8px;align-items:center;padding:6px 0;font-size:15px}
input[type=checkbox]{flex:none;width:20px;height:20px;accent-color:var(--acc)}
.picked{font-size:14px;color:var(--soft);min-height:20px}
button{width:100%;font-family:inherit;font-weight:600;font-size:17px;padding:15px;border:0;border-radius:10px;background:var(--acc);color:#fff;margin-top:6px}
.fx{display:grid;grid-template-columns:1fr 1fr;gap:8px}.fx button{font-size:15px;padding:12px 6px;margin:0}
.row{display:flex;gap:8px;align-items:center;margin-top:8px;font-size:14px;color:var(--soft)}
</style></head><body><main>
)HTML";

// The team lists are built in the browser from a compact list, which keeps
// the page small (the mini sends it in one go and has little memory).
static String jsq(const char* t) {   // a JavaScript string literal
  String o = "\"";
  for (; *t; t++) { if (*t == '"' || *t == '\\') o += '\\'; o += *t; }
  return o + "\"";
}

static String teamSection() {
  String h = "<div id=leagues></div><script>const LN=[";
  for (int lg = 0; lg < L_COUNT; lg++) { if (lg) h += ","; h += jsq(LEAGUE_NAMES[lg]); }
  h += "],LK=[\"NFL\",\"CFB\",\"MLB\",\"NHL\",\"NBA\"],T=[";
  for (int i = 0; i < NTEAMS; i++) {
    if (i) h += ",";
    h += "[" + String((int)TEAMS[i].league) + "," + jsq(TEAMS[i].abbr) + "," + jsq(TEAMS[i].name) + "]";
  }
  h += "],P=[";
  for (int k = 0; k < settings.npicks; k++) { if (k) h += ","; h += jsq(teamKey(settings.picks[k]).c_str()); }
  h += R"JS(];
(()=>{const box=document.getElementById('leagues');
LN.forEach((ln,lg)=>{const d=document.createElement('details'),items=T.filter(t=>t[0]===lg);
 d.innerHTML='<summary>'+ln+'<span class=n></span></summary><div class=teams></div>';
 const g=d.querySelector('.teams');
 items.forEach(t=>{const k=LK[lg]+':'+t[1],l=document.createElement('label'),i=document.createElement('input');
  i.type='checkbox';i.name='t';i.value=k;i.dataset.n=t[2];i.checked=P.includes(k);if(i.checked)d.open=true;
  l.append(i,t[2]);g.append(l);});
 box.append(d);});})();
</script>)JS";
  return h;
}

static String sel(const char* name, int cur, const char* const* labels, int n) {
  String h = String("<select name=") + name + ">";
  for (int i = 0; i < n; i++)
    h += "<option value=" + String(i) + (cur == i ? " selected" : "") + ">" + labels[i] + "</option>";
  return h + "</select>";
}

static void handleRoot() {
  portalUsedAt = millis();
  bool home = !apOn;
  String h = FPSTR(PAGE_HEAD);
  h.reserve(16000);
  h += "<h1>Mini Scoreboard</h1><p class=sub>";
  h += home ? "Settings. Changes show up on the mini within a few seconds."
            : "Two quick steps and the mini starts showing scores.";
  h += "</p><form method=post action=/save>";

  // Wi-Fi
  if (!home) {
    h += "<section><h2>1. Your home Wi-Fi</h2><p class=hint>The network the mini should use every day.</p>"
         "<label class=f>Network name</label><input type=text name=ssid list=nets autocomplete=off autocapitalize=none autocorrect=off spellcheck=false "
         "required value=\"" + esc(settings.ssid) + "\"><datalist id=nets>" + scanned + "</datalist>"
         "<label class=f>Password</label><input type=password name=pass id=pw value=\"\" autocomplete=off>"
         "<div class=row><input type=checkbox id=show onclick=\"pw.type=this.checked?'text':'password'\">"
         "<label for=show>Show password</label></div></section>";
  } else {
    h += "<section><h2>Wi-Fi</h2><p class=hint>On <b>" + esc(settings.ssid) + "</b>.";
    if (settings.nnets > 1) {
      h += " Also remembered: ";
      for (int i = 1; i < settings.nnets; i++) h += String(i > 1 ? ", " : "") + esc(settings.nets[i].ssid);
      h += ".";
    }
    h += " The mini joins whichever one it can find. On the go, tap the Wi-Fi button on its screen.</p>"
         "<details><summary>Add or change a Wi-Fi network</summary>"
         "<label class=f>Network name</label><input type=text name=ssid autocomplete=off autocapitalize=none autocorrect=off spellcheck=false>"
         "<label class=f>Password</label><input type=password name=pass autocomplete=off>"
         "<p class=hint>Leave blank to stay on the current network.</p></details></section>";
  }

  // teams
  h += String("<section><h2>") + (home ? "Teams" : "2. Your teams") +
       "</h2><p class=hint>Up to 10. Each gets a tile on the home screen (more than 5: the rest are on a "
       "second page, behind the MORE tile). AUTO goes through them, live games first.</p><div class=picked id=picked></div>";
  h += teamSection();
  h += "</section>";

  // the screen
  const char* tzl[NTZ];
  for (int i = 0; i < NTZ; i++) tzl[i] = TZS[i].label;
  const char* brl[NBRIGHT];
  for (int i = 0; i < NBRIGHT; i++) brl[i] = BRIGHTS[i].label;
  static const char* const COL[] = {"1 (normal)", "2", "3", "4"};
  h += "<section><h2>Screen</h2><label class=f>Time zone (for game times)</label>" + sel("tz", settings.tz, tzl, NTZ);
  h += "<label class=f>Brightness</label>" + sel("bright", settings.bright, brl, NBRIGHT);
  h += String("<div class=row><input type=checkbox name=flip value=1 id=flip") + (settings.flip ? " checked" : "") +
       "><label for=flip>Screen upside down (USB-C plug on the other side)</label></div>";
  h += "<details><summary>Colour mode</summary><p class=hint>Only change this if the colours look wrong "
       "(red looks blue, or everything looks like a photo negative). The mini restarts to apply it.</p>" +
       sel("colour", settings.colour, COL, 4) + "</details>";
  h += "</section><button type=submit>Save</button></form>";

  if (home) {
    h += "<section><h2>Touch</h2><p class=hint>If taps land in the wrong place, redo the touch setup "
         "(four arrows to press on the screen).</p><form method=post action=/touch>"
         "<button type=submit>Redo touch setup</button></form></section>";
    h += "<section><h2>Software</h2><p class=hint>Version " FW_VERSION ". Updates install by themselves overnight. " +
         esc(otaStatus()) + "</p><form method=post action=/update><button type=submit>Check for updates now</button>"
         "</form></section>";
  }

  h += R"JS(<script>
const boxes=[...document.querySelectorAll('input[name=t]')],picked=document.getElementById('picked');
function upd(){const on=boxes.filter(b=>b.checked);
 picked.textContent=on.length?('Picked: '+on.map(b=>b.dataset.n).join(', ')):'Pick at least one team.';
 boxes.forEach(b=>b.disabled=!b.checked&&on.length>=10);
 document.querySelectorAll('details').forEach(d=>{const n=d.querySelector('.n');if(!n)return;
  const c=[...d.querySelectorAll('input[name=t]')].filter(b=>b.checked).length;n.textContent=c?c+' picked':'';});}
boxes.forEach(b=>b.addEventListener('change',upd));upd();
document.querySelector('form').addEventListener('submit',e=>{if(!boxes.some(b=>b.checked)){e.preventDefault();alert('Pick at least one team.');}});
</script></main></body></html>)JS";
  server.send(200, "text/html; charset=utf-8", h);
}

static void page(const String& title, const String& body) {
  String h = FPSTR(PAGE_HEAD);
  h += "<h1>" + title + "</h1><section>" + body + "</section></main></body></html>";
  server.send(200, "text/html; charset=utf-8", h);
}

static void handleSave() {
  portalUsedAt = millis();
  String teams;
  for (int i = 0; i < server.args(); i++)
    if (server.argName(i) == "t") { if (teams.length()) teams += ","; teams += server.arg(i); }
  settings.setPicksFromString(teams);
  settings.tz = constrain(server.arg("tz").toInt(), 0, NTZ - 1);
  settings.bright = constrain(server.arg("bright").toInt(), 0, NBRIGHT - 1);
  bool flip = server.arg("flip") == "1";
  int colour = server.hasArg("colour") ? constrain(server.arg("colour").toInt(), 0, 3) : settings.colour;
  bool screenChanged = flip != settings.flip || colour != settings.colour;
  settings.flip = flip;
  settings.colour = colour;
  String ssid = server.arg("ssid");
  ssid.trim();
  bool wifiChanged = false;
  if (ssid.length()) {
    String pass = server.arg("pass");
    if (!apOn && ssid == settings.ssid && !pass.length()) pass = settings.pass;   // left blank: keep it
    wifiChanged = ssid != settings.ssid || pass != settings.pass;
    settings.addNet(ssid, pass);
  }
  settings.save();
  setenv("TZ", TZS[settings.tz].posix, 1);
  tzset();

  if (wifiChanged || apOn) {
    page("Saved", "<p>The mini is joining <b>" + esc(settings.ssid) +
                      "</b> now. Watch its screen: it says <b>Connected</b> when it's on.</p>"
                      "<p class=hint>You can close this page. Your phone goes back to your normal Wi-Fi on its own.</p>"
                      "<p class=hint>If the mini says it can't join, the network name or password was off. "
                      "Scan the code on its screen again and re-enter them.</p>");
    portalRestart = true;
    portalSavedAt = millis();
  } else if (screenChanged) {
    page("Saved", "<p>The mini is restarting to turn the screen. Give it 20 seconds.</p><p><a href=/>Back to settings</a></p>");
    portalRestart = true;
    portalSavedAt = millis();
  } else {
    page("Saved", "<p>The mini is updating now.</p><p><a href=/>Back to settings</a></p>");
    portalChanged = true;
  }
}

// Phones probe these to detect a sign-in page; send them to ours
static void captive() {
  String url = String("http://") + WiFi.softAPIP().toString() + "/";
  server.sendHeader("Location", url, true);
  server.sendHeader("Cache-Control", "no-cache, no-store");
  server.send(302, "text/html", "<a href=\"" + url + "\">Mini Scoreboard setup</a>");
}

// iPhones show whatever their probe gets back (anything but "Success" means
// "sign-in page"). Keep the answer tiny: phones repeat these checks.
static void appleProbe() {
  if (!apOn) {
    server.send(200, "text/html", "<HTML><HEAD><TITLE>Success</TITLE></HEAD><BODY>Success</BODY></HTML>");
    return;
  }
  String url = String("http://") + WiFi.softAPIP().toString() + "/";
  server.sendHeader("Cache-Control", "no-cache, no-store");
  server.send(200, "text/html",
              "<!doctype html><html><head><meta name=viewport content=\"width=device-width\">"
              "<meta http-equiv=refresh content=\"0;url=" + url + "\"><title>Mini Scoreboard</title></head>"
              "<body><a href=\"" + url + "\">Mini Scoreboard setup</a></body></html>");
}

// A picture of what's on the screen right now (mini.local/screen), for
// sending in a message when something looks off.
static void handleScreen() {
  const int w = lcd.width(), h = lcd.height();
  const uint32_t row = w * 3, size = 54 + row * h;
  uint8_t hdr[54] = {'B', 'M'};
  auto put32 = [&](int at, uint32_t v) { for (int i = 0; i < 4; i++) hdr[at + i] = v >> (8 * i); };
  put32(2, size); put32(10, 54); put32(14, 40); put32(18, w); put32(22, (uint32_t)-h);
  hdr[26] = 1; hdr[28] = 24; put32(34, row * h);
  WiFiClient c = server.client();
  c.print("HTTP/1.1 200 OK\r\nContent-Type: image/bmp\r\nContent-Length: " + String(size) +
          "\r\nContent-Disposition: inline; filename=\"mini-screen.bmp\"\r\nConnection: close\r\n\r\n");
  c.write(hdr, 54);
  static lgfx::rgb888_t line[SCREEN_W];
  static uint8_t out[SCREEN_W * 3];
  for (int y = 0; y < h; y++) {
    lcd.readRect(0, y, w, 1, line);
    for (int x = 0; x < w; x++) { out[x * 3] = line[x].b; out[x * 3 + 1] = line[x].g; out[x * 3 + 2] = line[x].r; }
    c.write(out, row);
  }
}

static void routes() {
  if (started) return;
  started = true;
  server.on("/", HTTP_GET, handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/hotspot-detect.html", HTTP_GET, appleProbe);
  server.on("/library/test/success.html", HTTP_GET, appleProbe);
  for (const char* p : {"/generate_204", "/gen_204", "/connecttest.txt", "/ncsi.txt", "/redirect", "/canonical.html",
                        "/success.txt"})
    server.on(p, HTTP_GET, [] { if (apOn) captive(); else server.send(404, "text/plain", ""); });
  server.on("/update", HTTP_POST, [] {
    otaRequest();
    page("Checking", "<p>The mini is checking GitHub now. If there's a new version, its screen says "
                     "Updating and it restarts by itself in a minute or two. If not, nothing changes.</p>"
                     "<p><a href=/>Back to settings</a> (the result shows under Software)</p>");
  });
  server.on("/touch", HTTP_POST, [] {
    settings.forgetCal();
    page("Touch setup", "<p>The mini is restarting. Arrows show up in the corners of its screen: "
                        "press the tip of each one firmly, then tap the green box.</p><p><a href=/>Back to settings</a></p>");
    portalRestart = true;
    portalSavedAt = millis();
  });
  server.on("/screen", HTTP_GET, handleScreen);
  server.on("/log", HTTP_GET, [] { server.send(200, "text/plain; charset=utf-8", mnLogText()); });
  server.on("/favicon.ico", HTTP_GET, [] { server.send(404, "text/plain", ""); });
  server.onNotFound([] {
    if (apOn) captive();
    else server.send(404, "text/plain", "Not found");
  });
  server.begin();
}

void portalStartAP(const String& apName) {
  WiFi.mode(WIFI_AP_STA);
  scanNetworks();
  WiFi.softAP(apName.c_str());
  delay(200);
  mnLog("setup network %s up at %s", apName.c_str(), WiFi.softAPIP().toString().c_str());
  dns.begin((uint32_t)WiFi.softAPIP());
  apOn = true;
  routes();
}

void portalStopAP() {
  if (!apOn) return;
  dns.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  apOn = false;
}

void portalStartHome() {
  if (MDNS.begin("mini")) MDNS.addService("http", "tcp", 80);
  routes();
}

void portalLoop() {
  if (apOn) dns.process();
  if (started) server.handleClient();
}

bool portalAPRunning() { return apOn; }
