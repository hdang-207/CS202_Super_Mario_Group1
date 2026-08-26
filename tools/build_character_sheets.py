#!/usr/bin/env python3
"""Builds the normalised player sprite sheets used by Player::PlayerAnimator.

The hand-made artwork in assets/character/ cannot be drawn directly:

  * every pose is cropped to its own bounding box, so a sprite scaled to a
    fixed height changes size and shifts sideways on every animation frame;
  * the Fire artwork is a 256px-tall render, roughly eight times the
    resolution of the rest of the game, so it can only be shown at a
    fractional zoom - which is what makes it look blurry next to the tiles;
  * there is no Small (16x16) artwork at all, so the Small form had to borrow
    the Super sprite and ended up about 40% too tall.

This script fixes all three offline. It writes one sheet per character and
form, every frame in a cell of the same size, the character bottom-aligned in
its cell and centred on its head, so the game can draw it at the project's
whole-number zoom with no per-frame scaling.

    python3 tools/build_character_sheets.py

Frame order in every sheet: idle, walk1, walk2, jump, crouch, then the six
authentic NES swimming frames.
"""

from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
ART = ROOT / "assets" / "character"
NES_CHARACTER_SHEET = ART / "nes_mario_luigi_source.png"

# Source artwork per character and form, in frame order (crouch is synthesised).
POSES = ("idle", "run1", "run2", "jump")
SOURCES = {
    ("mario", "super"): ["Mario_idle", "Mario_run1", "Mario_run2", "Mario_jump"],
    ("mario", "fire"): ["FireMarioIdle", "FireMarioRun1", "FireMarioRun2", "FireMarioJump"],
    ("luigi", "super"): ["Luigi_idle", "Luigi_run1", "Luigi_run2", "Luigi_jump"],
    ("luigi", "fire"): ["FireLuigiIdle", "FireLuigiRun1", "FireLuigiRun2", "FireLuigiJump"],
}

CELL_HEIGHT = {"small": 16, "super": 32, "fire": 32}
# How tall the character itself is drawn inside its cell. Super Mario is 32px
# in the original game but this artwork has slightly shorter proportions, so
# the pixel art keeps its native height and only the Fire render is resampled.
FIRE_CONTENT_HEIGHT = 30
ALPHA_CUTOFF = 110

# Coordinates in the user-supplied NES Mario & Luigi sheet. The Small row has
# no crouch pose, so its six-frame swimming cycle starts one cell earlier than
# the Big and Fire rows. Luigi's half of the sheet is offset by 288 pixels.
SWIM_X = {
    "small": (154, 174, 192, 210, 228, 246),
    "super": (174, 192, 210, 228, 246, 264),
    "fire": (174, 192, 210, 228, 246, 264),
}
SWIM_Y = {"small": 8, "super": 31, "fire": 140}
SWIM_HEIGHT = {"small": 16, "super": 32, "fire": 32}
SWIM_FRAME_COUNT = 6
FRAME_COUNT = len(POSES) + 1 + SWIM_FRAME_COUNT
SHEET_CHARACTER_X = {"mario": 0, "luigi": 288}
SHEET_BACKGROUNDS = {
    (146, 144, 255),  # lavender cell background
    (0, 41, 140),     # blue separator at the bottom of the Fire row
}


def load(name):
    image = Image.open(ART / f"{name}.png").convert("RGBA")
    return image.crop(image.getbbox())


def load_swim_frames(character, form):
    """Extracts one exact six-frame swim cycle from the NES source sheet."""
    source = Image.open(NES_CHARACTER_SHEET).convert("RGBA")
    # Fire Mario and Fire Luigi share the same artwork in SMB; the active
    # player palette decides which character it represents.
    character_offset = 0 if form == "fire" else SHEET_CHARACTER_X[character]
    y = SWIM_Y[form]
    height = SWIM_HEIGHT[form]
    frames = []
    for x in SWIM_X[form]:
        frame = source.crop((x + character_offset, y,
                             x + character_offset + 16, y + height))
        pixels = frame.load()
        for py in range(frame.height):
            for px in range(frame.width):
                if pixels[px, py][:3] in SHEET_BACKGROUNDS:
                    pixels[px, py] = (0, 0, 0, 0)
        frames.append(frame.crop(frame.getbbox()))
    return frames


def palette_of(image):
    """Every distinct opaque colour, so resampled frames can be snapped back."""
    colours = []
    seen = set()
    pixels = image.load()
    for y in range(image.height):
        for x in range(image.width):
            pixel = pixels[x, y]
            if pixel[3] > 0 and pixel[:3] not in seen:
                seen.add(pixel[:3])
                colours.append(pixel[:3])
    return colours


def snap(image, palette):
    """Rounds a resampled frame back onto the artwork's own colours.

    Resampling introduces thousands of in-between colours and soft edges;
    snapping keeps the result readable as pixel art instead of a smudge.
    """
    out = Image.new("RGBA", image.size, (0, 0, 0, 0))
    src, dst = image.load(), out.load()
    for y in range(image.height):
        for x in range(image.width):
            colour = src[x, y]
            if colour[3] < ALPHA_CUTOFF:
                continue
            nearest = min(palette, key=lambda c: sum((a - b) ** 2 for a, b in zip(c, colour[:3])))
            dst[x, y] = (*nearest, 255)
    return out


def to_pixel_art(image, height, colours=16):
    """Brings the high-resolution Fire render down onto the game's pixel grid."""
    width = max(1, round(image.width * height / image.height))
    small = image.resize((width, height), Image.LANCZOS)
    rgb = small.convert("RGB").quantize(colors=colours, method=Image.MEDIANCUT).convert("RGB")
    rgb.putalpha(small.split()[3].point(lambda v: 255 if v >= ALPHA_CUTOFF else 0))
    return rgb.crop(rgb.getbbox())


def make_small(image):
    """Derives the 16x16 Small form from a Super frame.

    Small Mario is not a shrunk Super Mario - he keeps a full-size head on a
    stubby body. Splitting the sprite in half and compressing each part by a
    different amount reproduces that, and keeps the face readable.
    """
    palette = palette_of(image)
    split = image.height // 2
    head = image.crop((0, 0, image.width, split)).resize((image.width, 9), Image.LANCZOS)
    body = image.crop((0, split, image.width, image.height)).resize((image.width, 7), Image.LANCZOS)
    merged = Image.new("RGBA", (image.width, 16), (0, 0, 0, 0))
    merged.paste(head, (0, 0))
    merged.paste(body, (0, 9))
    return snap(merged, palette)


def make_crouch(image):
    """Synthesises the ducking pose by squashing the legs under an intact head."""
    palette = palette_of(image)
    split = image.height // 2
    legs_height = max(1, round(image.height * 0.28))
    head = image.crop((0, 0, image.width, split))
    legs = image.crop((0, split, image.width, image.height)).resize(
        (image.width, legs_height), Image.LANCZOS)
    merged = Image.new("RGBA", (image.width, split + legs_height), (0, 0, 0, 0))
    merged.paste(head, (0, 0))
    merged.paste(legs, (0, split))
    return snap(merged, palette)


def head_centre(image):
    """Horizontal centre of the top third of the sprite.

    The head is the one part that stays put across every pose, so aligning on
    it keeps the walk cycle from wobbling sideways. Aligning on the feet or on
    the bounding box would drift, and on the Fire frames the outstretched gun
    would drag the whole body off-centre.
    """
    pixels = image.load()
    rows = max(1, image.height // 3)
    left, right = image.width, 0
    for y in range(rows):
        for x in range(image.width):
            if pixels[x, y][3] >= ALPHA_CUTOFF:
                left, right = min(left, x), max(right, x)
    if left > right:
        return image.width / 2.0
    return (left + right + 1) / 2.0


def build_sheet(frames, cell_height):
    cell_width = max(f.width for f in frames) + 2
    cell_width += cell_width % 2
    sheet = Image.new("RGBA", (cell_width * len(frames), cell_height), (0, 0, 0, 0))
    for index, frame in enumerate(frames):
        x = index * cell_width + round(cell_width / 2.0 - head_centre(frame))
        sheet.paste(frame, (x, cell_height - frame.height), frame)
    return sheet


def main():
    written = []
    for character in ("mario", "luigi"):
        super_frames = [load(name) for name in SOURCES[(character, "super")]]
        fire_frames = [to_pixel_art(load(name), FIRE_CONTENT_HEIGHT)
                       for name in SOURCES[(character, "fire")]]
        forms = {
            "small": [make_small(f) for f in super_frames],
            "super": list(super_frames),
            "fire": list(fire_frames),
        }
        for form, frames in forms.items():
            frames.append(make_crouch(frames[0]))
            frames.extend(load_swim_frames(character, form))
            path = ART / f"{character}_{form}.png"
            build_sheet(frames, CELL_HEIGHT[form]).save(path)
            written.append(path)

    for path in written:
        image = Image.open(path)
        print(f"{path.relative_to(ROOT)}  {image.width}x{image.height} "
              f"({image.width // FRAME_COUNT}x{image.height} per frame)")


if __name__ == "__main__":
    main()
