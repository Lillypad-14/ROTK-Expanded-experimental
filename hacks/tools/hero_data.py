"""The hero data region: everything about a hero that is tweaked often, as one binary at a fixed ROM
address (HERO_DATA in hacks/layout.cfg), written by the build and by hero_fast.py alike.

Code only ever points at fixed slots, never into data that moves, so a change to heroes.json
rewrites this region and nothing else: no code changes, and the PC port's recompiled code stays
valid (hero_fast.py patches a ROM in a second).

Layout (offsets from MEM_HERO_DATA):
    SOUNDS       HeroSounds rows, 4 u16 per hero id (sound slots, hero_registry.py)
    SFX_ENTRIES  sfx_play's SfxEntries with the hero sound mixes appended
    SFX_SETS     sfx_play's SfxVariantSets with the mixes' rows appended
    BODIES       per hero id a fixed slot: move, melee and cast banks (SpriteAnimation *[8][7]),
                 the hero select's idle and selected SpriteAnimation[5], then each bank pose's
                 SpriteAnimation[5], which the banks point at
    CLIPS        frame sets of the generated clips (hero_clips.py)

An actor holds a pointer to its pose's SpriteAnimation, which always sits in the fixed slot, and
reads the frame set through it every tick. So even a savestate stays valid across hero changes.
"""

from __future__ import annotations

from pathlib import Path
import struct

import hero_clips
import hero_registry

MAX_HEROES = 16
SOUNDS = 0x0000
SFX_ENTRIES = 0x1000
SFX_SETS = 0x2000
BODIES = 0x8000
CLIPS = 0x14000
BANK_BYTES = hero_clips.PARTS * hero_clips.POSES * 4
ANIMATION_BYTES = hero_clips.FACINGS * 16
# Within a body slot: the three banks, then the hero select's idle and selected animations.
BANK_OFFSETS = {"move": 0, "melee": BANK_BYTES, "cast": 2 * BANK_BYTES}
MENU_IDLE = 3 * BANK_BYTES
MENU_SELECTED = MENU_IDLE + ANIMATION_BYTES
POSE_RECORDS = MENU_SELECTED + ANIMATION_BYTES
BODY_BYTES = 0xA00
SFX_ENTRY_CAPACITY = (SFX_SETS - SFX_ENTRIES) // 4
SFX_SET_CAPACITY = (BODIES - SFX_SETS) // 32
assert POSE_RECORDS + len(BANK_OFFSETS) * hero_clips.POSES * ANIMATION_BYTES <= BODY_BYTES
assert BODIES + MAX_HEROES * BODY_BYTES <= CLIPS
STOCK_HERO_SOUNDS = 0x080585D4
ROM_BASE = hero_clips.ROM_BASE


def base_address(root: Path) -> tuple[int, int]:
    """MEM_HERO_DATA and its end, from hacks/layout.cfg."""
    for line in (root / "hacks/layout.cfg").read_text().splitlines():
        parts = line.split("#")[0].split()
        if len(parts) == 4 and parts[0] == "HERO_DATA":
            return int(parts[2], 16), int(parts[3], 16)
    raise ValueError("No HERO_DATA region in hacks/layout.cfg")


def body_slot(root: Path, hero_id: int) -> int:
    return base_address(root)[0] + BODIES + hero_id * BODY_BYTES


class Blob:
    def __init__(self, base: int):
        self.base = base
        self.data = bytearray(CLIPS)

    def put(self, offset: int, data: bytes) -> None:
        if len(self.data) < offset + len(data):
            self.data += bytes(offset + len(data) - len(self.data))
        self.data[offset:offset + len(data)] = data

    def append(self, data: bytes, align: int = 4) -> int:
        """Add data to the clips area. Returns its address."""
        self.data += bytes(-len(self.data) % align)
        offset = len(self.data)
        self.data += data
        return self.base + offset


def animation_records(facings: list[tuple[int, int, int]]) -> bytes:
    """SpriteAnimation[5]: tiles, frames, palette (none) and frameDuration per facing."""
    return b"".join(struct.pack("<IIIB3x", tiles, frames, 0, duration) for tiles, frames, duration in facings)


def build(root: Path, heroes: list[dict], rom: bytes) -> tuple[bytes, dict]:
    """The region's bytes and what went where (for hero_registry.py and the tools)."""
    base, end = base_address(root)
    blob = Blob(base)
    sources = hero_clips.Sources(root, rom)
    if len(heroes) > MAX_HEROES:
        raise ValueError(f"The hero data region holds {MAX_HEROES} heroes")

    # Sounds: every hero's row, mixes appended to copies of the stock tables.
    resolved = [dict(hero, sounds=list(hero["sounds"])) if "sounds" in hero else hero for hero in heroes]
    mixes = hero_registry.sound_mixes(rom, resolved)
    entries = [struct.unpack_from("<2H", rom, hero_registry.SFX_ENTRIES - ROM_BASE + i * 4)
               for i in range(hero_registry.SFX_ENTRY_COUNT)]
    entries += [(2, hero_registry.SFX_VARIANT_SET_COUNT + i) for i in range(len(mixes))]
    sets = [rom[hero_registry.SFX_VARIANT_SETS - ROM_BASE + r * 32:][:32] for r in range(hero_registry.SFX_VARIANT_SET_COUNT)]
    sets += [b"".join(struct.pack("<2H", s, v) for s, v in row) for row in mixes]
    if len(entries) > SFX_ENTRY_CAPACITY or len(sets) > SFX_SET_CAPACITY:
        raise ValueError("Too many hero sound mixes for the hero data region")
    blob.put(SFX_ENTRIES, b"".join(struct.pack("<2H", *e) for e in entries))
    blob.put(SFX_SETS, b"".join(sets))
    for hero in resolved:
        row = hero.get("sounds") or struct.unpack_from("<4H", rom, STOCK_HERO_SOUNDS - ROM_BASE + hero["stockHero"] * 8)
        blob.put(SOUNDS + hero["id"] * 8, struct.pack("<4H", *row))

    # Bodies: fixed bank slots, clips appended.
    placed = {}
    labels = {}
    for hero in heroes:
        if "body" not in hero:
            continue
        banks = hero_clips.body_banks(root, rom, hero, sources)
        slot = BODIES + hero["id"] * BODY_BYTES

        def records(clip: hero_clips.Clip) -> bytes:
            """The clip's SpriteAnimation[5]: the stock ones, or one over frame sets placed now."""
            if clip.stock:
                return rom[sources.addresses[clip.label] - ROM_BASE:][:ANIMATION_BYTES]
            if id(clip) not in placed:
                placed[id(clip)] = animation_records(
                    [(f.tiles_address, blob.append(f.frames), f.duration) for f in clip.facings])
            return placed[id(clip)]

        for number, (bank, offset) in enumerate(BANK_OFFSETS.items()):
            row = []
            for pose, clip in enumerate(banks[bank]):
                record = slot + POSE_RECORDS + (number * hero_clips.POSES + pose) * ANIMATION_BYTES
                blob.put(record, records(clip))
                row.append(base + record)
                labels[f"{hero['name']} {bank} {pose}"] = base + record
            blob.put(slot + offset, struct.pack(f"<{len(row) * hero_clips.PARTS}I", *(row * hero_clips.PARTS)))
        blob.put(slot + MENU_IDLE, records(banks["move"][4]))
        blob.put(slot + MENU_SELECTED, records(banks["move"][6]))
    if base + len(blob.data) > end + 1:
        raise ValueError(f"Hero data overflows HERO_DATA by {base + len(blob.data) - end - 1} bytes")
    return bytes(blob.data), {"base": base, "labels": labels}


def write(root: Path, heroes: list[dict], output: Path) -> None:
    """build/hero_data.bin, and its addresses for armips (build/hero_data.inc)."""
    rom = (root / "rotk.gba").read_bytes()
    data, info = build(root, heroes, rom)
    (root / "build/hero_data.bin").write_bytes(data)
    base = info["base"]
    lines = [f"HERO_DATA_SOUNDS equ 0x{base + SOUNDS:08x}",
             f"HERO_DATA_SFX_ENTRIES equ 0x{base + SFX_ENTRIES:08x}",
             f"HERO_DATA_SFX_SETS equ 0x{base + SFX_SETS:08x}"]
    (root / "build/hero_data.inc").write_text("\n".join(lines) + "\n", newline="\n")
