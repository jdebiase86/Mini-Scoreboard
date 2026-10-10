// Ported from Scoreboard's sb_net.cpp (the same checks, ESP32 instead of S3).
#include "mn_ota.h"
#include "mn_version.h"
#include "mn_log.h"
#include "mn_diag.h"
#include "mn_tls.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Update.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <esp_ota_ops.h>

volatile int otaPercent = -1;
char otaNewVersion[16] = "";
static volatile bool otaAsked = false;
static char otaLine[96] = "Not checked yet";

void otaRequest() { otaAsked = true; }
String otaStatus() { return String(otaLine); }

static void scopy(char* d, const char* s, size_t n = 16) { strncpy(d, s, n - 1); d[n - 1] = 0; }

int versionCompare(const char* a, const char* b) {
  while (*a || *b) {
    long x = strtol(a, (char**)&a, 10), y = strtol(b, (char**)&b, 10);
    if (x != y) return x < y ? -1 : 1;
    if (*a == '.') a++;
    if (*b == '.') b++;
    if (!isdigit((unsigned char)*a) && !isdigit((unsigned char)*b)) break;
  }
  return 0;
}

bool versionFromAsset(const char* name, char (&out)[16]) {
  const char* pre = "mini-";
  size_t lp = strlen(pre), n = strlen(name);
  if (n <= lp + 4 || strncasecmp(name, pre, lp) || strcasecmp(name + n - 4, ".bin")) return false;
  size_t len = n - lp - 4;
  if (len == 0 || len >= sizeof(out)) return false;
  for (size_t i = 0; i < len; i++) {
    char c = name[lp + i];
    if (!isdigit((unsigned char)c) && c != '.') return false;
  }
  memcpy(out, name + lp, len);
  out[len] = 0;
  return isdigit((unsigned char)out[0]) != 0;
}

static void otaNote(const char* fmt, const char* a = "", const char* b = "") {
  char when[16] = "";
  time_t now = time(nullptr);
  if (now > 1700000000) {
    struct tm lt;
    localtime_r(&now, &lt);
    int h = lt.tm_hour % 12;   // no "%-I" in the board's strftime
    snprintf(when, sizeof(when), "%d:%02d %s", h ? h : 12, lt.tm_min, lt.tm_hour < 12 ? "AM" : "PM");
  }
  char msg[80];
  snprintf(msg, sizeof(msg), fmt, a, b);
  snprintf(otaLine, sizeof(otaLine), "%s%s%s", when, when[0] ? ": " : "", msg);
  mnLog("update: %s", msg);
}

// Download and install one program file. Follows GitHub's redirect to its
// file server by hand. Only a complete, valid program is ever switched to -
// anything less and the mini keeps running what it has.
static bool otaInstall(String url, const char* version, long apiSize) {
  static uint8_t buf[2048];
  for (int hop = 0; hop < 5; hop++) {
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    http.useHTTP10(true);
    http.setReuse(false);
    http.setConnectTimeout(10000);
    http.setTimeout(20000);
    const char* keys[] = {"Location"};
    http.collectHeaders(keys, 1);
    if (!http.begin(client, url)) { otaNote("couldn't open the download"); return false; }
    http.setUserAgent("Mozilla/5.0 (Mini Scoreboard)");
    int code = http.GET();
    if (code == 301 || code == 302 || code == 303 || code == 307 || code == 308) {
      url = http.header("Location");
      http.end();
      if (!url.length()) { otaNote("download redirect went nowhere"); return false; }
      continue;
    }
    if (code != 200) {
      char c[8]; snprintf(c, sizeof(c), "%d", code);
      otaNote("download failed (HTTP %s)", c);
      http.end();
      return false;
    }
    long size = http.getSize();
    if (size <= 0) size = apiSize;
    else if (apiSize > 0 && size != apiSize) { otaNote("download size doesn't match, skipped"); http.end(); return false; }
    WiFiClient* s = http.getStreamPtr();
    if (!s || size < 100000 || size > 0x1E0000) { otaNote("download looks wrong, skipped"); http.end(); return false; }
    if (!Update.begin(size)) { otaNote("no room to install (%s)", Update.errorString()); http.end(); return false; }
    scopy(otaNewVersion, version);
    otaPercent = 0;
    long got = 0;
    uint32_t last = millis();
    while (got < size) {
      int a = s->available();
      if (a > 0) {
        int want = a < (int)sizeof(buf) ? a : sizeof(buf);
        if (want > size - got) want = size - got;
        int n = s->read(buf, want);
        if (n > 0 && got == 0) {
          // before anything touches flash: is this really an ESP32 program?
          // (byte 0 = 0xE9, chip id 0 at 12-13, app-description magic at 32)
          while (n < 48 && millis() - last < 20000) {
            int m = s->read(buf + n, 48 - n);
            if (m > 0) n += m; else delay(5);
          }
          bool ok = n >= 48 && buf[0] == 0xE9 && buf[12] == 0 && buf[13] == 0 &&
                    buf[32] == 0x32 && buf[33] == 0x54 && buf[34] == 0xCD && buf[35] == 0xAB;
          if (!ok) {
            Update.abort();
            otaPercent = -1;
            http.end();
            otaNote("that file isn't a mini scoreboard program - skipped");
            return false;
          }
        }
        if (n > 0) {
          if (Update.write(buf, n) != (size_t)n) break;
          got += n;
          otaPercent = (int)(got * 100 / size);
          last = millis();
        }
      } else {
        if (!s->connected() || millis() - last > 20000) break;
        delay(5);
      }
    }
    http.end();
    if (got != size || !Update.end(true)) {
      if (Update.isRunning()) Update.abort();
      otaPercent = -1;
      otaNote("install stopped part way (%s) - still on the old version", Update.errorString());
      return false;
    }
    Preferences p;
    p.begin("mini", false);
    p.putString("updated", version);
    p.end();
    otaNote("installed %s, restarting", version);
    otaPercent = 100;
    delay(1500);
    diagNote("installed an update");
    ESP.restart();
    return true;
  }
  otaNote("too many download redirects");
  return false;
}

// Set once this program has proved it works (Wi-Fi up, clock set). Until
// then a fresh update is "on trial": if it crashes or loses power, the chip
// goes back to the previous version by itself.
static bool appConfirmed = false;
static void confirmApp() {
  if (appConfirmed) return;
  appConfirmed = true;
  esp_ota_mark_app_valid_cancel_rollback();
  mnLog("this version (%s) is working - kept", FW_VERSION);
}

static void otaCheck(bool userAsked) {
  char bad[16] = "";
  {
    Preferences p;
    p.begin("mini", true);
    scopy(bad, p.getString("bad", "").c_str());
    p.end();
  }
  String api = String("https://api.github.com/repos/") + MN_REPO + "/releases/latest";
  WiFiClientSecure client;
  HTTPClient http;
  client.setInsecure();
  http.useHTTP10(true);
  http.setReuse(false);
  http.setConnectTimeout(10000);
  http.setTimeout(15000);
  if (!http.begin(client, api)) { otaNote("couldn't reach GitHub"); return; }
  http.setUserAgent("Mozilla/5.0 (Mini Scoreboard)");
  http.addHeader("Accept", "application/vnd.github+json");
  int code = http.GET();
  if (code == 404) { http.end(); otaNote("no updates published yet (on %s)", FW_VERSION); return; }
  if (code != 200) {
    char c[8]; snprintf(c, sizeof(c), "%d", code);
    http.end();
    otaNote("GitHub answered %s - will try later", c);
    return;
  }
  JsonDocument filter;
  filter["assets"][0]["name"] = true;
  filter["assets"][0]["browser_download_url"] = true;
  filter["assets"][0]["size"] = true;
  JsonDocument doc;
  DeserializationError e = deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter),
                                           DeserializationOption::NestingLimit(20));
  http.end();
  if (e) { otaNote("couldn't read GitHub's answer (%s)", e.c_str()); return; }
  char bestV[16] = "";
  String bestUrl;
  long bestSize = 0;
  for (JsonObjectConst a : doc["assets"].as<JsonArrayConst>()) {
    char v[16];
    if (!versionFromAsset(a["name"] | "", v)) continue;
    if (!bestV[0] || versionCompare(v, bestV) > 0) {
      scopy(bestV, v);
      bestUrl = a["browser_download_url"] | "";
      bestSize = a["size"] | 0L;
    }
  }
  if (!bestV[0]) { otaNote("latest release has no mini-x.y.bin file (on %s)", FW_VERSION); return; }
  if (versionCompare(bestV, FW_VERSION) <= 0) { otaNote("up to date (%s)", FW_VERSION); return; }
  if (!userAsked && !strcmp(bestV, bad)) {
    otaNote("%s didn't work on this mini last time - press Check for updates to try again", bestV);
    return;
  }
  otaNote("found %s, installing", bestV);
  otaInstall(bestUrl, bestV, bestSize);
}

static void otaTask(void*) {
  uint32_t bootCheckAt = 0;
  int checkedDay = -1;
  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(1000));
    if (WiFi.status() != WL_CONNECTED || time(nullptr) < 1700000000) continue;
    if (!appConfirmed) {
      confirmApp();
      bootCheckAt = millis() + 3 * 60000UL;
    }
    time_t now = time(nullptr);
    struct tm lt;
    localtime_r(&now, &lt);
    bool nightly = lt.tm_hour == 4 && lt.tm_yday != checkedDay;
    bool boot = bootCheckAt && (int32_t)(millis() - bootCheckAt) >= 0;
    if (otaAsked || boot || nightly) {
      bool asked = otaAsked;
      otaAsked = false;
      bootCheckAt = 0;
      if (nightly) checkedDay = lt.tm_yday;
      mnLog("checking for updates (heap %u KB)", (unsigned)(ESP.getFreeHeap() / 1024));
      if (mnTlsTake(60000)) {   // scores and logos take turns with downloads
        otaCheck(asked);
        mnTlsGive();
      }
    }
  }
}

void otaStart() {
  static bool started = false;
  if (started) return;
  started = true;
  xTaskCreatePinnedToCore(otaTask, "ota", 12288, nullptr, 1, nullptr, 0);
}
