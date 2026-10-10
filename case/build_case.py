"""Builds the Mini Scoreboard case: the designer's "with buttons" base made deeper
(battery and speaker under the board), heat-set insert holes kept short, and a
stylus tube along one long side (drawn from scratch; only the stylus width was
read from another design). Needs the designer's base and lid STL files, which
are not kept in this repo (they belong to their designer).

    python3 build_case.py <buttons base.stl> <lid.stl> <out folder>

Writes base_final.stl, lid.stl and a picture, case_v1.png.
"""
import sys, os, subprocess, trimesh, numpy as np, warnings; warnings.filterwarnings("ignore")
import manifold3d
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from zrender import render
from PIL import Image, ImageDraw, ImageFont
BASE, LID, OUT = sys.argv[1], sys.argv[2], sys.argv[3]
os.makedirs(OUT, exist_ok=True)
H = 10.0                       # extra depth (battery thickness)
BX = 78.0                      # battery centre along the case (the speaker is 29.5, clear of the RESET and BOOT fingers)
HERE = os.path.dirname(os.path.abspath(__file__))
tmp = os.path.join(OUT, "deep.stl")
subprocess.run([sys.executable, os.path.join(HERE, "deepen.py"), BASE, tmp, str(H), "67", "36", str(BX), "speaker"], check=True)
deep = trimesh.load(tmp)
def M(m): return manifold3d.Manifold(manifold3d.Mesh(vert_properties=np.asarray(m.vertices, np.float32), tri_verts=np.asarray(m.faces, np.uint32)))
BOT = -8.65 - H
# Screw holes: measured on the real board (centre to centre 103.68 / 103.99 across, 53.36 / 53.54 up and down), so the
# average of each pair is used about the old hole centre. The designer's holes (105.1 x 54.1 apart) are filled in.
OLD = ((2.5, 3.5), (2.5, 57.6), (107.6, 3.5), (107.6, 57.6))
CX, CY, HX, HY = 55.05, 30.55, (103.68 + 103.99) / 4, (53.36 + 53.54) / 4
HOLES = tuple((CX + sx * HX, CY + sy * HY) for sx in (-1, 1) for sy in (-1, 1))
acc = M(deep)
# 1. heat-set insert holes (M3, 4.5 mm wide) are only 4.7 mm deep: fill the rest of each hole so an insert can't sink
POST_TOP = -1.7
for hx, hy in OLD:
    plug = trimesh.creation.cylinder(radius=2.5, height=(POST_TOP - (BOT + 0.5)), sections=32)
    plug.apply_translation([hx, hy, (POST_TOP + BOT + 0.5) / 2])
    acc = acc + M(plug)
for hx, hy in HOLES:      # new holes: 4.5 mm wide, 5 mm deep, down from the top of the post
    hole = trimesh.creation.cylinder(radius=2.25, height=5.0 + 0.2, sections=48)
    hole.apply_translation([hx, hy, POST_TOP - 2.5 + 0.1])
    acc = acc - M(hole)
plain = acc
# 2. stylus holder on the top (y = 66.15) long side, away from any stand: the stylus lies in a half-round
#    groove cut into the wall (3.9 mm wide at the left end, 4.4 mm at the right, so it wedges tight as it is
#    pushed in) and three small loops hold it in. Each loop sticks out only 4.2 mm and has 45-degree
#    undersides, so it prints without supports. The stylus shows at both ends.
YW, ZC0 = 66.15, BOT / 2 + 0.0        # wall face, mid-depth of the base
def ring(x, r):
    t = np.linspace(0, 2 * np.pi, 48, endpoint=False)
    return np.c_[np.full_like(t, x), YW + r * np.cos(t), ZC0 + r * np.sin(t)]
bore = trimesh.convex.convex_hull(np.vstack([ring(-6.0, 1.95), ring(117.0, 2.2)]))
loops = []
for x0, x1 in ((16, 24), (58, 66), (100, 108)):
    pts = np.array([(YW - 1.0, ZC0 - 7.2), (YW + 4.2, ZC0 - 2.0), (YW + 4.2, ZC0 + 2.0), (YW - 1.0, ZC0 + 7.2)])
    verts = np.vstack([np.c_[np.full(4, x0), pts], np.c_[np.full(4, x1), pts]])
    loops.append(trimesh.convex.convex_hull(verts))
with_tube = plain
for lp in loops: with_tube = with_tube + M(lp)
with_tube = with_tube - M(bore)
def tomesh(m):
    mm = m.to_mesh()
    return trimesh.Trimesh(mm.vert_properties[:, :3], mm.tri_verts)
base_plain = tomesh(plain); base = tomesh(with_tube)
base_plain.export(os.path.join(OUT, "base_plain.stl"))
base.export(os.path.join(OUT, "base_final.stl"))
# lid: screw holes moved to the measured pattern, and the screen pocket made 1 mm deeper so the screen face sits flush
lm = trimesh.load(LID); L = M(lm)
def cyl(r, z0, z1, x, y):
    c = trimesh.creation.cylinder(radius=r, height=z1 - z0, sections=48); c.apply_translation([x, y, (z0 + z1) / 2]); return M(c)
POCKET_TOP, DEEPER = 3.0, 1.0
for hx, hy in ((2.5, 3.5), (2.5, 57.65), (107.6, 3.5), (107.6, 57.65)):
    L = L + cyl(2.85, 0.0, 5.2, hx, hy)
for hx, hy in HOLES:
    L = L + cyl(3.7, POCKET_TOP - 0.01, POCKET_TOP + DEEPER, hx, hy)     # keeps a wall between the screw head recess and the deeper pocket
    L = L - cyl(1.75, -1.0, 6.0, hx, hy) - cyl(2.79, 3.3, 6.0, hx, hy)
pocket = trimesh.creation.box(extents=[102.8 - 6.0, 61.2 - 0.0, DEEPER]); pocket.apply_translation([(6.0 + 102.8) / 2, 30.6, POCKET_TOP + DEEPER / 2])
pk = M(pocket)
for hx, hy in HOLES: pk = pk - cyl(3.7, POCKET_TOP - 1, POCKET_TOP + DEEPER + 1, hx, hy)
L = L - pk
for hx, hy in HOLES: L = L - cyl(1.75, -1.0, 6.0, hx, hy) - cyl(2.79, 3.3, 6.0, hx, hy)
lid = tomesh(L); lid.export(os.path.join(OUT, "lid.stl"))
print("base watertight", base.is_watertight, base_plain.is_watertight, "size", np.round(base.bounds[1] - base.bounds[0], 1).tolist())

# ---- the picture
WHITE = (0.95, 0.95, 0.96); BLUE = (0.3, 0.5, 0.95); GREEN = (0.25, 0.6, 0.3); DARK = (0.12, 0.13, 0.17)
GOLD = (0.9, 0.7, 0.2); RED = (0.9, 0.25, 0.25); GREY = (0.45, 0.47, 0.5)
fz = -6.7 - H
bat = trimesh.creation.box(extents=[67, 36, 10]); bat.apply_translation([BX, 30.575, fz + 5.5])
spk = trimesh.creation.box(extents=[25, 35, 6.8]); spk.apply_translation([29.5, 30.575, fz + 0.4 + 3.4])
ins = []
for hx, hy in HOLES:
    c = trimesh.creation.cylinder(radius=2.3, height=4.5, sections=24); c.apply_translation([hx, hy, -2.6 - 2.25 + 0.2]); ins.append((c, GOLD))
pcb = trimesh.creation.box(extents=[110.6, 60.6, 1.61]); pcb.apply_translation([55.55, 30.575, -1.7 + 0.8])
scr = trimesh.creation.box(extents=[94.6, 61.0, 2.9]); scr.apply_translation([55.55, 30.575, -0.1 + 1.45])
def stylus(pull):
    s = trimesh.creation.cylinder(radius=2.0, height=95, sections=32); s.apply_transform(trimesh.transformations.rotation_matrix(np.pi / 2, [0, 1, 0]))
    s.apply_translation([116.1 - 47.5 + pull, YW, ZC0]); return s
F = ImageFont.truetype("/usr/share/fonts/opentype/inter/Inter-Bold.otf", 22)
def lab(im, t):
    c = Image.new("RGB", (im.width, im.height + 40), "white"); c.paste(im, (0, 40))
    ImageDraw.Draw(c).text((im.width / 2, 20), t, font=F, fill=(30, 30, 40), anchor="mm"); return c
kw = dict(W=760, H=520, scale=5.0)
lid_up = lid.copy(); lid_up.apply_translation([0, 0, 22])
a = lab(render([(base, WHITE), (bat, BLUE), (spk, GREY)] + ins, elev=55, azim=-25, center=(55, 30, -12), **kw), "Base from above: battery (blue), speaker (grey)")
b = lab(render([(base, WHITE), (stylus(18), RED)], elev=-50, azim=-25, center=(55, 30, -12), **kw), "Back: speaker grille (tube is on the far edge)")
c = lab(render([(base, WHITE), (lid, WHITE), (pcb, GREEN), (scr, DARK), (stylus(0), RED)], elev=22, azim=150, center=(55, 30, -9), **kw), "Closed, from the top edge: stylus parked")
d = lab(render([(base, WHITE), (lid, WHITE), (pcb, GREEN), (scr, DARK), (stylus(30), RED)], elev=30, azim=125, center=(55, 30, -9), **kw), "Stylus pulled out the right end")
sheet = Image.new("RGB", (a.width * 2 + 30, a.height * 2 + 30), "white")
for i, im in enumerate([a, b, c, d]): sheet.paste(im, ((i % 2) * (a.width + 30), (i // 2) * (a.height + 30)))
sheet.save(os.path.join(OUT, "case_v1.png")); print("ok")
