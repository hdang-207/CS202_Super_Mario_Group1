#!/usr/bin/env python3
"""Cuts the player sprite sheets straight out of the NES source artwork.

Every frame the game draws comes from assets/character/nes_mario_luigi_source.png
and nothing else - no redrawing, no rescaling, no invented poses.

    python3 tools/build_character_sheets.py

The source lays each form out as a strip of 16-wide cells on a lavender panel,
with Luigi's copy of every strip 288px to the right of Mario's. Cells are copied
whole, the panel keyed out, and pasted at the offset the artist drew them at.
That is what keeps the poses aligned with one another: every sprite already
stands on the bottom of its panel, so a Small Mario comes out exactly one tile
tall, a Super Mario exactly two, and the walk cycle never shifts sideways.

Frame order in every sheet, matching entity::PlayerAnimator:

    idle, walk1, walk2, walk3, skid, jump, crouch, death, climb1, climb2, swim x6

The two climbing cells are also emitted as ``*_pole.png`` sheets so flagpole
sequences can use the original poses without depending on the full sheet's
frame indices.
"""

from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
ART = ROOT / "assets" / "character"
SOURCE = ART / "nes_mario_luigi_source.png"

CELL_WIDTH = 16
LUIGI_OFFSET_X = 288

# Left edge of every cell, in the order the source draws them. The sheet leaves
# a two-pixel gap between poses of one animation and four between animations,
# so the pitch is uneven and the positions have to be listed rather than
# stepped. Small Mario stops one cell short: he has no second climbing pose.
IDLE, WALK1, WALK2, WALK3, SKID, JUMP = 0, 20, 38, 56, 76, 96
# One cell serves two purposes across the rows: Small Mario cannot crouch, and
# Super Mario never dies on screen because he shrinks first, so each row draws
# whichever of the two poses it actually needs.
DUCK_OR_DEATH = 116
CLIMB1, CLIMB2 = 136, 154

# Top edge and height of each form's strip. The climbing and swimming cells sit
# one pixel higher than the rest of the row in the source art, so they get their
# own top edge; reading them off the standing edge is what used to slice the top
# row off every head. The Fire strip is drawn once and labelled
# "Fire Mario/Luigi": in SMB both characters share one white-and-red palette.
ROWS = {
    "small": {"y": 8, "climb_y": 8, "height": 16, "shared": False},
    "super": {"y": 32, "climb_y": 31, "height": 32, "shared": False},
    "fire": {"y": 140, "climb_y": 139, "height": 32, "shared": True},
}

# Where each row's six-frame swimming cycle begins. The Small row starts one
# cell earlier than the other two because it has no second climbing pose.
SWIM_X = {
    "small": (154, 174, 192, 210, 228, 246),
    "super": (174, 192, 210, 228, 246, 264),
    "fire": (174, 192, 210, 228, 246, 264),
}

# Lavender is the cell background; the two blues are the sheet's own panels,
# and show through above a crouch, whose panel is shorter than the row.
BACKGROUNDS = {(146, 144, 255), (0, 41, 140), (0, 0, 168)}


def cut(source, form, x, character):
    """Returns one cell, background removed, still at its in-cell offset."""
    row = ROWS[form]
    left = x + (0 if row["shared"] else LUIGI_OFFSET_X * (character == "luigi"))
    top = row["climb_y"] if x >= CLIMB1 else row["y"]
    cell = source.crop((left, top, left + CELL_WIDTH, top + row["height"]))
    pixels = cell.load()
    for y in range(cell.height):
        for x_px in range(cell.width):
            if pixels[x_px, y][:3] in BACKGROUNDS:
                pixels[x_px, y] = (0, 0, 0, 0)
    return cell


def frames_for(source, character, form):
    """The sixteen cells of one sheet, in the order the animator reads them."""
    def cell(x):
        return cut(source, form, x, character)

    jump = cell(JUMP)
    climb1 = cell(CLIMB1)

    if form == "small":
        # No crouch cell exists because Small Mario stands his ground; the row
        # spends it on the death pose instead, and climbs on one sprite plus
        # its mirror, exactly as the original game does.
        crouch, death = cell(IDLE), cell(DUCK_OR_DEATH)
        climb2 = climb1.transpose(Image.FLIP_LEFT_RIGHT)
    else:
        crouch, death = cell(DUCK_OR_DEATH), jump
        climb2 = cell(CLIMB2)

    return [cell(IDLE), cell(WALK1), cell(WALK2), cell(WALK3), cell(SKID), jump,
            crouch, death, climb1, climb2] + [cell(x) for x in SWIM_X[form]]


def build_sheet(frames):
    sheet = Image.new("RGBA", (CELL_WIDTH * len(frames), frames[0].height), (0, 0, 0, 0))
    for index, frame in enumerate(frames):
        sheet.paste(frame, (index * CELL_WIDTH, 0))
    return sheet


def main():
    source = Image.open(SOURCE).convert("RGBA")
    written = []

    for character in ("mario", "luigi"):
        for form in ROWS:
            frames = frames_for(source, character, form)
            path = ART / f"{character}_{form}.png"
            build_sheet(frames).save(path)
            written.append((path, len(frames)))

            pole_path = ART / f"{character}_{form}_pole.png"
            build_sheet(frames[8:10]).save(pole_path)
            written.append((pole_path, 2))

        # The menus show a single standing sprite rather than a sheet, so they
        # get their own tightly cropped copy with no empty cell around it.
        idle = cut(source, "super", IDLE, character)
        name = character.capitalize()
        for stem in (f"{name}_preview", f"{name}_idle"):
            path = ART / f"{stem}.png"
            idle.crop(idle.getbbox()).save(path)
            written.append((path, 1))

    for path, count in written:
        image = Image.open(path)
        print(f"{path.relative_to(ROOT)}  {image.width}x{image.height}"
              f"  ({count} frame{'s' if count > 1 else ''})")


if __name__ == "__main__":
    main()
