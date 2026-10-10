"""Lays out the 0.5 screens (out/n_*.ppm) with captions: design/mini_stage3.png."""
import glob, os
from PIL import Image, ImageDraw, ImageFont
HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "../../design/mini_stage3.png")
F = ImageFont.truetype(os.path.join(os.environ.get("FONTS", "/usr/share/fonts/opentype/inter"), "Inter-SemiBold.otf"), 20)
CAP = {
    "d01_home": "Home: Wi-Fi button and battery",
    "d02_home_offline_low": "No Wi-Fi (red) and a low battery",
    "d03_game_live_battery": "Game screen: battery, charging bolt *",
    "d04_teams_live_nfl": "Tap the teams: live game *",
    "d05_teams_pre_nfl": "Tap the teams: before the game (real)",
    "d06_teams_final_nfl": "Tap the teams: final (real)",
    "d07_teams_final_nba": "Tap the teams: basketball final (real)",
    "d08_teams_pre_mlb": "Tap the teams: baseball, starters (real)",
    "d09_teams_final_nhl": "Tap the teams: hockey final (real)",
    "d10_teams_cfb": "Tap the teams: college (real)",
    "d11_play": "Tap the last-play card *",
    "d12_situation": "Tap the field: situation *",
    "d13_situation_red_zone": "Situation in the red zone *",
    "d14_teams_loading": "While the details load",
    "w01_wifi_list": "Wi-Fi list",
    "w02_wifi_saved_sheet": "Tap a saved network",
    "w03_keyboard_empty": "Tap a secured network: keyboard",
    "w04_keyboard_typed": "Typing (stars, SHOW to check)",
    "w05_keyboard_symbols": "Symbols page",
    "w06_setup_screen": "First setup screen, new button",
    "w07_joining": "Joining",
}
files = sorted(glob.glob(os.path.join(HERE, "out/n_*.ppm")))
cols, W, H, pad, cap = 3, 480, 320, 24, 34
rows = (len(files) + cols - 1) // cols
sheet = Image.new("RGB", (pad + cols * (W + pad), pad + rows * (H + cap + pad) + 30), (234, 236, 240))
d = ImageDraw.Draw(sheet)
for i, f in enumerate(files):
    name = os.path.splitext(os.path.basename(f))[0][2:]
    x = pad + (i % cols) * (W + pad); y = pad + (i // cols) * (H + cap + pad)
    d.text((x, y), f"{i + 1}. {CAP.get(name, name)}", font=F, fill=(30, 32, 38))
    sheet.paste(Image.open(f), (x, y + cap))
d.text((pad, sheet.height - 34), "* made-up live game (no real one on right now). Others use real ESPN data. Not tried on the board yet.", font=F, fill=(90, 94, 104))
sheet.save(OUT)
print(OUT)
