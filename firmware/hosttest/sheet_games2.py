"""Lays out the 0.5 screens (out/n_*.ppm) with captions: design/mini_stage5.png."""
import glob, os
from PIL import Image, ImageDraw, ImageFont
HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "../../design/mini_stage5.png")
ONLY = os.environ.get("ONLY", "")
F = ImageFont.truetype(os.path.join(os.environ.get("FONTS", "/usr/share/fonts/opentype/inter"), "Inter-SemiBold.otf"), 20)
CAP = {
    "h01_home_with_games_button": "Home: the GAMES button",
    "g01_menu_page1": "Games menu, page 1",
    "g02_menu_page2": "Games menu, page 2 (swipe)",
    "g03_kick_aim": "Penalty kick: flick the ball",
    "g04_kick_flight": "Penalty kick: the ball is on its way",
    "g05_kick_result": "Penalty kick: result",
    "g06_kick_round_over": "Penalty kick: five shots, round over",
    "g07_free_throw_aim": "Free throw: flick",
    "g08_free_throw_flight": "Free throw: the shot",
    "g09_free_throw_result": "Free throw: result",
    "g10_memory_level1": "Logo match, level 1",
    "g11_memory_two_flipped": "Logo match: two flipped",
    "g12_sudoku_menu": "Sudoku: pick a level",
    "g13_sudoku_play": "Sudoku: playing",
    "g14_snake_start": "Snake: start",
    "g15_snake_playing": "Snake: playing",
    "g16_2048": "2048",
    "g17_simon": "Cheer Simon",
    "g18_connect4": "Connect Four against the mini",
    "g19_trivia_question": "Sports trivia: a question",
    "g20_trivia_answered": "Trivia: answered",
    "g21_reaction_go": "Reaction: tap now!",
    "g22_reaction_result": "Reaction: your time",
    "g23_die": "Roll a die",
    "g24_coin": "Flip a coin",
}
files = sorted(glob.glob(os.path.join(HERE, "out/n_*.ppm")))
if ONLY: files = [f for f in files if os.path.basename(f)[2:].startswith(ONLY)]
if ONLY: OUT = os.path.join(HERE, "../../design/mini_stage5.png")
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
