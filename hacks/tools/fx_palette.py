"""Preview stock FX and enemy animations in any palette, and build recolored FX palettes.

sheet:   every frame of one FX descriptor row (fx_spawnOnActor fxType) or of all 23.
grid:    FX rows against the fixed OBJ palette slots 2..8 (three frames each).
enemy:   one enemy pose animation (facing 0) in chosen ObjPalettes ids.
recolor: recolor an ObjPalettes palette (hue/sat/value), print the C array, preview on FX rows.

    uv run python hacks/tools/fx_palette.py sheet OUT_DIR [--type N] [--palette ID]
    uv run python hacks/tools/fx_palette.py grid OUT.png --types 0,5,7
    uv run python hacks/tools/fx_palette.py enemy OUT.png --enemy 64 --pose 2 --palettes 78,6
    uv run python hacks/tools/fx_palette.py recolor OUT.png --base 7 --hue 0.30 --sat 0.7 --val 1.15

Hue is 0..1 (0 red, 0.33 green, 0.66 blue, 0.75 violet). The recolor keeps each color's
brightness rank, so the FX shading survives. Paste the printed array into CustomPalettes in
hacks/lib/fx_kit.c and add an enum value in hacks/lib/fx_kit.h.
"""

import argparse
import colorsys
import os
import struct
import sys

from PIL import Image

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", "tools"))

from rotkit.carve.enemy_graphics import ENEMY_PALETTE_IDS_ADDR, TABLE_ADDR, _obj_palette  # noqa: E402
from rotkit.rom import ROMBASE, load_rom  # noqa: E402
from rotkit.spritegfx import (  # noqa: E402
    decode_frame_tiles,
    frame_bounds,
    parse_frame_set,
    render_frame,
)

FX_TABLE = 0x08287B7C
FX_STRIDE = 0x14
FX_COUNT = 23
FIXED_SLOTS = range(2, 9)
GREY = tuple(((31 - i * 2) * 0x421) if i else 0x7C1F for i in range(16))
BACKDROP = (70, 90, 60)

rom = load_rom()


def to_pil(image):
    pil = Image.frombytes("P", (image.width, image.height), bytes(image.pixels))
    flat = [c for rgb in image.palette for c in rgb]
    pil.putpalette(flat + [0] * (768 - len(flat)))
    mask = Image.frombytes(
        "L", (image.width, image.height), bytes(0 if p == 0 else 255 for p in image.pixels)
    )
    return pil.convert("RGBA"), mask


def frames_of(tiles, frames_addr, palette):
    frame_set = parse_frame_set(rom, frames_addr)
    bounds = frame_bounds(frame_set.frames)
    data = rom[tiles - ROMBASE : tiles - ROMBASE + 0x40000]
    return [
        to_pil(render_frame(f, decode_frame_tiles(data, f), bounds, palette))
        for f in frame_set.frames
    ]


def fx_row(fx_type):
    return struct.unpack_from("<II", rom, FX_TABLE - ROMBASE + fx_type * FX_STRIDE)


def paste_strip(images, cols, scale, out):
    w, h = images[0][0].size
    rows = (len(images) + cols - 1) // cols
    img = Image.new("RGB", (cols * (w + 2), rows * (h + 2)), BACKDROP)
    for k, (im, mask) in enumerate(images):
        img.paste(im, ((k % cols) * (w + 2), (k // cols) * (h + 2)), mask)
    img.resize((img.width * scale, img.height * scale), Image.NEAREST).save(out)


def picks(images):
    n = len(images)
    return [images[n // 4], images[n // 2], images[3 * n // 4]]


def grid(types, palettes, out, cell=64):
    img = Image.new("RGB", (len(palettes) * 3 * cell, len(types) * cell), BACKDROP)
    for row, fx_type in enumerate(types):
        tiles, frames_addr = fx_row(fx_type)
        for col, palette in enumerate(palettes):
            for k, (im, mask) in enumerate(picks(frames_of(tiles, frames_addr, palette))):
                scale = min(1.0, cell / max(im.width, im.height))
                if scale < 1:
                    size = (max(1, int(im.width * scale)), max(1, int(im.height * scale)))
                    im, mask = im.resize(size), mask.resize(size)
                img.paste(im, ((col * 3 + k) * cell, row * cell), mask)
    img.resize((img.width * 2, img.height * 2), Image.NEAREST).save(out)


def recolor(base, hue, sat, val):
    def unpack(c):
        return ((c & 31) / 31, ((c >> 5) & 31) / 31, ((c >> 10) & 31) / 31)

    def pack(r, g, b):
        q = lambda v: max(0, min(31, round(v * 31)))  # noqa: E731
        return q(r) | q(g) << 5 | q(b) << 10

    out = [base[0]]
    for color in base[1:]:
        h, s, v = colorsys.rgb_to_hsv(*unpack(color))
        out.append(pack(*colorsys.hsv_to_rgb(hue, min(1, s * sat), min(1, v * val))))
    return tuple(out)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawTextHelpFormatter)
    sub = parser.add_subparsers(dest="cmd", required=True)
    p = sub.add_parser("sheet")
    p.add_argument("out")
    p.add_argument("--type", type=int)
    p.add_argument("--palette", type=int, help="ObjPalettes id (default: grey ramp)")
    p = sub.add_parser("grid")
    p.add_argument("out")
    p.add_argument("--types", required=True)
    p = sub.add_parser("enemy")
    p.add_argument("out")
    p.add_argument("--enemy", type=int, required=True)
    p.add_argument("--pose", type=int, default=2)
    p.add_argument("--palettes", help="ObjPalettes ids (default: the enemy's own)")
    p = sub.add_parser("recolor")
    p.add_argument("out")
    p.add_argument("--base", type=int, default=7)
    p.add_argument("--hue", type=float, required=True)
    p.add_argument("--sat", type=float, default=1.0)
    p.add_argument("--val", type=float, default=1.0)
    p.add_argument("--types", default="0,5,7,16,18")
    args = parser.parse_args()

    if args.cmd == "sheet":
        os.makedirs(args.out, exist_ok=True)
        palette = _obj_palette(rom, args.palette) if args.palette is not None else GREY
        for fx_type in [args.type] if args.type is not None else range(FX_COUNT):
            images = frames_of(*fx_row(fx_type), palette)
            scale = 3 if images[0][0].width < 64 else 2
            paste_strip(images, min(len(images), 10), scale, os.path.join(args.out, f"fx{fx_type:02d}.png"))
    elif args.cmd == "grid":
        types = [int(t) for t in args.types.split(",")]
        grid(types, [_obj_palette(rom, s) for s in FIXED_SLOTS], args.out)
    elif args.cmd == "enemy":
        bank = struct.unpack_from("<I", rom, TABLE_ADDR - ROMBASE + (args.enemy * 6 + args.pose) * 4)[0]
        tiles, frames_addr = struct.unpack_from("<II", rom, bank - ROMBASE)
        own = rom[ENEMY_PALETTE_IDS_ADDR - ROMBASE + args.enemy]
        ids = [int(i) for i in args.palettes.split(",")] if args.palettes else [own]
        print(f"enemy {args.enemy} pose {args.pose}: own ObjPalettes id {own}")
        images = []
        for palette_id in ids:
            images += frames_of(tiles, frames_addr, _obj_palette(rom, palette_id))
        paste_strip(images, len(images) // len(ids), 2, args.out)
    else:
        palette = recolor(_obj_palette(rom, args.base), args.hue, args.sat, args.val)
        print("{" + ", ".join(f"0x{c:04x}" for c in palette) + "}")
        types = [int(t) for t in args.types.split(",")]
        grid(types, [_obj_palette(rom, args.base), palette], args.out)


main()
