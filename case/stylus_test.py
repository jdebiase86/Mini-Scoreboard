"""A little test strip for the stylus groove: six short pieces of the case's stylus wall, each with a different groove
width (same groove shape, same 8 mm rib, same wall as build_case.py), so one print shows which width the stylus slides into.

    python3 stylus_test.py <out folder> [sizes in mm, e.g. 5.1 5.3 5.5 5.7 5.9 6.1]

Writes stylus_test.stl and stylus_test.3mf. Print it like the case: wall standing up, groove running left to right, the
ribs sticking out sideways. The smallest groove is at the end with the small corner bump. Try the stylus in each.
"""
import sys, os, trimesh, numpy as np, warnings; warnings.filterwarnings("ignore")
import manifold3d
OUT = sys.argv[1]; SIZES = [float(a) for a in sys.argv[2:]] or [5.1, 5.3, 5.5, 5.7, 5.9, 6.1]
os.makedirs(OUT, exist_ok=True)
def M(m): return manifold3d.Manifold(manifold3d.Mesh(vert_properties=np.asarray(m.vertices, np.float32), tri_verts=np.asarray(m.faces, np.uint32)))
L, WALL, RIBL, PITCH = 22.0, 5.0, 8.0, 12.2   # piece length, wall thickness, rib length, spacing along y
CYF = 0.9                                      # groove axis, inside the wall face (as in build_case.py)
def box(x0, x1, y0, y1, z0, z1):
    b = trimesh.creation.box(extents=[x1 - x0, y1 - y0, z1 - z0]); b.apply_translation([(x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2]); return M(b)
def ring(x, cy, r):
    t = np.linspace(0, 2 * np.pi, 48, endpoint=False)
    return np.c_[np.full_like(t, x), cy + r * np.cos(t), r * np.sin(t)]
parts = None
for k, d in enumerate(SIZES):
    oy = 4.2 + k * PITCH                      # where this piece's wall face sits; the rib reaches 4.2 mm back toward -y
    wall = box(0, L, oy, oy + WALL, -7.2, 7.2)
    pts = np.array([(oy + 1.0, -7.2), (oy - 4.2, -2.0), (oy - 4.2, 2.0), (oy + 1.0, 7.2)])
    x0 = (L - RIBL) / 2
    rib = trimesh.convex.convex_hull(np.vstack([np.c_[np.full(4, x0), pts], np.c_[np.full(4, x0 + RIBL), pts]]))
    piece = wall + M(rib)
    bore = trimesh.convex.convex_hull(np.vstack([ring(-1.0, oy + CYF, d / 2), ring(L + 1.0, oy + CYF, d / 2)]))
    piece = piece - M(bore)
    parts = piece if parts is None else parts + piece
last = 4.2 + (len(SIZES) - 1) * PITCH + WALL
plate = box(0, L, 0.0, last, -8.7, -7.0)      # a thin base joining the pieces (the stylus never reaches it)
bump = box(0, 3.0, 0.0, 3.0, -7.0, -4.5)      # marks the smallest groove's end
m = (parts + plate + bump).to_mesh()
mesh = trimesh.Trimesh(m.vert_properties[:, :3], m.tri_verts)
mesh.apply_translation([0, 0, 8.7])
mesh.export(os.path.join(OUT, "stylus_test.stl")); mesh.export(os.path.join(OUT, "stylus_test.3mf"))
print(mesh.bounds, mesh.is_watertight, SIZES)
