"""Lays out the animation frames (out/x_*.ppm) in design/mini_fx.png (frames of one animation side by side)."""
import glob, os, re
from PIL import Image, ImageDraw, ImageFont
HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "../../design/mini_fx.png")
F = ImageFont.truetype(os.path.join(os.environ.get("FONTS", "/usr/share/fonts/opentype/inter"), "Inter-SemiBold.otf"), 20)
files = sorted(glob.glob(os.path.join(HERE, "out/x_*.ppm")))
groups = {}
for f in files:
    m = re.match(r"x_(.+)_(\d+)\.ppm", os.path.basename(f))
    groups.setdefault(m.group(1), []).append((int(m.group(2)), f))
order = ["td", "fg", "nogood", "theirs", "goal", "hr", "three", "win", "kickoff", "quarter", "flag", "first", "picked", "fumble", "sack", "stopped", "stonewall", "punt", "went", "nopunt", "turnover"]
W, H, pad, cap = 240, 160, 10, 26      # half size, up to four frames a row
rows = [g for g in order if g in groups]
sheet = Image.new("RGB", (pad + 4 * (W + pad), pad + len(rows) * (H + cap + pad)), (234, 236, 240))
d = ImageDraw.Draw(sheet)
for r, g in enumerate(rows):
    y = pad + r * (H + cap + pad)
    d.text((pad, y), g, font=F, fill=(30, 32, 38))
    for c, (i, f) in enumerate(sorted(groups[g])[:4]):
        sheet.paste(Image.open(f).resize((W, H)), (pad + c * (W + pad), y + cap))
sheet.save(OUT)
print(OUT, sheet.size)
