# Renders the closed case on a placeholder stand with mock-up screens on it.
# usage: scene.py out.png original_lid.stl deep_base.stl
import sys, trimesh, numpy as np, warnings; warnings.filterwarnings("ignore")
import os; sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from zrender import render
from PIL import Image, ImageDraw, ImageFont
from trimesh.transformations import rotation_matrix as RM
D = os.path.join(os.path.dirname(os.path.abspath(__file__)), "../design/")
def crop(sheet, i):
    im = Image.open(D + sheet).convert("RGB"); c, r = i % 3, i // 3
    x = 24 + c * (480 + 24); y = 24 + r * (320 + 34 + 24) + 34
    return im.crop((x, y, x + 480, y + 320))
base = trimesh.load(sys.argv[3]); lid = trimesh.load(sys.argv[2])
WHITE = (0.94, 0.94, 0.95); KEY = (1.0, 0.0, 1.0); GLASS = (0.04, 0.04, 0.06)
# black glass in the window, screen image area on top of it
glass = trimesh.creation.box(extents=[89, 61, 0.2]); glass.apply_translation([57.9, 30.6, 2.8])
act = trimesh.creation.box(extents=[83.5, 55.7, 0.2]); act.apply_translation([57.9, 30.6, 2.95])
AC = np.array([[57.9 - 41.75, 30.6 + 27.85, 3.05], [57.9 + 41.75, 30.6 + 27.85, 3.05], [57.9 - 41.75, 30.6 - 27.85, 3.05]])  # TL, TR, BL
def stand_T(tilt=17):
    T = RM(np.radians(90 - tilt), [1, 0, 0], [0, 0, 0])       # front faces -y and up, top leaning back
    return T
def place(mesh, T, lift):
    m = mesh.copy(); m.apply_transform(T); m.apply_translation([0, 0, lift]); return m
def scene(screen, T, lift, extra, elev, azim, center, scale, title, W=900, H=640):
    parts = [(place(base, T, lift), WHITE), (place(lid, T, lift), WHITE), (place(glass, T, lift), GLASS, True), (place(act, T, lift), KEY, True)] + extra
    SS = 2
    img, (R, sc, w, h, c) = render(parts, elev=elev, azim=azim, center=center, W=W * SS, H=H * SS, scale=scale * SS, want_proj=True)
    pts = trimesh.transform_points(AC, T) + [0, 0, lift]
    v = (pts - c) @ R.T
    P = np.c_[v[:, 0] * sc + w / 2, -v[:, 1] * sc + h / 2]
    # affine: output pixel -> screen image pixel
    src = np.array([[0, 0], [480, 0], [0, 320]], float)
    A = np.c_[P, np.ones(3)]; coef = np.linalg.solve(A, src)  # 3x2
    scr = screen.resize((960, 640), Image.LANCZOS)
    srcs = src * 2
    coef = np.linalg.solve(A, srcs)
    warped = scr.transform((w, h), Image.AFFINE, (coef[0, 0], coef[1, 0], coef[2, 0], coef[0, 1], coef[1, 1], coef[2, 1]), resample=Image.BICUBIC)
    a = np.asarray(img).astype(int)
    mask = (a[:, :, 0] > 200) & (a[:, :, 1] < 60) & (a[:, :, 2] > 200)
    out = np.asarray(img).copy(); wa = np.asarray(warped)
    out[mask] = wa[mask]
    im = Image.fromarray(out).resize((W, H), Image.LANCZOS)
    F = ImageFont.truetype("/usr/share/fonts/opentype/inter/Inter-Bold.otf", 24)
    c2 = Image.new("RGB", (W, H + 44), "white"); c2.paste(im, (0, 44))
    ImageDraw.Draw(c2).text((W / 2, 22), title, font=F, fill=(30, 30, 40), anchor="mm")
    return c2
DESK = (0.55, 0.40, 0.28); ORANGE = (0.95, 0.5, 0.15)
def desk(lift_z=0):
    d = trimesh.creation.box(extents=[400, 300, 10]); d.apply_translation([55, 60, -5 + lift_z]); return (d, DESK)
def stand_parts(tilt, dev):
    # simple placeholder cradle: base plate, front lip, back support leaning with the device
    v = dev.vertices
    front = v[v[:, 2] < 8][:, 1].min()
    band = v[(v[:, 2] > 25) & (v[:, 2] < 35)]; backy = band[:, 1].max()
    p = trimesh.creation.box(extents=[80, 50, 4]); p.apply_translation([55.5, front + 20, 2])
    lip = trimesh.creation.box(extents=[80, 4, 9]); lip.apply_translation([55.5, front - 2.2, 4.5])
    back = trimesh.creation.box(extents=[70, 4, 40]); back.apply_transform(RM(np.radians(-tilt), [1, 0, 0], [0, 0, 0]))
    back.apply_translation([55.5, backy + 2.2 - 30 * np.tan(np.radians(tilt)) + 20 * np.tan(np.radians(tilt)), 20])
    return [(p, ORANGE), (lip, ORANGE), (back, ORANGE)]
T = stand_T(17)
# device sits on the stand plate (top of plate z=4); lift so lowest point rests at z=4
lowest = trimesh.transform_points(np.vstack([base.vertices, lid.vertices]), T)[:, 2].min()
lift = 4.5 - lowest
# shift device so its back rests near the support: compute y range
yy = trimesh.transform_points(np.vstack([base.vertices]), T)[:, 1]
print("device y range", yy.min(), yy.max())
dev = trimesh.util.concatenate([place(base, T, lift), place(lid, T, lift)])
extra = [desk()] + stand_parts(17, dev)
home = crop("mini_mockups.png", 0); live = crop("mini_mockups.png", 1); td = crop("mini_mockups.png", 4)
defense = crop("mini_redzone_opp.png", 1); final = crop("mini_mockups.png", 2)
cen = (55, 0, 40)
a = scene(home, T, lift, extra, elev=12, azim=-28, center=cen, scale=5.0, title="On the desk: home screen")
b = scene(live, T, lift, extra, elev=6, azim=-8, center=cen, scale=6.2, title="Watching the Giants game")
c = scene(defense, T, lift, extra, elev=10, azim=18, center=cen, scale=5.4, title="Red zone pop-up: DEFENSE!")
Tf = RM(np.radians(0), [1, 0, 0])
lowf = base.vertices[:, 2].min()
d = scene(td, np.eye(4), -lowf, [desk()], elev=38, azim=-30, center=(55, 30, 10), scale=5.2, title="Off the stand on the patio table: touchdown!")
sheet = Image.new("RGB", (a.width * 2 + 24, a.height * 2 + 24), "white")
for i, im in enumerate([a, b, c, d]): sheet.paste(im, ((i % 2) * (a.width + 24), (i // 2) * (a.height + 24)))
sheet.save(sys.argv[1]); print("ok")
