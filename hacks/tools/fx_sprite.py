"""Build new effect sprites from stock enemy art and emit them as C SpriteAnimation data.

pool: a ground pool that spreads from the center, cut from the silhouette of one enemy
animation frame (default the Witch-king's last death frame, without his body).

    uv run python hacks/tools/fx_sprite.py pool hacks/lib/fx_sprites --preview OUT.png

The output pair (.c/.h) holds every sprite this tool knows, so rerun it with all of them
after changing one. Frames keep the source frame's OAM layout and tile size, so they fit
the VRAM an effect actor already has.
"""

import argparse
import os
import struct
import sys
from dataclasses import replace

from PIL import Image

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", "tools"))

from rotkit.carve.enemy_graphics import TABLE_ADDR, _obj_palette  # noqa: E402
from rotkit.compression import lz77  # noqa: E402
from rotkit.png import bgr555_to_rgb  # noqa: E402
from rotkit.rom import ROMBASE, load_rom  # noqa: E402
from rotkit.spritegfx import (  # noqa: E402
    SPRITE_FRAME_LZ77,
    decode_frame_tiles,
    parse_frame_set,
    pixels_to_tiles,
    tiles_to_pixels,
)

rom = load_rom()

# Pool art indices, darkest last (FX_COLOR_SHADOW and the Witch-king palette both fit).
POOL_FILL = 15
POOL_EDGE = 14
POOL_SHEEN = 13


def enemy_frame(enemy, pose, index):
    bank = struct.unpack_from("<I", rom, TABLE_ADDR - ROMBASE + (enemy * 6 + pose) * 4)[0]
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
    return frame_set, frame, oam, width, height, -left, -top, pixels


def pool_frames(mask, width, height, cx, cy, count, start):
    """count frames of the mask scaled about (cx, cy) from start to full size."""
    out = []
    for k in range(count):
        t = (k + 1) / count
        scale = start + (1 - start) * (1 - (1 - t) ** 2)
        inside = [False] * (width * height)
        for y in range(height):
            sy = round(cy + (y - cy) / scale)
            for x in range(width):
                sx = round(cx + (x - cx) / scale)
                inside[y * width + x] = 0 <= sx < width and 0 <= sy < height and mask[sy * width + sx]
        pixels = bytearray(width * height)
        for y in range(height):
            for x in range(width):
                if not inside[y * width + x]:
                    continue
                edge = any(
                    not (0 <= x + dx < width and 0 <= y + dy < height)
                    or not inside[(y + dy) * width + x + dx]
                    for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))
                )
                # A faint arc of sheen across the upper left of the surface.
                rx, ry = (x - cx) / (width / 2 * scale), (y - cy) / (height / 2 * scale)
                sheen = 0.18 < rx * rx + ry * ry < 0.26 and rx < 0 and ry < 0 and (x + y) % 2 == 0
                pixels[y * width + x] = POOL_EDGE if edge else POOL_SHEEN if sheen else POOL_FILL
        out.append(bytes(pixels))
    return out


def frame_set_bytes(src_set, src_frame, oam, frames_pixels, width, height):
    """A SpriteFrameSetHeader blob plus the LZ77 tile blob for frames of one OAM layout."""
    count = len(frames_pixels)
    tile_bytes = len(pixels_to_tiles(frames_pixels[0], oam, width, height))
    raw_oam = [o.pack() for o in src_frame.oam]
    records = []
    tiles = bytearray()
    for pixels in frames_pixels:
        chunk = lz77.encode(pixels_to_tiles(pixels, oam, width, height))
        rec = struct.pack("<BBBBHH", SPRITE_FRAME_LZ77 | len(oam), src_frame.field_0x1,
                          src_frame.width, src_frame.height, len(tiles), tile_bytes)
        for box in src_frame.boxes:
            rec += struct.pack("<bbbbBB", box.x_min, box.x_max, box.y_min, box.y_max, 0, 0)
        rec += b"".join(struct.pack("<I", w) for w in raw_oam)
        records.append(rec)
        tiles += chunk + bytes(-len(chunk) % 4)
    header = struct.pack(
        "<BB4bHHBB", src_set.width, src_set.height,
        src_set.movement_collision_box.x_min, src_set.movement_collision_box.x_max,
        src_set.movement_collision_box.y_min, src_set.movement_collision_box.y_max,
        count, tile_bytes, 0, len(src_frame.boxes),
    )
    offsets = []
    pos = count * 2
    for rec in records:
        offsets.append(pos)
        pos += len(rec)
    blob = header + b"".join(struct.pack("<H", o) for o in offsets) + b"".join(records)
    return blob + bytes(-len(blob) % 4), bytes(tiles)


def c_array(name, data):
    lines = [f"static const u8 {name}[{len(data):#x}] __attribute__((aligned(4))) = {{"]
    for i in range(0, len(data), 16):
        lines.append("    " + " ".join(f"0x{b:02x}," for b in data[i : i + 16]))
    return "\n".join(lines) + "\n};\n"


def preview(frames_pixels, width, height, palette, out):
    rgb = [bgr555_to_rgb(c) for c in palette]
    img = Image.new("RGB", (len(frames_pixels) * (width + 2), height), (70, 90, 60))
    for k, pixels in enumerate(frames_pixels):
        for i, p in enumerate(pixels):
            if p:
                img.putpixel((k * (width + 2) + i % width, i // width), rgb[p])
    img.resize((img.width * 3, img.height * 3), Image.NEAREST).save(out)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawTextHelpFormatter)
    sub = parser.add_subparsers(dest="cmd", required=True)
    p = sub.add_parser("pool")
    p.add_argument("out", help="output path without extension")
    p.add_argument("--enemy", type=int, default=64)
    p.add_argument("--pose", type=int, default=2)
    p.add_argument("--frame", type=int, default=23)
    p.add_argument("--frames", type=int, default=8)
    p.add_argument("--start", type=float, default=0.15, help="first frame's scale")
    p.add_argument("--preview", help="PNG strip of the frames in the Witch-king palette")
    args = parser.parse_args()

    src_set, src_frame, oam, width, height, cx, cy, pixels = enemy_frame(args.enemy, args.pose, args.frame)
    mask = [p != 0 for p in pixels]
    frames_pixels = pool_frames(mask, width, height, cx, cy, args.frames, args.start)
    if args.preview:
        preview(frames_pixels, width, height, _obj_palette(rom, 78), args.preview)
    frames_blob, tiles_blob = frame_set_bytes(src_set, src_frame, oam, frames_pixels, width, height)

    name = os.path.basename(args.out)
    gen = "// Generated by hacks/tools/fx_sprite.py. Rerun the tool, do not edit.\n"
    with open(args.out + ".c", "w", newline="\n") as fh:
        fh.write(gen + f'\n#include "{name}.h"\n\n')
        fh.write(c_array("ShadowPoolTiles", tiles_blob) + "\n")
        fh.write(c_array("ShadowPoolFrames", frames_blob) + "\n")
        fh.write(
            "const SpriteAnimation HackLibFxShadowPool = {\n"
            "    .tiles = ShadowPoolTiles,\n"
            "    .frames = (const SpriteFrameSetHeader *)ShadowPoolFrames,\n"
            "    .palette = NULL,\n"
            "    .frameDuration = 1,\n"
            "};\n"
        )
    with open(args.out + ".h", "w", newline="\n") as fh:
        fh.write("#pragma once\n\n" + gen + "\n")
        fh.write('#include "spriteAnimation.h"\n\n')
        fh.write(f"/** Shadow pool spreading over {args.frames} frames. Draw it in a dark palette. */\n")
        fh.write("extern const SpriteAnimation HackLibFxShadowPool;\n")
    print(f"{args.frames} frames {width}x{height}, tiles {len(tiles_blob)} bytes, frames {len(frames_blob)} bytes")


main()
