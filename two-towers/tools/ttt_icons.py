"""Render The Two Towers' skill icons, as the game draws them, to a PNG sheet.

One row per hero (Frodo, Legolas, Aragorn, Gandalf, Eowyn, Gimli), 11 icons each in the skill
record's order, 32x32 in the hero's palette: the skill screen draws them in BG palette bank 3, which
the hero's skill frame asset (16 colors, table 0x083297F0) loads. The tiles are BG assets packed with the C64
pucrunch (the ARM decoder at ROM 0x08000264); this is the PC port's `unpackTiles` in Python.

usage: python two-towers/tools/ttt_icons.py out.png [--scale 3]
"""

from __future__ import annotations

import argparse
import struct

from PIL import Image

from roms import TTT_ROM

ROM_BASE = 0x08000000
SKILL_ICONS = 0x0832EA64
# the hero skill frame BG assets, whose 16 colors the icons use
HERO_FRAMES = 0x083297F0
HEROES, SKILLS, SIZE = 6, 11, 32

rom = TTT_ROM.read_bytes()


def r32(a):
    return struct.unpack_from("<I", rom, a - ROM_BASE)[0]


def unpack_tiles(address):
    dest = 0x06000000
    out = bytearray()
    frame = bytearray(68)
    p = address - ROM_BASE + 6

    def byte():
        nonlocal p
        value = rom[p] if p < len(rom) else 0
        p += 1
        return value

    frame[4:8] = dest.to_bytes(4, "little")
    frame[0] = byte()
    p += 2
    frame[8] = byte()
    frame[0xC] = (8 - frame[8]) & 0xFF
    frame[0x10] = byte()
    frame[0x1C] = (9 - frame[0x10]) & 0xFF
    frame[0x14] = byte()
    frame[0x18] = (2 * frame[0x14] - 1) & 0xFF
    frame[0x20] = byte()
    p += 2
    for i in range(byte()):
        if 36 + i < len(frame):
            frame[36 + i] = byte()
    bits = 128

    def bit():
        nonlocal bits
        bits <<= 1
        if bits & 0xFF == 0:
            bits = byte() * 2 + 1
        return (bits >> 8) & 1

    def read(value, n):
        for _ in range(n):
            value = value * 2 + bit()
        return value

    def gamma():
        n = 1
        while bit():
            n += 1
            if n == frame[0x10]:
                break
        return read(1, n - 1)

    while len(out) < 0x4000:
        escape = frame[0]
        head = read(0, frame[8])
        if head != escape:
            out.append(read(head, frame[0xC]) & 0xFF)
            continue
        g = gamma()
        frame[4] = g & 0xFF
        if g >> 1:
            g2 = gamma()
            if g2 == frame[0x18]:
                break
            frame[5] = read(g2 - 1, frame[0x20]) & 0xFF
            low = read(frame[5], 8) & 0xFF
        elif bit() == 0:
            frame[5] = 0
            low = read(0, 8) & 0xFF
        elif bit() == 0:
            frame[0] = read(0, frame[8]) & 0xFF
            out.append(read(escape, frame[0xC]) & 0xFF)
            continue
        else:
            g = gamma()
            frame[4] = g & 0xFF
            high = 0
            if g >= frame[0x14]:
                frame[4] = read(g, frame[0x1C]) & 0xFF
                high = gamma() - 1
            rank = gamma()
            value = frame[35 + rank] if rank < 32 else read(rank, 3) & 0xFF
            out += bytes([value]) * (frame[4] + 1 + (high << 8))
            continue
        length = frame[4] + 1
        nxt = dest + len(out)
        total = (nxt & 0xFF) + low
        borrow = ((total >> 8) & 1) ^ 1
        frame[4] = total & 0xFF
        mid = (nxt >> 8) - frame[5] - borrow
        frame[5] = mid & 0xFF
        frame[6] = (mid >> 8) & 0xFF
        source = int.from_bytes(frame[4:8], "little") - dest
        if source + 1 == len(out) and len(out) & 1:
            out += bytes([out[-1]]) * length
            continue
        for k in range(length):
            at, stored = source + k, len(out) & ~1
            out.append(out[at] if 0 <= at < stored else 0)
    return bytes(out)


def palette(hero):
    colors = []
    asset = r32(HERO_FRAMES + hero * 4)
    for i in range(16):
        c = struct.unpack_from("<H", rom, asset + 2 + 2 * i - ROM_BASE)[0]
        colors.append(((c & 31) * 8, (c >> 5 & 31) * 8, (c >> 10 & 31) * 8))
    return colors


def icon(hero, skill, colors):
    tiles = unpack_tiles(r32(SKILL_ICONS + (hero * SKILLS + skill) * 4) + 4)
    image = Image.new("RGB", (SIZE, SIZE))
    for y in range(SIZE):
        for x in range(SIZE):
            at = ((y // 8) * 4 + x // 8) * 32 + (y % 8) * 4 + (x % 8) // 2
            value = (tiles[at] >> ((x & 1) * 4)) & 0xF if at < len(tiles) else 0
            image.putpixel((x, y), colors[value])
    return image


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("out")
    parser.add_argument("--scale", type=int, default=3)
    args = parser.parse_args()
    sheet = Image.new("RGB", (SKILLS * (SIZE + 4), HEROES * (SIZE + 4)), (30, 30, 40))
    for hero in range(HEROES):
        for skill in range(SKILLS):
            sheet.paste(icon(hero, skill, palette(hero)), (skill * (SIZE + 4) + 2, hero * (SIZE + 4) + 2))
    sheet.resize((sheet.width * args.scale, sheet.height * args.scale), Image.NEAREST).save(args.out)
    print(args.out)


if __name__ == "__main__":
    main()
