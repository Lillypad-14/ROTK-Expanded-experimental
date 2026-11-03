"""Check every hero in heroes.json in a second, before building: errors stop a build, warnings are
things that play badly (an idle that walks, a scream on every swing, a swing that deals no damage).

    uv run python hacks/tools/hero_check.py              # all heroes
    uv run python hacks/tools/hero_check.py Witch-king   # one hero

Exit status 1 when there is an error.
"""

from __future__ import annotations

import argparse
from pathlib import Path
import re
import struct
import sys

import hero_clips
import hero_data
import hero_kit
import hero_registry

ROOT = Path(__file__).resolve().parents[2]
ROM_BASE = 0x08000000
SOUND_DATA = 0x082808D4
OBJ_PALETTE_COUNT = 90
# Warn when a sound that plays on every swing is a long voice line more often than stock heroes.
LONG_SWING_SECONDS = 0.6
STOCK_VOICE_SHARE = 3
LONG_HURT_SECONDS = 1.0
LONG_HURT_FRAMES = 30


FONT_STYLE_INFOS = 0x08058264
DESCRIPTION_FONT = 1  # FONT_STYLE_8PX_B
# hero_select.c: the description box, (SIDE_W - 2) * CELL - 4 pixels wide, DESCRIPTION_LINES lines.
DESCRIPTION_WIDTH = (14 - 2) * 8 - 4
DESCRIPTION_LINES = 2


def glyph_widths(rom: bytes, style: int) -> tuple[int, int, bytes]:
    asset = struct.unpack_from("<I", rom, FONT_STYLE_INFOS - ROM_BASE + style * 8)[0] - ROM_BASE
    first, last = rom[asset], rom[asset + 1]
    widths = asset + int.from_bytes(rom[asset + 8:asset + 12], "big")
    return first, last, rom[widths:widths + last - first + 1]


def wrap(rom: bytes, text: str, max_width: int, style: int) -> list[str]:
    """Lines as font_drawLinesInBox breaks them (src/c/font.c font_drawLine). A word that never
    fits stops the drawing, like the game."""
    first, last, widths = glyph_widths(rom, style)
    width_of = lambda c: widths[ord(c) - first] if first <= ord(c) <= last else widths[1]
    lines = []
    while text:
        width, end, index = 0, None, 0
        while index < len(text) and text[index] != "\n" and width <= max_width:
            if text[index] == " ":
                end = index
            width += width_of(text[index])
            index += 1
        if index < len(text) and text[index] == "\n":
            end = index
        elif index == len(text) and width <= max_width:
            end = index
        if end is None:
            lines.append(text)
            return lines + ["<does not fit>"]
        lines.append(text[:end])
        text = text[end:].lstrip("\n").lstrip(" ")
    return lines


def seconds(rom: bytes, sample: int) -> float:
    header = struct.unpack_from("<I", rom, SOUND_DATA - ROM_BASE + sample * 4)[0]
    end, rate = struct.unpack_from("<2I", rom, header + 4 - ROM_BASE)
    return (end - header - 16) / rate


def frames(clip: hero_clips.Clip) -> int:
    return len(hero_clips.split_frames(clip.facings[0].frames, 0)[1])


def game_frames(clip: hero_clips.Clip) -> int:
    return frames(clip) * (clip.facings[0].duration + 1)


def hits(clip: hero_clips.Clip) -> list[int]:
    return hero_clips.hit_frames(clip.facings[0].frames)


def check_body(rom: bytes, hero: dict, sources, errors: list, warnings: list) -> None:
    try:
        banks = hero_clips.body_banks(ROOT, rom, hero, sources)
    except (ValueError, KeyError) as error:
        errors.append(f"body: {error}")
        return
    move, melee, cast = banks["move"], banks["melee"], banks["cast"]
    if move[4] is move[6] or (move[4].stock and move[6].stock and move[4].label == move[6].label):
        warnings.append("body: idle is the walk clip, so the hero walks in place (use {\"clip\": \"Walk\", \"frames\": [0, 0]})")
    elif frames(move[4]) > 1 and move[4].facings[0].duration == 0:
        errors.append("body: idle has speed 0 with several frames")
    for pose in (0, 2, 4):
        if not hits(melee[pose]):
            errors.append(f"body: melee {hero_clips.MELEE_POSES[pose]} has no hit frame, so it deals no damage")
    for pose in (1, 3):
        if hits(melee[pose]):
            warnings.append(f"body: melee {hero_clips.MELEE_POSES[pose]} has a hit frame (a recovery that strikes again)")
        if game_frames(melee[pose]) > 12:
            warnings.append(f"body: melee {hero_clips.MELEE_POSES[pose]} is {game_frames(melee[pose])} game frames, "
                            "it replays a long clip after each swing")
    for pose, clip in enumerate(cast):
        if hits(clip):
            warnings.append(f"body: cast {pose} has hit frames {hits(clip)}, so casting also deals melee hits")
            break
    if game_frames(move[3]) > LONG_HURT_FRAMES:
        warnings.append(f"body: hurt pose is {game_frames(move[3])} game frames, a hit stuns the hero that long")


def check_sounds(rom: bytes, hero: dict, errors: list, warnings: list) -> None:
    if "sounds" not in hero:
        return
    try:
        hero_registry.sound_mixes(rom, [dict(hero, sounds=list(hero["sounds"]))])
    except ValueError as error:
        errors.append(f"sounds: {error}")
        return
    swing = hero["sounds"][0]
    parts = swing["mix"] if isinstance(swing, dict) else [[swing, 8]]
    long_share = 0
    for sfx, count, *sample in parts:
        pool = [v for v in hero_registry.sfx_variants(rom, sfx) if not sample or v[0] == sample[0]]
        long_share += count * sum(seconds(rom, s) > LONG_SWING_SECONDS for s, _ in pool) / max(1, len(pool))
    if long_share > STOCK_VOICE_SHARE:
        warnings.append(f"sounds: a long voice plays on {long_share:.0f} of 8 swings (stock heroes: 2-3), "
                        'try {"mix": [[102, 6], [<voice sfx>, 2]]}')
    hurt = hero["sounds"][1]
    if isinstance(hurt, int):
        pool = hero_registry.sfx_variants(rom, hurt)
        if pool and all(seconds(rom, s) > LONG_HURT_SECONDS for s, _ in pool):
            warnings.append(f"sounds: the hurt sound (sfx {hurt}) is a {seconds(rom, pool[0][0]):.1f} s line on every hit taken")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("hero", nargs="?", help="one hero's name or id")
    args = parser.parse_args()
    rom = (ROOT / "rotk.gba").read_bytes()
    heroes = hero_registry.load(ROOT, True)
    sources = hero_clips.Sources(ROOT, rom)
    variables = ROOT / "build/include/variables.h"
    portraits = set(re.findall(r"DialogPortrait(\w+?)BgAsset", variables.read_text())) if variables.exists() else None
    names = hero_kit.catalog(ROOT)
    failed = False
    try:
        hero_data.build(ROOT, heroes, rom)
    except ValueError as error:
        print(f"ERROR hero data region: {error}")
        failed = True
    for index, hero in enumerate(heroes):
        name = hero.get("name") or hero_registry.STOCK_NAMES[hero["stockHero"]]
        if args.hero and args.hero.lower() not in (name.lower(), str(hero["id"])):
            continue
        errors, warnings = [], []
        if hero["id"] != index:
            errors.append(f"id {hero['id']} should be {index} (ids are consecutive)")
        if not 0 <= hero.get("palette", 0) < OBJ_PALETTE_COUNT:
            errors.append(f"palette {hero['palette']} is not 0..{OBJ_PALETTE_COUNT - 1}")
        lines = wrap(rom, hero.get("description", ""), DESCRIPTION_WIDTH, DESCRIPTION_FONT)
        if len(lines) > DESCRIPTION_LINES:
            errors.append(f"description is cut off on the hero select: it shows {lines[:DESCRIPTION_LINES]}, "
                          f"loses {lines[DESCRIPTION_LINES:]}")
        portrait = hero.get("portrait")
        if portrait and portraits is not None and portrait not in portraits:
            errors.append(f"no dialog portrait {portrait}")
        if index >= 8 and "stats" not in hero:
            errors.append("an expanded hero needs base stats")
        if "body" in hero:
            check_body(rom, hero, sources, errors, warnings)
        check_sounds(rom, hero, errors, warnings)
        if index >= 8 and hero_kit.spec_path(ROOT, name).exists():
            try:
                hero_kit.compile_hero(ROOT, hero, names)
            except (ValueError, SystemExit, KeyError) as error:
                errors.append(f"skill spec: {error}")
        elif index >= 8 and "kit" not in hero:
            warnings.append("no kit: plays its base hero's stock skills with no page 2")
        status = "ERROR" if errors else "WARN " if warnings else "ok   "
        print(f"{status} {hero['id']:2} {name}")
        for line in errors:
            print(f"        error: {line}")
        for line in warnings:
            print(f"        warn:  {line}")
        failed |= bool(errors)
    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    main()
