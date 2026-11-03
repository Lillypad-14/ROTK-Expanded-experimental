"""Prototype: ROTK hero animations converted to The Two Towers' sprite format, rendered to check them.

ROTK keeps a hero's animations in banks (HeroMoveAnimationBanks / Melee / Cast, per hero a bank of
part * 7 + pose entries), each entry 5 inline 16-byte SpriteAnimation records, one per facing.
The Two Towers' player draws from a pose table (0x08329664: hero * 6 + pose -> 5 pointers to 8-byte
records {tiles, frame set}). This converts one ROTK animation to that format (raw tiles, 12-byte OAM
pieces, 8-byte trigger boxes) and renders it with ttt_sprite's renderer, the data placed past the end
of an in-memory copy of the Two Towers ROM. The PC host does the same conversion in C++.

usage: python two-towers/tools/rotk_hero_sprites.py OUT.png [--hero 2] [--bank move|melee|cast]
           [--part 0] [--facing 2]
"""

from __future__ import annotations

import argparse
import struct

from PIL import Image

import ttt_sprite as T
from roms import ROTK_ROM

ROM_BASE = 0x08000000
BANKS = {"move": 0x08282898, "melee": 0x082828B8, "cast": 0x082828D8}
OBJ_PALETTES = 0x082830B0  # ObjPalettes
HERO_PALETTE_BASE = 0x21   # ObjPalettes id of hero h is 0x21 + h (stock heroes)
POSES_PER_PART = 7
FACINGS = 5
RECORD = 16

rotk = ROTK_ROM.read_bytes()


def r8(a):
    return rotk[a - ROM_BASE]


def r16(a):
    return struct.unpack_from("<H", rotk, a - ROM_BASE)[0]


def r32(a):
    return struct.unpack_from("<I", rotk, a - ROM_BASE)[0]


def lz77(data, p):
    """ROTK's decompress_lz77 (0x0800D1D4): like the Two Towers' but runs are 3, copies 2 and long
    runs 0x42 longer."""
    out = bytearray()
    while len(out) < 0x4000:
        c = data[p]
        p += 1
        if c == 0:
            break
        if c < 0x40:
            out += data[p:p + c]
            p += c
        elif c < 0x80:
            out += bytes([data[p]]) * ((c & 0x3F) + 3)
            p += 1
        else:
            o = data[p]
            p += 1
            d, n = (c & 0x7F) << 3 | o >> 5, o & 0x1F
            if n == 0:
                out += bytes([data[p]]) * (d + 0x42)
                p += 1
            else:
                for _ in range(n + 2):
                    out.append(out[-d] if d and d <= len(out) else 0)
    return bytes(out)


def run_length(data, p):
    size = struct.unpack_from("<I", data, p)[0] >> 8
    p += 4
    out = bytearray()
    while len(out) < size:
        f = data[p]
        p += 1
        if f & 0x80:
            out += bytes([data[p]]) * ((f & 0x7F) + 3)
            p += 1
        else:
            n = (f & 0x7F) + 1
            out += data[p:p + n]
            p += n
    return bytes(out[:size])


def rotk_animation(hero, bank, part, pose, facing):
    """Address of a ROTK SpriteAnimation record."""
    entry = r32(r32(BANKS[bank] + 4 * hero) + 4 * (part * POSES_PER_PART + pose))
    return entry + facing * RECORD


def convert(animation):
    """(tiles, frame set) bytes of a ROTK animation in the Two Towers' format, offsets relative."""
    tiles_base, frame_set = r32(animation), r32(animation + 4)
    header = rotk[frame_set - ROM_BASE:frame_set - ROM_BASE + 12]
    count = r16(frame_set + 6)
    hotspots, boxes = header[10], header[11]
    tiles = bytearray()
    frames = []
    for f in range(count):
        frame = frame_set + 12 + r16(frame_set + 12 + 2 * f)
        flags, width, height = r8(frame), r8(frame + 2), r8(frame + 3)
        tile_offset, tile_bytes = r16(frame + 4), r16(frame + 6)
        pieces = flags & 0x1F
        source = tiles_base + tile_offset - ROM_BASE
        if flags & 0x20:
            data = run_length(rotk, source)
        elif flags & 0x40:
            data = lz77(rotk, source)
        else:
            data = rotk[source:source + tile_bytes]
        data = bytes(data[:tile_bytes]).ljust(tile_bytes, b"\0")
        offset = len(tiles)
        tiles += data
        while len(tiles) % 4:
            tiles.append(0)
        p = frame + 8
        hotspot_data = rotk[p - ROM_BASE:p - ROM_BASE + 2 * hotspots]
        p += 2 * hotspots
        box_data = rotk[p - ROM_BASE:p - ROM_BASE + 6 * boxes]
        p += 6 * boxes
        out = bytearray([pieces, 0, width, height]) + struct.pack("<H", offset)
        out += hotspot_data
        for b in range(boxes):
            out += box_data[6 * b:6 * b + 6] + b"\0\0"
        for k in range(pieces):
            v = r32(p + 4 * k)
            x, y = v & 0x1FF, (v >> 9) & 0x1FF
            size, shape, tile = (v >> 18) & 3, (v >> 20) & 3, (v >> 22) & 0x3FF
            w, h = T.OBJ_DIMENSIONS[(shape, size)]
            out += struct.pack("<HHBBBBI", x, y, size, shape, w, h, tile * 32)
        frames.append(bytes(out))
    frame_set_out = bytearray(header)
    table = bytearray()
    body = bytearray()
    for frame in frames:
        table += struct.pack("<H", 2 * count + len(body))
        body += frame
        if len(body) % 2:
            body.append(0)
    frame_set_out += table + body
    return bytes(tiles), bytes(frame_set_out)


def hero_palette(hero):
    return rotk[r32(OBJ_PALETTES + 4 * (HERO_PALETTE_BASE + hero)) - ROM_BASE:][:32]


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("out")
    parser.add_argument("--hero", type=int, default=2)
    parser.add_argument("--bank", default="move", choices=sorted(BANKS))
    parser.add_argument("--part", type=int, default=0)
    parser.add_argument("--facing", type=int, default=2)
    parser.add_argument("--poses", type=int, default=7)
    args = parser.parse_args()
    # the converted data goes past the end of the Two Towers ROM copy the renderer reads
    rom = bytearray(T.rom)
    T.rom = rom
    palette = len(rom) + ROM_BASE
    rom += hero_palette(args.hero)
    strips = []
    for pose in range(args.poses):
        try:
            source = rotk_animation(args.hero, args.bank, args.part, pose, args.facing)
            tiles, frame_set = convert(source)
        except (IndexError, KeyError, struct.error) as error:
            print("pose", pose, "failed:", error)
            continue
        while len(rom) % 4:
            rom.append(0)
        tiles_at = len(rom) + ROM_BASE
        rom += tiles
        while len(rom) % 4:
            rom.append(0)
        frames_at = len(rom) + ROM_BASE
        rom += frame_set
        while len(rom) % 4:
            rom.append(0)
        record = len(rom) + ROM_BASE
        rom += struct.pack("<II", tiles_at, frames_at)
        name = f"{args.out}.{pose}.png"
        n = T.render(record, palette, name, 12)
        print("pose", pose, "duration", r8(source + 12), "frames", n)
        strips.append(Image.open(name))
    width = max(s.width for s in strips)
    sheet = Image.new("RGB", (width, sum(s.height for s in strips)), (40, 40, 60))
    y = 0
    for s in strips:
        sheet.paste(s, (0, y))
        y += s.height
    sheet.save(args.out)
    print(args.out)


if __name__ == "__main__":
    main()
