"""Every name the tools know for a Two Towers address.

Three sources, later ones winning: the matcher's ROTK names (build/ttt/symbols.json, from
ttt_match.py), the cast table in the ROM (one `ttt_<hero>_<skill>_cast` per skill), and the
hand-found addresses in two-towers/ttt_labels.txt.
"""

from __future__ import annotations

import json
import re
import struct
from dataclasses import dataclass
from pathlib import Path

from roms import ROM_BASE, TTT_ROM

ROOT = Path(__file__).resolve().parents[2]
SYMBOLS = ROOT / "build/ttt/symbols.json"
LABELS = ROOT / "two-towers/ttt_labels.txt"

HEROES = ["Frodo", "Legolas", "Aragorn", "Gandalf", "Eowyn", "Gimli"]
CAST_TABLE = 0x08057AB0
SKILLS_PER_HERO = 11


@dataclass(frozen=True)
class Skill:
    hero: int
    index: int          # skillLevels index: player +0x30 + index
    name: str           # "Zap"
    level: int          # player level the first rank needs
    cast: int           # cast function (even address)

    @property
    def symbol(self) -> str:
        return f"ttt_{slug(HEROES[self.hero])}_{slug(self.name)}_cast"


def slug(text: str) -> str:
    return re.sub(r"[^a-z0-9]+", "_", text.lower()).strip("_")


def rom_bytes() -> bytes:
    return TTT_ROM.read_bytes()


def word(rom: bytes, address: int) -> int:
    return struct.unpack_from("<I", rom, address - ROM_BASE)[0]


def skills(rom: bytes) -> list[Skill]:
    """All 66 skills with their names from the ROM's text."""
    import ttt_save
    items = ttt_save.Items(rom)
    out = []
    for hero in range(len(HEROES)):
        for index in range(SKILLS_PER_HERO):
            m = re.match(r"(.*?) \(Level (\d+)\)", items.skill(hero, index))
            name, level = (m.group(1), int(m.group(2))) if m else (f"skill{index}", 0)
            cast = word(rom, CAST_TABLE + hero * 0x2C + index * 4) & ~1
            out.append(Skill(hero, index, name, level, cast))
    return out


def find_skill(rom: bytes, query: str) -> Skill:
    """A skill by "Gandalf Zap", "gandalf/zap", "3:6" (hero:index) or a unique skill name."""
    every = skills(rom)
    if m := re.fullmatch(r"(\d+)[:/](\d+)", query):
        return every[int(m.group(1)) * SKILLS_PER_HERO + int(m.group(2))]
    words = slug(query).split("_")
    hero = next((h for h, n in enumerate(HEROES) if slug(n) == words[0]), None)
    rest = "_".join(words[1:] if hero is not None else words)
    found = [s for s in every if (hero is None or s.hero == hero) and slug(s.name).startswith(rest)]
    if len(found) != 1:
        raise SystemExit(f"{query!r} matches {len(found)} skills: "
                         + ", ".join(f"{HEROES[s.hero]} {s.name}" for s in found or every))
    return found[0]


def labels() -> dict[int, tuple[str, str]]:
    """address -> (name, comment) from ttt_labels.txt."""
    out = {}
    for line in LABELS.read_text().splitlines():
        body, _, comment = line.partition("#")
        parts = body.split()
        if len(parts) >= 2:
            out[int(parts[0], 16)] = (parts[1], comment.strip())
    return out


def matched() -> tuple[dict[int, str], dict[int, str]]:
    """(functions, globals) the matcher named: address -> ROTK name."""
    if not SYMBOLS.exists():
        return {}, {}
    data = json.loads(SYMBOLS.read_text())
    functions = {v["ttt"] & ~1: name for name, v in data.get("functions", {}).items()
                 if isinstance(v, dict) and v.get("ttt")}
    globals_ = {v["ttt"]: name for name, v in data.get("globals", {}).items()
                if isinstance(v, dict) and v.get("ttt")}
    return functions, globals_


def all_names(rom: bytes) -> dict[int, str]:
    """address -> best name, code and data."""
    functions, globals_ = matched()
    names = {**globals_, **functions}
    names.update({s.cast: s.symbol for s in skills(rom)})
    names.update({a: n for a, (n, _) in labels().items()})
    return names
