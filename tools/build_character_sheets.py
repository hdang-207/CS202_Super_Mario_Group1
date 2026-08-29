#!/usr/bin/env python3
"""Cuts the player sprite sheets straight out of the NES source artwork.

Every frame the game draws comes from assets/character/nes_mario_luigi_source.png
and nothing else. Earlier versions of this script had to invent most of what
they wrote - the Small form was a squashed Super Mario, the crouch was a Super
Mario with compressed legs, and the Fire form was a 256px render resampled onto
the pixel grid - because the hand-made artwork it read had only four poses. The
NES sheet has them all, so none of that guesswork is left.

    python3 tools/build_character_sheets.py

Cells are copied whole, background keyed out, and pasted at the same offset they
sit at in the source. That is what keeps the poses aligned with each other: the
artist already drew every sprite standing on the bottom of its 16-wide cell, so
a Small Mario comes out exactly one tile tall, a Super Mario exactly two, and
the walk cycle no longer shifts sideways between frames.

Frame order in every sheet, matching entity::PlayerAnimator:

    idle, walk1, walk2, skid, jump, crouch, death, climb1, climb2, swim x6
"""

from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
ART = ROOT / "assets" / "character"
SOURCE = ART / "nes_mario_luigi_source.png"

# Top edge and height of each form's row of cells. The Fire row is drawn once
# and labelled "Fire Mario/Luigi" on the sheet: in SMB both characters share it.
ROWS = {
    "small": {"y": 8, "height": 16, "shared": False},
    "super": {"y": 32, "height": 32, "shared": False},
    "fire": {"y": 140, "height": 32, "shared": True},
}

CELL_WIDTH = 16
LUIGI_OFFSET_X = 288

# Left edge of each cell. The sheet groups them by leaving a four-pixel gap
# between poses that belong to different animations and two within one.
IDLE, WALK1, WALK2, SKID, JUMP = 20, 38, 56, 76, 96
# One cell serves two purposes across the rows: Small Mario cannot crouch, and
# Super Mario never dies on screen because he shrinks first, so each row draws
# whichever of the two poses it actually needs.
DUCK_OR_DEATH, CLIMB1, CLIMB2 = 116, 136, 154

# Where each row's six-frame swimming cycle begins. The Small row starts one
# cell earlier than the other two because it has no crouch to fit in first.
SWIM_X = {
    "small": (154, 174, 192, 210, 228, 246),
    "super": (174, 192, 210, 228, 246, 264),
    "fire": (174, 192, 210, 228, 246, 264),
}

# Lavender is the cell background; the two blues are the sheet's own panels.
BACKGROUNDS = {(146, 144, 255), (0, 41, 140), (0, 0, 168)}


def cut(source, form, x, character):
    """Returns one cell, background removed, still at its in-cell offset."""
    row = ROWS[form]
    left = x + (0 if row["shared"] else LUIGI_OFFSET_X * (character == "luigi"))
    cell = source.crop((left, row["y"], left + CELL_WIDTH, row["y"] + row["height"]))
    pixels = cell.load()
    for y in range(cell.height):
        for x_px in range(cell.width):
            if pixels[x_px, y][:3] in BACKGROUNDS:
                pixels[x_px, y] = (0, 0, 0, 0)
    return cell


def frames_for(source, character, form):
    """The fifteen cells of one sheet, in the order the animator reads them."""
    def cell(x):
        return cut(source, form, x, character)

    idle = cell(IDLE)
    jump = cell(JUMP)
    climb = cell(CLIMB1)

    if form == "small":
        # No crouch cell exists because Small Mario stands his ground; the row
        # spends it on the death pose instead, and climbs on one sprite plus
        # its mirror, exactly as the original game does.
        crouch, death = idle, cell(DUCK_OR_DEATH)
        climb2 = climb.transpose(Image.FLIP_LEFT_RIGHT)
    else:
        crouch, death = cell(DUCK_OR_DEATH), jump
        climb2 = cell(CLIMB2)

    return [idle, cell(WALK1), cell(WALK2), cell(SKID), jump,
            crouch, death, climb, climb2] + [cell(x) for x in SWIM_X[form]]


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
