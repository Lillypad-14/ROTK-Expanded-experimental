"""Render a Two Towers sprite animation to a PNG, to check art and palettes before a port.

Each frame is drawn side by side on a dark background, its trigger boxes outlined in red. The
animation is a Two Towers SpriteAnimation (or an 8-byte record: tiles, frames); frames may be raw,
Two Towers LZ77 (flag 0x40) or the BIOS run-length format (flag 0x20).

usage: python two-towers/tools/ttt_sprite.py ANIMATION PALETTE OUT.png [--frames N]
"""

from __future__ import annotations

import argparse
import struct

from PIL import Image

from roms import TTT_ROM

ROM_BASE = 0x08000000
# OBJ (shape, size) -> width, height
OBJ_DIMENSIONS = {
    (0, 0): (8, 8), (0, 1): (16, 16), (0, 2): (32, 32), (0, 3): (64, 64),
    (1, 0): (16, 8), (1, 1): (32, 8), (1, 2): (32, 16), (1, 3): (64, 32),
    (2, 0): (8, 16), (2, 1): (8, 32), (2, 2): (16, 32), (2, 3): (32, 64),
}
CELL = 96

rom = TTT_ROM.read_bytes()


def r8(a):
    return rom[a - ROM_BASE]


def r16(a):
    return struct.unpack_from("<H", rom, a - ROM_BASE)[0]


def r32(a):
    return struct.unpack_from("<I", rom, a - ROM_BASE)[0]


def lz77(a):
    out, p = bytearray(), a - ROM_BASE
    while len(out) < 0x4000:
        c = rom[p]
        p += 1
        if c == 0:
            break
        if c < 0x40:
            out += rom[p:p + c]
            p += c
        elif c < 0x80:
            out += bytes([rom[p]]) * (c & 0x3F)
            p += 1
        else:
            o = rom[p]
            p += 1
            d, n = (c & 0x7F) << 3 | o >> 5, o & 0x1F
            if n == 0:
                out += bytes([rom[p]]) * (d + 64)
                p += 1
            else:
                for _ in range(n):
                    out.append(out[-d] if d and d <= len(out) else 0)
    return bytes(out)


def run_length(a):
    p = a - ROM_BASE
    size = struct.unpack_from("<I", rom, p)[0] >> 8
    p += 4
    out = bytearray()
    while len(out) < size:
        f = rom[p]
        p += 1
        if f & 0x80:
            out += bytes([rom[p]]) * ((f & 0x7F) + 3)
            p += 1
        else:
            n = (f & 0x7F) + 1
            out += rom[p:p + n]
            p += n
    return bytes(out[:size])


def palette(a):
    colors = []
    for i in range(16):
        c = r16(a + 2 * i)
        colors.append(((c & 31) * 8, (c >> 5 & 31) * 8, (c >> 10 & 31) * 8, 0 if i == 0 else 255))
    return colors


def frames(animation):
    """Per frame: pieces (x, y, w, h, tile byte offset), tiles, trigger boxes (x0, x1, y0, y1)."""
    tile_base, frame_set = r32(animation), r32(animation + 4)
    hotspots, boxes = r8(frame_set + 10), r8(frame_set + 11)
    result = []
    for f in range(r16(frame_set + 6)):
        frame = frame_set + 12 + r16(frame_set + 12 + 2 * f)
        flags = r8(frame)
        first = frame + 6 + hotspots * 2 + boxes * 8
        pieces, need = [], 0
        for k in range(flags & 0x1F):
            piece = first + 12 * k
            x, y = r16(piece) & 0x1FF, r16(piece + 2) & 0x1FF
            x, y = x - 512 if x >= 256 else x, y - 512 if y >= 256 else y
            w, h = OBJ_DIMENSIONS[(r8(piece + 5) & 3, r8(piece + 4) & 3)]
            offset = r32(piece + 8)
            need = max(need, offset + w * h // 2)
            pieces.append((x, y, w, h, offset))
        source = tile_base + r16(frame + 4)
        if flags & 0x20:
            tiles = run_length(source)
        elif flags & 0x40:
            tiles = lz77(source)
        else:
            tiles = rom[source - ROM_BASE:source - ROM_BASE + need]
        box_data = frame + 6 + hotspots * 2
        trigger = [struct.unpack_from("<bbbb", rom, box_data + 8 * b - ROM_BASE) for b in range(boxes)]
        result.append((pieces, tiles, trigger))
    return result


def render(animation, palette_address, out, max_frames=16):
    shown = frames(animation)[:max_frames]
    colors = palette(palette_address)
    image = Image.new("RGBA", (CELL * len(shown), CELL), (40, 40, 60, 255))
    for i, (pieces, tiles, trigger) in enumerate(shown):
        ox, oy = i * CELL + CELL // 2, CELL * 2 // 3
        for x, y, w, h, offset in pieces:
            for ty in range(h // 8):
                for tx in range(w // 8):
                    tile = offset + (ty * (w // 8) + tx) * 32
                    for py in range(8):
                        for px in range(8):
                            at = tile + py * 4 + px // 2
                            value = (tiles[at] >> ((px & 1) * 4)) & 15 if at < len(tiles) else 0
                            if value:
                                image.putpixel((ox + x + tx * 8 + px, oy + y + ty * 8 + py), colors[value])
        for x0, x1, y0, y1 in trigger:
            for xx in range(x0, x1 + 1):
                for yy in (y0, y1):
                    if 0 <= ox + xx < image.width and 0 <= oy + yy < CELL:
                        image.putpixel((ox + xx, oy + yy), (255, 0, 0, 255))
    image.resize((image.width * 2, image.height * 2), Image.NEAREST).save(out)
    return len(frames(animation))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("animation", type=lambda v: int(v, 16))
    parser.add_argument("palette", type=lambda v: int(v, 16))
    parser.add_argument("out")
    parser.add_argument("--frames", type=int, default=16)
    args = parser.parse_args()
    print(f"{args.out}: {render(args.animation, args.palette, args.out, args.frames)} frames")


if __name__ == "__main__":
    main()
