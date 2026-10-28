"""ROM paths and helpers shared by the comparison scripts.

The ROMs default to rotk.gba and ttt.gba in the repository root (both gitignored); set ROTK_ROM or
TTT_ROM to use other paths.
"""

import bisect
import os
import struct
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
ROTK_ROM = Path(os.environ.get("ROTK_ROM", REPO / "rotk.gba"))
TTT_ROM = Path(os.environ.get("TTT_ROM", REPO / "ttt.gba"))
ROTK_FUNCTIONS = REPO / "config" / "functions.cfg"
ROM_BASE = 0x08000000


def load(path):
    return path.read_bytes()


def rotk_functions(mode="thumb"):
    """(address, name) pairs from the decomp config, sorted."""
    funcs = []
    for line in ROTK_FUNCTIONS.read_text().splitlines():
        parts = line.split("#")[0].split()
        if len(parts) >= 3 and parts[0].startswith("0x") and (mode is None or parts[1] == mode):
            funcs.append((int(parts[0], 16), parts[2]))
    funcs.sort()
    return funcs


def owner(funcs, address):
    """Name+offset of the decomp function containing a ROTK address."""
    addrs = [a for a, _ in funcs]
    i = bisect.bisect_right(addrs, address) - 1
    return f"{funcs[i][1]}+{address - funcs[i][0]:#x}" if i >= 0 else "?"


def masked_thumb(rom, offset, count):
    """Thumb halfwords with BL targets and literal-pool offsets masked, for shape comparisons."""
    out = []
    for i in range(count):
        h = struct.unpack_from("<H", rom, offset + 2 * i)[0]
        if (h & 0xF800) in (0xF000, 0xF800):
            h &= 0xF800
        elif (h & 0xF800) == 0x4800:
            h = 0x4800 | (h & 0x0700)
        out.append(h)
    return tuple(out)


def shape_index(rom, length, limit=0x400000):
    """Map of masked instruction windows to the first ROM address they occur at."""
    index = {}
    for off in range(0, min(len(rom), limit) - 2 * length, 2):
        index.setdefault(masked_thumb(rom, off, length), ROM_BASE + off)
    return index
