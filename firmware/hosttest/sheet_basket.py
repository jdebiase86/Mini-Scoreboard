"""Lays out the Basket Toss pictures (out/n_g25..g31) in design/mini_basket.png."""
import glob, os
from PIL import Image, ImageDraw, ImageFont
HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "../../design/mini_basket.png")
F = ImageFont.truetype(os.path.join(os.environ.get("FONTS", "/usr/share/fonts/opentype/inter"), "Inter-SemiBold.otf"), 20)
CAP = {
    "g25_basket_start": "Start screen",
    "g26_basket_rising": "She is thrown (the ring shows where she lands)",
    "g27_basket_apex": "At the top, spinning",
    "g28_basket_coming_down": "Coming down: slide to get under her",
    "g29_basket_caught": "Caught!",
    "g30_basket_later": "Later level: narrower arms, no landing ring",
    "g30b_basket_later_b": "Later level, another toss",
    "g30c_basket_later_c": "Later level, another toss",
    "g31_basket_over": "Three misses: game over",
}
files = [os.path.join(HERE, "out/n_%s.ppm" % k) for k in CAP if os.path.exists(os.path.join(HERE, "out/n_%s.ppm" % k))]
cols, W, H, pad, cap = 2, 480, 320, 24, 34
rows = (len(files) + cols - 1) // cols
sheet = Image.new("RGB", (pad + cols * (W + pad), pad + rows * (H + cap + pad)), (234, 236, 240))
d = ImageDraw.Draw(sheet)
for i, f in enumerate(files):
    name = os.path.splitext(os.path.basename(f))[0][2:]
    x = pad + (i % cols) * (W + pad); y = pad + (i // cols) * (H + cap + pad)
    d.text((x, y), f"{i + 1}. {CAP.get(name, name)}", font=F, fill=(30, 32, 38))
    sheet.paste(Image.open(f), (x, y + cap))
sheet.save(OUT)
print(OUT)
