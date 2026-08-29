#!/usr/bin/env python3
"""Extract the exact World 3-3 artwork from the supplied NES guide.

The guide is a 163x15 native-tile night stage: a starting ledge, a long
bottomless pit spanned by green-topped brick pillars and orange lifts, and
the flagpole in front of the big castle. Static terrain, scenery and the
lift/pulley hardware are cropped straight from it; the animated blocks and
coins are rebuilt from the tileset sheet in the same orange night palette
World 3-2 uses.

    python3 tools/build_world33_assets.py
"""

from collections import deque
from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parent.parent
SOURCE = ROOT / "tools" / "source_art"
TEXTURES = ROOT / "assets" / "textures"

STAGE_PATH = SOURCE / "world3-3.png"
TILESET_PATH = SOURCE / "nes_tileset.png"

NIGHT_SKY = (0, 0, 0)
SHEET_BACKGROUNDS = {
    (0, 41, 140),
    (0, 0, 168),
    (146, 144, 255),
    (148, 148, 255),
}
NIGHT_PALETTE = {
    (230, 156, 33): (252, 152, 56),
    (156, 74, 0): (200, 76, 12),
    (82, 33, 0): (0, 0, 0),
}


def clear_black(image):
    """Clear every black pixel: the lift bar's holes look through to whatever
    the bar is passing in front of."""
    rgba = image.convert("RGBA")
    pixels = rgba.load()
    for y in range(rgba.height):
        for x in range(rgba.width):
            if pixels[x, y][:3] == NIGHT_SKY:
                pixels[x, y] = (0, 0, 0, 0)
    return rgba


def clear_night_sky(image):
    """Clear only border-connected black, preserving black sprite outlines."""
    rgba = image.convert("RGBA")
    pixels = rgba.load()
    queue = deque()
    seen = set()

    for x in range(rgba.width):
        queue.extend(((x, 0), (x, rgba.height - 1)))
    for y in range(rgba.height):
        queue.extend(((0, y), (rgba.width - 1, y)))

    while queue:
        x, y = queue.popleft()
        if (x, y) in seen or not (0 <= x < rgba.width and 0 <= y < rgba.height):
            continue
        seen.add((x, y))
        if pixels[x, y][:3] != NIGHT_SKY:
            continue
        pixels[x, y] = (0, 0, 0, 0)
        queue.extend(((x - 1, y), (x + 1, y), (x, y - 1), (x, y + 1)))

    return rgba


def crop_stage(stage, box, transparent=False):
    crop = stage.crop(box)
    return clear_night_sky(crop) if transparent else crop


def crop_tile(stage, column, row, transparent=False):
    return crop_stage(
        stage,
        (column * 16, row * 16, (column + 1) * 16, (row + 1) * 16),
        transparent,
    )


def key_colours(image):
    rgba = image.convert("RGBA")
    pixels = rgba.load()
    for y in range(rgba.height):
        for x in range(rgba.width):
            rgb = pixels[x, y][:3]
            if rgb in SHEET_BACKGROUNDS:
                pixels[x, y] = (0, 0, 0, 0)
            elif rgb in NIGHT_PALETTE:
                pixels[x, y] = (*NIGHT_PALETTE[rgb], 255)
    return rgba


def build_strip(source, positions):
    strip = Image.new("RGBA", (16 * len(positions), 16), (0, 0, 0, 0))
    for index, (x, y) in enumerate(positions):
        frame = key_colours(source.crop((x, y, x + 16, y + 16)))
        strip.alpha_composite(frame, (index * 16, 0))
    return strip


def main():
    stage = Image.open(STAGE_PATH).convert("RGBA")
    tileset = Image.open(TILESET_PATH).convert("RGBA")

    if stage.size != (163 * 16, 15 * 16):
        raise ValueError(f"World 3-3 source must be 2608x240, got {stage.size}")
    if tileset.size != (680, 776):
        raise ValueError(f"Tileset source must be 680x776, got {tileset.size}")

    assets = {
        "world33_ground.png": crop_tile(stage, 0, 13),
        # Every pillar cell in the guide is the same tile, so one crop covers
        # trunks one column wide and the ten-column plateaus alike.
        "world33_pillar.png": crop_tile(stage, 56, 4),
        "world33_platform_left.png": crop_tile(stage, 55, 3),
        "world33_platform_middle.png": crop_tile(stage, 56, 3),
        "world33_platform_right.png": crop_tile(stage, 58, 3),
        "world33_cloud_big.png": crop_stage(
            stage, (18 * 16, 2 * 16, 22 * 16, 4 * 16), transparent=True),
        "world33_cloud_small.png": crop_stage(
            stage, (9 * 16, 7 * 16, 12 * 16, 9 * 16), transparent=True),
        "world33_start_castle.png": crop_stage(
            stage, (0, 8 * 16, 5 * 16, 13 * 16), transparent=True),
        "world33_end_castle.png": crop_stage(
            stage, (153 * 16, 2 * 16, 163 * 16, 13 * 16), transparent=True),
        "world33_goal_pole.png": crop_stage(
            stage, (150 * 16 + 8, 2 * 16 + 8, 152 * 16, 13 * 16),
            transparent=True),
        # The bar is three tiles wide and eight pixels tall, exactly the size
        # the lift physics already assumes.
        "world33_lift.png": clear_black(stage.crop(
            (30 * 16, 4 * 16 + 1, 33 * 16, 4 * 16 + 9))),
        # Pulley headers: the two wheels plus the rope strung between them. The
        # ropes hanging below them change length while the lifts swing, so the
        # game draws those itself.
        "world33_pulley_wide.png": crop_stage(
            stage, (82 * 16, 2 * 16, 90 * 16, 3 * 16), transparent=True),
        "world33_pulley_short.png": crop_stage(
            stage, (137 * 16, 2 * 16, 142 * 16, 3 * 16), transparent=True),
        "world33_question_block.png": build_strip(
            tileset, ((298, 78), (315, 78), (332, 78), (315, 78))),
        "world33_empty_block.png": build_strip(tileset, ((349, 78),)),
        "world33_coin.png": build_strip(
            tileset, ((298, 95), (315, 95), (332, 95), (315, 95))),
    }

    for name, image in assets.items():
        path = TEXTURES / name
        image.save(path)
        print(f"{path.relative_to(ROOT)}  {image.width}x{image.height}")


if __name__ == "__main__":
    main()
