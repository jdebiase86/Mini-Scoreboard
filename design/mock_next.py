"""Mock-ups for the next update (Oct 11): in-game alert banner, 24-card Logo Match board, quarter-style coin.
Needs Pillow and the Inter font.   python3 mock_next.py   ->  mini_next.png
"""
import os, math, random
from PIL import Image, ImageDraw, ImageFont, ImageFilter
OUT = os.path.dirname(os.path.abspath(__file__))
LOGOS = os.path.join(OUT, "../firmware/hosttest/logos")
FONTS = os.environ.get("FONTS", "/usr/share/fonts/opentype/inter")
S = 2; W, H = 480, 320
FB = os.path.join(FONTS, "Inter-Bold.otf")
_fc = {}
def font(s):
    if s not in _fc: _fc[s] = ImageFont.truetype(FB, s * S)
    return _fc[s]
BG = (12, 14, 20); WHITE = (240, 242, 246); YELLOW = (250, 210, 40); GREEN = (60, 200, 110)
def R(x0, y0, x1, y1): return [x0 * S, y0 * S, x1 * S, y1 * S]
def new():
    im = Image.new("RGBA", (W * S, H * S), BG + (255,)); return im, ImageDraw.Draw(im)
def text(d, x, y, s, f, fill, a="mm"): d.text((x * S, y * S), s, font=f, fill=fill, anchor=a)
def logo(name, size):
    im = Image.open(os.path.join(LOGOS, name)).convert("RGBA"); im = im.crop(im.getbbox())
    sc = size * S / max(im.size); return im.resize((max(1, round(im.width * sc)), max(1, round(im.height * sc))), Image.LANCZOS)
def paste_logo(img, name, cx, cy, size):
    l = logo(name, size); img.alpha_composite(l, (round(cx * S - l.width / 2), round(cy * S - l.height / 2)))
NAMES = ["nfl_nyg_76.png", "nba_atl_76.png", "mlb_cle_76.png", "nba_dal_76.png", "nba_hou_76.png", "nba_min_76.png", "mlb_chw_76.png", "nfl_nyg_112.png"]
def topbar(d, left, right):
    d.rectangle(R(0, 0, W, 34), fill=(24, 28, 40)); text(d, 12, 17, left, font(16), WHITE, "lm"); text(d, 468, 17, right, font(13), (150, 156, 170), "rm")
def card(im, d, x, y, w, h, up, i):
    if not up:
        d.rounded_rectangle(R(x, y, x + w, y + h), 10 * S, fill=(30, 50, 100), outline=(70, 110, 190), width=2 * S)
        text(d, x + w / 2, y + h / 2, "?", font(28 if h > 70 else 22), (120, 160, 230))
    else:
        d.rounded_rectangle(R(x, y, x + w, y + h), 10 * S, fill=(40, 46, 66), outline=WHITE, width=2 * S)
        if i % 3 == 2:
            col = [(250, 210, 40), (70, 190, 120), (226, 40, 46)][i % 3 - 2 + (i // 3) % 3]
            d.ellipse(R(x + w / 2 - 14, y + h / 2 - 14, x + w / 2 + 14, y + h / 2 + 14), fill=col)
        else:
            paste_logo(im, NAMES[i % len(NAMES)], x + w / 2, y + h / 2, min(w, h) - 18)
def board(cols, rows, cw, ch, gap, ups=()):
    im, d = new(); topbar(d, "Memory  L12", "Moves 9/26   0:32")
    w = cols * (cw + gap) - gap; h = rows * (ch + gap) - gap
    x0 = (W - w) // 2; y0 = 36 + (284 - h) // 2 + 6
    for i in range(cols * rows):
        card(im, d, x0 + (i % cols) * (cw + gap), y0 + (i // cols) * (ch + gap), cw, ch, i in ups, i)
    return im, d
def bannerA():
    im, d = board(4, 3, 92, 76, 10, ups=(1, 6))
    d.rounded_rectangle(R(40, 40, 440, 84), 12 * S, fill=(0, 0, 0), outline=(250, 210, 40), width=2 * S)
    paste_logo(im, "nfl_nyg_76.png", 70, 62, 32); d = ImageDraw.Draw(im)
    text(d, 98, 62, "TOUCHDOWN - GIANTS", font(18), YELLOW, "lm"); text(d, 424, 62, "28-21", font(16), WHITE, "rm"); return im
def bannerB():
    im, d = board(4, 3, 92, 76, 10, ups=(1, 6))
    d.rectangle(R(0, 34, W, 74), fill=(226, 40, 46)); paste_logo(im, "nfl_nyg_76.png", 34, 54, 30); d = ImageDraw.Draw(im)
    text(d, 62, 54, "GIANTS SCORE!", font(18), WHITE, "lm"); text(d, 468, 54, "28-21", font(16), WHITE, "rm"); return im
def big24():
    return board(6, 4, 66, 62, 8, ups=(3, 14))[0]
def coin(face, gold):
    im, d = new(); topbar(d, "Coin & Dice", "")
    rim, mid, hi = ((196, 150, 40), (232, 190, 70), (255, 232, 150)) if gold else ((150, 156, 168), (200, 206, 216), (245, 247, 252))
    cx, cy, r = 240, 140, 92
    d.ellipse(R(cx - r + 4, cy - r + 10, cx + r + 4, cy + r + 10), fill=(0, 0, 0))
    d.ellipse(R(cx - r, cy - r, cx + r, cy + r), fill=rim)
    for k in range(72):                                   # the ridged edge
        a = k * 2 * math.pi / 72; d.line([(cx + math.cos(a) * (r - 6)) * S, (cy + math.sin(a) * (r - 6)) * S, (cx + math.cos(a) * r) * S, (cy + math.sin(a) * r) * S], fill=hi if k % 2 else (rim[0] - 40, rim[1] - 40, rim[2] - 40), width=S)
    d.ellipse(R(cx - r + 8, cy - r + 8, cx + r - 8, cy + r - 8), fill=mid, outline=rim, width=2 * S)
    if face == "H":                                       # a simple head profile
        d.ellipse(R(cx - 22, cy - 46, cx + 22, cy + 2), fill=rim); d.polygon([(cx - 12) * S, (cy - 2) * S, (cx + 12) * S, (cy - 2) * S, (cx + 36) * S, (cy + 50) * S, (cx - 36) * S, (cy + 50) * S], fill=rim)
        text(d, cx, cy - 70, "LIBERTY", font(12), rim); text(d, cx, cy + 68, "2026", font(12), rim)
    else:                                                 # a simple eagle
        d.polygon([(cx) * S, (cy - 40) * S, (cx + 56) * S, (cy - 10) * S, (cx + 20) * S, (cy + 4) * S, (cx + 10) * S, (cy + 40) * S, (cx - 10) * S, (cy + 40) * S, (cx - 20) * S, (cy + 4) * S, (cx - 56) * S, (cy - 10) * S], fill=rim)
        text(d, cx, cy - 70, "QUARTER", font(12), rim); text(d, cx, cy + 68, "DOLLAR", font(12), rim)
    sh = Image.new("RGBA", im.size, (0, 0, 0, 0)); ImageDraw.Draw(sh).polygon([(cx - 70) * S, (cy - 30) * S, (cx - 20) * S, (cy - 80) * S, (cx - 4) * S, (cy - 70) * S, (cx - 56) * S, (cy - 18) * S], fill=(255, 255, 255, 80)); im.alpha_composite(sh.filter(ImageFilter.GaussianBlur(2 * S)))
    d = ImageDraw.Draw(im); text(d, 240, 268, "HEADS" if face == "H" else "TAILS", font(30), YELLOW); return im
def edge():
    im, d = new(); topbar(d, "Coin & Dice", "")
    d.rounded_rectangle(R(226, 56, 254, 216), 6 * S, fill=(200, 206, 216), outline=(150, 156, 168), width=S)
    for k in range(10): d.line([228 * S, (62 + k * 16) * S, 252 * S, (62 + k * 16) * S], fill=(150, 156, 168), width=S)
    d.ellipse(R(196, 232, 284, 246), fill=(5, 6, 9)); text(d, 240, 284, "flipping...", font(20), (150, 156, 170)); return im
shots = [("1. Alert banner, option A (small, black)", bannerA()), ("2. Alert banner, option B (red strip)", bannerB()), ("3. Logo Match: 24 cards (12 pairs)", big24()),
         ("4. Coin: silver quarter, heads", coin("H", False)), ("5. Coin: silver, tails", coin("T", False)), ("6. Coin in the air: edge-on, ridged", edge()),
         ("7. Coin: gold option, heads", coin("H", True))]
gap, lab = 24, 34
rows = (len(shots) + 2) // 3
sheet = Image.new("RGB", (3 * W + 4 * gap, rows * (H + lab + gap) + gap), (235, 236, 240))
sd = ImageDraw.Draw(sheet); lf = ImageFont.truetype(FB, 20)
for i, (nm, im) in enumerate(shots):
    c, r = i % 3, i // 3; x = gap + c * (W + gap); y = gap + r * (H + lab + gap)
    sd.text((x, y), nm, font=lf, fill=(30, 30, 40)); sheet.paste(im.convert("RGB").resize((W, H), Image.LANCZOS), (x, y + lab))
sheet.save(os.path.join(OUT, "mini_next.png")); print(sheet.size)
