"""See a hero's or enemy's animations and sounds in a second, without building or running the game.

    uv run python hacks/tools/hero_preview.py Witch-king          # from heroes.json, no build
    uv run python hacks/tools/hero_preview.py Witch-king --gif    # plus one animated GIF per pose
    uv run python hacks/tools/hero_preview.py Ringwraith --built  # what build/rotk_pc.gba holds
    uv run python hacks/tools/hero_preview.py --enemy WitchKing   # every clip of an enemy
    uv run python hacks/tools/hero_preview.py --enemy WitchKing --clip Attack   # all 5 facings

Writes build/preview/<name>.png: frames numbered (the numbers "frames" and "hit" in a hero's body
take), hit frames boxed red. A hero with a "body" (hero_clips.py) is drawn from heroes.json, so a
change shows here before any build. Prints the hero's sound row: what each slot plays.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import struct
import sys

from PIL import Image, ImageDraw

import hero_clips
from hero_clips import Facing
import hero_registry
from rotkit.png import bgr555_to_rgb
from rotkit.spritegfx import decode_frame_tiles, frame_bounds, parse_frame_set, render_frame

ROOT = Path(__file__).resolve().parents[2]
ROM_BASE = hero_clips.ROM_BASE
# Literal pool words the build repoints at the hero tables (hacks/patches/ringwraith/ringwraith.asm).
TABLE_SITES = {"move": 0x08025AEC, "melee": 0x08025D5C, "cast": 0x08027430}
SOUND_TABLE_SITE = 0x08023D08
SFX_ENTRIES_SITE = 0x08046F18
SFX_VARIANTS_SITE = 0x08046F90
SPRITE_ANIMATION = 16
SOUND_SLOTS = ("every swing", "hurt", "death", "spare")
SCALE = 2
BACKGROUND = (48, 44, 52, 255)
TEXT = (255, 230, 140, 255)
HIT = (255, 60, 60, 255)
NUMBER = (150, 150, 170, 255)
LABEL_HEIGHT = 14


def u16(data: bytes, address: int) -> int:
    return struct.unpack_from("<H", data, address - ROM_BASE)[0]


def u32(data: bytes, address: int) -> int:
    return struct.unpack_from("<I", data, address - ROM_BASE)[0]


def obj_palette(rom: bytes, palette_id: int) -> tuple[int, ...]:
    table = next(int(line.split()[0], 16) for line in (ROOT / "config/data.cfg").read_text().splitlines()
                 if len(line.split()) > 1 and line.split()[1] == "ObjPalettes")
    return struct.unpack_from("<16H", rom, u32(rom, table + palette_id * 4) - ROM_BASE)


def enemy_palette(rom: bytes, enemy: str) -> int:
    import new_hero
    unit_type, _ = new_hero.find(enemy)
    return rom[new_hero.ENEMY_PALETTES - ROM_BASE + unit_type]


def render(rom: bytes, facing: Facing, colors: tuple[int, ...]) -> tuple[list[Image.Image], list[int]]:
    """A facing's frames as RGBA images and its hit frame indexes."""
    frame_set = parse_frame_set(facing.frames, ROM_BASE)
    bounds = frame_bounds(frame_set.frames)
    tiles = facing.tiles_data if facing.tiles_data is not None else rom[facing.tiles_address - ROM_BASE:]
    rgb = [c for color in colors for c in bgr555_to_rgb(color)]
    images = []
    for frame in frame_set.frames:
        indexed = render_frame(frame, decode_frame_tiles(tiles, frame), bounds, colors)
        image = Image.frombytes("P", (indexed.width, indexed.height), indexed.pixels)
        image.putpalette(rgb)
        image.info["transparency"] = 0
        images.append(image.convert("RGBA"))
    return images, hero_clips.hit_frames(facing.frames)


def built_facing(rom: bytes, animation: int) -> Facing:
    tiles, frames = u32(rom, animation), u32(rom, animation + 4)
    return Facing("", tiles, hero_clips.frame_set_bytes(rom, frames), rom[animation + 12 - ROM_BASE])


def sheet(rows: list[tuple[str, list[Image.Image], list[int]]], path: Path) -> None:
    cell = max((max(max(i.width, i.height) for i in images) for _, images, _ in rows if images), default=32)
    width = max(max((len(images) for _, images, _ in rows), default=1) * (cell + 2), 640)
    height = sum(LABEL_HEIGHT + (cell + 12 if images else 0) for _, images, _ in rows)
    out = Image.new("RGBA", (width, height), BACKGROUND)
    draw = ImageDraw.Draw(out)
    y = 0
    for title, images, hits in rows:
        draw.text((2, y + 1), title, fill=TEXT)
        y += LABEL_HEIGHT
        for k, image in enumerate(images):
            x = k * (cell + 2)
            out.alpha_composite(image, (x + (cell - image.width) // 2, y + (cell - image.height) // 2))
            if k in hits:
                draw.rectangle((x, y, x + cell, y + cell), outline=HIT)
            draw.text((x + 2, y + cell), str(k), fill=HIT if k in hits else NUMBER)
        if images:
            y += cell + 12
    path.parent.mkdir(parents=True, exist_ok=True)
    out.resize((out.width * SCALE, out.height * SCALE), Image.NEAREST).save(path)


PALETTE_IDS = 90
# ObjPalettes ids loaded into fixed OBJ slots at every level start: safe for skill colors.
FIXED_PALETTES = range(2, 9)


def palette_sheet(rom: bytes, sources, enemy: str, clip: str, facing: int, frame: int | None, out: Path) -> None:
    """One frame of a clip in every ObjPalettes id, labelled, fixed slots marked green: pick a skill
    color ("OBJ:<id>") in one look."""
    record = sources.facings(sources.symbol(enemy, clip))[facing]
    own = enemy_palette(rom, enemy)
    images = []
    for ident in range(PALETTE_IDS):
        frames, _ = render(rom, record, obj_palette(rom, ident))
        images.append(frames[frame if frame is not None else len(frames) // 2])
    cell = max(max(i.width, i.height) for i in images) + 4
    columns = 15
    sheet_image = Image.new("RGBA", (columns * cell, (PALETTE_IDS + columns - 1) // columns * (cell + LABEL_HEIGHT)),
                            BACKGROUND)
    draw = ImageDraw.Draw(sheet_image)
    for ident, image in enumerate(images):
        x, y = ident % columns * cell, ident // columns * (cell + LABEL_HEIGHT)
        fixed = ident in FIXED_PALETTES
        label = f"{ident}" + (" fixed" if fixed else "") + (" own" if ident == own else "")
        draw.text((x + 2, y), label, fill=(96, 224, 96) if fixed else TEXT)
        sheet_image.alpha_composite(image, (x + (cell - image.width) // 2, y + LABEL_HEIGHT + (cell - image.height) // 2))
    path = out / f"palettes_{enemy}_{clip}.png"
    path.parent.mkdir(parents=True, exist_ok=True)
    sheet_image.resize((sheet_image.width * 2, sheet_image.height * 2), Image.NEAREST).save(path)
    print(f"Wrote {path.relative_to(ROOT)}: {clip} facing {facing} in ObjPalettes 0..{PALETTE_IDS - 1}. Green ids "
          f"2..8 are fixed slots (always loaded, never clash); others borrow a shared slot. Use as \"OBJ:<id>\".")


def gif(images: list[Image.Image], duration: int, path: Path) -> None:
    w, h = max(i.width for i in images), max(i.height for i in images)
    frames = []
    for image in images:
        canvas = Image.new("RGBA", (w, h), BACKGROUND)
        canvas.alpha_composite(image, ((w - image.width) // 2, (h - image.height) // 2))
        frames.append(canvas.resize((w * 4, h * 4), Image.NEAREST))
    # A frame shows for frameDuration + 1 game frames at 60 per second.
    frames[0].save(path, save_all=True, append_images=frames[1:], loop=0,
                   duration=max(20, round((duration + 1) * 1000 / 60)))


def describe(images: list, hits: list[int], duration: int) -> str:
    text = f"{len(images)} frames, speed {duration} (~{len(images) * (duration + 1)} game frames)"
    return text + (f", hit on {hits}" if hits else "")


def sound_lines(rom: bytes, hero: dict, built: bool) -> list[str]:
    """What each sound slot plays: a built ROM's tables, else heroes.json resolved like the build."""
    if built:
        table, entries, variants = (u32(rom, s) for s in (SOUND_TABLE_SITE, SFX_ENTRIES_SITE, SFX_VARIANTS_SITE))
        row = [u16(rom, table + hero["id"] * 8 + 2 * slot) for slot in range(4)]
        lookup = lambda sfx: (u16(rom, entries + sfx * 4), u16(rom, entries + sfx * 4 + 2))
        variant = lambda index, v: u16(rom, variants + index * 32 + v * 4)
        mixes = []
    else:
        if "sounds" not in hero:
            return ["  (the stock hero's row; --built shows it)"]
        heroes = [json.loads(json.dumps(hero))]
        mixes = hero_registry.sound_mixes(rom, heroes)
        row = heroes[0]["sounds"]
        lookup = lambda sfx: (2, sfx) if sfx >= hero_registry.SFX_ENTRY_COUNT else struct.unpack_from(
            "<2H", rom, hero_registry.SFX_ENTRIES - ROM_BASE + sfx * 4)
        variant = None
    lines = []
    for slot, sfx in enumerate(row):
        kind, index = lookup(sfx)
        if not built and sfx >= hero_registry.SFX_ENTRY_COUNT:
            sounds = [s for s, _ in mixes[sfx - hero_registry.SFX_ENTRY_COUNT]]
        elif kind == 1:
            sounds = [u16(rom, hero_registry.SFX_SINGLES + index * 4)]
        elif built:
            sounds = [variant(index, v) for v in range(8)]
        else:
            sounds = [u16(rom, hero_registry.SFX_VARIANT_SETS + index * 32 + v * 4) for v in range(8)]
        counts = {}
        for sound in sounds:
            counts[sound] = counts.get(sound, 0) + 1
        mix = ", ".join(("silence" if s == 0xFF else f"sample {s}") + f" x{n}" for s, n in counts.items())
        lines.append(f"  [{slot}] {SOUND_SLOTS[slot]:12} sfx {sfx:3}: {mix}" + (" (always)" if kind == 1 else " (of 8)"))
    return lines


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("hero", nargs="?", help="hero name or id")
    parser.add_argument("--enemy", help="browse an enemy's clips instead of a hero")
    parser.add_argument("--clip", help="with --enemy: one clip, all 5 facings")
    parser.add_argument("--built", action="store_true", help="read build/rotk_pc.gba instead of heroes.json")
    parser.add_argument("--facing", type=int, default=2, help="facing 0..4 (default 2)")
    parser.add_argument("--gif", action="store_true", help="also write an animated GIF per pose")
    parser.add_argument("--palette", type=int, help="draw in this ObjPalettes index instead (try recolors)")
    parser.add_argument("--all-palettes", action="store_true",
                        help="with --enemy and --clip: one frame in every ObjPalettes id, one sheet (pick a color)")
    parser.add_argument("--frame", type=int, help="with --all-palettes: the frame (default the middle one)")
    args = parser.parse_args()
    rom = (ROOT / "rotk.gba").read_bytes()
    out = ROOT / "build/preview"
    sources = hero_clips.Sources(ROOT, rom)
    rows = []
    gifs = []

    if args.all_palettes:
        if not (args.enemy and args.clip):
            parser.error("--all-palettes needs --enemy and --clip")
        palette_sheet(rom, sources, args.enemy, args.clip, args.facing, args.frame, out)
        return
    if args.enemy:
        colors = obj_palette(rom, args.palette if args.palette is not None else enemy_palette(rom, args.enemy))
        prefix = f"EnemyAnimationBank_EnemyType{args.enemy}_"
        names = [args.clip] if args.clip else sorted(n.removeprefix(prefix) for n in sources.banks if n.startswith(prefix))
        for name in names:
            facings = sources.facings(sources.symbol(args.enemy, name))
            for n, facing in enumerate(facings):
                if not args.clip and n != args.facing:
                    continue
                images, hits = render(rom, facing, colors)
                rows.append((f"{name} facing {n}: {describe(images, hits, facing.duration)}", images, hits))
                gifs.append((f"enemy_{args.enemy}_{name}_{n}", images, facing.duration))
        stem = "enemy_" + args.enemy + (f"_{args.clip}" if args.clip else "")
    else:
        if not args.hero:
            parser.error("give a hero (name or id) or --enemy")
        heroes = json.loads((ROOT / "hacks/content/heroes.json").read_text())
        hero = next((h for h in heroes if str(h["id"]) == args.hero or h.get("name", "").lower() == args.hero.lower()), None)
        if hero is None:
            sys.exit(f"No hero {args.hero}: " + ", ".join(f"{h['id']} {h.get('name', '')}" for h in heroes))
        built = args.built or "body" not in hero
        stock_rom = rom
        if built:
            rom = (ROOT / "build/rotk_pc.gba").read_bytes()
        colors = obj_palette(rom, args.palette if args.palette is not None else hero["palette"])
        if built:
            banks = {}
            for bank, site in TABLE_SITES.items():
                address = u32(rom, u32(rom, site) + hero["id"] * 4)
                banks[bank] = [u32(rom, address + 4 * pose) for pose in range(hero_clips.POSES)]
        else:
            banks = hero_clips.body_banks(ROOT, rom, hero, sources)
        for bank in hero_clips.BANK_NAMES:
            seen = {}
            for pose, clip in enumerate(banks[bank]):
                title = f"{bank} {pose} {hero_clips.POSE_NAMES[bank][pose]}"
                key = clip if built else id(clip)
                if built and not clip:
                    rows.append((title + ": none", [], []))
                    continue
                if key in seen:
                    rows.append((f"{title}: same as {seen[key]}", [], []))
                    continue
                seen[key] = f"{bank} {pose}"
                facing = built_facing(rom, clip + args.facing * SPRITE_ANIMATION) if built else clip.facings[args.facing]
                images, hits = render(rom, facing, colors)
                source = "" if built else f" [{clip.label}]"
                rows.append((f"{title}{source}: {describe(images, hits, facing.duration)}", images, hits))
                gifs.append((f"{''.join(c for c in hero['name'] if c.isalnum())}_{bank}{pose}", images, facing.duration))
        stem = "".join(c for c in hero["name"] if c.isalnum())
        print(f"{hero['name']} (hero {hero['id']}), poses from {'build/rotk_pc.gba' if built else 'heroes.json'}")
        print("Sounds" + (" (build/rotk_pc.gba):" if args.built else " (heroes.json):"))
        print("\n".join(sound_lines(rom if args.built else stock_rom, hero, args.built)))

    path = out / f"{stem}.png"
    sheet(rows, path)
    if args.gif:
        for name, images, duration in gifs:
            if images:
                gif(images, duration, out / f"{name}.gif")
    print(f"Wrote {path.relative_to(ROOT)}" + (f" and {len(gifs)} GIFs" if args.gif else ""))


if __name__ == "__main__":
    main()
