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
