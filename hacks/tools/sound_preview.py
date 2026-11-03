"""Hear the game's sound effects: write them as WAV files and show who uses them.

    uv run python hacks/tools/sound_preview.py 235 102            # sfx ids: each sample they play
    uv run python hacks/tools/sound_preview.py --hero Witch-king  # a hero's sound row (mixes too)
    uv run python hacks/tools/sound_preview.py --enemy WitchKing  # an enemy's sound set
    uv run python hacks/tools/sound_preview.py --find 161         # sfx ids and enemies that play sample 161

WAVs go to build/preview/sounds/ (sfx<id>_sample<n>.wav), ready to click. An sfx id is what
heroes.json "sounds" and {"mix": [[sfx, count], ...]} take.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import struct
import wave

import hero_registry
import new_hero

ROOT = Path(__file__).resolve().parents[2]
ROM_BASE = 0x08000000
SOUND_DATA = 0x082808D4
ENEMY_SET = ("attack", "aggro", "hit", "crit", "death", "miss")
OUT = ROOT / "build/preview/sounds"


def u32(rom: bytes, address: int) -> int:
    return struct.unpack_from("<I", rom, address - ROM_BASE)[0]


def sample_wav(rom: bytes, sample: int, path: Path) -> float:
    """Write one sample as a WAV. Returns its length in seconds."""
    header = u32(rom, SOUND_DATA + sample * 4)
    end, rate = u32(rom, header + 4), u32(rom, header + 8)
    pcm = rom[header + 16 - ROM_BASE:end - ROM_BASE]
    path.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(path), "wb") as out:
        out.setnchannels(1)
        out.setsampwidth(1)
        out.setframerate(rate)
        out.writeframes(pcm)
    return len(pcm) / rate


def describe(rom: bytes, sfx: int, label: str = "") -> None:
    pairs = hero_registry.sfx_variants(rom, sfx)
    counts = {}
    for sound, _ in pairs:
        counts[sound] = counts.get(sound, 0) + 1
    print(f"sfx {sfx}{label}: " + (", ".join(f"sample {s} x{n}" for s, n in counts.items()) or "silent"))
    for sound in counts:
        seconds = sample_wav(rom, sound, OUT / f"sfx{sfx}_sample{sound}.wav")
        print(f"    sample {sound}: {seconds:.2f} s  build/preview/sounds/sfx{sfx}_sample{sound}.wav")


def enemy_sets(rom: bytes) -> dict[str, tuple[int, ...]]:
    sets = {}
    for unit_type, (body, _) in enumerate(new_hero.enemy_rows()):
        if body and body not in sets:
            sets[body] = struct.unpack_from("<6H", rom, new_hero.ENEMY_SOUNDS - ROM_BASE + unit_type * 12)
    return sets


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("sfx", nargs="*", type=int, help="sfx ids to write")
    parser.add_argument("--hero", help="a hero's sound row, from heroes.json")
    parser.add_argument("--enemy", help="an enemy's sound set (sprite set name, see new_hero.py --list)")
    parser.add_argument("--find", type=int, help="which sfx ids and enemies play this sample")
    args = parser.parse_args()
    rom = (ROOT / "rotk.gba").read_bytes()
    for sfx in args.sfx:
        describe(rom, sfx)
    if args.hero:
        heroes = json.loads((ROOT / "hacks/content/heroes.json").read_text())
        hero = next(h for h in heroes if h.get("name", "").lower() == args.hero.lower() or str(h["id"]) == args.hero)
        row = hero.get("sounds")
        if row is None:
            row = list(struct.unpack_from("<4H", rom, 0x080585D4 - ROM_BASE + hero["stockHero"] * 8))
        for slot, (name, sound) in enumerate(zip(("every swing", "hurt", "death", "spare"), row)):
            if isinstance(sound, int):
                describe(rom, sound, f"  [{slot}] {name}")
            else:
                print(f"[{slot}] {name}: mix {sound['mix']}")
                for sfx, count, *sample in sound["mix"]:
                    describe(rom, sfx, f"  ({count} of 8" + (f", sample {sample[0]} only)" if sample else ")"))
    if args.enemy:
        sets = enemy_sets(rom)
        if args.enemy not in sets:
            raise SystemExit(f"No enemy {args.enemy}: see new_hero.py --list")
        for name, sfx in zip(ENEMY_SET, sets[args.enemy]):
            describe(rom, sfx, f"  {name}")
    if args.find is not None:
        users = [s for s in range(hero_registry.SFX_ENTRY_COUNT)
                 if any(sound == args.find for sound, _ in hero_registry.sfx_variants(rom, s))]
        print(f"sample {args.find} plays in sfx {users}")
        for body, values in enemy_sets(rom).items():
            hits = [name for name, sfx in zip(ENEMY_SET, values) if sfx in users]
            if hits:
                print(f"    {body}: {', '.join(hits)}")
        sample_wav(rom, args.find, OUT / f"sample{args.find}.wav")
        print(f"    build/preview/sounds/sample{args.find}.wav")
    if not (args.sfx or args.hero or args.enemy or args.find is not None):
        parser.print_help()


if __name__ == "__main__":
    main()
