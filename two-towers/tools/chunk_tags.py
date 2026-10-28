"""Counts ROTK-style tagged chunk headers (decompress_chunk input) in both ROMs."""

import collections
import struct

from roms import ROM_BASE, ROTK_ROM, TTT_ROM, load


def tags(rom):
    kinds = collections.Counter()
    offsets = []
    for off in range(0, len(rom) - 8, 4):
        b = rom[off:off + 8]
        if (b[0] in (0x60, 0xE0) and b[3] == 0 and b[4] in (4, 8, 0xC) and b[5] < 4 and b[6] < 2
                and b[7] < 4 and struct.unpack_from("<H", b, 1)[0] > 0x40):
            kinds[(b[0], b[4])] += 1
            offsets.append(ROM_BASE + off)
    return kinds, offsets


for name, path in (("ROTK", ROTK_ROM), ("TTT", TTT_ROM)):
    kinds, offsets = tags(load(path))
    print(f"{name}: {len(offsets)} headers {dict(kinds.most_common(6))} first {[hex(o) for o in offsets[:3]]}")
