#!/usr/bin/env python3
"""Extract the exact World 3-2 artwork from the supplied NES guide.

The guide is a 222x15 native-tile night stage. Static terrain/scenery is
cropped directly from it; animated blocks and coins are rebuilt from the
supplied tileset sheet in the same orange night palette.

    python3 tools/build_world32_assets.py
"""

from collections import deque
from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parent.parent
SOURCE = ROOT / "tools" / "source_art"
TEXTURES = ROOT / "assets" / "textures"

STAGE_PATH = SOURCE / "world3-2.png"
TILESET_PATH = SOURCE / "nes_tileset.png"

NIGHT_SKY = (0, 0, 0)
SHEET_BACKGROUNDS = {
    (0, 41, 140),
    (0, 0, 168),
    (146, 144, 255),
    (148, 148, 255),
}
WORLD32_PALETTE = {
    (230, 156, 33): (252, 152, 56),
    (156, 74, 0): (200, 76, 12),
    (82, 33, 0): (0, 0, 0),
}


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
            elif rgb in WORLD32_PALETTE:
                pixels[x, y] = (*WORLD32_PALETTE[rgb], 255)
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

    if stage.size != (222 * 16, 15 * 16):
        raise ValueError(f"World 3-2 source must be 3552x240, got {stage.size}")
    if tileset.size != (680, 776):
        raise ValueError(f"Tileset source must be 680x776, got {tileset.size}")

    assets = {
        "world32_ground.png": crop_tile(stage, 0, 13),
        "world32_hard_block.png": crop_tile(stage, 199, 5),
        "world32_brick.png": crop_tile(stage, 77, 5),
        "world32_pipe_top_left.png": crop_tile(stage, 169, 10),
        "world32_pipe_top_right.png": crop_tile(stage, 170, 10),
        "world32_pipe_body_left.png": crop_tile(stage, 169, 11),
        "world32_pipe_body_right.png": crop_tile(stage, 170, 11),
        "world32_tree_tall.png": crop_stage(
            stage, (13 * 16, 10 * 16 + 2, 14 * 16, 13 * 16), transparent=True),
        "world32_tree_short.png": crop_stage(
            stage, (11 * 16 + 1, 11 * 16 + 2, 12 * 16 - 1, 13 * 16),
            transparent=True),
        "world32_fence.png": crop_tile(stage, 38, 12, transparent=True),
        # Wide versions free cells where enemies stand in front of the fence.
        "world32_fence_group.png": crop_stage(
            stage, (14 * 16, 12 * 16, 18 * 16, 13 * 16), transparent=True),
        "world32_fence_pair_offset.png": crop_stage(
            stage, (133 * 16, 12 * 16, 136 * 16, 13 * 16), transparent=True),
        "world32_cloud_big.png": crop_stage(
            stage, (0, 3 * 16, 4 * 16, 5 * 16), transparent=True),
        "world32_cloud_small.png": crop_stage(
            stage, (18 * 16, 3 * 16, 21 * 16, 5 * 16), transparent=True),
        "world32_start_castle.png": crop_stage(
            stage, (0, 8 * 16, 5 * 16, 13 * 16), transparent=True),
        "world32_end_castle.png": crop_stage(
            stage, (213 * 16, 8 * 16, 218 * 16, 13 * 16), transparent=True),
        "world32_goal_pole.png": crop_stage(
            stage, (208 * 16 + 8, 2 * 16 + 8, 210 * 16, 13 * 16),
            transparent=True),
        "world32_question_block.png": build_strip(
            tileset, ((298, 78), (315, 78), (332, 78), (315, 78))),
        "world32_empty_block.png": build_strip(tileset, ((349, 78),)),
        "world32_coin.png": build_strip(
            tileset, ((298, 95), (315, 95), (332, 95), (315, 95))),
    }

    for name, image in assets.items():
        path = TEXTURES / name
        image.save(path)
        print(f"{path.relative_to(ROOT)}  {image.width}x{image.height}")


if __name__ == "__main__":
    main()
