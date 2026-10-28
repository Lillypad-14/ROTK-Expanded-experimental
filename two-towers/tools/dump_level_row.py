"""Prints one row of ROTK's level asset table (0x08069A58, 0x4C bytes per mission id) and the first
8 bytes of every chunk it points to. Usage: dump_level_row.py [mission id, default 0]."""

import struct
import sys

from roms import ROM_BASE, ROTK_ROM, load

TABLE = 0x08069A58
STRIDE = 0x4C

rom = load(ROTK_ROM)
mission = int(sys.argv[1], 0) if len(sys.argv) > 1 else 0
row = rom[TABLE - ROM_BASE + mission * STRIDE:TABLE - ROM_BASE + (mission + 1) * STRIDE]
print(f"mission {mission:#x}, row at {TABLE + mission * STRIDE:#x}")
for field in range(0, 0x48, 4):
    value = struct.unpack_from("<I", row, field)[0]
    head = ""
    if ROM_BASE <= value < ROM_BASE + len(rom):
        head = rom[value - ROM_BASE:value - ROM_BASE + 8].hex(" ")
    print(f"  +{field:02x}  {value:08x}  {head}")
print(f"  +48  {row[0x48:].hex(' ')}  (tile count u16, mode bytes)")
