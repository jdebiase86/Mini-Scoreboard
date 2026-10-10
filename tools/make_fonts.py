"""Turns the Inter font (the one the mock-ups use) into smooth-edged screen
fonts for the mini: firmware/mini/mn_fonts.h, in the "VLW" format
LovyanGFX draws with anti-aliasing. Letters, digits and punctuation only.

    python3 tools/make_fonts.py            (FONTS= folder holding Inter-Bold.otf etc.)
"""
import os, struct
from PIL import Image, ImageDraw, ImageFont

FONTS = os.environ.get("FONTS", "/usr/share/fonts/opentype/inter")
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "../firmware/mini/mn_fonts.h")

# name in the code, Inter file, pixel size
WANT = [
    ("B12", "Inter-Bold.otf", 12),
    ("B16", "Inter-Bold.otf", 16),
    ("B18", "Inter-Bold.otf", 18),
    ("B24", "Inter-Bold.otf", 24),
    ("B36", "Inter-Bold.otf", 36),
    ("S13", "Inter-SemiBold.otf", 13),
    ("M12", "Inter-Medium.otf", 12),
    ("M15", "Inter-Medium.otf", 15),
    ("B62", "Inter-Bold.otf", 62, "0123456789- "),   # the big scores: digits only
]
CHARS = [chr(c) for c in range(32, 127)]


def vlw(path, size, chars=None):
    f = ImageFont.truetype(path, size)
    ascent, descent = f.getmetrics()
    glyphs, bitmaps = [], []
    for ch in sorted(set(chars or CHARS)):   # the loader looks glyphs up in sorted order
        adv = round(f.getlength(ch))
        pad = size
        im = Image.new("L", (size * 3, size * 3), 0)
        ImageDraw.Draw(im).text((pad, pad + ascent), ch, font=f, fill=255, anchor="ls")
        bb = im.getbbox()
        if not bb:                       # space: one clear dot
            glyphs.append((ord(ch), 1, 1, adv, 0, 0))
            bitmaps.append(b"\0")
            continue
        x0, y0, x1, y1 = bb
        g = im.crop(bb)
        top = (pad + ascent) - y0        # baseline to the glyph's top
        left = x0 - pad
        glyphs.append((ord(ch), y1 - y0, x1 - x0, adv, top, left))
        bitmaps.append(g.tobytes())
    out = struct.pack(">6i", len(glyphs), 11, size, 0, ascent, descent)
    for (u, h, w, adv, top, left) in glyphs:
        out += struct.pack(">7i", u, h, w, adv, top, left, 0)
    for b in bitmaps:
        out += b
    return out


def main():
    lines = ["// Made by tools/make_fonts.py from Inter (SIL Open Font License). Don't edit by hand.",
             "#pragma once", "#include <stdint.h>", ""]
    total = 0
    for name, file, size, *extra in WANT:
        data = vlw(os.path.join(FONTS, file), size, extra[0] if extra else None)
        total += len(data)
        lines.append(f"// {file} {size} px")
        lines.append(f"static const uint8_t FONT_{name}[{len(data)}] = {{")
        for i in range(0, len(data), 24):
            lines.append("  " + ",".join(str(b) for b in data[i:i + 24]) + ",")
        lines.append("};")
        lines.append("")
    open(OUT, "w").write("\n".join(lines))
    print(f"{OUT}: {len(WANT)} fonts, {total // 1024} KB")


main()
