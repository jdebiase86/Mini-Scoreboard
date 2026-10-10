"""Fetches real ESPN scoreboard feeds and the matching logos for the host
renderer (render_games.cpp), trimmed to the fields the mini reads.

    python3 get_feeds.py            # writes feeds/*.json and logos/*.png
Public data only. Re-run any time; the games change with the calendar.
"""
import json, os, subprocess, sys, datetime, urllib.parse

HERE = os.path.dirname(os.path.abspath(__file__))
BASE = "https://site.api.espn.com/apis/site/v2/sports/"
LEAGUES = {"nfl": "football/nfl", "cfb": "football/college-football", "mlb": "baseball/mlb",
           "nhl": "hockey/nhl", "nba": "basketball/nba"}
# (league, conference group or "", days ahead from today or None)
WANT = [("nfl", "", None), ("cfb", "8", None), ("cfb", "5", None)] + \
       [(l, "", d) for l in ("mlb", "nhl", "nba") for d in range(0, 8)]


def curl(url, out=None):
    cmd = ["curl", "-s", "--max-time", "40", "-A", "Mozilla/5.0", url]
    if out: cmd += ["-o", out]
    r = subprocess.run(cmd, capture_output=True)
    return r.stdout if not out else r


def trim(d):
    keep = []
    for e in d.get("events", []):
        c = e["competitions"][0]
        keep.append({
            "date": e.get("date"),
            "status": {"period": e["status"].get("period"), "displayClock": e["status"].get("displayClock"),
                       "type": {k: e["status"]["type"].get(k) for k in ("state", "name", "shortDetail")}},
            "competitions": [{
                "broadcasts": [{"names": b.get("names", [])[:1]} for b in c.get("broadcasts", [])[:1]],
                "competitors": [{
                    "homeAway": p.get("homeAway"), "score": p.get("score"),
                    "records": [{"summary": r.get("summary")} for r in p.get("records", [])[:1]],
                    "team": {k: p["team"].get(k) for k in ("id", "abbreviation", "displayName", "shortDisplayName", "logo", "logoDark")},
                } for p in c["competitors"]]}]})
    return {"events": keep}


today = datetime.date.today()
for lg, group, ahead in WANT:
    q = []
    day = ""
    if lg == "cfb": q.append(f"groups={group}&limit=300")
    if ahead is not None:
        day = (today + datetime.timedelta(days=ahead)).strftime("%Y%m%d")
        q.append("dates=" + day)
    url = BASE + LEAGUES[lg] + "/scoreboard" + ("?" + "&".join(q) if q else "")
    name = f"{lg}{group}_{day or 'now'}.json"
    raw = curl(url)
    try:
        d = trim(json.loads(raw))
    except Exception as ex:
        print("skip", name, ex); continue
    json.dump(d, open(os.path.join(HERE, "feeds", name), "w"), separators=(",", ":"))
    print(name, len(d["events"]), "events")

# logos: only for the demo favourites and whoever they play (only the sizes the screens use)
DEMO = {"nfl": {"NYG", "DAL"}, "cfb": {"FLA", "LSU"}, "mlb": {"NYY"}, "nhl": {"NYR"}, "nba": {"NY"}}
seen = set()
for f in sorted(os.listdir(os.path.join(HERE, "feeds"))):
    d = json.load(open(os.path.join(HERE, "feeds", f)))
    lg = f[:3]
    for e in d["events"]:
        cps = e["competitions"][0]["competitors"]
        if not any(p["team"]["abbreviation"] in DEMO[lg] for p in cps): continue
        for p in cps:
            t = p["team"]; u = t.get("logoDark") or t.get("logo") or ""
            if "/i/teamlogos/" not in u: continue
            path = u[u.index("/i/teamlogos/"):]
            parts = path.split("/teamlogos/")[1].split("/")
            base = parts[-1].rsplit(".", 1)[0]
            for size in (76, 112):
                key = f"{parts[0]}_{base}_{size}"
                if key in seen: continue
                seen.add(key)
                out = os.path.join(HERE, "logos", key + ".png")
                if os.path.exists(out): continue
                img = f"https://a.espncdn.com/combiner/i?img={path}&w={size}&h={size}"
                curl(img, out)
                if not os.path.exists(out) or os.path.getsize(out) < 100:   # no dark version: the ordinary one
                    curl(img.replace("/500-dark/", "/500/"), out)
print(len(seen), "logo files")
