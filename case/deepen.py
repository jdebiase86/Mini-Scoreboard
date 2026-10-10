"""Deeper base for the ESP32-32E 4.0" case: same top (standoffs, USB-C slot, lid fit),
a spacer section added under the board for a flat LiPo, battery cradle on the floor,
switch slot in the right end wall."""
import sys, trimesh, numpy as np, warnings; warnings.filterwarnings("ignore")
from shapely.geometry import box
from shapely.ops import unary_union
SRC, OUT = sys.argv[1], sys.argv[2]
H = float(sys.argv[3]) if len(sys.argv) > 3 else 10.0     # extra depth, mm (= battery thickness)
BL = float(sys.argv[4]) if len(sys.argv) > 4 else 60.0    # battery length (along the case), mm
BW = float(sys.argv[5]) if len(sys.argv) > 5 else 50.0    # battery width, mm
BX = float(sys.argv[6]) if len(sys.argv) > 6 else 55.55   # battery centre along the case, mm
SPK = len(sys.argv) > 7 and sys.argv[7] == "speaker"     # 25 x 35 mm speaker at the USB-C end
ZC = -6.3            # cut height: inside the standoff zone, above the floor (floor top -6.7)
FLOOR_TOP = -6.7
base = trimesh.load(SRC)
top = trimesh.intersections.slice_mesh_plane(base, [0, 0, 1], [0, 0, ZC], cap=True)
bot = trimesh.intersections.slice_mesh_plane(base, [0, 0, -1], [0, 0, ZC], cap=True)
bot.apply_translation([0, 0, -H])
sec = base.section(plane_origin=[0, 0, ZC], plane_normal=[0, 0, 1])
p2, T = sec.to_2D(to_2D=np.eye(4))
prof = unary_union(p2.polygons_full)
polys = list(prof.geoms) if prof.geom_type == "MultiPolygon" else [prof]
spacer = trimesh.util.concatenate([trimesh.creation.extrude_polygon(pg, H) for pg in polys])
spacer.apply_translation([0, 0, ZC - H])
parts = [top, bot, spacer]
# close the USB-C slot below the original floor line (slot stays exactly as before above it)
fill = trimesh.creation.box(extents=[4.3, 12.4, H + 0.4])
fill.apply_translation([-2.85, 30.58, FLOOR_TOP - H / 2])
parts.append(fill)
# battery cradle: L-shaped corner ribs, 3 mm tall, around a pocket 2 mm bigger than the battery, centred under the board
bw, bh, rt, rh = BL + 2, BW + 2, 1.6, 3.0
bx0, by0 = BX - bw / 2, 30.575 - bh / 2
fz = FLOOR_TOP - H
for cx, cy in ((bx0, by0), (bx0 + bw, by0), (bx0, by0 + bh), (bx0 + bw, by0 + bh)):
    sx = -1 if cx == bx0 else 1; sy = -1 if cy == by0 else 1
    rx = trimesh.creation.box(extents=[10, rt, rh]); rx.apply_translation([cx - sx * (5 - rt), cy + sy * rt / 2, fz + rh / 2])
    ry = trimesh.creation.box(extents=[rt, 10, rh]); ry.apply_translation([cx + sx * rt / 2, cy - sy * (5 - rt), fz + rh / 2])
    parts += [rx, ry]
import manifold3d
def M(m): return manifold3d.Manifold(manifold3d.Mesh(vert_properties=np.asarray(m.vertices, np.float32), tri_verts=np.asarray(m.faces, np.uint32)))
acc = M(parts[0])
for p in parts[1:]:
    for q in (p.split(only_watertight=True) if len(p.split(only_watertight=True)) > 1 else [p]): acc = acc + M(q)
# switch slot through the right end wall (x = 111..116), mid-depth of the new section
if SPK:
    # speaker pocket for the 35 x 25 x 6.8 mm rectangular speaker (CQRobot, 25 along the case, 35 across): corner ribs, and a grille of 1.8 mm holes through the floor under it
    SX, SY, sw_, sh_ = 29.5, 30.575, 27.0, 37.0
    for cx, cy in ((SX - sw_ / 2, SY - sh_ / 2), (SX + sw_ / 2, SY - sh_ / 2), (SX - sw_ / 2, SY + sh_ / 2), (SX + sw_ / 2, SY + sh_ / 2)):
        sx = -1 if cx < SX else 1; sy = -1 if cy < SY else 1
        rx = trimesh.creation.box(extents=[6, rt, 4.0]); rx.apply_translation([cx - sx * (3 - rt), cy + sy * rt / 2, fz + 2.0])
        ry = trimesh.creation.box(extents=[rt, 6, 4.0]); ry.apply_translation([cx + sx * rt / 2, cy - sy * (3 - rt), fz + 2.0])
        acc = acc + M(rx) + M(ry)
    for i in range(-4, 5):
        for j in range(-7, 8):
            hx = SX + i * 2.6 + (1.3 if j % 2 else 0); hy = SY + j * 2.3
            if (hx - SX) ** 2 / 9.5 ** 2 + (hy - SY) ** 2 / 15.0 ** 2 > 1: continue
            c = trimesh.creation.cylinder(radius=0.9, height=6, sections=16); c.apply_translation([hx, hy, fz - 1])
            acc = acc - M(c)
import os
if os.environ.get("SWITCH", "0") == "1":   # Oct 7: Joe dropped the on/off switch; SWITCH=1 puts the slot back
    sw = trimesh.creation.box(extents=[8, 9.5, 4.5]); sw.apply_translation([113.5, 30.58, ZC - H / 2 - 0.5])
    acc = acc - M(sw)
mm = acc.to_mesh()
out = trimesh.Trimesh(mm.vert_properties[:, :3], mm.tri_verts)
out.export(OUT)
print("watertight", out.is_watertight, "bounds", np.round(out.bounds, 2).tolist())
