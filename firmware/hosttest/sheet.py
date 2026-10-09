"""Lays out the rendered screens (out/*.ppm) with captions: design/mini_stage1.png."""
import glob, os
from PIL import Image, ImageDraw, ImageFont
HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "../../design/mini_stage1.png")
F = ImageFont.truetype(os.path.join(os.environ.get("FONTS", "/usr/share/fonts/opentype/inter"), "Inter-SemiBold.otf"), 20)
CAP = {"01_splash": "Start-up", "02_setup": "First start: scan to set up", "03_joining": "Joining your Wi-Fi",
       "04_cant_join": "Wrong password / Wi-Fi down", "05_connected": "Connected (8 s, or tap)",
       "06_home": "Home: five teams + AUTO", "07_team": "Tap a team (game screen next stage)",
       "08_auto": "Tap AUTO", "09_updating": "Installing an update", "10_home_two": "Home with two teams",
       "11_home_eight_p1": "8 teams: first 5 + MORE", "12_home_eight_p2": "Tap MORE: the other 3 + AUTO",
       "13_no_teams": "No teams picked yet", "14_boot_hold": "Holding BOOT to reset Wi-Fi"}
import sys
PICK = len(sys.argv) > 1 and sys.argv[1] == "picker"
if PICK:
    OUT = os.path.join(HERE, "../../design/mini_picker_real.png")
    CAP = {"p1_home_edit": "Home: EDIT next to the clock", "p2_leagues": "Tap EDIT: leagues + DONE",
           "p3_nfl_page1": "Tap NFL: page 1 of 4", "p4_nfl_page3_jets": "Swipe up twice (or arrow), tap Jets",
           "p5_college": "BACK, tap COLLEGE", "p6_sec_lsu": "Tap SEC, tap LSU",
           "p7_leagues_after": "BACK twice: counts updated", "p8_home_after": "Tap DONE: 7 teams, MORE shows",
           "p9_full": "Already 10 picked, tap another"}
files = sorted(f for f in glob.glob(os.path.join(HERE, "out/*.ppm")) if os.path.basename(f).startswith("p") == PICK)
cols, W, H, pad, cap = 3, 480, 320, 24, 34
rows = (len(files) + cols - 1) // cols
sheet = Image.new("RGB", (pad + cols * (W + pad), pad + rows * (H + cap + pad)), (234, 236, 240))
d = ImageDraw.Draw(sheet)
for i, f in enumerate(files):
    name = os.path.splitext(os.path.basename(f))[0]
    x = pad + (i % cols) * (W + pad); y = pad + (i // cols) * (H + cap + pad)
    d.text((x, y), f"{i + 1}. {CAP.get(name, name)}", font=F, fill=(30, 32, 38))
    sheet.paste(Image.open(f), (x, y + cap))
sheet.save(OUT)
print(OUT)
