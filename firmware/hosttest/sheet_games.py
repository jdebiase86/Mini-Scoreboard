"""Lays out the score screens (out/g*.ppm) with captions: design/mini_stage2.png."""
import glob, os
from PIL import Image, ImageDraw, ImageFont
HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "../../design/mini_stage2.png")
F = ImageFont.truetype(os.path.join(os.environ.get("FONTS", "/usr/share/fonts/opentype/inter"), "Inter-SemiBold.otf"), 20)
CAP = {
    "g01_loading": "Just started: scores loading",
    "g02_home_real": "Home with real ESPN data",
    "g03_home_live": "A game goes live: jumps first *",
    "g04_home_page2": "Second page (MORE): the rest plus AUTO",
    "g05_football_live": "Game screen, live football *",
    "g06_football_upcoming": "Game screen, next game (real)",
    "g07_football_final": "Game screen, final (real)",
    "g08_college_today_auto": "College, today; AUTO tag (real)",
    "g09_hockey_live": "Hockey, live *",
    "g10_hockey_upcoming": "Hockey, next game (real)",
    "g11_basketball_3digits": "Basketball, three-digit scores *",
    "g12_no_game": "A team with no game this week (real)",
    "g13_baseball_live_made_up": "Baseball, live (made-up game) *",
    "g14_logos_missing": "Logo not downloaded yet: letters",
}
files = sorted(glob.glob(os.path.join(HERE, "out/g[0-9]*.ppm")))
cols, W, H, pad, cap = 3, 480, 320, 24, 34
rows = (len(files) + cols - 1) // cols
sheet = Image.new("RGB", (pad + cols * (W + pad), pad + rows * (H + cap + pad) + 30), (234, 236, 240))
d = ImageDraw.Draw(sheet)
for i, f in enumerate(files):
    name = os.path.splitext(os.path.basename(f))[0]
    x = pad + (i % cols) * (W + pad); y = pad + (i // cols) * (H + cap + pad)
    d.text((x, y), f"{i + 1}. {CAP.get(name, name)}", font=F, fill=(30, 32, 38))
    sheet.paste(Image.open(f), (x, y + cap))
d.text((pad, sheet.height - 34), "* ESPN has no game on right now, so these live games are real games with the score and clock changed.", font=F, fill=(90, 94, 104))
sheet.save(OUT)
print(OUT)
