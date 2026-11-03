"""Compile hero skill specs (hacks/content/heroes/<hero>.json) into kits for spec_kit.c.

A spec lists the hero's first page (6 skills, the last one in Herbal Healing's slot) and 9 stock
passives, and optionally page 2: 5 or 6 skills plus 9 more stock passives (page2Passives). Each skill is one of:

  bolt     a projectile: damage, speed, radius, pierce, splash, stun, status
  burst    a burst around the caster: damage, radius, stun, status
  blast    a burst at the nearest foe within range: damage, radius, range, stun, status
  heal     heal the caster: amount (HP per level)
  ward     take less damage: amount (percent per level), seconds
  empower  melee hits deal more: amount (percent per level), seconds

Per-level values are lists of 5 (skill levels 1..5). Descriptions are written from "text" with
{damage}, {amount}, {seconds} and {stun} filled in.

The kits are linked apart from the modcode, into fixed per hero slots at MEM_HERO_KITS
(SkillExpansionKitSlot, hacks/layout.cfg). hero_registry.py writes their C (generate), the build
links it against the modcode's symbols (link), and hero_fast.py does both again against the last
built game, so a spec change needs no rebuild:

    uv run python hacks/tools/hero_kit.py link build/hack.elf build/hero_kits.bin
"""

from __future__ import annotations

import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

SOUNDS = {"none": 0, "missile": 0x13, "fireburst": 0x0D, "stun": 0x09}
STATUSES = ("none", "poison", "burn", "slow")
CASTS = {"quick": "SpecKitQuickPolicy", "normal": "SpecKitNormalPolicy", "long": "SpecKitLongPolicy",
         "ultimate": "SpecKitUltimatePolicy"}
EFFECTS = {"bolt": 0, "burst": 0, "blast": 0, "heal": 1, "ward": 2, "empower": 3}
SKILL_FIELDS = {"name", "cost", "levels", "cast", "icon", "text", "effect", "fx", "damage", "speed",
                "radius", "pierce", "splash", "stun", "status", "statusSeconds", "statusPower", "travel",
                "impact", "sound", "range", "amount", "seconds", "lifetime", "knockback"}
FRAMES = 30
SOURCE = Path("build/generated/hero_spec_kits.c")
# The modcode's compiler flags, written by the build (make/hack.mk) so the kits compile the same way.
CFLAGS = Path("build/modcode_cflags.txt")
TOOLS = Path(os.environ.get("LOCALAPPDATA", "")) / "rotk-tools/arm-gcc/bin"


class SpecError(ValueError):
    pass


def slug(name: str) -> str:
    return re.sub(r"[^A-Za-z0-9]", "", name.title())


def spec_path(root: Path, name: str) -> Path:
    return root / "hacks/content/heroes" / f"{slug(name).lower()}.json"


def require(condition: bool, where: str, message: str) -> None:
    if not condition:
        raise SpecError(f"{where}: {message}")


def levels5(value, where: str, field: str, low: int, high: int) -> list[int]:
    require(isinstance(value, list) and len(value) == 5 and all(isinstance(v, int) for v in value),
            where, f"{field} needs 5 numbers, one per skill level")
    require(all(low <= v <= high for v in value), where, f"{field} values must be {low}..{high}")
    return value


def catalog(root: Path) -> dict:
    """Names a spec may use: icons, fx shapes and colors, delivery effects, passives."""
    icons = {}
    for header in (root / "hacks/patches").glob("*/*_icons.h"):
        for symbol in re.findall(r"extern const \w+ (\w+Icon\w+)", header.read_text()):
            icons[symbol] = header.relative_to(root / "hacks").as_posix()
    fx = (root / "hacks/lib/fx_kit.h").read_text()
    effects = {e["key"]: e["id"] for e in json.loads((root / "hacks/content/skill_fx.json").read_text())["effects"]}
    passives = set(re.findall(r"PASSIVE_SKILL_(\w+) = \d+,", (root / "include/skill.h").read_text()))
    return {
        "icons": icons,
        "shapes": set(re.findall(r"FX_SHAPE_(\w+) = ", fx)),
        "colors": set(re.findall(r"FX_COLOR_(\w+)\b", fx)) - {"CUSTOM", "OBJ_PALETTE"},
        "effects": effects,
        "passives": passives - {"COUNT"},
    }


def describe(skill: dict) -> str:
    def span(values):
        return str(values[0]) if values[0] == values[-1] else f"{values[0]}-{values[-1]}"

    fills = {
        "damage": span(skill.get("damage", [0] * 5)),
        "amount": span(skill.get("amount", [0] * 5)),
        "seconds": str(skill.get("seconds", 0)),
        "stun": f"{skill.get('stun', 0)}s",
    }
    text = skill.get("text", "")
    for key, value in fills.items():
        text = text.replace("{" + key + "}", value)
    return f"{skill['name']} (Level @1) - {text} @2 Spirit."


def delivery(skill: dict, where: str, names: dict) -> str:
    effect = skill["effect"]
    damage = levels5(skill.get("damage"), where, "damage", 0, 9999)
    status = skill.get("status", "none")
    require(status in STATUSES, where, f"status must be one of {', '.join(STATUSES)}")
    power = levels5(skill.get("statusPower", [1] * 5 if status != "none" else [0] * 5), where, "statusPower", 0, 999)
    radius = skill.get("radius", 6 if effect == "bolt" else 48)
    require(1 <= radius <= 128, where, "radius must be 1..128")
    speed = skill.get("speed", 6 if effect == "bolt" else 0)
    require(effect != "bolt" or 1 <= speed <= 16, where, "a bolt needs speed 1..16")
    pierce = skill.get("pierce", 1)
    stun = int(skill.get("stun", 0) * FRAMES)
    for key in ("travel", "impact"):
        require(skill.get(key) is None or skill[key] in names["effects"], where,
                f"{key} must be one of {', '.join(names['effects'])}")
    sound = skill.get("sound", "missile" if effect == "bolt" else "fireburst")
    require(sound in SOUNDS, where, f"sound must be one of {', '.join(SOUNDS)}")
    travel = names["effects"].get(skill.get("travel", "preview_ward" if effect == "bolt" else None), 65535)
    impact = names["effects"].get(skill.get("impact", "preview_impact"), 65535)
    fields = {
        "damage": "{" + ",".join(map(str, damage)) + "}",
        "lifetime": skill.get("lifetime", 45 if effect == "bolt" else 1),
        "travelEffect": travel, "trailEffect": 65535, "impactEffect": impact,
        "launchSound": 0, "impactSound": SOUNDS[sound], "stunTicks": min(stun, 300),
        "kind": int(effect != "bolt"), "speed": speed, "radius": radius,
        "splashRadius": skill.get("splash", 0), "maxTargets": 8, "pierceTargets": pierce,
        "maxRepeats": 1, "repeatTicks": 15, "trailTicks": 6, "knockback": skill.get("knockback", 0),
        "stopOnWalls": 1, "elements": 0, "elementsKnown": 0, "status": STATUSES.index(status),
        "statusTicks": int(skill.get("statusSeconds", 0) * FRAMES), "statusPower": "{" + ",".join(map(str, power)) + "}",
        "cooldownTicks": 0,
    }
    require(status == "none" or fields["statusTicks"] > 0, where, "a status needs statusSeconds")
    return "{" + ", ".join(f".{k} = {v}" for k, v in fields.items()) + "}"


def compile_hero(root: Path, hero: dict, names: dict) -> tuple[str, set[str]]:
    path = spec_path(root, hero["name"])
    where = path.relative_to(root).as_posix()
    spec = json.loads(path.read_text())
    symbol = slug(hero["name"])
    hero_id = hero["id"]
    skills = spec.get("skills", [])
    page2 = spec.get("page2", [])
    require(len(skills) == 6, where, "skills needs 6 entries (the 6th takes Herbal Healing's slot)")
    # Page 2 is all or nothing: the skill tree shows it whole or not at all.
    require(len(page2) in (0, 5, 6), where, "page2 needs 5 or 6 skills, or none (no page 2)")
    passives = spec.get("passives", [])
    require(len(passives) == 9 and all(p in names["passives"] for p in passives), where,
            "passives needs 9 stock passive names, e.g. FEARLESS, HARDY, WISE")
    page2_passives = spec.get("page2Passives", [])
    require(len(page2_passives) == (9 if page2 else 0), where,
            "page2Passives needs 9 stock passive names when page2 has skills, none otherwise")
    require(all(p in names["passives"] for p in page2_passives), where, "unknown page2Passives name")
    require(len(set(passives + page2_passives)) == len(passives) + len(page2_passives), where,
            "a passive appears twice (each page's passives must be distinct)")
    headers = set()
    out = [f"// {hero['name']} ({where})"]
    rows = ["NULL"] * 13
    page2_defs = []
    for index, skill in enumerate(skills + page2):
        row = index if index < 6 else index + 1
        at = f"{where} skill {skill.get('name', index)}"
        require(set(skill) <= SKILL_FIELDS, at, f"unknown fields {sorted(set(skill) - SKILL_FIELDS)}")
        for key in ("name", "cost", "levels", "icon", "effect"):
            require(key in skill, at, f"needs {key}")
        require(skill["effect"] in EFFECTS, at, f"effect must be one of {', '.join(EFFECTS)}")
        require(skill["icon"] in names["icons"], at, "unknown icon (any *Icon* symbol in hacks/patches/*/*_icons.h)")
        headers.add(names["icons"][skill["icon"]])
        levels = levels5(skill["levels"], at, "levels", 1, 50)
        require(levels == sorted(levels), at, "levels must rise")
        require(skill.get("cast", "normal") in CASTS, at, f"cast must be one of {', '.join(CASTS)}")
        require(len(describe(skill)) <= 160, at, "description too long for the skill tree")
        fx = skill.get("fx")
        if fx is not None:
            require(fx.get("shape") in names["shapes"] and fx.get("color") in names["colors"], at,
                    "fx needs a shape (FX_SHAPE_*) and a color (FX_COLOR_*) from hacks/lib/fx_kit.h")
        name = f"{symbol}Row{row}"
        effect = skill["effect"]
        if effect in ("bolt", "burst", "blast"):
            out.append(f"static const SkillDeliveryDef {name}Delivery = {delivery(skill, at, names)};")
            deliver = f"&{name}Delivery"
            amount = [0] * 5
        else:
            deliver = "NULL"
            amount = levels5(skill.get("amount"), at, "amount", 1, 999)
            require(effect == "heal" or max(amount) <= 90, at, "ward and empower percents must be at most 90")
        frames = int(skill.get("seconds", 0) * FRAMES)
        require(effect not in ("ward", "empower") or frames > 0, at, "ward and empower need seconds")
        target = skill.get("range", 128) if effect == "blast" else 0
        require(0 <= target <= 255, at, "range must be 0..255")
        out.append(
            f"static const SpecSkill {name} = {{.delivery = {deliver}, .amount = {{{', '.join(map(str, amount))}}}, "
            f".frames = {frames}, .effect = {EFFECTS[effect]}, .targetRange = {target}, "
            f".fxShape = {'FX_SHAPE_' + fx['shape'] if fx else 0}, .fxColor = {'FX_COLOR_' + fx['color'] if fx else '0xff'}}};"
        )
        rows[row] = f"&{name}"
        out.append(f"static const ActiveSkill {name}Active = SPEC_KIT_CAST_ROW({int(skill['cost'])});")
        out.append(f"static const s8 {name}Levels[] = {{{', '.join(map(str, levels))}, -1}};")
        if index >= 6:
            page2_defs.append(
                f"static const SkillExpansionActiveDef {name}Def = {{.row = &{name}Active, .icon = {skill['icon']}, "
                f".requiredLevels = {name}Levels, .description = {json.dumps(describe(skill))}, "
                f".resolve = specKit_resolve, .baseRow = 0, .policy = &{CASTS[skill.get('cast', 'normal')]}, "
                f".id = 0x{((hero_id + 1) << 8) | (row + 1):x}}};"
            )
    out += page2_defs
    first = range(6)
    sym = lambda i: f"{symbol}Row{i}"
    out += [
        f"static const SpecSkill *const {symbol}SpecRows[13] = {{{', '.join(rows)}}};",
        f"static const ActiveSkill {symbol}Actives[HERO_ACTIVE_SKILL_COUNT] = {{"
        + ", ".join(f"SPEC_KIT_CAST_ROW({int(skills[i]['cost'])})" for i in first) + "};",
        f"static const void *const {symbol}Icons[HERO_ACTIVE_SKILL_COUNT] = {{"
        + ", ".join(skills[i]["icon"] for i in first) + "};",
        f"static const s8 {symbol}Levels[HERO_ACTIVE_SKILL_COUNT][SKILL_EXPANSION_REQUIRED_LEVEL_COUNT] = {{"
        + ", ".join("{" + ", ".join(map(str, skills[i]["levels"])) + ", -1}" for i in first) + "};",
        f"static const char *const {symbol}Descriptions[HERO_ACTIVE_SKILL_COUNT] = {{"
        + ", ".join(json.dumps(describe(skills[i])) for i in first) + "};",
        f"static const u16 {symbol}ActiveIds[HERO_ACTIVE_SKILL_COUNT] = {{"
        + ", ".join(f"0x{((hero_id + 1) << 8) | (i + 1):x}" for i in first) + "};",
        f"static const u8 {symbol}Passives[HERO_PASSIVE_SKILL_COUNT] = {{"
        + ", ".join(f"PASSIVE_SKILL_{p}" for p in passives) + "};",
    ]
    slot = [
        f"[{hero_id}] = {{.specRows = {symbol}SpecRows, .kit = {{",
        f"    .activeSkills = {symbol}Actives, .icons = {symbol}Icons, .requiredLevels = {symbol}Levels,",
        f"    .descriptions = {symbol}Descriptions, .passiveSkillIds = {symbol}Passives,",
        f"    .artHeroId = {hero['stockHero']}, .reset = specKit_reset, .cleanup = specKit_cleanup,",
        "    .tick = specKit_tick, .meleeDamage = specKit_meleeDamage, .incomingDamage = specKit_incomingDamage,",
        "    .resolvers = {specKit_resolve, specKit_resolve, specKit_resolve, specKit_resolve, specKit_resolve, specKit_resolve},",
        "    .policies = {" + ", ".join(f"&{CASTS[skills[i].get('cast', 'normal')]}" for i in first) + "},",
        f"    .activeIds = {symbol}ActiveIds,",
        "    .newActives = {" + ", ".join(
            f"&{sym(i + 7)}Def" if i < len(page2) else "NULL" for i in range(6)) + "},",
        "    .newPassiveSkillIds = {" + ", ".join(
            [f"PASSIVE_SKILL_{p}" for p in page2_passives] or ["SKILL_EXPANSION_PASSIVE_ID_NONE"] * 9) + "},",
        "}}",
    ]
    return "\n".join(out), "\n".join(slot), headers


def spec_heroes(root: Path, heroes: list[dict]) -> list[dict]:
    """The heroes that play a kit compiled from a spec (hero_registry.py marks them)."""
    return [hero for hero in heroes if hero.get("specKit") and spec_path(root, hero["name"]).exists()]


def generate(root: Path, heroes: list[dict], output: Path) -> None:
    """The kits' C source: their tables, then the slot array the linker puts at MEM_HERO_KITS."""
    names = catalog(root)
    bodies, headers, slots = [], set(), []
    for hero in spec_heroes(root, heroes):
        require(hero["id"] < 16, hero["name"], "kit slots exist for hero ids 0..15")
        body, slot, used = compile_hero(root, hero, names)
        bodies.append(body)
        slots.append("    " + slot.replace("\n", "\n    "))
        headers |= used
    # First in the region (hacks/hero_kits.ld): slot N is hero N's. Empty slots are zeros.
    array = ("const SkillExpansionKitSlot HeroKitSlots[SKILL_EXPANSION_KIT_SLOTS]\n"
             '    __attribute__((section(".hero_kit_slots"), used)) = ')
    text = "// Generated by hacks/tools/hero_kit.py from hacks/content/heroes/*.json.\n"
    text += '#include "patches/heroes/spec_kit.h"\n#include "lib/fx_kit.h"\n'
    # A first page row's own ActiveSkill and levels are used only when it moves to page 2.
    text += '#pragma GCC diagnostic ignored "-Wunused-const-variable"\n'
    text += "".join(f'#include "{h}"\n' for h in sorted(headers))
    text += "\n#if PC_EDITION && HACK_skill_expansion && HACK_ringwraith\n\n"
    text += "".join(body + "\n\n" for body in bodies)
    text += array + "{\n" + ",\n".join(slots) + "\n};\n"
    text += "#else\n" + array + "{{0}};\n#endif\n"
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(text, newline="\n")
    # Kits linked into the modcode came from this include.
    (root / "build/hack_include/hero_spec_kits.inc").unlink(missing_ok=True)


def tool(name: str) -> str:
    found = shutil.which(name) or (TOOLS / (name + ".exe") if (TOOLS / (name + ".exe")).exists() else None)
    if not found:
        raise SpecError(f"{name} not found (on PATH or in {TOOLS})")
    return str(found)


def link(root: Path, source: Path, elf: Path, cflags: Path) -> bytes:
    """The region's bytes: source compiled with the modcode's flags, linked at MEM_HERO_KITS
    against elf's symbols (the modcode it runs with)."""
    flags = cflags.read_text().split()
    gcc, objcopy = tool("arm-none-eabi-gcc"), tool("arm-none-eabi-objcopy")
    with tempfile.TemporaryDirectory() as temp:
        obj, out, binary = (Path(temp) / name for name in ("kits.o", "kits.elf", "kits.bin"))
        steps = [
            [gcc, *flags, str(source), "-o", str(obj)],
            [gcc, "-nostartfiles", "-nodefaultlibs", "-mthumb", "-mthumb-interwork", "-T", "hacks/hero_kits.ld",
             "-Wl,-L,build", f"-Wl,--just-symbols={elf}", "-o", str(out), str(obj)],
            [objcopy, "-O", "binary", "-j", ".kits", str(out), str(binary)],
        ]
        for step in steps:
            result = subprocess.run(step, cwd=root, capture_output=True, text=True)
            if result.returncode:
                raise SpecError(f"{Path(step[0]).name} failed:\n{result.stdout}{result.stderr}")
        return binary.read_bytes()


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description="Link the spec kits into the hero kits region.")
    parser.add_argument("command", choices=["link"])
    parser.add_argument("elf", type=Path, help="the linked modcode (build/hack.elf)")
    parser.add_argument("output", type=Path, help="the region's bytes (build/hero_kits.bin)")
    args = parser.parse_args()
    here = Path(__file__).resolve().parents[2]
    args.output.write_bytes(link(here, here / SOURCE, args.elf.resolve(), here / CFLAGS))
    print(f"Wrote {args.output} ({args.output.stat().st_size} bytes)")
