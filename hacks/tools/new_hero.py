"""Make an enemy playable: a PC-only hero with the enemy's body, ready for its own kit.

    uv run python hacks/tools/new_hero.py "Witch-king" --enemy WitchKing --base Gandalf
    uv run python hacks/tools/new_hero.py --list

Writes hacks/patches/heroes/<slug>.c (body banks and base stats) and appends the hero to
hacks/content/heroes.json. Until the hero has a kit it plays its base hero's stock skills with no
page 2. That is a placeholder for testing the body, not the hero's real skills.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import re
import struct

import hero_clips

ROOT = Path(__file__).resolve().parents[2]
BANKS = ROOT / "carved/data/enemy_graphics/EnemyAnimationBanks.c"
CLIPS = ROOT / "carved/data/enemy_graphics"
HEROES = ROOT / "hacks/content/heroes.json"
OUT = ROOT / "hacks/patches/heroes"
# Byte per enemy type: its ObjPalettes index (read by actor_applyTypeDefaults).
ENEMY_PALETTES = 0x0806D27C
# EnemySfxSets: per enemy type, 6 u16 (attack, aggro, hit, crit, death, miss).
ENEMY_SOUNDS = 0x08057B94
# A stock sfx id that is only the blade whoosh of the stock heroes' swings.
SFX_WHOOSH = 102
STOCK = ("Frodo", "Legolas", "Aragorn", "Gandalf", "Eowyn", "Gimli", "Sam", "Smeagol")
# The stock skill block each stock hero plays (heroes.json skillBlock).
SKILL_BLOCKS = (0, 3, 4, 5, 6, 7, 1, 2)
# EnemyAnimationBanks pose slots.
SLOTS = ("walk", "stand", "death", "stagger", "stunned", "attack")
MULTI_PIECE = re.compile(r"Part\d|AttachedPart|FlagPart")
HUGE = {"MountainTroll", "Mumakil", "Grond", "Shelob"}
FLIERS = {"Crebain", "Bat", "FlySwarm", "FlyingNazgul"}


def enemy_rows() -> list[tuple[str, dict[str, str]]]:
    """(sprite set, pose slot -> bank symbol) per enemy type, in EnemyType order."""
    rows = []
    for line in BANKS.read_text().splitlines():
        symbols = re.findall(r"EnemyAnimationBank_EnemyType(\w+?)_(\w+)", line)
        if len(symbols) == len(SLOTS):
            sets = {name for name, _ in symbols}
            body = symbols[SLOTS.index("walk")][0] if len(sets) > 1 else symbols[0][0]
            rows.append((body, {slot: f"EnemyAnimationBank_EnemyType{n}_{c}" for slot, (n, c) in zip(SLOTS, symbols)}))
        elif "{" in line and "}" in line and "EnemyType" not in line:
            rows.append(("", {}))
    return rows


def clips(body: str) -> list[str]:
    files = list(CLIPS.glob(f"EnemyAnimationBanks_*_EnemyType{body}.c"))
    if not files:
        return []
    return sorted(set(re.findall(rf"EnemyAnimationBank_EnemyType{body}_(\w+)\[", files[0].read_text())))


def find(body: str) -> tuple[int, dict[str, str]]:
    for index, (name, slots) in enumerate(enemy_rows()):
        if name == body:
            return index, slots
    raise SystemExit(f"No enemy type uses the {body} sprites. See --list.")


def warnings(body: str, names: list[str]) -> list[str]:
    found = []
    if any(MULTI_PIECE.match(name) for name in names):
        found.append("drawn in several pieces: only the main piece shows (multi-piece bodies are not supported yet)")
    if body in HUGE:
        found.append("huge sprite: check collision, doors and the camera in play")
    if body in FLIERS:
        found.append("a flier: it will walk on the ground like a hero")
    return found


def slug(name: str) -> str:
    return re.sub(r"[^A-Za-z0-9]", "", name.title())


def body_source(symbol: str, name: str, stats: list[int]) -> str:
    return f"""// {name}: a playable enemy body made by hacks/tools/new_hero.py. Its animations are "body" in
// hacks/content/heroes.json (hacks/tools/hero_clips.py). Edit the stats freely.
#include "patches/ringwraith/hero_registry.h"

#if PC_EDITION

const HeroBaseStats {symbol}BaseStats = {{
    .courage = {stats[0]},
    .defense = {stats[1]},
    .health = {stats[2]},
    .accuracy = {stats[3]},
    .strength = {stats[4]},
}};

#endif
"""


def default_body(enemy: str, slots: dict[str, str]) -> dict:
    """Stock-like poses from the enemy's clips: one swing per press, and a still walk frame when the
    enemy has no stand clip."""
    prefix = f"EnemyAnimationBank_EnemyType{enemy}_"
    short = lambda slot: slots[slot].removeprefix(prefix)
    stand = {"clip": short("walk"), "frames": [0, 0]} if still_idle(slots) else short("stand")
    hurt = short("stagger")
    return {"enemy": enemy,
            "move": [hurt, short("death"), hurt, hurt, stand, hurt, short("walk")],
            "swings": [short("attack")],
            "cast": [{"clip": short("attack"), "hit": False}] * 7}


def still_idle(slots: dict[str, str]) -> bool:
    """Whether the enemy stands with its walk clip (it has no stand clip of its own)."""
    return slots["stand"] == slots["walk"]


def portraits() -> set[str]:
    """Dialog portrait names (DialogPortrait<Name>BgAsset) the ROM carries."""
    text = (ROOT / "build/include/variables.h").read_text()
    return set(re.findall(r"DialogPortrait(\w+?)BgAsset", text))


def default_portrait(body: str, known: set[str]) -> str:
    """The enemy's own portrait, else one from its kind."""
    if body in known:
        return body
    for prefix, portrait in (("Nazgul", "WitchKing"), ("FlyingNazgul", "WitchKing"),
                             ("OrcCaptain", "OrcCaptain"), ("Orc", "Orc"), ("Goblin", "Orc"),
                             ("Uruk", "Orc"), ("Skeleton", "KingOfTheDead"), ("Ghost", "KingOfTheDead")):
        if body.startswith(prefix) and portrait in known:
            return portrait
    return "Orc"


def write_heroes(heroes: list[dict]) -> None:
    HEROES.write_text("[\n" + ",\n".join("  " + json.dumps(hero) for hero in heroes) + "\n]\n")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("name", nargs="?", help="the hero's name as players see it")
    parser.add_argument("--enemy", help="sprite set, e.g. WitchKing (see --list)")
    parser.add_argument("--base", default="Aragorn", choices=STOCK,
                        help="stock hero for equipment, loot and the placeholder skills")
    parser.add_argument("--stats", default="24,24,24,24,24",
                        help="courage,defense,health,accuracy,strength (stock heroes use about 20..36)")
    parser.add_argument("--group", default="Servants of Sauron", help="hero select group (its tab)")
    parser.add_argument("--description", default="", help="hero select text, two short lines (about 40 characters)")
    parser.add_argument("--portrait", help="dialog portrait name, e.g. Denethor (default: the enemy's own, else its kind's)")
    parser.add_argument("--remove", action="store_true",
                        help="unregister NAME and delete its body file (only the last hero: IDs are saved)")
    parser.add_argument("--dry-run", action="store_true", help="print what would be written")
    parser.add_argument("--list", action="store_true", help="list the enemy sprite sets and their animations")
    args = parser.parse_args()

    if args.list:
        seen = set()
        for body, _ in enemy_rows():
            if body and body not in seen:
                seen.add(body)
                names = clips(body)
                notes = "; ".join(warnings(body, names))
                print(f"{body:18} {', '.join(n for n in names if not MULTI_PIECE.match(n))}"
                      + (f"   [{notes}]" if notes else ""))
        return
    if args.remove:
        heroes = json.loads(HEROES.read_text())
        if not args.name or not heroes or heroes[-1].get("name") != args.name:
            raise SystemExit("--remove takes the last registered hero's name (hero IDs are saved)")
        path = OUT / f"{slug(args.name).lower()}.c"
        if path.exists():
            path.unlink()
        write_heroes(heroes[:-1])
        print(f"removed {args.name}")
        return
    if not args.name or not args.enemy:
        parser.error("give a name and --enemy (or --list)")
    if len(args.description) > 44:
        parser.error("--description is longer than two lines of the hero select (44 characters)")

    unit_type, slots = find(args.enemy)
    palette = (ROOT / "rotk.gba").read_bytes()[ENEMY_PALETTES - 0x08000000 + unit_type]
    stats = [int(x, 0) for x in args.stats.split(",")]
    if len(stats) != 5:
        parser.error("--stats needs 5 numbers")
    symbol = slug(args.name)
    heroes = json.loads(HEROES.read_text())
    if any(hero.get("name") == args.name for hero in heroes):
        raise SystemExit(f"{args.name} is already registered")
    base = STOCK.index(args.base)
    known = portraits()
    portrait = args.portrait or default_portrait(args.enemy, known)
    if portrait not in known:
        parser.error(f"no portrait {portrait}: one of {', '.join(sorted(known))}")
    rom = (ROOT / "rotk.gba").read_bytes()
    attack, _, hit, _, death, _ = struct.unpack_from("<6H", rom, ENEMY_SOUNDS - 0x08000000 + unit_type * 12)
    hero = {
        "id": len(heroes), "name": args.name, "stockHero": base, "equipmentHero": base,
        "skillBlock": SKILL_BLOCKS[base], "palette": palette, "stats": f"{symbol}BaseStats",
        "detachHands": True, "pcOnly": True,
        "group": args.group, "description": args.description, "portrait": portrait,
        # The hero sound row: every swing, hurt, death, spare (hero_registry.py HeroSounds). Like a
        # stock hero, a swing is mostly a blade whoosh with the enemy's attack voice 2 times in 8.
        "sounds": [{"mix": [[SFX_WHOOSH, 6], [attack, 2]]}, hit, death, hit],
    }
    hero["body"] = default_body(args.enemy, slots)
    hero_clips.body_banks(ROOT, rom, hero)  # checks the body
    source = body_source(symbol, args.name, stats)
    path = OUT / f"{symbol.lower()}.c"
    for note in warnings(args.enemy, clips(args.enemy)):
        print("warning:", note)
    if args.dry_run:
        print(json.dumps(hero))
        print(f"--- {path.relative_to(ROOT)}\n{source}")
        return
    if path.exists():
        raise SystemExit(f"{path} exists")
    OUT.mkdir(exist_ok=True)
    path.write_text(source)
    write_heroes(heroes + [hero])
    print(f"{args.name}: hero {hero['id']}, enemy type {unit_type}, palette {palette}, portrait {portrait}, "
          f"wrote {path.relative_to(ROOT)}")
    print("Next: build (make hack-pc), then give it a kit (docs/wip-docs/howto-heroes-skills.md).")


if __name__ == "__main__":
    main()
