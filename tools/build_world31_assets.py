#!/usr/bin/env python3
"""Extract the exact World 3-1 artwork from the supplied NES reference sheets.

World 3-1 is the night stage: its sky is black, its pipes and trees are white
and its stone blocks are pink, so almost none of the reusable overworld tiles
fit it.  The stage guide is a 213x45 grid of native 16px tiles holding three
strips - the sky bonus (rows 0-14), the stage itself (rows 15-29) and the hidden
coin room (rows 30-44) - and every static tile below is cropped straight from
it so the palette agrees pixel-for-pixel.  Animated coins/question blocks come
from the tileset sheet, recoloured into the guide's palette.

    python3 tools/build_world31_assets.py
"""

from collections import deque
from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parent.parent
SOURCE = ROOT / "tools" / "source_art"
TEXTURES = ROOT / "assets" / "textures"

STAGE_PATH = SOURCE / "world3-1.png"
TILESET_PATH = SOURCE / "nes_tileset.png"
ENEMIES_PATH = SOURCE / "nes_enemies.png"

# Row the stage strip starts on inside the guide, in tiles.
STAGE_ROW = 15

NIGHT_SKY = (0, 0, 0)
SHEET_BACKGROUNDS = {
    (0, 41, 140),
    (0, 0, 168),
    (146, 144, 255),
    (148, 148, 255),
}

# Convert the tileset's colour samples into the palette the World 3-1 guide is
# drawn with. The teal entry is the hidden room's underground colour.
WORLD31_PALETTE = {
    (230, 156, 33): (252, 152, 56),
    (156, 74, 0): (200, 76, 12),
    (82, 33, 0): (0, 0, 0),
    (0, 123, 140): (0, 128, 136),
}


def clear_night_sky(image):
    """Make the black night sky around a sprite transparent.

    The stage art outlines itself in the same black as its sky, so only the
    black reachable from the crop's border is cleared; enclosed black - castle
    windows, pipe seams - stays opaque exactly as the guide draws it.
    """
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
    """Crop one 16px tile, addressed by its column and its row in the strip."""
    top = (STAGE_ROW + row) * 16
    return crop_stage(stage, (column * 16, top, (column + 1) * 16, top + 16),
                      transparent)


def crop_rows(stage, columns, rows, transparent=True):
    """Crop a whole object, addressed by tile ranges inside the strip."""
    first_column, last_column = columns
    first_row, last_row = rows
    return crop_stage(
        stage,
        (first_column * 16, (STAGE_ROW + first_row) * 16,
         (last_column + 1) * 16, (STAGE_ROW + last_row + 1) * 16),
        transparent,
    )


def key_colours(image, backgrounds, palette=None):
    """Make sheet backgrounds transparent and remap NES colours."""
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


# The thrower's hand shares the hammer's cell in the sheet and is cropped away.
THROWER_HAND = (255, 206, 197)


def build_sprite_strip(source, cells, size, drop=()):
    """Lay sprites of one size side by side, centred in equal cells."""
    strip = Image.new("RGBA", (size[0] * len(cells), size[1]), (0, 0, 0, 0))
    for index, box in enumerate(cells):
        sprite = key_colours(source.crop(box), SHEET_BACKGROUNDS | set(drop))
        strip.alpha_composite(
            sprite,
            (index * size[0] + (size[0] - sprite.width) // 2,
             (size[1] - sprite.height) // 2))
    return strip


def build_strip(source, positions):
    strip = Image.new("RGBA", (16 * len(positions), 16), (0, 0, 0, 0))
    for index, (x, y) in enumerate(positions):
        frame = source.crop((x, y, x + 16, y + 16))
        frame = key_colours(frame, SHEET_BACKGROUNDS, WORLD31_PALETTE)
        strip.alpha_composite(frame, (index * 16, 0))
    return strip


def main():
    stage = Image.open(STAGE_PATH).convert("RGBA")
    tileset = Image.open(TILESET_PATH).convert("RGBA")
    enemies = Image.open(ENEMIES_PATH).convert("RGBA")

    if stage.size != (213 * 16, 45 * 16):
        raise ValueError(f"World 3-1 source must be 3408x720, got {stage.size}")
    if tileset.size != (680, 776):
        raise ValueError(f"Tileset source must be 680x776, got {tileset.size}")
    if enemies.size != (436, 530):
        raise ValueError(f"Enemy source must be 436x530, got {enemies.size}")

    assets = {
        # Terrain. The night palette keeps the overworld shapes but swaps their
        # browns, so these cannot share the existing ground/brick artwork.
        "world31_ground.png": crop_tile(stage, 0, 13),
        "world31_brick.png": crop_tile(stage, 90, 5),
        "world31_hard_block.png": crop_tile(stage, 190, 5),
        "world31_bridge_deck.png": crop_tile(stage, 77, 9),
        "world31_bridge_rail.png": crop_tile(stage, 77, 8, transparent=True),
        # The two pools are drawn, never solid: falling in is a pit death.
        "world31_water.png": crop_tile(stage, 77, 13),
        "world31_water_surface.png": crop_tile(stage, 77, 12),
        # White pipes, and the hidden room's green one further below.
        "world31_pipe_top_left.png": crop_tile(stage, 38, 9),
        "world31_pipe_top_right.png": crop_tile(stage, 39, 9),
        "world31_pipe_body_left.png": crop_tile(stage, 38, 10),
        "world31_pipe_body_right.png": crop_tile(stage, 39, 10),
        # Scenery. The trees share the World 2-1 horsetail geometry, so they
        # drop straight into the 'T' and 't' markers.
        "world31_tree_tall.png": crop_stage(
            stage, (13 * 16, 402, 14 * 16, 448), transparent=True),
        "world31_tree_short.png": crop_stage(
            stage, (11 * 16 + 1, 418, 12 * 16 - 1, 448), transparent=True),
        "world31_fence.png": crop_tile(stage, 14, 12, transparent=True),
        # Keep the same transparent canvas sizes as the engine's other clouds.
        "world31_cloud_small.png": crop_rows(stage, (123, 125), (2, 3)),
        "world31_cloud_big.png": crop_rows(stage, (126, 129), (3, 4)),
        "world31_start_castle.png": crop_rows(stage, (0, 6), (2, 12)),
        "world31_end_castle.png": crop_rows(stage, (204, 208), (8, 12)),
        "world31_goal_pole.png": crop_stage(
            stage, (199 * 16 + 8, STAGE_ROW * 16 + 2 * 16 + 8,
                    201 * 16, STAGE_ROW * 16 + 13 * 16),
            transparent=True),
        # Hidden coin room, cropped from the guide's third strip.
        "world31_room_ground.png": crop_stage(
            stage, (32 * 16, 43 * 16, 33 * 16, 44 * 16)),
        "world31_room_brick.png": crop_stage(
            stage, (32 * 16, 32 * 16, 33 * 16, 33 * 16)),
        # The room's exit pipe and the shaft it climbs, as one piece of scenery
        # backed by invisible 'p' colliders in the map.
        "world31_room_pipe.png": crop_stage(
            stage, (45 * 16, 32 * 16, 48 * 16, 43 * 16), transparent=True),
        # Three source phases repeated into the familiar four-step ping-pong.
        "world31_question_block.png": build_strip(
            tileset, ((298, 78), (315, 78), (332, 78), (315, 78))),
        "world31_empty_block.png": build_strip(tileset, ((349, 78),)),
        "world31_coin.png": build_strip(
            tileset, ((298, 95), (315, 95), (332, 95), (315, 95))),
        "world31_room_coin.png": build_strip(
            tileset, ((397, 95), (414, 95), (431, 95), (414, 95))),
        # Hammer Bros first appear in this stage. Unlike the terrain they keep
        # the enemy sheet's own palette, which is the one the Goombas and
        # Koopas already in the game were cut from.
        "hammer_bro.png": build_sprite_strip(
            enemies, ((0, 182, 16, 206), (18, 182, 34, 206)), (16, 24)),
        "hammer.png": build_sprite_strip(
            enemies,
            ((74, 172, 86, 182), (96, 172, 104, 182),
             (114, 172, 122, 182), (132, 172, 140, 182)),
            (16, 16), drop=(THROWER_HAND,)),
    }

    for name, image in assets.items():
        path = TEXTURES / name
        image.save(path)
        print(f"{path.relative_to(ROOT)}  {image.width}x{image.height}")


if __name__ == "__main__":
    main()
