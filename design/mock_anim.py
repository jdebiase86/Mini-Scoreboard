"""Mock-ups of the score animations and the dimmer screen (Oct 11). The real firmware code will draw these
with shapes (rays, tape, confetti, a ball on an arc), not pictures. Needs Pillow and the Inter font; logos from
firmware/hosttest/logos.   python3 mock_anim.py   ->  mini_anims.png
"""
import os, math, random, sys
from PIL import Image, ImageDraw, ImageFont, ImageFilter
sys.argv = [sys.argv[0], "none"]
OUT = os.path.dirname(os.path.abspath(__file__))
LOGOS = os.path.join(OUT, "../firmware/hosttest/logos")
FONTS = os.environ.get("FONTS", "/usr/share/fonts/opentype/inter")
S = 2; W, H = 480, 320
FB = os.path.join(FONTS, "Inter-Bold.otf"); FS = os.path.join(FONTS, "Inter-SemiBold.otf")
_fc = {}
def font(p, s):
    if (p, s) not in _fc: _fc[(p, s)] = ImageFont.truetype(p, s * S)
    return _fc[(p, s)]
BG = (12, 14, 20); WHITE = (240, 242, 246); YELLOW = (250, 210, 40); RED = (226, 40, 46); BLK = (16, 16, 18)
BLUE_T, RED_T = (11, 34, 101), (167, 25, 48)
def R(x0, y0, x1, y1): return [x0 * S, y0 * S, x1 * S, y1 * S]
def new(bg=BG):
    im = Image.new("RGBA", (W * S, H * S), bg + (255,)); return im, ImageDraw.Draw(im)
def logo(name, size):
    im = Image.open(os.path.join(LOGOS, name)).convert("RGBA"); bb = im.getbbox(); im = im.crop(bb)
    sc = size * S / max(im.size); return im.resize((max(1, round(im.width * sc)), max(1, round(im.height * sc))), Image.LANCZOS)
def paste_logo(img, name, cx, cy, size):
    l = logo(name, size); img.alpha_composite(l, (round(cx * S - l.width / 2), round(cy * S - l.height / 2)))
def text(d, x, y, s, f, fill, a="mm"): d.text((x * S, y * S), s, font=f, fill=fill, anchor=a)
def big(d, s, y, col=WHITE, size=54):
    text(d, 243, y + 3, s, font(FB, size), (0, 0, 0), "mm"); text(d, 240, y, s, font(FB, size), col, "mm")
def sub(d, s, y=294, fg=YELLOW):
    f = font(FB, 15); w = d.textlength(s, font=f) / S + 36
    d.rounded_rectangle(R(240 - w / 2, y - 16, 240 + w / 2, y + 16), 16 * S, fill=(0, 0, 0)); text(d, 240, y, s, f, fg)
def glow(img, cx, cy, r=90, a=130):
    g = Image.new("RGBA", img.size, (0, 0, 0, 0)); ImageDraw.Draw(g).ellipse(R(cx - r, cy - r, cx + r, cy + r), fill=(255, 255, 255, a))
    img.alpha_composite(g.filter(ImageFilter.GaussianBlur(16 * S)))
def rays(d, cx, cy, cols, n=24, rot=0):
    for k in range(n):
        a0 = rot + k * 360 / n; d.pieslice(R(cx - 460, cy - 460, cx + 460, cy + 460), a0, a0 + 360 / n, fill=cols[k % 2])
def ball(d, cx, cy, w=22, ang=0):
    h = w * .6; d.ellipse(R(cx - w / 2, cy - h / 2, cx + w / 2, cy + h / 2), fill=(140, 80, 40))
    d.line(R(cx - w * .22, cy, cx + w * .22, cy), fill=WHITE, width=max(1, int(w / 12 * S)))
def tape(d, y0, y1):
    d.rectangle(R(0, y0, W, y1), fill=BLK)
    for yy in (y0 - 10, y1):
        d.rectangle(R(0, yy, W, yy + 10), fill=(250, 200, 20))
        for k in range(-1, 26):
            x = k * 20; d.polygon([(x * S, (yy + 10) * S), ((x + 10) * S, yy * S), ((x + 18) * S, yy * S), ((x + 8) * S, (yy + 10) * S)], fill=BLK)
def warn(d, tx, ty, s=1.0):
    d.polygon([(tx * S, (ty - 34 * s) * S), ((tx + 38 * s) * S, (ty + 30 * s) * S), ((tx - 38 * s) * S, (ty + 30 * s) * S)], fill=(250, 200, 20))
    text(d, tx, ty + 8 * s, "!", font(FB, int(40 * s)), BLK)
def turf(d, top=150):
    for k in range(14):
        y = top + k * 14 + k * k * 0.9
        d.rectangle(R(0, y, W, y + 12 + k), fill=(30 + (k % 2) * 8, 105 + (k % 2) * 10, 48))
def posts(d, cx, base, spread, height, col=YELLOW, wid=5):
    d.line(R(cx, base, cx, base + height * .5), fill=col, width=wid * S)               # the stem
    d.line(R(cx - spread, base, cx + spread, base), fill=col, width=wid * S)            # the crossbar
    for sx in (-1, 1): d.line(R(cx + sx * spread, base, cx + sx * spread, base - height), fill=col, width=wid * S)
def finish(im): return im.convert("RGB").resize((W, H), Image.LANCZOS)

def fg1():
    im, d = new((8, 22, 14)); turf(d, 120); posts(d, 240, 150, 26, 52, wid=3); ball(d, 240, 262, 24)
    d.rectangle(R(0, 0, W, 60), fill=(0, 0, 0, 255)); text(d, 240, 22, "FIELD GOAL TRY", font(FB, 26), YELLOW); text(d, 240, 46, "42 yards", font(FS, 15), WHITE)
    paste_logo(im, "nfl_nyg_112.png", 60, 40, 44); d = ImageDraw.Draw(im); return finish(im)
def fg2():
    im, d = new((8, 22, 14)); turf(d, 120); posts(d, 240, 120, 48, 90, wid=4)
    for t in range(1, 9):
        u = t / 9; x = 240 + math.sin(u * 2) * 6; y = 262 - u * 120 - math.sin(u * math.pi) * 40
        d.ellipse(R(x - 3, y - 3, x + 3, y + 3), fill=(255, 255, 255))
    ball(d, 240, 118, 15); text(d, 240, 22, "IT'S UP...", font(FB, 28), WHITE); return finish(im)
def fg3():
    im, d = new(); rays(d, 240, 150, [BLUE_T, RED_T]); glow(im, 240, 110, 100, 170); d = ImageDraw.Draw(im)
    posts(d, 240, 150, 105, 130, wid=7); ball(d, 240, 105, 30)
    big(d, "IT'S GOOD!", 214, WHITE, 50); sub(d, "FIELD GOAL 42 YDS - GIANTS 20  EAGLES 17"); return finish(im)
def fg4():
    im, d = new((8, 22, 14)); turf(d, 120); posts(d, 240, 150, 105, 130, wid=7); ball(d, 372, 90, 24)
    for t in range(1, 7): u = t / 7; d.ellipse(R(240 + u * 130 - 3, 262 - u * 170 - 3, 240 + u * 130 + 3, 262 - u * 170 + 3), fill=(255, 255, 255))
    tape(d, 214, 262); text(d, 240, 238, "NO GOOD", font(FB, 40), (255, 120, 110)); sub(d, "WIDE RIGHT - GIANTS KEEP 17", 294); return finish(im)
def td():
    im, d = new(); rays(d, 240, 120, [BLUE_T, RED_T]); glow(im, 240, 120, 95, 120); paste_logo(im, "nfl_nyg_112.png", 240, 120, 150); d = ImageDraw.Draw(im)
    big(d, "TOUCHDOWN", 240, WHITE, 52); sub(d, "GIANTS 28   EAGLES 17"); return finish(im)
def td_bad():
    im, d = new((14, 14, 18)); tape(d, 98, 214); warn(d, 62, 156); text(d, 290, 142, "TOUCHDOWN", font(FB, 46), YELLOW)
    paste_logo(im, "nfl_phi_112.png", 160, 190, 24); d = ImageDraw.Draw(im); text(d, 310, 190, "Eagles score - Giants 21, Eagles 24", font(FB, 15), WHITE); return finish(im)
def goal():
    im, d = new(); rays(d, 240, 125, [(150, 20, 30), (210, 40, 50)], 20); glow(im, 240, 125, 95, 130)
    paste_logo(im, "nhl_nyr_112.png", 240, 125, 150); d = ImageDraw.Draw(im)
    for x in (24, 456):                                   # the goal light
        d.ellipse(R(x - 20, 40, x + 20, 80), fill=(255, 40, 40)); d.ellipse(R(x - 8, 48, x + 8, 64), fill=(255, 190, 190))
    big(d, "GOAL!", 240, WHITE, 60); sub(d, "RANGERS 3   FLYERS 2 - 2ND PERIOD"); return finish(im)
def hr():
    im, d = new((20, 36, 70)); rays(d, 240, 300, [(30, 54, 100), (24, 44, 86)], 24)
    d.rectangle(R(0, 196, W, 320), fill=(30, 100, 48)); d.rectangle(R(0, 150, W, 200), fill=(18, 56, 40)); d.rectangle(R(0, 196, W, 202), fill=YELLOW)
    for t in range(0, 14): u = t / 13; d.ellipse(R(40 + u * 330 - 3, 236 - math.sin(u * math.pi) * 170 - 3, 40 + u * 330 + 3, 236 - math.sin(u * math.pi) * 170 + 3), fill=WHITE)
    ball(d, 376, 190 - 14, 20); paste_logo(im, "mlb_cle_112.png", 240, 74, 90); d = ImageDraw.Draw(im)
    big(d, "HOME RUN!", 240, WHITE, 52); sub(d, "GUARDIANS 4   WHITE SOX 2 - 7TH"); return finish(im)
def win():
    im, d = new((12, 22, 60)); random.seed(4)
    for i in range(150):
        x, y = random.randint(0, W), random.randint(0, H); c = random.choice([RED, YELLOW, WHITE, (60, 140, 255), (60, 210, 120)])
        a = random.random() * 3; w_, h_ = random.randint(4, 9), random.randint(8, 16)
        d.polygon([((x + math.cos(a) * w_) * S, (y + math.sin(a) * h_) * S), ((x - math.cos(a) * w_) * S, (y - math.sin(a) * h_) * S), ((x - math.cos(a) * w_ + 5) * S, (y - math.sin(a) * h_ + 4) * S)], fill=c)
    glow(im, 240, 112, 90, 100); paste_logo(im, "nfl_nyg_112.png", 240, 112, 140); d = ImageDraw.Draw(im)
    big(d, "GIANTS WIN!", 232, WHITE, 52); sub(d, "FINAL  31 - 24"); return finish(im)
def home_dim(mult, label):
    p = os.path.join(OUT, "../firmware/hosttest/out/p_p08_home_with_bye_tiles.ppm")
    im = Image.open(p).convert("RGB") if os.path.exists(p) else Image.new("RGB", (W, H), BG)
    px = im.load()
    for y in range(H):
        for x in range(W):
            r, g, b = px[x, y]; px[x, y] = (int(r * mult), int(g * mult), int(b * mult))
    return im

shots = [("1. Field goal: the kick is lined up", fg1()), ("2. In the air toward the posts", fg2()), ("3. Through the uprights", fg3()),
         ("4. No good: wide right (caution tape)", fg4()), ("5. Touchdown (ours): big and bright", td()), ("6. Their touchdown: caution tape", td_bad()),
         ("7. Goal (hockey): red lights", goal()), ("8. Home run", hr()), ("9. Final win: confetti", win()),
         ("10. Brightness now (full)", home_dim(1.0, "")), ("11. Dim today (about a third as bright)", home_dim(0.32, "")), ("12. New dimmer (about a fifth)", home_dim(0.2, ""))]
gap, lab = 24, 34
rows = (len(shots) + 2) // 3
sheet = Image.new("RGB", (3 * W + 4 * gap, rows * (H + lab + gap) + gap), (235, 236, 240))
sd = ImageDraw.Draw(sheet); lf = ImageFont.truetype(FB, 20)
for i, (nm, im) in enumerate(shots):
    c, r = i % 3, i // 3; x = gap + c * (W + gap); y = gap + r * (H + lab + gap)
    sd.text((x, y), nm, font=lf, fill=(30, 30, 40)); sheet.paste(im, (x, y + lab))
sheet.save(os.path.join(OUT, "mini_anims.png")); print(sheet.size)
