"""Build the Fell Beast dive sprite from the game's flying Nazgul.

    uv run --with pillow python hacks/tools/fell_beast_art.py

Takes the 12 wing-beat frames of enemy 69, shrinks them onto one origin-aligned 144x128 canvas and
darkens them. An effect frame holds at most 2304 bytes of tiles, so the canvas is cut into four
72x64 pieces that play as four layers of one effect. Writes
hacks/content/fx/fell_beast_{left,right}_{0..3}.png (right is mirrored) and fell_beast_palette.png,
which the skill_fx.json assets share, and prints their manifest entries. Needs the base ROM, like
fx_sprite.py.
"""

import json
import os
import struct
import sys
from collections import Counter
from dataclasses import replace

from PIL import Image

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", "tools"))

from rotkit.carve.enemy_graphics import TABLE_ADDR, _palette  # noqa: E402
from rotkit.png import bgr555_to_rgb  # noqa: E402
from rotkit.rom import ROMBASE, load_rom  # noqa: E402
from rotkit.spritegfx import decode_frame_tiles, parse_frame_set, tiles_to_pixels  # noqa: E402

OUT = os.path.join(os.path.dirname(__file__), "..", "content", "fx")
ENEMY = 69
POSE_WALK = 0
PIECE_W = 72
PIECE_H = 64
PIECES_X = 2
PIECES_Y = 2
WIDTH = PIECE_W * PIECES_X
HEIGHT = PIECE_H * PIECES_Y
COLUMNS = 6
# Destination pixels per source pixel. The canvas clips the shadow and the farthest wing tips.
SCALE = 0.75
# Where the actor origin lands on the canvas.
ANCHOR = (66, 88)
# A destination pixel is drawn when this share of its source box is opaque.
COVERAGE = 0.3
# Shading applied to the stock colors: darker, cold steel.
TINT = (0.50, 0.74, 0.84)

rom = load_rom()


def enemy_frame(index):
    bank = struct.unpack_from("<I", rom, TABLE_ADDR - ROMBASE + (ENEMY * 6 + POSE_WALK) * 4)[0]
    tiles, frames_addr = struct.unpack_from("<II", rom, bank - ROMBASE)
    frame_set = parse_frame_set(rom, frames_addr)
    frame = frame_set.frames[index]
    data = rom[tiles - ROMBASE : tiles - ROMBASE + 0x40000]
    left = min(o.x for o in frame.oam)
    top = min(o.y for o in frame.oam)
    oam = tuple(replace(o, x=o.x - left, y=o.y - top) for o in frame.oam)
    width = max(o.x + o.size[0] for o in oam)
    height = max(o.y + o.size[1] for o in oam)
    pixels = tiles_to_pixels(decode_frame_tiles(data, frame), oam, width, height)
    return pixels, width, height, -left, -top, len(frame_set.frames)


def shrink(frames):
    """Each frame on the canvas with its origin at ANCHOR."""
    out = []
    for pixels, width, height, cx, cy in frames:
        canvas = [0] * (WIDTH * HEIGHT)
        for oy in range(HEIGHT):
            y0 = cy + (oy - ANCHOR[1]) / SCALE
            y1 = y0 + 1 / SCALE
            for ox in range(WIDTH):
                x0 = cx + (ox - ANCHOR[0]) / SCALE
                x1 = x0 + 1 / SCALE
                votes = Counter()
                total = 0
                for sy in range(max(0, int(y0)), min(height, int(y1) + 1)):
                    for sx in range(max(0, int(x0)), min(width, int(x1) + 1)):
                        total += 1
                        index = pixels[sy * width + sx]
                        if index:
                            votes[index] += 1
                if votes and sum(votes.values()) >= COVERAGE * max(total, 1):
                    canvas[oy * WIDTH + ox] = votes.most_common(1)[0][0]
        out.append(canvas)
    return out


def main():
    count = enemy_frame(0)[5]
    frames = []
    for i in range(count):
        pixels, width, height, cx, cy, _ = enemy_frame(i)
        frames.append((pixels, width, height, cx, cy))
    small = shrink(frames)

    palette = []
    for color in _palette(rom, ENEMY):
        r, g, b = bgr555_to_rgb(color)
        palette.append(tuple(min(255, round(v * t)) for v, t in zip((r, g, b), TINT)))
    used = sorted({p for f in small for p in f if p})
    # The importer reserves index 0 for transparency, so at most 15 colors.
    assert len(used) <= 15, used

    rows = (count + COLUMNS - 1) // COLUMNS
    assets = []
    effects = []
    for name, mirror in (("left", False), ("right", True)):
        anchor_x = WIDTH - 1 - ANCHOR[0] if mirror else ANCHOR[0]
        layers = []
        for piece in range(PIECES_X * PIECES_Y):
            px, py = piece % PIECES_X, piece // PIECES_X
            sheet = Image.new("RGBA", (COLUMNS * PIECE_W, rows * PIECE_H), (0, 0, 0, 0))
            for k, canvas in enumerate(small):
                for y in range(PIECE_H):
                    for x in range(PIECE_W):
                        cx = WIDTH - 1 - (px * PIECE_W + x) if mirror else px * PIECE_W + x
                        index = canvas[(py * PIECE_H + y) * WIDTH + cx]
                        if index:
                            sheet.putpixel((k % COLUMNS * PIECE_W + x, k // COLUMNS * PIECE_H + y),
                                           (*palette[index], 255))
            key = f"fell_beast_{name}_{piece}"
            sheet.save(os.path.join(OUT, key + ".png"))
            assets.append({"key": key, "id": 2, "sheet": f"fx/{key}.png",
                           "palette": "fx/fell_beast_palette.png", "width": PIECE_W,
                           "height": PIECE_H, "frameTicks": 2,
                           "anchor": [anchor_x - px * PIECE_W, ANCHOR[1] - py * PIECE_H]})
            layers.append({"asset": key, "attachment": "fixed", "layer": "front"})
        effects.append({"key": f"fell_beast_{name}", "name": f"Fell Beast {name}", "layers": layers})
    strip = Image.new("RGBA", (len(used), 1))
    for i, index in enumerate(used):
        strip.putpixel((i, 0), (*palette[index], 255))
    strip.save(os.path.join(OUT, "fell_beast_palette.png"))
    print(f"{count} frames, scale {SCALE}, {len(used)} colors")
    for asset in assets:
        print("asset ", json.dumps(asset))
    for effect in effects:
        print("effect", json.dumps(effect))


main()
