import numpy as np
from PIL import Image
def rot(elev, azim):
    a, e = np.radians(azim), np.radians(elev)
    Rz = np.array([[np.cos(a), -np.sin(a), 0], [np.sin(a), np.cos(a), 0], [0, 0, 1]])
    Rx = np.array([[1, 0, 0], [0, np.cos(e), -np.sin(e)], [0, np.sin(e), np.cos(e)]])
    return Rx @ Rz
def render(items, elev=35, azim=-30, W=900, H=600, scale=5.0, center=(55, 30, -8), bg=(255, 255, 255)):
    """items: list of (trimesh, rgb 0..1). Orthographic; view from -y rotated."""
    R = rot(-90 + elev, azim)
    img = np.ones((H, W, 3)) * np.array(bg) / 255.0
    zb = np.full((H, W), -1e9)
    L = np.array([0.35, 0.55, 0.75]); L /= np.linalg.norm(L)
    for m, col in items:
        v = (m.vertices - center) @ R.T
        n = m.face_normals @ R.T
        X = v[:, 0] * scale + W / 2; Y = -v[:, 1] * scale + H / 2; Z = v[:, 2]
        for fi, (a, b, c) in enumerate(m.faces):
            if n[fi, 2] <= 0: continue
            xs = np.array([X[a], X[b], X[c]]); ys = np.array([Y[a], Y[b], Y[c]]); zs = np.array([Z[a], Z[b], Z[c]])
            x0, x1 = int(max(xs.min(), 0)), int(min(xs.max() + 1, W)); y0, y1 = int(max(ys.min(), 0)), int(min(ys.max() + 1, H))
            if x0 >= x1 or y0 >= y1: continue
            px, py = np.meshgrid(np.arange(x0, x1) + .5, np.arange(y0, y1) + .5)
            d = (ys[1] - ys[2]) * (xs[0] - xs[2]) + (xs[2] - xs[1]) * (ys[0] - ys[2])
            if abs(d) < 1e-9: continue
            w0 = ((ys[1] - ys[2]) * (px - xs[2]) + (xs[2] - xs[1]) * (py - ys[2])) / d
            w1 = ((ys[2] - ys[0]) * (px - xs[2]) + (xs[0] - xs[2]) * (py - ys[2])) / d
            w2 = 1 - w0 - w1
            inside = (w0 >= -1e-6) & (w1 >= -1e-6) & (w2 >= -1e-6)
            if not inside.any(): continue
            z = w0 * zs[0] + w1 * zs[1] + w2 * zs[2]
            sub = zb[y0:y1, x0:x1]
            upd = inside & (z > sub)
            sub[upd] = z[upd]
            sh = 0.35 + 0.65 * max(0.0, float(n[fi] @ L))
            img[y0:y1, x0:x1][upd] = np.clip(np.array(col) * sh, 0, 1)
    # outline: depth discontinuities
    dz = np.zeros_like(zb); dz[1:, :] = np.abs(np.diff(zb, axis=0)); dz[:, 1:] = np.maximum(dz[:, 1:], np.abs(np.diff(zb, axis=1)))
    img[(dz > 1.5) & (zb > -1e8)] *= 0.45
    return Image.fromarray((img * 255).astype(np.uint8))
