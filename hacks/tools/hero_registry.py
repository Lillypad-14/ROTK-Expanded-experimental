"""Generate native hero tables and C registrations from the hero manifest."""

import json
from pathlib import Path
import re


def generate(root: Path) -> None:
    heroes = json.loads((root / "hacks/content/heroes.json").read_text())
    if not 8 <= len(heroes) <= 16:
        raise ValueError("The native hero save field supports 8..16 registered heroes")
    for index, hero in enumerate(heroes):
        if hero["id"] != index or not 0 <= hero["stockHero"] < 8:
            raise ValueError(
                "Hero IDs must be consecutive and stockHero must be a native hero"
            )
        if not 0 <= hero["equipmentHero"] < 8 or not 0 <= hero["palette"] < 256:
            raise ValueError("Invalid equipment hero or palette")
        if not 0 <= hero["skillBlock"] < 8:
            raise ValueError("Invalid stock skill block")
        if index >= 8 and ("kit" not in hero or "stats" not in hero):
            raise ValueError("An expanded hero requires a skill kit and base stats")
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
    definitions = []
    kits = []
    for hero in heroes:
        pointer = lambda field: "&" + hero[field] if field in hero else "NULL"
        definitions.append(
            "{" + f"{hero['stockHero']}, {hero['equipmentHero']}, {hero['palette']}, "
            f"{int(hero.get('detachHands', False))}, {json.dumps(hero.get('name')) if hero.get('name') else 'NULL'}, "
            f"{pointer('stats')}, "
            f"{hero.get('menuIdle', 'NULL')}, {hero.get('menuSelected', 'NULL')}" + "},"
        )
        kits.append(f"HERO_KIT({hero['kit']})," if "kit" in hero else "NULL,")
    (output / "hero_definitions.inc").write_text("\n".join(definitions) + "\n")
    (output / "hero_kits.inc").write_text("\n".join(kits) + "\n")
    (output / "hero_kit_declarations.h").write_text(
        "\n".join(
            f"extern const SkillExpansionHeroKit {hero['kit']};"
            for hero in heroes
            if "kit" in hero
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
            if field in hero
        )
        + "\n"
    )
    (output / "hero_skill_blocks.inc").write_text(
        ", ".join(str(hero["skillBlock"]) for hero in heroes) + "\n"
    )
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
            if field and field in hero:
                lines.append(f"    .word {hero[field]}")
            else:
                lines.append(
                    f'    .incbin "rotk.gba", {source} - 0x08000000 + {hero["stockHero"] * stride}, {stride}'
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
    generate(Path(__file__).resolve().parents[2])
