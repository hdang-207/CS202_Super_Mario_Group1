#!/usr/bin/env python3
"""Extract the castle-course artwork from the supplied NES reference sheets.

Worlds 1-4, 2-4 and 3-4 are drawn from the same castle palette: their walls,
lava, bridge, Fire-Bar pivots and axe are pixel-identical, so all three stages
share one set of ``castle_*`` textures cut here. Only the pieces a single stage
needs - World 2-4's vertical elevator, the brick it and World 3-4 build with,
and the three enemies each stage's fake Bowser turns back into - come from just
one of the guides. World 3-4 asks for nothing the first two have not already
cut, so its guide is not opened here at all.

Both level guides are native 160x15 tile images. Animated blocks, coins and the
sprite work come from the matching tileset and enemy sheets.

    python3 tools/build_castle_assets.py
"""

from collections import deque
from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parent.parent
SOURCE = ROOT / "tools" / "source_art"
TEXTURES = ROOT / "assets" / "textures"

WORLD14_PATH = SOURCE / "world1-4.png"
WORLD24_PATH = SOURCE / "world2-4.png"
TILESET_PATH = SOURCE / "nes_tileset.png"
ENEMIES_PATH = SOURCE / "nes_enemies.png"

NIGHT_SKY = (0, 0, 0)
SHEET_BACKGROUNDS = {
    (0, 41, 140),
    (0, 0, 168),
    (146, 144, 255),
    (148, 148, 255),
}

# The tileset sheet is dumped from a different emulator's palette than the
# stage guides, so its blocks and coins have to be converted before they can sit
# next to castle stone cut straight out of a guide.
CASTLE_PALETTE = {
    (230, 156, 33): (252, 152, 56),
    (156, 74, 0): (200, 76, 12),
    (99, 99, 99): (116, 116, 116),
    (82, 33, 0): (0, 0, 0),
}

# Both guides draw a lift as an 8px girder whose pattern repeats every 8px.
LIFT_PERIOD = 8


def clear_border_black(image):
    """Clear border-connected castle sky while preserving enclosed outlines."""
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


def key_colours(image, palette=None):
    """Make the sheet background transparent and optionally remap NES colours."""
    palette = palette or {}
    rgba = image.convert("RGBA")
    pixels = rgba.load()
    for y in range(rgba.height):
        for x in range(rgba.width):
            rgb = pixels[x, y][:3]
            if rgb in SHEET_BACKGROUNDS:
                pixels[x, y] = (0, 0, 0, 0)
            elif rgb in palette:
                pixels[x, y] = (*palette[rgb], 255)
    return rgba


def crop_tile(stage, column, row):
    return stage.crop(
        (column * 16, row * 16, (column + 1) * 16, (row + 1) * 16)
    )


def repair_lift(image, columns):
    """Paint the guide's travel-path annotation back out of an elevator.

    World 2-4 draws each elevator's route as a white line behind the platform,
    which shows through the girder's holes. The girder repeats every eight
    pixels, so the intact segment one period to the left restores them.
    """
    rgba = image.convert("RGBA")
    pixels = rgba.load()
    for x in columns:
        for y in range(rgba.height):
            pixels[x, y] = pixels[x - LIFT_PERIOD, y]
    return rgba


def build_strip(source, positions, cell_size=(16, 16), palette=None):
    width, height = cell_size
    strip = Image.new("RGBA", (width * len(positions), height), (0, 0, 0, 0))
    for index, (x, y) in enumerate(positions):
        frame = key_colours(source.crop((x, y, x + width, y + height)), palette)
        strip.alpha_composite(frame, (index * width, 0))
    return strip


def main():
    world14 = Image.open(WORLD14_PATH).convert("RGBA")
    world24 = Image.open(WORLD24_PATH).convert("RGBA")
    tileset = Image.open(TILESET_PATH).convert("RGBA")
    enemies = Image.open(ENEMIES_PATH).convert("RGBA")

    for name, stage in (("World 1-4", world14), ("World 2-4", world24)):
        if stage.size != (160 * 16, 15 * 16):
            raise ValueError(f"{name} source must be 2560x240, got {stage.size}")
    if tileset.size != (680, 776):
        raise ValueError(f"Tileset source must be 680x776, got {tileset.size}")
    if enemies.size != (436, 530):
        raise ValueError(f"Enemy source must be 436x530, got {enemies.size}")

    assets = {
        "castle_wall.png": crop_tile(world14, 0, 2),
        # World 2-4's run above the bridge and World 3-4's barrier in front of
        # its Bowser are the only breakable castle brick in the campaign.
        "castle_brick.png": crop_tile(world24, 128, 5),
        "castle_firebar_block.png": crop_tile(world14, 23, 6),
        "castle_lava_surface.png": crop_tile(world14, 13, 12),
        "castle_lava.png": crop_tile(world14, 13, 13),
        "castle_bridge.png": crop_tile(world14, 128, 10),
        # The bridge-room lift is four girder segments wide, half a tile tall.
        "castle_lift.png": world14.crop((138 * 16, 6 * 16 + 1,
                                         140 * 16, 6 * 16 + 9)),
        # World 2-4's two elevators are three segments wide, and the guide's
        # travel line has to be cleaned back out of them.
        "castle_elevator.png": repair_lift(
            world24.crop((1372, 161, 1396, 169)), (11, 12)
        ),
        # Include the diagonal chain to the lower-left of the axe head.
        "castle_axe.png": clear_border_black(
            world14.crop((140 * 16, 8 * 16, 142 * 16, 10 * 16))
        ),
        "castle_question_block.png": build_strip(
            tileset, ((490, 78), (507, 78), (524, 78), (507, 78)),
            palette=CASTLE_PALETTE
        ),
        "castle_empty_block.png": build_strip(
            tileset, ((541, 78),), palette=CASTLE_PALETTE
        ),
        "castle_coin.png": build_strip(
            tileset, ((490, 95), (507, 95), (524, 95), (507, 95)),
            palette=CASTLE_PALETTE
        ),
        # A trigger before each bridge swaps the castle's green palette for the
        # overworld one, so Bowser and anything else green arrives bright green.
        "castle_bowser.png": build_strip(
            enemies, ((0, 208), (34, 208), (68, 208), (102, 208)), (32, 32)
        ),
        # The Podoboos leaping out of the lava in Worlds 2-4 and 3-4.
        "castle_podoboo.png": build_strip(enemies, ((90, 370),)),
        # World 1-4's Bowser is a disguised Little Goomba: its two walking
        # frames and its flattened frame, in the Castle palette.
        "castle_goomba.png": build_strip(
            enemies, ((147, 17), (164, 17), (181, 17))
        ),
        # World 2-4's is a disguised green Koopa Troopa, which the bridge
        # trigger leaves in the overworld palette alongside Bowser.
        "castle_koopa.png": build_strip(
            enemies, ((0, 112), (18, 112)), (16, 24)
        ),
        # World 3-4's is a disguised Buzzy Beetle. It is not part of the green
        # palette group the bridge trigger repaints, so it keeps the Castle
        # colours it walks in, exactly like World 1-4's Goomba.
        "castle_buzzy.png": build_strip(enemies, ((147, 34), (164, 34))),
    }

    for name, image in assets.items():
        path = TEXTURES / name
        image.save(path)
        print(f"{path.relative_to(ROOT)}  {image.width}x{image.height}")


if __name__ == "__main__":
    main()
