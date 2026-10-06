import sys, trimesh, numpy as np, warnings; warnings.filterwarnings("ignore")
import os; sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from zrender import render
from PIL import Image, ImageDraw, ImageFont
# usage: views2.py original_base.stl original_lid.stl deep_base.stl extra_depth_mm out.png
orig = trimesh.load(sys.argv[1]); lid = trimesh.load(sys.argv[2])
deep = trimesh.load(sys.argv[3]); H = float(sys.argv[4])

WHITE = (0.95, 0.95, 0.96); BLUE = (0.3, 0.5, 0.95); GREEN = (0.25, 0.6, 0.3); DARK = (0.12, 0.13, 0.17)
bat = trimesh.creation.box(extents=[50, 34, 10]); bat.apply_translation([55.5, 30.5, -6.7 - H + 5])
F = ImageFont.truetype("/usr/share/fonts/opentype/inter/Inter-Bold.otf", 22)
def lab(im, t):
    c = Image.new("RGB", (im.width, im.height + 40), "white"); c.paste(im, (0, 40))
    ImageDraw.Draw(c).text((im.width / 2, 20), t, font=F, fill=(30, 30, 40), anchor="mm"); return c
kw = dict(W=760, H=520, scale=5.2)
a = lab(render([(orig, WHITE)], elev=50, azim=-25, center=(55, 30, -6), **kw), "Original back (8.7 mm deep)")
b = lab(render([(deep, WHITE), (bat, BLUE)], elev=50, azim=-25, center=(55, 30, -9), **kw), "New back (%.1f mm deep), battery in blue" % (8.65 + H))
pcb = trimesh.creation.box(extents=[110.6, 60.6, 1.61]); pcb.apply_translation([55.5, 30.5, -1.7 + 0.8])
scr = trimesh.creation.box(extents=[94.6, 61.0, 2.9]); scr.apply_translation([55.5, 30.5, -0.1 + 1.45])
lid_on = lid.copy()
c = lab(render([(orig, WHITE), (lid_on, WHITE), (pcb, GREEN), (scr, DARK)], elev=14, azim=-28, center=(55, 30, -6), **kw), "Closed: original (about 14 mm thick)")
d = lab(render([(deep, WHITE), (lid, WHITE), (pcb, GREEN), (scr, DARK)], elev=14, azim=-28, center=(55, 30, -9), **kw), "Closed: new (about %.0f mm thick), switch slot on the end" % (8.65 + H + 5.2))
sheet = Image.new("RGB", (a.width * 2 + 30, a.height * 2 + 30), "white")
for i, im in enumerate([a, b, c, d]): sheet.paste(im, ((i % 2) * (a.width + 30), (i // 2) * (a.height + 30)))
sheet.save(sys.argv[5]); print("ok")
