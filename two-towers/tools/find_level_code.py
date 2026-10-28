"""Looks for ROTK's level loaders and decompressors in TTT, by instruction shape (Thumb) or exact bytes
(the ARM decompressors)."""

from roms import ROM_BASE, ROTK_ROM, TTT_ROM, load, masked_thumb, shape_index

K = 12
THUMB = {
    "decompress_chunk": 0x08019D54,
    "level loader 0x0803EEC0": 0x0803EEC0,
    "BG3 loader 0x0803EF7C": 0x0803EF7C,
    "loader 0x0803F048": 0x0803F048,
    "BG2 loader 0x0803F0BC": 0x0803F0BC,
    "0x08034104": 0x08034104,
}
ARM = {"decompress_pucrunch": (0x080002DC, 0x80), "decompress_lz77": (0x0800D1D4, 0x80)}

rotk = load(ROTK_ROM)
ttt = load(TTT_ROM)
index = shape_index(ttt, K)
for name, address in THUMB.items():
    hit = index.get(masked_thumb(rotk, address - ROM_BASE, K))
    print(f"{name:28} {hex(hit) if hit else 'not in TTT'}")
for name, (address, size) in ARM.items():
    at = ttt.find(rotk[address - ROM_BASE:address - ROM_BASE + size])
    print(f"{name:28} {hex(ROM_BASE + at) if at >= 0 else 'not in TTT'}")
