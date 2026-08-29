#!/usr/bin/env python3
"""Cuts the objects every world shares from the NES source sheets.

The per-stage tools next to this one each cut one world's terrain. What was
left over afterwards were the objects that are not tied to a stage - the coin
and question block of the default overworld and underground palettes, the Fire
Flower, Mario's fireball and its burst, and the springboard - and those were
still the imported artwork the project started with: soft-edged, anti-aliased
and several hundred colours where the NES has three.

    python3 tools/build_object_assets.py

Frame counts and image sizes match what the game already expects, so nothing
downstream has to change except the explosion, whose cells go from 189x220
down to the 16x16 the NES actually draws.
"""

from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
SOURCE = ROOT / "tools" / "source_art"
OUT = ROOT / "assets" / "textures"

# Lavender is the cell background behind every sprite - the tileset and the
# object sheet each use their own shade of it - and the two blues are the
# sheets' own panels and labels.
BACKGROUNDS = {(146, 144, 255), (148, 148, 255), (0, 41, 140), (0, 0, 168)}

# Palette 3 of the tileset holds the blinking tiles, one panel per environment.
QUESTION_ROW, COIN_ROW = 78, 95
OVERWORLD_PHASES = (298, 315, 332)
UNDERGROUND_PHASES = (394, 411, 428)


def load(name):
    return Image.open(SOURCE / f"nes_{name}.png").convert("RGBA")


def cut(sheet, box):
    """Crops @p box and turns the sheet's own backgrounds transparent."""
    frame = sheet.crop(box)
    pixels = frame.load()
    for y in range(frame.height):
        for x in range(frame.width):
            if pixels[x, y][:3] in BACKGROUNDS:
                pixels[x, y] = (0, 0, 0, 0)
    return frame


def strip(frames, cell, height=None):
    """Lays frames left to right in equal cells, each sitting on the bottom."""
    height = height or max(f.height for f in frames)
    out = Image.new("RGBA", (cell * len(frames), height), (0, 0, 0, 0))
    for index, frame in enumerate(frames):
        out.alpha_composite(frame, (index * cell, height - frame.height))
    return out


def burst_grid(items):
    """The fireball's three bursts, growing across the top row and dying on the
    bottom one, in the 3x2 arrangement the drawing code indexes."""
    sizes = (180, 198, 216)
    grid = Image.new("RGBA", (48, 32), (0, 0, 0, 0))
    for index, x in enumerate(sizes + tuple(reversed(sizes))):
        frame = cut(items, (x, 64, x + 16, 80))
        grid.alpha_composite(frame, ((index % 3) * 16, (index // 3) * 16))
    return grid


def tile_cycle(tileset, row, phases, order):
    """One blinking tile, its bright/medium/dark phases played in @p order."""
    frames = [cut(tileset, (phases[i], row, phases[i] + 16, row + 16)) for i in order]
    return strip(frames, 16)


def main():
    tileset, items = load("tileset"), load("items")

    written = {
        # The three drawn phases, played as the four-step ping-pong the rest of
        # the game's blinking tiles already use.
        "question_block.png": tile_cycle(
            tileset, QUESTION_ROW, OVERWORLD_PHASES, (0, 1, 2, 1)),
        "coin.png": tile_cycle(
            tileset, COIN_ROW, OVERWORLD_PHASES, (0, 1, 2, 1)),
        # Underground blocks are given six cells rather than four, which is the
        # cycle the caves have always run; the extra pair holds the bright
        # phase a beat longer.
        "question_block_underground.png": tile_cycle(
            tileset, QUESTION_ROW, UNDERGROUND_PHASES, (0, 1, 2, 1, 0, 0)),

        # The coin that spins out of a block, standing in for the HUD icon.
        "coinHUD.png": cut(items, (180, 36, 188, 52)),
        # The flower's four colour phases are a sprite animation in the
        # original; the game draws it as one picture, so it takes the first.
        "FireFlower.png": cut(items, (32, 8, 48, 24)),
        # The fireball turns as it flies; the game draws one picture, so it
        # takes the frame whose pinwheel sits squarest in its cell.
        "Bullet.png": cut(items, (200, 54, 208, 62)),
        # Three burst sizes, grown and then let go again over six cells laid out
        # as the 3x2 grid PlayState::drawExplosions steps through.
        "explosion.png": burst_grid(items),
    }

    # The springboard's three heights all stand on the same base, so each one is
    # padded at the top to a common cell. Drawing them at one fixed position
    # then keeps the base still and lets only the plate ride up and down.
    spring = {"trampoline_normal.png": (32, 75),
              "trampoline_launch.png": (50, 83),
              "trampoline_compressed.png": (68, 91)}
    tallest = 105 - min(y for _, y in spring.values()) + 1
    for name, (x, y) in spring.items():
        board = cut(items, (x, y, x + 16, 106))
        cell = Image.new("RGBA", (16, tallest), (0, 0, 0, 0))
        cell.alpha_composite(board, (0, tallest - board.height))
        written[name] = cell

    for name, image in written.items():
        image.save(OUT / name)
        print(f"assets/textures/{name}  {image.width}x{image.height}")


if __name__ == "__main__":
    main()
