"""Patch the arena ROM for a self-driving hero sweep and parse the rows it prints through mGBA's log."""

from pathlib import Path
import struct
import subprocess

ROM_BASE = 0x08000000
CONFIG_SYMBOL = "testArena_config"
# Values printed by each ARENA_ROW line, in order.
ROW_FIELDS = (
    "row",
    "accepted",
    "recast",
    "hits",
    "kills",
    "damage",
    "playerHp",
    "lagTicks",
    "worstTickVblanks",
    "heapRoom",
    "allocFailures",
    "invalidFrees",
    "bssGuardIntact",
    "heroStateMisuse",
    "effectsAfterClear",
    "playerDowned",
)
# Values printed by each ARENA_LAG line, in order.
LAG_FIELDS = ("row", "step", "vblanks", "ticksSinceReset", "actors")


def config_offset(elf: Path, nm: str = "arm-none-eabi-nm") -> int:
    """ROM file offset of TestArenaConfig in the arena build."""
    symbols = subprocess.run([nm, str(elf)], capture_output=True, text=True, check=True)
    for line in symbols.stdout.splitlines():
        parts = line.split()
        if len(parts) == 3 and parts[2] == CONFIG_SYMBOL:
            return int(parts[0], 16) - ROM_BASE
    raise LookupError(f"{CONFIG_SYMBOL} not found in {elf}")


def patch_rom(
    rom: bytes,
    offset: int,
    hero: int,
    ticks: int = 0,
    fixture: int = 0,
    bench_ticks: int = 0,
    bench_crowd: int = 0,
    bench_sample: bool = False,
) -> bytes:
    """A copy of the arena ROM that boots hero and runs a sweep or a benchmark on its own."""
    if struct.unpack_from("<6I", rom, offset)[1:] != (0, 0, 0, 0, 0):
        raise ValueError("arena ROM config does not hold the expected defaults")
    patched = bytearray(rom)
    struct.pack_into(
        "<6I", patched, offset, hero, ticks, fixture, bench_ticks, bench_crowd, bench_sample
    )
    return bytes(patched)


def parse_log(text: str) -> dict:
    """Rows and the end state from an mGBA log of a sweep run."""
    rows = []
    lags = []
    done = failed = None
    for line in text.splitlines():
        _, marker, payload = line.partition("ARENA_")
        if not marker:
            continue
        kind, _, values = payload.partition(" ")
        try:
            numbers = [int(value) for value in values.split()]
        except ValueError:
            # The emulator may still be writing the last line.
            continue
        if kind == "ROW" and len(numbers) == len(ROW_FIELDS):
            row = dict(zip(ROW_FIELDS, numbers))
            row["problems"] = problems(row)
            rows.append(row)
        elif kind == "LAG" and len(numbers) == len(LAG_FIELDS):
            lags.append(dict(zip(LAG_FIELDS, numbers)))
        elif kind == "DONE":
            done = dict(zip(("ticks", "lagTicks", "heapRoom"), numbers))
        elif kind == "FAIL":
            failed = numbers
    return dict(rows=rows, lags=lags, done=done, failed=failed)


def problems(row: dict) -> list[str]:
    """Health failures for one sweep row. A rejected cast is reported, not failed."""
    found = []
    if not row["bssGuardIntact"]:
        found.append("bss guard overwritten")
    if row["heroStateMisuse"]:
        found.append("hero state accessed by another hero")
    if row["allocFailures"]:
        found.append("heap allocation failures")
    if row["invalidFrees"]:
        found.append("invalid frees")
    if row["effectsAfterClear"]:
        found.append("timed effects left after clear")
    if row["accepted"] and not row["recast"]:
        found.append("second cast rejected after reset")
    return found
