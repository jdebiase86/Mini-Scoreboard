"""Mini scoreboard mock-ups, 480x320 wide, using the real ESPN logos from the
Scoreboard repo (assets/logos). Needs Pillow and the Inter font.

    python3 mock_mini.py        -> mini_mockups.png (the six main screens)
    python3 mock_mini.py rz     -> mini_redzone.png (our team in the red zone)
    python3 mock_mini.py opp    -> mini_redzone_opp.png (red zone pop-ups, ours vs theirs)
    python3 mock_mini.py plays  -> mini_plays.png (defense, turnover and punt animations)
    python3 mock_mini.py game   -> mini_game_extras.png (win chance bar, drive tracker options, close game, stats)
    python3 mock_mini.py picker -> mini_picker.png (team picker on the mini itself)
    python3 mock_mini.py game2  -> mini_game_v2.png (picked: drive on the field, play pop-up, small HOME, no AUTO button)

LOGOS points at Scoreboard/assets/logos (default: a Scoreboard checkout next
to this repo). FONTS points at the folder holding Inter-Bold.otf etc.
"""
import os, sys, math
from PIL import Image, ImageDraw, ImageFont, ImageFilter
OUT = os.path.dirname(os.path.abspath(__file__))
LOGOS = os.environ.get("LOGOS", os.path.join(OUT, "../../Scoreboard/assets/logos"))
FONTS = os.environ.get("FONTS", "/usr/share/fonts/opentype/inter")
S = 2                      # draw at 2x, shrink for smooth edges
W, H = 480, 320
FB = os.path.join(FONTS, "Inter-Bold.otf")
FS = os.path.join(FONTS, "Inter-SemiBold.otf")
FR = os.path.join(FONTS, "Inter-Medium.otf")
_fc = {}
def font(path, size):
    k = (path, size)
    if k not in _fc: _fc[k] = ImageFont.truetype(path, size * S)
    return _fc[k]

BG = (12, 14, 20); TILE = (28, 32, 42); TILE_HI = (38, 44, 58); EDGE = (52, 58, 74)
WHITE = (240, 242, 246); GREY = (150, 156, 170); DIM = (100, 106, 120)
RED = (226, 40, 46); GREEN = (40, 190, 90); YELLOW = (250, 210, 40)

def logo(path, size):
    im = Image.open(os.path.join(LOGOS, path)).convert("RGBA")
    bb = im.getbbox()
    if bb: im = im.crop(bb)
    w, h = im.size; sc = size * S / max(w, h)
    return im.resize((max(1, round(w * sc)), max(1, round(h * sc))), Image.LANCZOS)

def paste_logo(img, path, cx, cy, size, maxw=None):
    l = logo(path, size)
    if maxw and l.width > maxw * S:
        l = l.resize((round(maxw * S), max(1, round(l.height * maxw * S / l.width))), Image.LANCZOS)
    img.alpha_composite(l, (round(cx * S - l.width / 2), round(cy * S - l.height / 2)))

def new():
    img = Image.new("RGBA", (W * S, H * S), BG + (255,))
    return img, ImageDraw.Draw(img)

def R(x0, y0, x1, y1): return [x0 * S, y0 * S, x1 * S, y1 * S]

def rbox(d, x0, y0, x1, y1, fill, r=10, outline=None, width=1):
    d.rounded_rectangle(R(x0, y0, x1, y1), r * S, fill=fill, outline=outline, width=width * S)

def text(d, x, y, s, f, fill, anchor="la"):
    d.text((x * S, y * S), s, font=f, fill=fill, anchor=anchor)

def pill(d, x, y, s, bg, fg=WHITE, size=12, anchor="l"):
    f = font(FB, size); tw = d.textlength(s, font=f) / S
    pw, ph = tw + 14, size + 8
    if anchor == "r": x -= pw
    rbox(d, x, y, x + pw, y + ph, bg, r=ph / 2)
    text(d, x + pw / 2, y + ph / 2, s, f, fg, "mm")
    return pw

def football(d, cx, cy, w=22, h=13):
    d.ellipse(R(cx - w / 2, cy - h / 2, cx + w / 2, cy + h / 2), fill=(140, 80, 40))
    d.line(R(cx - w * .22, cy, cx + w * .22, cy), fill=WHITE, width=2 * S)
    for i in range(-2, 3):
        x = cx + i * w * .09
        d.line(R(x, cy - 2.5, x, cy + 2.5), fill=WHITE, width=1 * S)

def button(d, x0, y0, x1, y1, label, sub=None, hi=False, icon=None):
    rbox(d, x0, y0, x1, y1, TILE_HI if hi else TILE, r=12, outline=(255, 255, 255) if hi else EDGE, width=2 if hi else 1)
    cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
    if icon == "auto":
        auto_icon(d, x0 + 30, cy, 13); cx += 14
    if icon == "home":
        home_icon(d, x0 + 30, cy, 13); cx += 14
    if sub:
        text(d, cx, cy - 8, label, font(FB, 18), WHITE, "mm")
        text(d, cx, cy + 12, sub, font(FR, 11), GREY, "mm")
    else:
        text(d, cx, cy, label, font(FB, 18), WHITE, "mm")

def auto_icon(d, cx, cy, r, col=WHITE, w=3):
    d.arc(R(cx - r, cy - r, cx + r, cy + r), 200, 340, fill=col, width=w * S)
    d.arc(R(cx - r, cy - r, cx + r, cy + r), 20, 160, fill=col, width=w * S)
    for ang, sgn in ((340, 1), (160, 1)):
        a = math.radians(ang); px, py = cx + r * math.cos(a), cy + r * math.sin(a)
        t = math.radians(ang + 90)
        tip = (px + 6 * math.cos(t), py + 6 * math.sin(t))
        n = math.radians(ang)
        p1 = (px + 6 * math.cos(n), py + 6 * math.sin(n)); p2 = (px - 6 * math.cos(n), py - 6 * math.sin(n))
        d.polygon([(tip[0] * S, tip[1] * S), (p1[0] * S, p1[1] * S), (p2[0] * S, p2[1] * S)], fill=col)

def home_icon(d, cx, cy, r, col=WHITE):
    d.polygon([((cx - r) * S, (cy - 1) * S), (cx * S, (cy - r) * S), ((cx + r) * S, (cy - 1) * S)], fill=col)
    d.rectangle(R(cx - r * .65, cy - 2, cx + r * .65, cy + r * .8), fill=col)
    d.rectangle(R(cx - 3, cy + 3, cx + 3, cy + r * .8), fill=TILE)

def remote_icon(d, cx, cy, col=WHITE):
    rbox(d, cx - 5, cy - 9, cx + 5, cy + 9, None, r=3, outline=col, width=2)
    d.ellipse(R(cx - 2, cy - 5, cx + 2, cy - 1), fill=col)

def finish(img, name):
    out = img.convert("RGB").resize((W, H), Image.LANCZOS)
    return out


# ---------------------------------------------------------------- on-device team picker (Oct 7 idea)
def _teams():
    import re
    src = open(os.path.join(OUT, "../firmware/mini/mn_teams.h")).read()
    return re.findall(r'\{L_(\w+),"([^"]+)","([^"]+)",(\d+)\}', src)

def check(d, x, y, on, size=22):
    if on:
        rbox(d, x, y, x + size, y + size, GREEN, r=5)
        d.line([((x + 5) * S, (y + size / 2) * S), ((x + 9.5) * S, (y + size - 6) * S), ((x + size - 5) * S, (y + 6) * S)], fill=WHITE, width=3 * S)
    else:
        rbox(d, x, y, x + size, y + size, None, r=5, outline=GREY, width=2)

def back_btn(d):
    rbox(d, 6, 3, 96, 29, TILE, r=13, outline=EDGE)
    d.polygon([(18 * S, 16 * S), (26 * S, 9 * S), (26 * S, 23 * S)], fill=WHITE)
    text(d, 60, 16, "BACK", font(FB, 12), WHITE, "mm")

def picker_home():
    img = home().convert("RGBA").resize((W * S, H * S), Image.LANCZOS); d = ImageDraw.Draw(img)
    rbox(d, 286, 3, 368, 29, BG, r=13)
    rbox(d, 288, 3, 366, 29, TILE, r=13, outline=YELLOW, width=2)
    d.line([(300 * S, 22 * S), (309 * S, 10 * S)], fill=WHITE, width=3 * S)   # a pencil
    d.polygon([(298 * S, 25 * S), (299 * S, 20 * S), (302 * S, 23 * S)], fill=WHITE)
    text(d, 337, 16, "EDIT", font(FB, 12), WHITE, "mm")
    return img.convert("RGB").resize((W, H), Image.LANCZOS)

def picker_leagues():
    img, d = new()
    text(d, 12, 16, "Pick your teams", font(FB, 16), WHITE, "lm")
    text(d, 470, 16, "3 of 10 picked", font(FS, 13), GREY, "rm")
    tiles = [("NFL", "1 picked"), ("COLLEGE", "1 picked"), ("MLB", "1 picked"), ("NHL", ""), ("NBA", "")]
    for i, (nm, sub) in enumerate(tiles):
        col, row = i % 3, i // 3
        x0, y0 = 6 + col * 158, 34 + row * 143; x1, y1 = x0 + 152, y0 + 137; cx = (x0 + x1) / 2
        rbox(d, x0, y0, x1, y1, TILE, r=14, outline=EDGE)
        text(d, cx, y0 + 60, nm, font(FB, 28 if len(nm) < 5 else 22), WHITE, "mm")
        if sub: text(d, cx, y0 + 100, sub, font(FB, 13), GREEN, "mm")
        else: text(d, cx, y0 + 100, "none yet", font(FS, 13), DIM, "mm")
    x0, y0 = 6 + 2 * 158, 34 + 143
    rbox(d, x0, y0, x0 + 152, y0 + 137, (22, 70, 40), r=14, outline=GREEN, width=2)
    d.line([((x0 + 54) * S, (y0 + 52) * S), ((x0 + 70) * S, (y0 + 68) * S), ((x0 + 100) * S, (y0 + 36) * S)], fill=WHITE, width=7 * S)
    text(d, x0 + 76, y0 + 98, "DONE", font(FB, 24), WHITE, "mm")
    text(d, x0 + 76, y0 + 120, "save and go home", font(FR, 12), (170, 220, 185), "mm")
    return finish(img, "")

PICKED = {"New York Giants", "LSU", "New York Yankees"}

def picker_list(league="NFL", page=0, title="NFL"):
    img, d = new()
    back_btn(d)
    text(d, 240, 16, title, font(FB, 16), WHITE, "mm")
    text(d, 470, 16, "1 picked", font(FB, 13), GREEN, "rm")
    names = [t[2] for t in _teams() if t[0] == league]
    per = 10; pages = (len(names) + per - 1) // per
    show = names[page * per:(page + 1) * per]
    for i, nm in enumerate(show):
        col, row = i // 5, i % 5
        x0 = 6 + col * 202; y0 = 36 + row * 56
        on = nm in PICKED
        rbox(d, x0, y0, x0 + 196, y0 + 50, TILE_HI if on else TILE, r=10, outline=GREEN if on else EDGE, width=2 if on else 1)
        check(d, x0 + 12, y0 + 14, on)
        f = font(FS, 15 if len(nm) < 18 else 13)
        text(d, x0 + 44, y0 + 25, nm, f, WHITE, "lm")
    # page arrows on the right
    for (y0, up) in ((36, True), (204, False)):
        rbox(d, 414, y0, 474, y0 + 106, TILE, r=12, outline=EDGE)
        cy = y0 + 53
        if up: d.polygon([(444 * S, (cy - 16) * S), (424 * S, (cy + 10) * S), (464 * S, (cy + 10) * S)], fill=WHITE if page else DIM)
        else: d.polygon([(444 * S, (cy + 16) * S), (424 * S, (cy - 10) * S), (464 * S, (cy - 10) * S)], fill=WHITE if page < pages - 1 else DIM)
    text(d, 444, 172, f"{page + 1} of {pages}", font(FS, 13), GREY, "mm")
    return finish(img, "")

def picker_grid(league="NFL", page=0):
    img, d = new()
    back_btn(d)
    text(d, 240, 16, "NFL", font(FB, 16), WHITE, "mm")
    text(d, 470, 16, "1 picked", font(FB, 13), GREEN, "rm")
    names = [t[2] for t in _teams() if t[0] == league]
    per = 9; pages = (len(names) + per - 1) // per
    for i, nm in enumerate(names[page * per:(page + 1) * per]):
        col, row = i % 3, i // 3
        x0 = 6 + col * 158; y0 = 36 + row * 80; x1 = x0 + 152; y1 = y0 + 74
        on = nm in PICKED
        rbox(d, x0, y0, x1, y1, TILE_HI if on else TILE, r=12, outline=GREEN if on else EDGE, width=2 if on else 1)
        city, _, team = nm.rpartition(" ")
        text(d, (x0 + x1) / 2, y0 + 28, team, font(FB, 17), WHITE, "mm")
        text(d, (x0 + x1) / 2, y0 + 51, city, font(FR, 12), GREY, "mm")
        if on: check(d, x1 - 26, y0 + 6, True, 18)
    # bottom: page buttons
    rbox(d, 6, 278, 120, 316, TILE, r=12, outline=EDGE)
    d.polygon([(50 * S, 297 * S), (70 * S, 286 * S), (70 * S, 308 * S)], fill=DIM if page == 0 else WHITE)
    rbox(d, 360, 278, 474, 316, TILE, r=12, outline=EDGE)
    d.polygon([(430 * S, 297 * S), (410 * S, 286 * S), (410 * S, 308 * S)], fill=WHITE)
    text(d, 240, 297, f"Page {page + 1} of {pages}", font(FS, 13), GREY, "mm")
    return finish(img, "")

def picker_college():
    img, d = new()
    back_btn(d)
    text(d, 240, 16, "College football", font(FB, 16), WHITE, "mm")
    text(d, 470, 16, "1 picked", font(FB, 13), GREEN, "rm")
    confs = [("SEC", "1 picked"), ("BIG TEN", ""), ("ACC", ""), ("BIG 12", ""), ("OTHERS", "Notre Dame, Army, Navy")]
    for i, (nm, sub) in enumerate(confs):
        col, row = i % 3, i // 3
        x0, y0 = 6 + col * 158, 34 + row * 143; x1, y1 = x0 + 152, y0 + 137; cx = (x0 + x1) / 2
        rbox(d, x0, y0, x1, y1, TILE, r=14, outline=EDGE)
        text(d, cx, y0 + 60, nm, font(FB, 26 if len(nm) < 6 else 22), WHITE, "mm")
        if sub == "1 picked": text(d, cx, y0 + 100, sub, font(FB, 13), GREEN, "mm")
        elif sub: text(d, cx, y0 + 100, sub, font(FR, 11), GREY, "mm")
    return finish(img, "")


# ---------------------------------------------------------------- home
def home(rz=False):
    img, d = new()
    text(d, 12, 16, "My Teams", font(FB, 16), WHITE, "lm")
    text(d, 240, 16, "Tue 7:42 PM", font(FS, 13), GREY, "mm")
    # Big board found on Wi-Fi -> small Board button
    rbox(d, 376, 3, 474, 29, TILE, r=13, outline=EDGE)
    remote_icon(d, 394, 16)
    text(d, 432, 16, "BOARD", font(FB, 12), WHITE, "mm")
    tiles = [
        dict(logo="nfl/nyg.png", live=True, big="21 - 17", small="Q3 4:12  vs PHI") if not rz else
        dict(logo="nfl/nyg.png", live=True, big="21 - 17", small="RED ZONE  1st & Goal", rz=True),
        dict(logo="nhl/nyr.png", live=True, big="2 - 1", small="2nd 8:31  vs PIT"),
        dict(logo="ncaa/57.png", big="Final 31-24", small="Win vs LSU", win=True),
        dict(logo="mlb/nyy.png", big="Wed 7:08 PM", small="vs Boston", smallbig=True),
        dict(logo="nba/ny.png", big="Fri 7:30 PM", small="vs Boston", smallbig=True),
        dict(auto=True),
    ]
    for i, t in enumerate(tiles):
        col, row = i % 3, i // 3
        x0, y0 = 6 + col * 158, 34 + row * 143
        x1, y1 = x0 + 152, y0 + 137
        cx = (x0 + x1) / 2
        if t.get("auto"):
            rbox(d, x0, y0, x1, y1, (24, 40, 70), r=14, outline=(70, 110, 190), width=2)
            auto_icon(d, cx, y0 + 50, 26, w=5)
            text(d, cx, y0 + 98, "AUTO", font(FB, 24), WHITE, "mm")
            text(d, cx, y0 + 120, "rotate my teams", font(FR, 12), (170, 190, 230), "mm")
            continue
        live = t.get("live")
        rbox(d, x0, y0, x1, y1, TILE, r=14, outline=RED if live else EDGE, width=2 if live else 1)
        paste_logo(img, t["logo"], cx, y0 + 46, 74)
        if live: pill(d, x0 + 8, y0 + 8, "LIVE", RED, size=11)
        if t.get("smallbig"):
            text(d, cx, y0 + 104, t["big"], font(FB, 19), WHITE, "mm")
        else:
            text(d, cx, y0 + 104, t["big"], font(FB, 24 if live else 21), WHITE, "mm")
        if t.get("rz"):
            rbox(d, x0 + 10, y0 + 116, x1 - 10, y0 + 133, RED, r=8)
            text(d, cx, y0 + 125, t["small"], font(FB, 11), WHITE, "mm")
        else:
            text(d, cx, y0 + 125, t["small"], font(FS, 12), GREEN if t.get("win") else GREY, "mm")
    return finish(img, "1_home" + ("_rz" if rz else ""))

COMPACT = False   # Oct 7: game screens get only a smaller HOME button; AUTO lives on the home screen

def game_bottom(d, auto_on=False, mid=None):
    if COMPACT:
        rbox(d, 8, 274, 116, 314, TILE, r=12, outline=EDGE)
        home_icon(d, 32, 294, 11); text(d, 74, 294, "HOME", font(FB, 15), WHITE, "mm")
        if auto_on:
            pill(d, 472, 284, "AUTO  next game in 14s", (24, 40, 70), fg=(170, 190, 230), size=12, anchor="r")
        return
    button(d, 8, 266, 168, 314, "HOME", icon="home")
    button(d, 312, 266, 472, 314, "AUTO", icon="auto", hi=auto_on)
    if mid: text(d, 240, 290, mid, font(FS, 12), GREY, "mm")

def teams_row(d, img, away, home_, ascore, hscore, a_dim=False, h_dim=False):
    paste_logo(img, away["logo"], 70, 98, 110, maxw=98)
    paste_logo(img, home_["logo"], 410, 98, 110, maxw=98)
    text(d, 70, 166, away["name"], font(FS, 13), GREY, "mm")
    text(d, 410, 166, home_["name"], font(FS, 13), GREY, "mm")
    if ascore is not None:
        text(d, 168, 96, ascore, font(FB, 62), DIM if a_dim else WHITE, "mm")
        text(d, 312, 96, hscore, font(FB, 62), DIM if h_dim else WHITE, "mm")

# ---------------------------------------------------------- live game
def live(rz=False, alert=False, opp=False, bad=None, extra=None, line=None, q="3RD", clock="4:12", ascore="17", hscore="21", down="2nd & 6"):
    img, d = new()
    pill(d, 10, 7, "LIVE", RED, size=12)
    text(d, 62, 17, "NFL  Week 5", font(FS, 13), GREY, "lm")
    if not rz: text(d, 470, 17, "FOX", font(FB, 13), GREY, "rm")
    teams_row(d, img, dict(logo="nfl/phi.png", name="Eagles 3-1"), dict(logo="nfl/nyg.png", name="Giants 3-1"), ascore, hscore)
    if rz: pill(d, 470, 7, "RED ZONE", RED, size=12, anchor="r")
    text(d, 240, 70, q, font(FS, 13), RED if rz else GREY, "mm")
    text(d, 240, 94, clock, font(FB, 28), RED if rz else WHITE, "mm")
    for side, n in ((168, 2), (312, 3)):
        for k in range(3):
            d.rounded_rectangle(R(side - 22 + k * 16, 130, side - 10 + k * 16, 134), 2 * S, fill=YELLOW if k < n else DIM)
    football(d, 168 if opp else 312, 150)   # who has the ball
    text(d, 240, 122, "1st & Goal" if rz else down, font(FB, 16), RED if rz else YELLOW, "mm")
    text(d, 240, 142, ("at NYG 8" if opp else "at PHI 8") if rz else "at PHI 34", font(FS, 12), GREY, "mm")
    # field: away (PHI) end zone left, home (NYG) right; Giants attack left
    fx0, fx1, fy0, fy1 = 12, 468, 184, 222
    ez = 30
    gx0, gx1 = fx0 + ez, fx1 - ez
    yd = (gx1 - gx0) / 100
    d.rectangle(R(gx0, fy0, gx1, fy1), fill=(38, 120, 52))
    if opp: d.rectangle(R(gx1 - 20 * yd, fy0, gx1, fy1), fill=(200, 40, 40))
    else: d.rectangle(R(gx0, fy0, gx0 + 20 * yd, fy1), fill=(200, 40, 40) if rz else (130, 60, 50))      # red zone tint
    for y in range(10, 100, 10):
        x = gx0 + y * yd
        d.line(R(x, fy0, x, fy1), fill=(90, 170, 100) if y != 50 else (160, 210, 165), width=S)
    d.rounded_rectangle(R(fx0, fy0, gx0, fy1), 6 * S, fill=(0, 76, 84))
    d.rectangle(R(gx0 - 6, fy0, gx0, fy1), fill=(0, 76, 84))
    d.rounded_rectangle(R(gx1, fy0, fx1, fy1), 6 * S, fill=(11, 34, 101))
    d.rectangle(R(gx1, fy0, gx1 + 6, fy1), fill=(11, 34, 101))
    text(d, (fx0 + gx0) / 2, (fy0 + fy1) / 2, "PHI", font(FB, 11), WHITE, "mm")
    text(d, (gx1 + fx1) / 2, (fy0 + fy1) / 2, "NYG", font(FB, 11), WHITE, "mm")
    if rz: rbox(d, fx0 - 3, fy0 - 3, fx1 + 3, fy1 + 3, None, r=8, outline=RED, width=2)
    lx = gx0 + 28 * yd
    if not rz: d.line(R(lx, fy0, lx, fy1), fill=YELLOW, width=2 * S)
    bx = gx1 - 8 * yd if opp else gx0 + (8 if rz else 34) * yd
    d.line(R(bx, fy0, bx, fy1), fill=(80, 150, 255), width=2 * S)
    football(d, bx, (fy0 + fy1) / 2, 16, 10)
    if line is None:
        text(d, 240, 240, ("Eagles pass for 19 yards to the NYG 8" if opp else "Pass complete for 26 yards to the PHI 8") if rz else "Run up the middle for 4 yards", font(FR, 12), GREY, "mm")
    game_bottom(d)
    if extra:
        d = extra(img, d, dict(fx0=fx0, fx1=fx1, fy0=fy0, fy1=fy1, gx0=gx0, gx1=gx1, yd=yd, bx=bx))
    if alert:
        ov = Image.new("RGBA", img.size, (0, 0, 0, 0)); od = ImageDraw.Draw(ov)
        od.rectangle(R(0, 0, W, H), fill=(0, 0, 0, 120))
        img.alpha_composite(ov)
        for k in range(10):
            x = -40 + k * 56
            d.polygon([(x * S, 108 * S), ((x + 28) * S, 108 * S), ((x + 58) * S, 212 * S), ((x + 30) * S, 212 * S)], fill=(180, 20, 28))
        d.rectangle(R(0, 108, W, 212), outline=None)
        band = Image.new("RGBA", img.size, (0, 0, 0, 0)); bd = ImageDraw.Draw(band)
        bd.rectangle(R(0, 108, W, 212), fill=(226, 40, 46, 150))
        img.alpha_composite(band)
        d = ImageDraw.Draw(img)
        d.rectangle(R(0, 104, W, 108), fill=WHITE); d.rectangle(R(0, 212, W, 216), fill=WHITE)
        paste_logo(img, "nfl/nyg.png", 70, 160, 80)
        text(d, 283, 147, "RED ZONE", font(FB, 50), (0, 0, 0), "mm")
        text(d, 280, 144, "RED ZONE", font(FB, 50), WHITE, "mm")
        text(d, 280, 188, "Giants 1st & Goal at the PHI 8", font(FB, 15), WHITE, "mm")
    if bad:
        ov = Image.new("RGBA", img.size, (0, 0, 0, 0)); od = ImageDraw.Draw(ov)
        od.rectangle(R(0, 0, W, H), fill=(0, 0, 0, 150))
        img.alpha_composite(ov); d = ImageDraw.Draw(img)
        if bad == "defense":
            d.rectangle(R(0, 108, W, 212), fill=(16, 16, 18))
            for y0 in (100, 212):           # hazard tape top and bottom
                d.rectangle(R(0, y0, W, y0 + 10), fill=(250, 200, 20))
                for k in range(-1, 26):
                    x = k * 20
                    d.polygon([(x * S, (y0 + 10) * S), ((x + 10) * S, y0 * S), ((x + 18) * S, y0 * S), ((x + 8) * S, (y0 + 10) * S)], fill=(16, 16, 18))
            # warning triangle
            tx, ty = 62, 160
            d.polygon([(tx * S, (ty - 34) * S), ((tx + 38) * S, (ty + 30) * S), ((tx - 38) * S, (ty + 30) * S)], fill=(250, 200, 20))
            text(d, tx, ty + 8, "!", font(FB, 40), (16, 16, 18), "mm")
            text(d, 290, 146, "DEFENSE!", font(FB, 50), (250, 200, 20), "mm")
            paste_logo(img, "nfl/phi.png", 160, 189, 22)
            text(d, 300, 189, "Eagles 1st & Goal at the NYG 8", font(FB, 15), WHITE, "mm")
        else:
            d.rectangle(R(0, 108, W, 212), fill=(30, 30, 34))
            d.rectangle(R(0, 104, W, 108), fill=(90, 20, 24)); d.rectangle(R(0, 212, W, 216), fill=(90, 20, 24))
            paste_logo(img, "nfl/phi.png", 70, 160, 80)
            text(d, 283, 147, "UH OH...", font(FB, 50), (0, 0, 0), "mm")
            text(d, 280, 144, "UH OH...", font(FB, 50), (200, 60, 60), "mm")
            text(d, 280, 188, "Eagles in the red zone, NYG 8", font(FB, 15), GREY, "mm")
    return finish(img, "2_live" + ("_rz" if rz else "") + ("_alert" if alert else "") + ("_opp" if opp else "") + ("_" + bad if bad else ""))

# ---------------------------------------------------------- final
def final():
    img, d = new()
    pill(d, 10, 7, "FINAL", (70, 76, 92), size=12)
    text(d, 72, 17, "College Football  Sat Oct 3", font(FS, 13), GREY, "lm")
    text(d, 470, 17, "SEC", font(FB, 13), GREY, "rm")
    teams_row(d, img, dict(logo="ncaa/99.png", name="LSU 4-1"), dict(logo="ncaa/57.png", name="Florida 4-1"), "24", "31", a_dim=True)
    text(d, 240, 96, "FINAL", font(FB, 22), WHITE, "mm")
    text(d, 312, 136, "WIN", font(FB, 13), GREEN, "mm")
    rbox(d, 40, 190, 440, 244, TILE, r=12)
    text(d, 60, 206, "NEXT GAME", font(FB, 11), GREY, "lm")
    text(d, 60, 228, "Sat Oct 10, 3:30 PM  at Tennessee", font(FS, 15), WHITE, "lm")
    paste_logo(img, "ncaa/2633.png", 410, 217, 38)
    game_bottom(d, auto_on=True, mid="Auto: next in 14s")
    return finish(img, "3_final")

# ---------------------------------------------------------- upcoming
def upcoming():
    img, d = new()
    pill(d, 10, 7, "TOMORROW", (40, 90, 170), size=12)
    text(d, 104, 17, "AL Division Series  Game 2", font(FS, 13), GREY, "lm")
    text(d, 470, 17, "TBS", font(FB, 13), GREY, "rm")
    teams_row(d, img, dict(logo="mlb/bos.png", name="Red Sox 92-70"), dict(logo="mlb/nyy.png", name="Yankees 96-66"), None, None)
    text(d, 240, 70, "WED OCT 7", font(FB, 15), GREY, "mm")
    text(d, 240, 102, "7:08 PM", font(FB, 40), WHITE, "mm")
    text(d, 240, 136, "Yankee Stadium", font(FS, 13), GREY, "mm")
    rbox(d, 40, 190, 440, 244, TILE, r=12)
    text(d, 240, 206, "SERIES", font(FB, 11), GREY, "mm")
    text(d, 240, 228, "Yankees lead 1-0", font(FS, 16), WHITE, "mm")
    game_bottom(d)
    return finish(img, "4_upcoming")

# ---------------------------------------------------------- animation
def anim():
    img, d = new()
    cx, cy = 240, 120
    cols = [(11, 34, 101), (167, 25, 48)]
    for k in range(24):
        a0 = k * 15; c = cols[k % 2]
        d.pieslice(R(cx - 420, cy - 420, cx + 420, cy + 420), a0, a0 + 15, fill=c)
    glow = Image.new("RGBA", img.size, (0, 0, 0, 0)); gd = ImageDraw.Draw(glow)
    gd.ellipse(R(cx - 95, cy - 95, cx + 95, cy + 95), fill=(255, 255, 255, 120))
    glow = glow.filter(ImageFilter.GaussianBlur(18 * S))
    img.alpha_composite(glow)
    paste_logo(img, "nfl/nyg.png", cx, cy, 150)
    f = font(FB, 52)
    text(d, 243, 243, "TOUCHDOWN", f, (0, 0, 0), "mm")
    text(d, 240, 240, "TOUCHDOWN", f, WHITE, "mm")
    rbox(d, 130, 278, 350, 310, (0, 0, 0, 255), r=16)
    text(d, 240, 294, "GIANTS 28   EAGLES 17", font(FB, 15), YELLOW, "mm")
    return finish(img, "5_touchdown")

# ---------------------------------------------------------- remote
def remote():
    img, d = new()
    text(d, 12, 17, "Big Board Remote", font(FB, 16), WHITE, "lm")
    d.ellipse(R(370, 12, 380, 22), fill=GREEN)
    text(d, 386, 17, "Connected", font(FS, 13), GREY, "lm")
    teams = [("nfl/nyg.png", "GIANTS"), ("ncaa/57.png", "GATORS"), ("mlb/nyy.png", "YANKEES"),
             ("nhl/nyr.png", "RANGERS"), ("nba/ny.png", "KNICKS")]
    for i, (lg, nm) in enumerate(teams):
        x0 = 8 + i * 94; x1 = x0 + 88
        rbox(d, x0, 36, x1, 150, TILE, r=12, outline=EDGE)
        paste_logo(img, lg, (x0 + x1) / 2, 82, 64)
        text(d, (x0 + x1) / 2, 134, nm, font(FB, 12), WHITE, "mm")
    modes = [("AUTO", "auto", True), ("ALL NFL", None, False), ("ALL COLLEGE", None, False), ("FULL GAME", None, False)]
    for i, (lb, ic, hi) in enumerate(modes):
        x0 = 8 + i * 117; x1 = x0 + 113
        rbox(d, x0, 158, x1, 252, TILE_HI if hi else TILE, r=12, outline=WHITE if hi else EDGE, width=2 if hi else 1)
        cx = (x0 + x1) / 2
        if ic:
            auto_icon(d, cx, 192, 13, w=3)
            text(d, cx, 226, lb, font(FB, 15), WHITE, "mm")
        else:
            text(d, cx, 205, lb, font(FB, 15), WHITE, "mm")
    button(d, 8, 266, 168, 314, "BACK")
    text(d, 184, 280, "Board is showing", font(FS, 11), GREY, "lm")
    text(d, 184, 299, "AUTO  Giants 21-17 Eagles", font(FB, 14), WHITE, "lm")
    return finish(img, "6_remote")

# ---------------------------------------------------------- play animations
BLUE_T, RED_T = (11, 34, 101), (167, 25, 48)

def fit_font(d, s, maxw, start=56, path=FB):
    size = start
    while size > 20 and d.textlength(s, font=font(path, size)) / S > maxw: size -= 2
    return font(path, size)

def big_word(d, s, y, col=WHITE, maxw=450, start=56):
    f = fit_font(d, s, maxw, start)
    text(d, 243, y + 3, s, f, (0, 0, 0), "mm"); text(d, 240, y, s, f, col, "mm")

def sub_pill(d, s, y=294, fg=YELLOW, bg=(0, 0, 0)):
    f = font(FB, 15); w = d.textlength(s, font=f) / S + 36
    rbox(d, 240 - w / 2, y - 16, 240 + w / 2, y + 16, bg, r=16)
    text(d, 240, y, s, f, fg, "mm")

def glow_at(img, cx, cy, r=90, a=120):
    g = Image.new("RGBA", img.size, (0, 0, 0, 0)); gd = ImageDraw.Draw(g)
    gd.ellipse(R(cx - r, cy - r, cx + r, cy + r), fill=(255, 255, 255, a))
    img.alpha_composite(g.filter(ImageFilter.GaussianBlur(18 * S)))

def rays(d, cx, cy, cols, n=24):
    for k in range(n):
        a0 = k * 360 / n
        d.pieslice(R(cx - 460, cy - 460, cx + 460, cy + 460), a0, a0 + 360 / n, fill=cols[k % 2])

def stripes(d, cols, w=34, slope=0.6):
    for k in range(-12, 30):
        x = k * w
        d.polygon([(x * S, 0), ((x + w) * S, 0), ((x + w - H * slope) * S, H * S), ((x - H * slope) * S, H * S)], fill=cols[k % 2])

def picked_off():
    img, d = new(); stripes(d, [BLUE_T, (20, 50, 140)])
    # the pass: dotted arc cut off by the logo
    for t in range(0, 14):
        x = 30 + t * 14; y = 200 - math.sin(t / 13 * math.pi) * 120
        d.ellipse(R(x - 3, y - 3, x + 3, y + 3), fill=(255, 255, 255))
    glow_at(img, 260, 115); paste_logo(img, "nfl/nyg.png", 260, 115, 140)
    d = ImageDraw.Draw(img); big_word(d, "PICKED OFF!", 236)
    sub_pill(d, "INTERCEPTION - GIANTS BALL AT PHI 35"); return finish(img, "p_int")

def fumble():
    img, d = new(); rays(d, 240, 110, [RED_T, BLUE_T])
    glow_at(img, 240, 110); paste_logo(img, "nfl/nyg.png", 240, 110, 130)
    d = ImageDraw.Draw(img)
    for i, (bx, by, a) in enumerate(((70, 70, 0.6), (95, 150, 1.0), (400, 160, 0.8))):
        football(d, bx, by, 34 * a + 10, 20 * a + 6)
    big_word(d, "FUMBLE!", 222, start=60)
    sub_pill(d, "GIANTS BALL - RECOVERED AT THE NYG 40", y=284); return finish(img, "p_fum")

def stopped():
    img, d = new(); d.rectangle(R(0, 0, W, H), fill=BLUE_T)
    cx, cy, r = 240, 128, 112
    pts = [((cx + r * math.cos(math.radians(22.5 + 45 * k))) * S, (cy + r * math.sin(math.radians(22.5 + 45 * k))) * S) for k in range(8)]
    d.polygon(pts, fill=(255, 255, 255))
    pts2 = [((cx + (r - 8) * math.cos(math.radians(22.5 + 45 * k))) * S, (cy + (r - 8) * math.sin(math.radians(22.5 + 45 * k))) * S) for k in range(8)]
    d.polygon(pts2, fill=(205, 30, 40))
    text(d, cx, cy - 6, "STOPPED!", fit_font(d, "STOPPED!", 180, 44), WHITE, "mm")
    text(d, cx, cy + 34, "3RD DOWN", font(FB, 16), WHITE, "mm")
    paste_logo(img, "nfl/nyg.png", 62, 270, 54)
    d = ImageDraw.Draw(img); sub_pill(d, "EAGLES FACE 4TH & 3", y=286); return finish(img, "p_stop")

def stonewalled():
    img, d = new(); d.rectangle(R(0, 0, W, H), fill=(60, 64, 76))
    bh, bw = 26, 64
    for row in range(int(H / bh) + 1):
        off = (row % 2) * bw / 2
        for col in range(-1, int(W / bw) + 2):
            x = col * bw - off; y = row * bh
            c = BLUE_T if (row + col) % 3 else (24, 52, 130)
            d.rectangle(R(x + 2, y + 2, x + bw - 2, y + bh - 2), fill=c)
    glow_at(img, 240, 105, 85, 90); paste_logo(img, "nfl/nyg.png", 240, 105, 120)
    d = ImageDraw.Draw(img); big_word(d, "STONEWALLED!", 222)
    sub_pill(d, "TURNOVER ON DOWNS - GIANTS BALL", y=284); return finish(img, "p_wall")

def sacked():
    img, d = new(); rays(d, 240, 150, [BLUE_T, RED_T], 32)
    # impact burst
    cx, cy = 240, 150; pts = []
    for k in range(28):
        rr = 205 if k % 2 == 0 else 158
        a = math.radians(k * 360 / 28)
        pts.append(((cx + rr * math.cos(a)) * S, (cy + rr * 0.62 * math.sin(a)) * S))
    d.polygon(pts, fill=(255, 220, 40))
    paste_logo(img, "nfl/nyg.png", 240, 104, 70)
    d = ImageDraw.Draw(img)
    f = fit_font(d, "SACKED!", 250, 62)
    text(d, 240, 172, "SACKED!", f, BLUE_T, "mm")
    sub_pill(d, "EAGLES LOSE 8 YARDS", y=286); return finish(img, "p_sack")

def puntastic():
    img, d = new(); rays(d, 240, 300, [BLUE_T, (22, 50, 135)])
    # the kick: dotted arc away from us
    for t in range(0, 18):
        x = 60 + t * 21; y = 200 - math.sin(t / 17 * math.pi) * 150
        d.ellipse(R(x - 2.5, y - 2.5, x + 2.5, y + 2.5), fill=(255, 255, 255, 255))
    football(d, 420, 120, 30, 18)
    paste_logo(img, "nfl/nyg.png", 240, 110, 96)
    d = ImageDraw.Draw(img); big_word(d, "PUNT-ASTIC!", 222)
    sub_pill(d, "3 AND OUT - EAGLES HAVE TO PUNT", y=284); return finish(img, "p_puntastic")

def went_for_it():
    img, d = new(); stripes(d, [(20, 110, 60), (28, 140, 74)], 40, 0.0)
    pill(d, 240, 30, "4TH & 1", (0, 0, 0), size=18, anchor="c") if False else None
    rbox(d, 190, 22, 290, 52, (0, 0, 0), r=15); text(d, 240, 37, "4TH & 1", font(FB, 17), YELLOW, "mm")
    glow_at(img, 240, 125, 80, 90); paste_logo(img, "nfl/nyg.png", 240, 125, 120)
    d = ImageDraw.Draw(img); big_word(d, "WENT FOR IT!", 228)
    sub_pill(d, "AND MADE IT - FIRST DOWN GIANTS", y=286); return finish(img, "p_went")

def no_punt():
    img, d = new(); d.rectangle(R(0, 0, W, H), fill=(48, 50, 58))
    for t in range(0, 18):
        x = 60 + t * 21; y = 190 - math.sin(t / 17 * math.pi) * 110
        d.ellipse(R(x - 2, y - 2, x + 2, y + 2), fill=(120, 124, 136))
    football(d, 420, 130, 26, 15)
    paste_logo(img, "nfl/nyg.png", 240, 105, 80)
    lg = Image.new("RGBA", img.size, (48, 50, 58, 110)); img.alpha_composite(lg)   # logo dimmed
    d = ImageDraw.Draw(img)
    f = fit_font(d, "NO PUNT INTENDED", 430, 44)
    text(d, 240, 212, "NO PUNT INTENDED", f, (215, 218, 226), "mm")
    text(d, 240, 246, "4th & 7 at their own 28", font(FS, 15), (150, 156, 170), "mm")
    sub_pill(d, "GIANTS PUNT 46 YARDS", y=288, fg=(220, 222, 230), bg=(28, 30, 36)); return finish(img, "p_nopunt")

def turnover_bad():
    img = live(rz=False, opp=False)
    img = img.convert("RGBA").resize((W * S, H * S), Image.LANCZOS)
    ov = Image.new("RGBA", img.size, (0, 0, 0, 150)); img.alpha_composite(ov); d = ImageDraw.Draw(img)
    d.rectangle(R(0, 108, W, 212), fill=(16, 16, 18))
    for y0 in (100, 212):
        d.rectangle(R(0, y0, W, y0 + 10), fill=(250, 200, 20))
        for k in range(-1, 26):
            x = k * 20
            d.polygon([(x * S, (y0 + 10) * S), ((x + 10) * S, y0 * S), ((x + 18) * S, y0 * S), ((x + 8) * S, (y0 + 10) * S)], fill=(16, 16, 18))
    tx, ty = 62, 160
    d.polygon([(tx * S, (ty - 34) * S), ((tx + 38) * S, (ty + 30) * S), ((tx - 38) * S, (ty + 30) * S)], fill=(250, 200, 20))
    text(d, tx, ty + 8, "!", font(FB, 40), (16, 16, 18), "mm")
    text(d, 290, 146, "TURNOVER", font(FB, 48), (250, 200, 20), "mm")
    paste_logo(img, "nfl/phi.png", 150, 189, 22)
    text(d, 300, 189, "Picked off - Eagles ball at the NYG 30", font(FB, 15), WHITE, "mm")
    return finish(img, "p_turnover")

# ---------------------------------------------------------- game screen extras
TEAL = (0, 76, 84)
def win_bar(d, y=229, away_pct=38):
    x0, x1 = 12, 468; xm = x0 + (x1 - x0) * away_pct / 100
    d.rounded_rectangle(R(x0, y, x1, y + 12), 6 * S, fill=BLUE_T)
    d.rounded_rectangle(R(x0, y, xm + 6, y + 12), 6 * S, fill=TEAL)
    d.rectangle(R(xm - 1, y, xm + 1, y + 12), fill=WHITE)
    text(d, x0 + 8, y + 6, "%d%%" % away_pct, font(FB, 9), WHITE, "lm")
    text(d, x1 - 8, y + 6, "%d%%" % (100 - away_pct), font(FB, 9), WHITE, "rm")
    text(d, 240, y + 6, "WIN CHANCE", font(FB, 8), (200, 205, 220), "mm")

def opt_a():
    def ex(img, d, c):
        win_bar(d)
        text(d, 240, 253, "This drive: 6 plays, 48 yards, 3:12", font(FS, 12), GREY, "mm")
        d.ellipse(R(226, 291, 232, 297), fill=DIM); d.ellipse(R(248, 291, 254, 297), fill=WHITE)
        return d
    return live(line="", extra=ex)

def opt_b():
    def ex(img, d, c):
        win_bar(d)
        rbox(d, 8, 244, 472, 316, (34, 40, 54), r=14, outline=(90, 100, 125))
        text(d, 22, 260, "LAST PLAY", font(FB, 10), GREY, "lm")
        text(d, 458, 260, "DRIVE: 6 PLAYS, 48 YDS, 3:12", font(FB, 10), GREY, "rm")
        text(d, 240, 290, "Run up the middle for 4 yards", font(FB, 19), WHITE, "mm")
        return d
    return live(line="", extra=ex)

def opt_c():
    def ex(img, d, c):
        # the drive drawn on the field: from where it started (NYG 25) to the ball (PHI 34)
        start = c["gx1"] - 25 * c["yd"]; ball = c["bx"]
        band = Image.new("RGBA", img.size, (0, 0, 0, 0)); bd = ImageDraw.Draw(band)
        bd.rectangle(R(ball, c["fy0"] + 4, start, c["fy1"] - 4), fill=(255, 255, 255, 60))
        img.alpha_composite(band); d = ImageDraw.Draw(img)
        d.line(R(start, c["fy0"] + 2, start, c["fy1"] - 2), fill=WHITE, width=S)
        football(d, ball, (c["fy0"] + c["fy1"]) / 2, 16, 10)
        win_bar(d)
        text(d, 14, 253, "Run up the middle for 4 yards", font(FR, 12), GREY, "lm")
        text(d, 466, 253, "Drive: 6 plays, 48 yds, 3:12", font(FS, 12), (170, 176, 192), "rm")
        return d
    return live(line="", extra=ex)

def close_game():
    def ex(img, d, c):
        win_bar(d, away_pct=47)
        ov = Image.new("RGBA", img.size, (0, 0, 0, 130)); img.alpha_composite(ov); d = ImageDraw.Draw(img)
        d.rectangle(R(0, 104, W, 216), fill=(18, 20, 26))
        d.rectangle(R(0, 104, W, 108), fill=(245, 150, 30)); d.rectangle(R(0, 212, W, 216), fill=(245, 150, 30))
        # clock icon
        d.ellipse(R(36, 128, 96, 188), outline=(245, 150, 30), width=5 * S)
        d.line(R(66, 158, 66, 138), fill=(245, 150, 30), width=4 * S); d.line(R(66, 158, 80, 166), fill=(245, 150, 30), width=4 * S)
        text(d, 290, 145, "CLOSE GAME", font(FB, 46), (245, 150, 30), "mm")
        text(d, 290, 189, "Giants 21, Eagles 20 - 1:48 left in the 4th", font(FB, 15), WHITE, "mm")
        return d
    return live(line="", extra=ex, q="4TH", clock="1:48", ascore="20", hscore="21", down="3rd & 4")

def stats_page():
    img, d = new()
    paste_logo(img, "nfl/phi.png", 40, 26, 36); paste_logo(img, "nfl/nyg.png", 440, 26, 36)
    d = ImageDraw.Draw(img)
    text(d, 240, 20, "EAGLES 17  -  GIANTS 21", font(FB, 18), WHITE, "mm")
    text(d, 240, 40, "3RD  4:12", font(FS, 12), GREY, "mm")
    rows = [("PASSING", "14/22, 168 yds, 1 TD", "17/25, 201 yds, 2 TD"),
            ("RUSHING", "11 car, 64 yds", "15 car, 88 yds, 1 TD"),
            ("RECEIVING", "5 rec, 71 yds", "6 rec, 94 yds, 1 TD"),
            ("TOTAL YARDS", "232", "289"), ("TURNOVERS", "1", "0")]
    y = 64
    for lab_, a, b in rows:
        rbox(d, 8, y, 472, y + 36, TILE, r=10)
        text(d, 240, y + 9, lab_, font(FB, 9), GREY, "mm")
        text(d, 20, y + 23, a, font(FS, 13), WHITE, "lm"); text(d, 460, y + 23, b, font(FS, 13), WHITE, "rm")
        y += 41
    text(d, 240, 288 + 18, "Tap anywhere to go back", font(FS, 11), DIM, "mm")
    return finish(img, "stats")

# ---------------------------------------------------------- Oct 7 game screen: drive on the field, play pop-up, small HOME
def game_v2(popup=False):
    def ex(img, d, c):
        start = c["gx1"] - 25 * c["yd"]; ball = c["bx"]
        band = Image.new("RGBA", img.size, (0, 0, 0, 0)); bd = ImageDraw.Draw(band)
        bd.rectangle(R(ball, c["fy0"] + 4, start, c["fy1"] - 4), fill=(255, 255, 255, 60))
        img.alpha_composite(band); d = ImageDraw.Draw(img)
        d.line(R(start, c["fy0"] + 2, start, c["fy1"] - 2), fill=WHITE, width=S)
        football(d, ball, (c["fy0"] + c["fy1"]) / 2, 16, 10)
        win_bar(d)
        if popup:
            rbox(d, 124, 248, 472, 316, (34, 40, 54), r=14, outline=(90, 100, 125))
            text(d, 138, 263, "LAST PLAY", font(FB, 10), GREY, "lm")
            text(d, 458, 263, "2ND & 6 AT PHI 34", font(FB, 10), GREY, "rm")
            text(d, 298, 292, "Run up the middle for 4 yards", font(FB, 17), WHITE, "mm")
        return d
    return live(line="", extra=ex)

if len(sys.argv) > 1 and sys.argv[1] == "rz":
    shots = [("1. Home (Giants in red zone)", home(rz=True)), ("2. Red zone alert (3 sec)", live(rz=True, alert=True)),
             ("3. Game screen in red zone", live(rz=True))]
    name = "mini_redzone.png"
elif len(sys.argv) > 1 and sys.argv[1] == "opp":
    shots = [("Ours: RED ZONE (good)", live(rz=True, alert=True)), ("Theirs, option A: DEFENSE!", live(rz=True, opp=True, bad="defense")),
             ("Theirs, option B: UH OH...", live(rz=True, opp=True, bad="uhoh"))]
    name = "mini_redzone_opp.png"
elif len(sys.argv) > 1 and sys.argv[1] == "picker":
    shots = [("1. Home: EDIT button (yellow)", picker_home()), ("2. Tap EDIT: pick a league", picker_leagues()),
             ("3. College: pick a conference", picker_college()), ("Option A: list, 10 a page", picker_list()),
             ("Option A: page 3 (Giants ticked)", picker_list(page=2)), ("Option B: tiles, 9 a page", picker_grid())]
    name = "mini_picker.png"
elif len(sys.argv) > 1 and sys.argv[1] == "game":
    shots = [("A. Play line switches to drive", opt_a()), ("B. Play pops up, then hides", opt_b()), ("C. Drive drawn on the field", opt_c()),
             ("Close game alert", close_game()), ("Tap the score: stats", stats_page())]
    name = "mini_game_extras.png"
elif len(sys.argv) > 1 and sys.argv[1] == "game2":
    COMPACT = True
    shots = [("1. Live game (drive shown on the field)", game_v2()), ("2. A play just happened: it pops up", game_v2(popup=True)),
             ("3. Final, while Auto is rotating", final()), ("4. Upcoming game", upcoming())]
    name = "mini_game_v2.png"
elif len(sys.argv) > 1 and sys.argv[1] == "plays":
    shots = [("1. Interception (ours)", picked_off()), ("2. Fumble recovery (ours)", fumble()), ("3. Third-down stop", stopped()),
             ("4. Turnover on downs", stonewalled()), ("5. Sack", sacked()), ("6. They punt", puntastic()),
             ("7. We go for it and make it", went_for_it()), ("8. We punt", no_punt()), ("9. We turn it over", turnover_bad())]
    name = "mini_plays.png"
else:
    shots = [("1. Home", home()), ("2. Live game", live()), ("3. Final (Auto on)", final()),
             ("4. Upcoming game", upcoming()), ("5. Touchdown animation", anim()), ("6. Big board remote", remote())]
    name = "mini_mockups.png"
gap, lab = 24, 34
rows = (len(shots) + 2) // 3
sheet = Image.new("RGB", (3 * W + 4 * gap, rows * (H + lab + gap) + gap), (235, 236, 240))
sd = ImageDraw.Draw(sheet); lf = ImageFont.truetype(FB, 20)
for i, (nm, im) in enumerate(shots):
    c, r = i % 3, i // 3
    x = gap + c * (W + gap); y = gap + r * (H + lab + gap)
    sd.text((x, y), nm, font=lf, fill=(30, 30, 40))
    sheet.paste(im, (x, y + lab))
sheet.save(os.path.join(OUT, name))
print(sheet.size)
