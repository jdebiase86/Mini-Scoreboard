"""Lays out the 0.10 screens (out/p_*.ppm) with captions: design/mini_stage7.png."""
import glob, os
from PIL import Image, ImageDraw, ImageFont
HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "../../design/mini_stage7.png")
F = ImageFont.truetype(os.path.join(os.environ.get("FONTS", "/usr/share/fonts/opentype/inter"), "Inter-SemiBold.otf"), 20)
CAP = {
    "p01_upcoming_football": "Upcoming game (football)",
    "p02_upcoming_baseball_auto": "Upcoming game, baseball, Auto tag",
    "p03_upcoming_soon": "Starting soon: countdown",
    "p04_upcoming_soon_30min_later": "30 minutes later (middle redraws only)",
    "p05_upcoming_college": "Upcoming game (college)",
    "p06_bye_week_football": "Bye week (football)",
    "p07_no_game_basketball": "No game (other sports)",
    "p08_home_with_bye_tiles": "Home with the bye week / off season tiles",
    "p09_about_status": "About page (tap the battery)",
    "p10_about_before_restart": "About: BEFORE RESTART (made-up example)",
    "p11_home_page2": "More teams page: back button clear of the clock",
    "p12_home_no_wifi": "No Wi-Fi: says so where the time is",
}
files = sorted(glob.glob(os.path.join(HERE, "out/p_*.ppm")))
cols, W, H, pad, cap = 3, 480, 320, 24, 34
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
