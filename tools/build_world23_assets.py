#!/usr/bin/env python3
"""Extract the exact World 2-3 artwork from the supplied NES reference sheets.

The stage guide is a 237x15 grid of native 16px tiles.  Static scenery and
terrain are cropped from that guide so their palette agrees pixel-for-pixel.
Animated coins/question blocks and the red flying Cheep-Cheep are taken from
the corresponding tileset and enemy sheets.

    python3 tools/build_world23_assets.py
"""

from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parent.parent
SOURCE = ROOT / "tools" / "source_art"
TEXTURES = ROOT / "assets" / "textures"

STAGE_PATH = SOURCE / "world2-3.png"
TILESET_PATH = SOURCE / "nes_tileset.png"
ENEMIES_PATH = SOURCE / "nes_enemies.png"

SKY = (92, 148, 252)
SHEET_BACKGROUNDS = {
    (0, 41, 140),
    (0, 0, 168),
    (146, 144, 255),
    (148, 148, 255),
}

# Convert the colour samples beside the reference tiles into the palette used
# by the supplied World 2-3 guide image.
WORLD23_PALETTE = {
    (230, 156, 33): (252, 152, 56),
    (156, 74, 0): (200, 76, 12),
    (82, 33, 0): (0, 0, 0),
}


def key_colours(image, backgrounds, palette=None):
    """Make sheet backgrounds transparent and optionally remap NES colours."""
    palette = palette or {}
    rgba = image.convert("RGBA")
    pixels = rgba.load()
    for y in range(rgba.height):
        for x in range(rgba.width):
            rgb = pixels[x, y][:3]
            if rgb in backgrounds:
                pixels[x, y] = (0, 0, 0, 0)
            elif rgb in palette:
                pixels[x, y] = (*palette[rgb], 255)
    return rgba


def crop_stage(stage, box, transparent=False):
    crop = stage.crop(box)
    return key_colours(crop, {SKY}) if transparent else crop


def crop_tile(stage, column, row, transparent=False):
    return crop_stage(
        stage,
        (column * 16, row * 16, (column + 1) * 16, (row + 1) * 16),
        transparent,
    )


def build_strip(source, positions):
    strip = Image.new("RGBA", (16 * len(positions), 16), (0, 0, 0, 0))
    for index, (x, y) in enumerate(positions):
        frame = source.crop((x, y, x + 16, y + 16))
        frame = key_colours(frame, SHEET_BACKGROUNDS, WORLD23_PALETTE)
        strip.alpha_composite(frame, (index * 16, 0))
    return strip


def main():
    stage = Image.open(STAGE_PATH).convert("RGBA")
    tileset = Image.open(TILESET_PATH).convert("RGBA")
    enemies = Image.open(ENEMIES_PATH).convert("RGBA")

    if stage.size != (237 * 16, 15 * 16):
        raise ValueError(f"World 2-3 source must be 3792x240, got {stage.size}")
    if tileset.size != (680, 776):
        raise ValueError(f"Tileset source must be 680x776, got {tileset.size}")
    if enemies.size != (436, 530):
        raise ValueError(f"Enemy source must be 436x530, got {enemies.size}")

    assets = {
        "world23_ground.png": crop_tile(stage, 0, 13),
        "world23_hard_block.png": crop_tile(stage, 99, 9),
        "world23_bridge_rail.png": crop_tile(stage, 15, 9, transparent=True),
        "world23_bridge_deck.png": crop_tile(stage, 15, 10, transparent=True),
        "world23_island_left.png": crop_tile(stage, 8, 13),
        "world23_island_middle.png": crop_tile(stage, 9, 13),
        "world23_island_right.png": crop_tile(stage, 15, 13),
        "world23_island_trunk.png": crop_tile(stage, 9, 14),
        # Keep the same transparent canvas sizes as the engine's other clouds.
        "world23_cloud_big.png": crop_stage(
            stage, (18 * 16, 2 * 16, 22 * 16, 4 * 16), transparent=True),
        "world23_cloud_small.png": crop_stage(
            stage, (38 * 16, 6 * 16, 41 * 16, 8 * 16), transparent=True),
        "world23_start_castle.png": crop_stage(
            stage, (0, 8 * 16, 5 * 16, 13 * 16), transparent=True),
        "world23_end_castle.png": crop_stage(
            # Include the last cloud tip that peeks out from behind its left
            # wall; the guide intentionally layers both into one exact crop.
            stage, (227 * 16, 2 * 16, 237 * 16, 13 * 16), transparent=True),
        "world23_goal_pole.png": crop_stage(
            stage, (224 * 16 + 8, 2 * 16 + 8, 226 * 16, 13 * 16),
            transparent=True),
        # The source supplies three palette phases; the middle phase is repeated
        # to form the familiar four-step ping-pong animation.
        "world23_coin.png": build_strip(
            tileset, ((298, 95), (315, 95), (332, 95), (315, 95))),
        "world23_question_block.png": build_strip(
            tileset, ((298, 78), (315, 78), (332, 78), (315, 78))),
        "world23_empty_block.png": build_strip(tileset, ((349, 78),)),
        "flying_cheep_cheep.png": build_strip(
            enemies, ((0, 370), (18, 370))),
    }

    for name, image in assets.items():
        path = TEXTURES / name
        image.save(path)
        print(f"{path.relative_to(ROOT)}  {image.width}x{image.height}")


if __name__ == "__main__":
    main()
