"""Generate native hero tables and C registrations from the hero manifest."""

import argparse
import json
from pathlib import Path
import re
import struct

import hero_data
import hero_kit

STOCK_NAMES = ("Frodo", "Legolas", "Aragorn", "Gandalf", "Eowyn", "Gimli", "Sam", "Smeagol")

# sfx_play's tables (include/sfx.h). Hero sound mixes are new SfxEntries ids past the stock ones,
# each a new SfxVariantSets row.
SFX_ENTRIES = 0x08289450
SFX_ENTRY_COUNT = 296
SFX_SINGLES = 0x08287DF8
SFX_VARIANT_SETS = 0x08288010
SFX_VARIANT_SET_COUNT = 162
SFX_SILENT = 0xFF


def sfx_variants(rom: bytes, sfx: int) -> list[tuple[int, int]]:
    """The (sound, volume) pairs one stock sfx id plays from, silent slots left out."""
    if not 0 <= sfx < SFX_ENTRY_COUNT:
        raise ValueError(f"No stock sound effect {sfx}")
    kind, index = struct.unpack_from("<2H", rom, SFX_ENTRIES - 0x08000000 + sfx * 4)
    if kind == 1:
        return [struct.unpack_from("<2H", rom, SFX_SINGLES - 0x08000000 + index * 4)]
    if kind == 2:
        row = [struct.unpack_from("<2H", rom, SFX_VARIANT_SETS - 0x08000000 + index * 32 + v * 4) for v in range(8)]
        return [pair for pair in row if pair[0] != SFX_SILENT]
    return []


def sound_mixes(rom: bytes, heroes: list[dict]) -> list[list[tuple[int, int]]]:
    """Resolve each hero sound given as {"mix": [[sfx, count], ...]} (counts adding up to 8, a part
    [sfx, count, sample] keeps one sample of the sfx) to a
    new sfx id, in place. Returns the new variant rows in id order."""
    rows = []
    for hero in heroes:
        sounds = hero.get("sounds")
        if sounds is None:
            continue
        if len(sounds) != 4:
            raise ValueError(f"Hero {hero['id']}: sounds needs 4 entries")
        for slot, sound in enumerate(sounds):
            if isinstance(sound, int):
                if not 0 <= sound < SFX_ENTRY_COUNT:
                    raise ValueError(f"Hero {hero['id']}: no stock sound effect {sound}")
                continue
            parts = sound.get("mix") if isinstance(sound, dict) else None
            if not parts or sum(part[1] for part in parts) != 8:
                raise ValueError(f"Hero {hero['id']}: a sound mix needs [sfx, count] parts adding up to 8")
            row = []
            # A part [sfx, count, sample] takes only that sample's variants of the sfx.
            for sfx, count, *sample in parts:
                pool = [v for v in sfx_variants(rom, sfx) if not sample or v[0] == sample[0]]
                if not pool:
                    raise ValueError(f"Hero {hero['id']}: sound effect {sfx} has no sample {sample or ''}")
                row += [pool[i % len(pool)] for i in range(count)]
            sounds[slot] = SFX_ENTRY_COUNT + len(rows)
            rows.append(row)
    return rows


def load(root: Path, pc: bool) -> list[dict]:
    """The registered heroes of one edition: PC-only heroes come last and only in the PC edition."""
    heroes = json.loads((root / "hacks/content/heroes.json").read_text())
    pc_only = [hero.get("pcOnly", False) for hero in heroes]
    if pc_only != sorted(pc_only):
        raise ValueError("PC-only heroes must come after every hero of both editions")
    heroes = [hero for hero in heroes if pc or not hero.get("pcOnly", False)]
    # A hero with a skill spec plays the kit hero_kit.py compiles from it (PC edition).
    for hero in heroes:
        if pc and "kit" not in hero and hero.get("name") and hero_kit.spec_path(root, hero["name"]).exists():
            hero["kit"] = hero_kit.slug(hero["name"]) + "HeroKit"
            # Linked into its fixed slot at MEM_HERO_KITS (hero_kit.py), not into the modcode.
            hero["specKit"] = True
    return heroes


def stock_stats(root: Path) -> list[list[int]]:
    """The stock heroes' starting stats, read from player_initHeroStats (a switch, not a table)."""
    source = (root / "src/c/player_initHeroStats.c").read_text()
    stats = []
    for name in STOCK_NAMES:
        # The last case ends at the switch's closing brace instead of a break.
        block = re.search(rf"case HERO_ID_{name.upper()}:(.*?)(?:break;|\}})", source, re.S)
        if block is None:
            raise ValueError(f"No stats for {name} in player_initHeroStats.c")
        values = dict(re.findall(r"baseStats\.(\w+) = (\d+);", block.group(1)))
        stats.append([int(values[field]) for field in ("courage", "defense", "health", "accuracy", "strength")])
    return stats


def write_menu(root: Path, output: Path, heroes: list[dict]) -> None:
    """The hero select's data: group, description, portrait and starting stats per hero."""
    groups = list(dict.fromkeys(hero.get("group", "Heroes") for hero in heroes))
    stock = stock_stats(root)
    rows = []
    variables = root / "build/include/variables.h"
    known = set(re.findall(r"DialogPortrait(\w+?)BgAsset", variables.read_text())) if variables.exists() else None
    for hero in heroes:
        portrait = hero.get("portrait", STOCK_NAMES[hero["stockHero"]])
        if not re.fullmatch(r"[A-Za-z]+", portrait) or (known is not None and portrait not in known):
            raise ValueError(f"Hero {hero['id']}: no dialog portrait {portrait}")
        # The hero select shows two lines of about 22 characters.
        if len(hero.get("description", "")) > 44:
            raise ValueError(f"Hero {hero['id']}: description longer than 44 characters")
        stats = (
            f"&{hero['stats']}" if "stats" in hero else
            "&(const HeroBaseStats){0, " + ", ".join(map(str, stock[hero["id"]])) + "}"
        )
        rows.append(
            "{" + f"{json.dumps(hero.get('description', ''))}, {groups.index(hero.get('group', 'Heroes'))}, "
            f"DialogPortrait{portrait}BgAsset, {stats}" + "},\n"
        )
    (output / "hero_menu.inc").write_text("".join(rows))
    (output / "hero_groups.inc").write_text("".join(f"{json.dumps(group)},\n" for group in groups))
    (output / "hero_group_count.h").write_text(f"#pragma once\n#define HERO_GROUP_COUNT {len(groups)}\n")


def generate(root: Path, pc: bool = False) -> None:
    heroes = load(root, pc)
    if not 8 <= len(heroes) <= hero_data.MAX_HEROES:
        raise ValueError(f"8..{hero_data.MAX_HEROES} heroes: hero kit slots and body slots exist for that many")
    for index, hero in enumerate(heroes):
        if hero["id"] != index or not 0 <= hero["stockHero"] < 8:
            raise ValueError(
                "Hero IDs must be consecutive and stockHero must be a native hero"
            )
        if not 0 <= hero["equipmentHero"] < 8 or not 0 <= hero["palette"] < 256:
            raise ValueError("Invalid equipment hero or palette")
        if not 0 <= hero.get("lootHero", hero["stockHero"]) < 8:
            raise ValueError("Invalid loot hero")
        if not 0 <= hero["skillBlock"] < 8:
            raise ValueError("Invalid stock skill block")
        # A hero without a kit plays its stockHero's stock skills and has no page 2.
        if index >= 8 and "stats" not in hero:
            raise ValueError("An expanded hero requires base stats")
        for field in (
            "stats",
            "kit",
            "move",
            "melee",
            "cast",
            "menuIdle",
            "menuSelected",
        ):
            if field in hero and not re.fullmatch(
                r"[A-Za-z_][A-Za-z_0-9]*", hero[field]
            ):
                raise ValueError(f"Invalid symbol in {field}")
    output = root / "build/hack_include"
    output.mkdir(parents=True, exist_ok=True)
    (output / "hero_count.h").write_text(
        f"#pragma once\n#define HACK_HERO_COUNT {len(heroes)}\n"
    )
    # A hero with a "body" keeps its banks and hero select animations in the hero data region.
    def menu(hero: dict, field: str, offset: int) -> str:
        if "body" in hero:
            return f"(const SpriteAnimation *)0x{hero_data.body_slot(root, hero['id']) + offset:08x}"
        return hero.get(field, "NULL")

    definitions = []
    kits = []
    for hero in heroes:
        pointer = lambda field: "&" + hero[field] if field in hero else "NULL"
        definitions.append(
            "{"
            + f"{hero['stockHero']}, {hero['equipmentHero']}, {hero.get('lootHero', hero['stockHero'])}, "
            + f"{hero['palette']}, "
            f"{int(hero.get('detachHands', False))}, {json.dumps(hero.get('name')) if hero.get('name') else 'NULL'}, "
            f"{pointer('stats')}, "
            f"{menu(hero, 'menuIdle', hero_data.MENU_IDLE)}, {menu(hero, 'menuSelected', hero_data.MENU_SELECTED)}, "
            f"{json.dumps(hero['shortName']) if hero.get('shortName') else 'NULL'}"
            + "},"
        )
        # A spec kit has no modcode symbol: skillExpansion_getHeroKit finds it in its slot.
        kits.append(f"HERO_KIT({hero['kit']})," if "kit" in hero and not hero.get("specKit") else "NULL,")
    (output / "hero_definitions.inc").write_text("\n".join(definitions) + "\n")
    (output / "hero_kits.inc").write_text("\n".join(kits) + "\n")
    (output / "hero_kit_declarations.h").write_text(
        "\n".join(
            f"extern const SkillExpansionHeroKit {hero['kit']};"
            for hero in heroes
            if "kit" in hero and not hero.get("specKit")
        )
        + "\n"
    )
    (output / "hero_stat_declarations.h").write_text(
        "\n".join(
            f"extern const HeroBaseStats {hero['stats']};"
            for hero in heroes
            if "stats" in hero
        )
        + "\n"
        + "\n".join(
            f"extern const SpriteAnimation {hero[field]}[];"
            for hero in heroes
            for field in ("menuIdle", "menuSelected")
            if field in hero and "body" not in hero
        )
        + "\n"
    )
    def stock_table(prefix: str) -> str:
        return "".join(
            "NULL,\n" if "kit" in hero and index >= 8 else f"{prefix}{STOCK_NAMES[hero['stockHero']]},\n"
            for index, hero in enumerate(heroes)
        )

    (output / "hero_stock_skills.inc").write_text(stock_table("ActiveSkills"))
    (output / "hero_stock_passives.inc").write_text(stock_table("PassiveSkillIds"))
    # Permanent save identities of the active rows: (id + 1) << 8 | row, row 6 unused. Heroes with
    # "legacyIds": false keep all zeros (their kit supplies its own identities).
    legacy = []
    for index, hero in enumerate(heroes):
        if hero.get("legacyIds", True):
            ids = [0 if row == 6 else ((index + 1) << 8) | (row + 1) for row in range(13)]
        else:
            ids = [0] * 13
        legacy.append("{" + ", ".join(f"0x{i:x}" for i in ids) + "},")
    (output / "hero_legacy_ids.inc").write_text("".join(line + "\n" for line in legacy))
    # Page 2 passives of heroes past the stock 8 without a kit: none.
    (output / "hero_page2_none.inc").write_text(
        "".join(
            f"[{index}] = {{" + ", ".join(["SKILL_EXPANSION_PASSIVE_ID_NONE"] * 9) + "},\n"
            for index, hero in enumerate(heroes)
            if index >= 8 and "kit" not in hero
        )
    )
    (output / "hero_skill_blocks.inc").write_text(
        ", ".join(str(hero["skillBlock"]) for hero in heroes) + "\n"
    )
    write_menu(root, output, heroes)
    hero_data.write(root, heroes, output)
    hero_kit.generate(root, heroes, root / hero_kit.SOURCE)
    # These native table views and jump-table addresses are the existing Ringwraith hooks.
    tables = (
        ("HeroMoveAnimationBanks", 4, "move"),
        ("HeroMeleeAnimationBanks", 4, "melee"),
        ("HeroCastAnimationBanks", 4, "cast"),
        ("HeroPortraitBgAssets", 4, None),
        ("SkillTreeHeroTileBgAssets", 4, None),
        ("LootHeroRegionProgressions", 4, None),
        ("LootHeroItemTypeRolls", 16, None),
        ("HeroStartMissions", 1, None),
        ("HeroStartVariants", 1, None),
        ("StartItemsCases", 4, None),
        ("BackpackCases", 4, None),
        ("SpawnOffsets", 8, None),
        ("CameraHeights", 1, None),
    )
    native = {
        "StartItemsCases": "0x08026460",
        "BackpackCases": "0x0802a578",
        "SpawnOffsets": "0x0805858c",
        "CameraHeights": "0x080585cc",
    }
    lines = [f"HACK_HERO_LAST equ {len(heroes) - 1}"]
    for name, stride, field in tables:
        source = native.get(name, name)
        label = "Ringwraith" + name
        lines += [".align 4", label + ":"]
        for hero in heroes:
            if field and "body" in hero:
                lines.append(f"    .word 0x{hero_data.body_slot(root, hero['id']) + hero_data.BANK_OFFSETS[field]:08x}")
            elif field and field in hero:
                lines.append(f"    .word {hero[field]}")
            else:
                source_hero = (
                    hero.get("lootHero", hero["stockHero"])
                    if name == "LootHeroItemTypeRolls"
                    else hero["stockHero"]
                )
                lines.append(
                    f'    .incbin "rotk.gba", {source} - 0x08000000 + {source_hero * stride}, {stride}'
                )
    lines += [
        "HeroPaletteIds:",
        ".byte " + ", ".join(str(h["palette"]) for h in heroes),
        "HeroEquipmentIds:",
        ".byte " + ", ".join(str(h["equipmentHero"]) for h in heroes),
        ".align 2",
    ]
    (root / "build/hero_tables.asm").write_text("\n".join(lines) + "\n")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pc", action="store_true", help="include the PC-only heroes")
    generate(Path(__file__).resolve().parents[2], parser.parse_args().pc)
