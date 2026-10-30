"""Everything one Two Towers skill does, gathered for a port: a fact sheet.

    uv run python two-towers/tools/ttt_skill.py "Gimli Axe Throw"    (or gimli/axe, 5:6)
    uv run python two-towers/tools/ttt_skill.py --list

Writes build/ttt/skills/<hero>_<skill>.md and prints its path. The sheet has the skill's text, its
cast function and its effect function (from the hero's trigger, HERO_TRIGGERS below) decompiled with
the Two Towers Ghidra project (ttt_ghidra.py), then every Two Towers-only function they reach
(callbacks stored on the effect actors, calls), so a port reads one file. It lists the ROM
addresses the code uses (animations, palettes, tables) with any known names, and the numbers worth
checking: damage, timers, radii and speeds. Field offsets in the C are Two Towers actor offsets;
ACTOR_FIELD_NOTES maps the ones known to ROTK's.
"""

from __future__ import annotations

import argparse
import re

import ttt_names
from ttt_ghidra import Decompiler

OUT = ttt_names.ROOT / "build/ttt/skills"
# Calls into code the matcher named are ROTK engine code; the sheet stops there.
MAX_FUNCTIONS = 14
MAX_DEPTH = 3

# When a cast pose ends, player_tick calls the hero's trigger function (by hero id), which switches
# on the cast skill (player +0x1F) to the skill's effect; the sheet reads that switch. Gandalf's
# spells start from his own jump table in player_tick (0x0801C4A4, skills 5..10, character type
# 0x0A; 0x0801C644 for type 0x06) instead, listed here. A cast can also start its effect itself.
HERO_TRIGGERS = {0: 0x0801F590, 1: 0x0801F520, 2: 0x0801F4D4, 3: 0x0801F5E8, 4: 0x0801F664, 5: 0x0801F6C8}
EFFECTS = {
    (3, 5): 0x080185B4,  # Shield
    (3, 6): 0x08018FD4,  # Zap
    (3, 7): 0x08018880,  # Sunburst
    (3, 8): 0x08018280,  # Fireworks
    (3, 9): 0x08017AD4,  # Summon Eagle
    (3, 10): 0x0801E658,  # Thunderstrike
}
TRIGGER_CASE = re.compile(r"case (\d+):|== (\d+)\)")

ACTOR_FIELD_NOTES = """\
| Two Towers | ROTK | field |
|---|---|---|
| 0x0C | 0x0C | parentActor |
| 0x14 | 0x14 | tickCallback |
| 0x18, 0x1C | 0x18, 0x1C | triggerCallbacks[0], [1] |
| 0x58 | 0x50 | actionStateTimer |
| 0x67 | 0x62 | actionState (stun 0x1A in both) |
| 0x75 | 0x72 | interactionRadius |
| 0x90 | (LegacyFxState.amount) | damage |
| 0xFA | 0xF8 | facing |
| player +0x1F | castingActiveSkill | the skill being cast |
| player +0x30 + index | legacy record | skill levels (player stride 0x128) |

Hit mask 0x10040400 and actor flags match. Effects tick at 30 Hz in both."""

FUNC_REF = re.compile(r"\b(?:FUN|thunk_FUN)_([0-9a-f]{8})\b")
CONST_REF = re.compile(r"0x(0?8[0-9a-f]{6})\b")
LITERAL = re.compile(r"(?<!goto )\b_?(?:PTR_DAT_|DAT_|LAB_)([0-9a-f]{8})\b")
NUMBER = re.compile(r"(?<![\w.])(0x[0-9a-f]+|\d+)(?![\w.])")


def is_code(address: int) -> bool:
    """A Thumb function start in the code region."""
    return 0x08000000 <= address < 0x08060000


def is_gameplay(address: int) -> bool:
    """Where the effects, their callbacks and the casts live. Calls outside it are engine code
    the matcher did not name; the sheet follows only callbacks (code pointers) there."""
    return 0x08010000 <= address < 0x08030000 or 0x0803A000 <= address < 0x0803C800


def described(name_of: dict[int, str], address: int) -> str:
    name = name_of.get(address & ~1)
    return f"`{address:#010x}` {name}" if name else f"`{address:#010x}`"


def with_literals(c: str, rom: bytes, name_of: dict[int, str]) -> str:
    """Annotate each literal-pool load (DAT_<address> in ROM) with the word it loads."""
    def note(m: re.Match) -> str:
        address = int(m.group(1), 16)
        if not 0x08000000 <= address < 0x08000000 + len(rom) - 3:
            return m.group(0)
        value = ttt_names.word(rom, address)
        name = name_of.get(value & ~1, "")
        return f"{m.group(0)}/*={value:#010x}{' ' + name if name else ''}*/"
    return LITERAL.sub(note, c)


def gather(d: Decompiler, roots: list[int], engine: set[int], rom: bytes,
           name_of: dict[int, str]) -> list[tuple[int, int, str]]:
    """(address, depth, C) for the roots and the Two Towers-only code they reach."""
    seen, out, queue = set(), [], [(r, 0) for r in roots]
    while queue and len(out) < MAX_FUNCTIONS:
        address, depth = queue.pop(0)
        if address in seen or address in engine:
            continue
        seen.add(address)
        c = with_literals(d.c(address), rom, name_of)
        out.append((address, depth, c))
        if depth >= MAX_DEPTH:
            continue
        calls = [int(m, 16) for m in FUNC_REF.findall(c)]
        calls += [target for _, target in symbol_calls(d, c)]
        callbacks = [int(m, 16) & ~1 for m in CONST_REF.findall(c) if int(m, 16) & 1]
        found = [f for f in calls if is_gameplay(f)] + [f for f in callbacks if is_code(f)]
        queue.extend((f, depth + 1) for f in found if f not in seen)
    return out


def symbol_calls(d: Decompiler, c: str):
    """Named Two Towers-only functions the C calls (fx_*, ttt_*)."""
    for name in set(re.findall(r"\b((?:fx|ttt)_\w+)\s*\(", c)):
        try:
            yield name, d.resolve(name)
        except SystemExit:
            pass


def numbers(c: str) -> list[str]:
    """Constants in the C that are not addresses, offsets or 0/1."""
    body = c.split("{", 1)[-1]
    body = re.sub(r"/\*.*?\*/", "", body)
    body = re.sub(r"\+ 0x[0-9a-f]+\)", ")", body)  # field offsets
    found = []
    for n in NUMBER.findall(body):
        value = int(n, 0)
        if value > 1 and value < 0x02000000 and n not in found:
            found.append(n)
    return found


def trigger_effects(d: Decompiler, hero: int) -> dict[int, int]:
    """skill index -> the function the hero's trigger calls for it."""
    out, key = {}, None
    for line in d.c(HERO_TRIGGERS[hero]).splitlines():
        if m := TRIGGER_CASE.search(line):
            key = int(m.group(1) or m.group(2))
        if (m := FUNC_REF.search(line)) and key is not None and key not in out:
            out[key] = int(m.group(1), 16)
    return out


def sheet(skill: ttt_names.Skill) -> str:
    rom = ttt_names.rom_bytes()
    import ttt_save
    text = ttt_save.Items(rom).text(ttt_save.SKILL_TEXT[skill.hero] + skill.index)
    name_of = ttt_names.all_names(rom)
    engine = set(ttt_names.matched()[0])
    d = Decompiler()

    hero = ttt_names.HEROES[skill.hero]
    lines = [f"# {hero}: {skill.name}", "",
             f"Hero {skill.hero}, skill index {skill.index} (player +0x30 + {skill.index}), "
             f"first rank at player level {skill.level}.", "", f"> {text}", ""]
    effect = EFFECTS.get((skill.hero, skill.index))
    trigger = trigger_effects(d, skill.hero).get(skill.index)
    roots = [r for r in (skill.cast, effect, trigger) if r]
    functions = gather(d, roots, engine, rom, name_of)

    lines += ["## Functions", ""]
    for address, depth, _ in functions:
        role = ("cast" if address == skill.cast else "effect" if address == effect else
                f"trigger, from {HERO_TRIGGERS[skill.hero]:#010x}" if address == trigger else
                f"reached, depth {depth}")
        lines.append(f"- {described(name_of, address)} ({role})")
    lines.append("")

    refs, calls = {}, set()
    for _, _, c in functions:
        for m in CONST_REF.findall(c):
            value = int(m, 16)
            if not (is_code(value) and value & 1):
                refs[value] = described(name_of, value)
        calls |= set(re.findall(r"\b([a-z]\w+_\w+)\s*\(", c))
    lines += ["## ROM data it uses", ""]
    lines += [f"- {v}" for _, v in sorted(refs.items())] or ["- none"]
    lines += ["", "## Engine calls (ROTK names)", "", ", ".join(sorted(calls)) or "none", ""]

    lines += ["## Numbers", ""]
    for address, _, c in functions:
        lines.append(f"- {described(name_of, address)}: {', '.join(numbers(c)) or 'none'}")
    lines.append("")

    if effect is None and trigger is None:
        lines += ["No trigger case for this skill: the cast (or a passive's readers) does the work.", ""]
    lines += ["## Actor fields", "", ACTOR_FIELD_NOTES, "", "## Code", ""]
    for address, _, c in functions:
        lines += [f"### {described(name_of, address)}", "", "```c", c.strip(), "```", ""]
    return "\n".join(lines)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("skill", nargs="?")
    parser.add_argument("--list", action="store_true", help="every skill with its cast and effect")
    args = parser.parse_args()
    rom = ttt_names.rom_bytes()
    if args.list or not args.skill:
        d = Decompiler()
        triggers = {h: trigger_effects(d, h) for h in HERO_TRIGGERS}
        for s in ttt_names.skills(rom):
            effect = EFFECTS.get((s.hero, s.index))
            trigger = triggers[s.hero].get(s.index)
            print(f"{s.hero}:{s.index:<2} {ttt_names.HEROES[s.hero]:8} {s.name:22} cast {s.cast:#010x}"
                  + (f"  effect {effect:#010x}" if effect else "")
                  + (f"  trigger {trigger:#010x}" if trigger else ""))
        return
    skill = ttt_names.find_skill(rom, args.skill)
    OUT.mkdir(parents=True, exist_ok=True)
    path = OUT / f"{ttt_names.slug(ttt_names.HEROES[skill.hero])}_{ttt_names.slug(skill.name)}.md"
    path.write_text(sheet(skill), encoding="utf-8")
    print(path)


if __name__ == "__main__":
    main()
