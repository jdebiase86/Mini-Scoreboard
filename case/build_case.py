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
# Screw holes: the real board's four measured centre-to-centre distances (metal inserts, no give): bottom pair 103.68,
# top pair 103.99, left pair 53.36, right pair 53.54 mm. Taking the bottom edge level and the left edge upright, the
# four distances fix the corners exactly (solved here), centred on the old hole centre. The designer's holes
# (105.1 x 54.1 apart) are filled in.
OLD = ((2.5, 3.5), (2.5, 57.6), (107.6, 3.5), (107.6, 57.6))
def board_holes(bot, top, left, right):
    from scipy.optimize import fsolve
    tx, ty = fsolve(lambda p: [(p[0] - bot) ** 2 + p[1] ** 2 - right ** 2, p[0] ** 2 + (p[1] - left) ** 2 - top ** 2], [bot, left])
    pts = [(0, 0), (bot, 0), (0, left), (tx, ty)]
    mx = sum(p[0] for p in pts) / 4; my = sum(p[1] for p in pts) / 4
    return tuple((55.05 + p[0] - mx, 30.55 + p[1] - my) for p in pts)
HOLES = board_holes(103.68, 103.99, 53.36, 53.54)
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
# 1b. the USB opening (in the left end wall): the faceplate no longer has a notch, so the base opening alone is the hole. It was
#     11.9 mm wide (y 24.6 to 36.5) and 6.45 mm deep from the rim; now 1 mm narrower each side and its floor 1.5 mm higher
#     (9.9 x 4.95 mm, snug round the port). Only the wall itself (x -5 to -0.9) changes: the pocket behind it is left alone.
def box(x0, x1, y0, y1, z0, z1):
    b = trimesh.creation.box(extents=[x1 - x0, y1 - y0, z1 - z0]); b.apply_translation([(x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2]); return M(b)
USB_Y0, USB_Y1, USB_FLOOR, USB_NEW_FLOOR = 24.6, 36.5, -6.45, -4.95
acc = acc + box(-5.0, -0.9, USB_Y0 - 0.05, USB_Y0 + 1.0, USB_FLOOR - 0.05, 0.0)       # the left side moves in 1 mm
acc = acc + box(-5.0, -0.9, USB_Y1 - 1.0, USB_Y1 + 0.05, USB_FLOOR - 0.05, 0.0)       # the right side moves in 1 mm
acc = acc + box(-5.0, -0.9, USB_Y0 - 0.05, USB_Y1 + 0.05, USB_FLOOR - 0.05, USB_NEW_FLOOR)   # the floor comes up 1.5 mm
plain = acc
# 2. stylus holder on the top long side (the y = 0 wall, so it is at the top when the unit is laid out): the stylus (86.87 mm
#    long, 4.7 mm across) slides into a round groove cut into the wall until it stops against the closed end, and is then
#    flush with the right end of the case. Three tube-shaped ribs, 26 mm apart and all inside the stylus's length, hold it
#    snugly (4.9 mm bore). The ribs have 45-degree undersides, so the base prints without supports. The head end (the
#    bump to put a fingernail under) stays in the clear stretch beyond the last rib.
SL, SD = 86.87, 4.7
YW, ZC0 = -5.0, BOT / 2 + 0.0        # wall face (the outer face of the y = 0 wall), mid-depth of the base
CY = YW + 0.9                         # groove axis 0.9 mm inside the face: the stylus sticks out 1.45 mm, 1.65 mm of wall is left behind it
XEND = 116.1                          # the right end face of the case
XSTOP = XEND - SL - 0.4               # the closed end of the groove
BORE_R = 2.45                         # 4.9 mm bore for the 4.7 mm stylus (printed holes come out a little smaller)
def ring(x, r):
    t = np.linspace(0, 2 * np.pi, 48, endpoint=False)
    return np.c_[np.full_like(t, x), CY + r * np.cos(t), ZC0 + r * np.sin(t)]
bore = trimesh.convex.convex_hull(np.vstack([ring(XSTOP, BORE_R), ring(XEND + 2.0, BORE_R)]))
RIBS = ((40.0, 48.0), (66.0, 74.0), (92.0, 100.0))
loops = []
for x0, x1 in RIBS:
    pts = np.array([(YW + 1.0, ZC0 - 7.2), (YW - 4.2, ZC0 - 2.0), (YW - 4.2, ZC0 + 2.0), (YW + 1.0, ZC0 + 7.2)])
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
L = L + box(-5.0, 6.1, USB_Y0 - 0.05, USB_Y1 + 0.05, -0.02, 1.0)   # no USB notch in the faceplate
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
    s = trimesh.creation.cylinder(radius=SD / 2, height=SL, sections=32); s.apply_transform(trimesh.transformations.rotation_matrix(np.pi / 2, [0, 1, 0]))
    s.apply_translation([XEND - SL / 2 + pull, CY, ZC0]); return s
F = ImageFont.truetype("/usr/share/fonts/opentype/inter/Inter-Bold.otf", 22)
def lab(im, t):
    c = Image.new("RGB", (im.width, im.height + 40), "white"); c.paste(im, (0, 40))
    ImageDraw.Draw(c).text((im.width / 2, 20), t, font=F, fill=(30, 30, 40), anchor="mm"); return c
kw = dict(W=760, H=520, scale=5.0)
lid_up = lid.copy(); lid_up.apply_translation([0, 0, 22])
a = lab(render([(base, WHITE), (bat, BLUE), (spk, GREY)] + ins, elev=55, azim=-25, center=(55, 30, -12), **kw), "Base from above: battery (blue), speaker (grey)")
b = lab(render([(base, WHITE), (lid, WHITE), (pcb, GREEN), (scr, DARK), (stylus(0), RED)], elev=22, azim=-30, center=(55, 30, -9), **kw), "Closed, top edge: stylus parked flush")
c = lab(render([(base, WHITE), (lid, WHITE), (pcb, GREEN), (scr, DARK), (stylus(-30), RED)], elev=30, azim=-55, center=(55, 30, -9), **kw), "Stylus slid out (it stops at the closed end)")
d = lab(render([(base, WHITE), (lid, WHITE), (pcb, GREEN), (scr, DARK)], elev=12, azim=200, center=(0, 30, -6), **kw), "USB end: plain faceplate, snug opening")
sheet = Image.new("RGB", (a.width * 2 + 30, a.height * 2 + 30), "white")
for i, im in enumerate([a, b, c, d]): sheet.paste(im, ((i % 2) * (a.width + 30), (i // 2) * (a.height + 30)))
sheet.save(os.path.join(OUT, "case_v1.png")); print("ok")
