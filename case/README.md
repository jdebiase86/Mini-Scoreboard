# Case with a built-in battery (draft)

Based on the MakerWorld enclosure for the ESP32-32E 4.0" display (base + lid,
M3x6 screws). The original files are not kept here (they belong to their
designer); download them from MakerWorld into this folder to rebuild.

deepen.py keeps everything at board level the same (standoffs, USB-C slot,
lid fit) and adds a section under the board for a flat LiPo:

    python3 deepen.py esp32-4inch-case-with-buttons_base.stl base_deep.stl 10 67 36   # depth, battery length, width
    python3 views2.py esp32-4inch-case_base.stl esp32-4inch-case_lid.stl base_deep.stl 10 case_draft1.png

Draft 1 (Oct 6, 2026): 10 mm extra depth for a ~2000 mAh 10 x 34 x 50 mm
battery; corner ribs on the floor hold it (plus foam tape); 8 x 9.5 mm
switch slot in the end wall opposite the USB-C. Base 18.65 mm, closed case
about 24 mm. The lid is unchanged. Board: 111.07 x 61.06 mm, PCB 1.61 mm,
screen 2.90 mm above the PCB; the case pocket is 111.1 x 61.15 mm.

Waiting on: battery plug photo (picks the battery, which sets the depth),
the exact switch (sets the slot), and a stand that fits the deeper case.
Needs: pip install trimesh manifold3d shapely scipy networkx

Buttons version (Oct 6): the designer's "with buttons" base has two flexing
tabs in the floor with posts that press RESET and BOOT on the back of the
board (labelled R and B outside). deepen.py works on it unchanged: the posts
grow by the same 10 mm. Use this one. The lid is the same in both versions.
Free floor space under the board is about 90 x 58 mm between the corner
posts and the button posts, so a bigger battery (about 50 x 60 mm, ~3000 mAh)
fits at the same depth.

Draft 2 (Oct 6): Joe picked a 10 mm thick ~3000 mAh battery, about 60 x 50 mm
(often sold as "105060"). Buttons version base, 10 mm deeper, battery guides
moved to a 62 x 52 pocket. Closed case about 24 mm thick. About 8 to 10 hours
with the screen bright.

Draft 3 (Oct 6): battery picked - JLJLUP LP103665, 3.7 V 3000 mAh, about
67 x 36 x 10 mm (seller drawing; listing says 65), protection circuit, JST 1.25 mm 2-pin plug already on it
(Amazon 4-pack, one per board plus a spare). Guides moved to a 69 x 38 pocket.
Before first plug-in: check the red wire lands on the "+" mark beside the
board's BAT plug (left "-", right "+" seen from the back, plug at the bottom).

Speaker option (Oct 7): python3 deepen.py <buttons base> base_spk.stl 10 67 36 66.5 speaker
puts a 20 x 30 mm cavity speaker pocket at the USB-C end with a grille of
1.8 mm holes in the back, and moves the battery 11 mm toward the switch end.

Oct 7: switch dropped. deepen.py no longer cuts the switch slot unless SWITCH=1.

Draft 4 (Oct 10): build_case.py builds the whole case from the designer's
"with buttons" base and lid (not kept here): base 10 mm deeper, speaker pocket
for a 25 x 35 mm speaker at the left end with a grille in the floor, battery
pocket moved to x = 71.5, the M3 heat-set insert holes (4.5 mm wide, 4.7 mm deep,
as the designer drew them) with the rest of each hole filled so an insert can't
sink, and an optional stylus holder drawn from scratch on the top long side, away
from any stand: a half-round groove in the wall (3.9 mm wide at the left end,
4.4 mm at the right, so the stylus wedges tight as it is pushed in) and three
small 8 mm loops (4.2 mm proud, 45-degree undersides, no supports); the stylus
shows at both ends. build_case.py writes base_plain.stl and base_final.stl (with the stylus loops). The stylus width comes from another design's groove
(4.2 mm); none of its shape is used. Closed case about 24 mm thick, 75.3 mm wide
with the loops. Print: base floor-down, lid face-down (flipped), PLA.
    python3 build_case.py <buttons base.stl> <lid.stl> <out folder>

Draft 5 (Oct 10, after the first print): the speaker pocket's corner ribs sat on top of the RESET / BOOT
fingers (the ribs started at x = 5.4 and the fingers' slits run to x = 12.5), so the buttons couldn't
move. The speaker pocket now starts at x = 14.4 (speaker centre 29.5) and the battery moved to x = 78
(it just fits between the speaker and the right wall: 67.6 mm for a 65 to 67 mm battery).

Draft 6 (Oct 10, after fitting the board): the real board's four screw holes are 103.68 / 103.99 mm apart
across and 53.36 / 53.54 mm up and down (centre to centre, metal inserts, no give), against 105.1 x 54.1
in the designer's files. The designer's holes are filled in and new ones cut so all four measured distances come out
exactly (bottom edge level, left edge upright, solved in build_case.py), centred on the old centre, in both base and lid (the lid keeps a thin collar around each
screw-head recess). The lid's screen pocket is 1.0 mm deeper (ceiling z 3.0 to 4.0, front lip 1.2 mm) so
the screen face sits flush. The stylus holder stays on the same side for now.
