"""Run the busy fight benchmark for one hero and report lag and where the CPU spends its time."""

import argparse
import bisect
from collections import Counter
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import time

from arena_hero_sweep import config_offset, patch_rom

ROOT = Path(__file__).resolve().parents[2]
SECTION_NAMES = ("player", "passives", "hero", "targets", "effects", "summons", "fx")
EMULATOR_CONFIG = (
    "audioSync=0",
    "videoSync=0",
    "fpsTarget=10000",
    "mute=1",
    "logToFile=1",
    "logToStdout=0",
)
LOG_WARNINGS = 4


def symbol_table(elf: Path, nm: str) -> tuple[list[int], list[str]]:
    """Function start addresses and names from the game config and the arena ELF."""
    entries = {}
    for line in (ROOT / "config/functions.cfg").read_text().splitlines():
        match = re.match(r"\s*0x([0-9a-fA-F]+)\s+\w+\s+(\S+)", line)
        if match:
            entries[int(match.group(1), 16)] = match.group(2)
    listing = subprocess.run([nm, "--defined-only", str(elf)], capture_output=True, text=True)
    for line in listing.stdout.splitlines():
        parts = line.split()
        if len(parts) == 3 and parts[1] in "tT":
            entries[int(parts[0], 16) & ~1] = parts[2]
    starts = sorted(entries)
    return starts, [entries[start] for start in starts]


def region(pc: int) -> str:
    if pc < 0x02000000:
        return "bios"
    if pc < 0x03000000:
        return "ewram"
    if pc < 0x04000000:
        return "iwram"
    if pc >= 0x09000000:
        return "mod"
    return "game"


def name_of(pc: int, starts: list[int], names: list[str]) -> str:
    kind = region(pc)
    if kind in ("game", "mod"):
        index = bisect.bisect_right(starts, pc) - 1
        if index >= 0 and pc - starts[index] < 0x2000:
            return f"{kind}:{names[index]}"
    if kind in ("iwram", "ewram"):
        return f"{kind}:{pc & ~0xFF:08x}"
    return f"{kind}:{pc:08x}"


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--emulator", type=Path, required=True, help="mGBA desktop executable")
    parser.add_argument("--nm", default=shutil.which("arm-none-eabi-nm") or "arm-none-eabi-nm")
    parser.add_argument("--hero", type=int, default=2)
    parser.add_argument("--ticks", type=int, default=1200, help="Measured benchmark ticks")
    parser.add_argument("--crowd", type=int, default=8, help="Living foes to keep around")
    parser.add_argument("--no-samples", action="store_true", help="Measure lag only")
    parser.add_argument("--timeout", type=int, default=600)
    parser.add_argument("--top", type=int, default=40)
    parser.add_argument("--output", type=Path, required=True, help="New report directory")
    args = parser.parse_args()
    emulator = args.emulator.resolve(strict=True)
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    elf = ROOT / "build/test_arena.elf"
    rom = (ROOT / "build/test_arena.gba").read_bytes()
    test_rom = output / "arena.gba"
    test_rom.write_bytes(
        patch_rom(
            rom,
            config_offset(elf, args.nm),
            args.hero,
            bench_ticks=args.ticks,
            bench_crowd=args.crowd,
            bench_sample=not args.no_samples,
        )
    )
    log = output / "mgba.log"
    command = [str(emulator)]
    for value in (*EMULATOR_CONFIG, f"logFile={log}"):
        command += ["-C", value]
    startup = None
    if os.name == "nt":
        startup = subprocess.STARTUPINFO()
        startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
        startup.wShowWindow = 0
    process = subprocess.Popen(
        [*command, "-l", str(LOG_WARNINGS), str(test_rom)],
        cwd=emulator.parent,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        startupinfo=startup,
    )
    started = time.monotonic()
    text = ""
    try:
        while time.monotonic() - started < args.timeout:
            time.sleep(0.25)
            if log.is_file():
                text = log.read_text(errors="replace")
                if re.search(r"ARENA_BENCH \d", text) or "ARENA_FAIL" in text:
                    break
    finally:
        process.terminate()
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()
    samples = Counter()
    for line in text.splitlines():
        _, marker, values = line.partition("ARENA_PC ")
        if marker:
            for value in values.split():
                if len(value) == 8:
                    samples[int(value, 16)] += 1
    dropped = re.search(r"ARENA_PC_DROPPED (\d+)", text)
    report = dict(
        hero=args.hero,
        crowd=args.crowd,
        samples=sum(samples.values()),
        droppedSamples=int(dropped.group(1)) if dropped else None,
    )
    bench = re.search(r"ARENA_BENCH ((?:-?\d+ ?)+)", text)
    if bench:
        ticks, vblanks, lag, worst, foe_ticks, casts = map(int, bench.group(1).split())
        report.update(
            ticks=ticks,
            vblanksPerTick=round(vblanks / ticks, 3),
            lagTicks=lag,
            worstTickVblanks=worst,
            averageFoes=round(foe_ticks / ticks, 2),
            casts=casts,
        )
    profile = {}
    for match in re.finditer(r"ARENA_PROFILE (\d+) (\d+) (\d+) (\d+) (\d+)", text):
        section, calls, total, peak, work = map(int, match.groups())
        profile[SECTION_NAMES[section]] = dict(
            calls=calls, linesPerTick=round(total / max(report.get("ticks", 1), 1), 1),
            peakLines=peak, work=work,
        )
    report["profile"] = profile
    if samples:
        starts, names = symbol_table(elf, args.nm)
        total = sum(samples.values())
        regions = Counter()
        functions = Counter()
        for pc, count in samples.items():
            regions[region(pc)] += count
            functions[name_of(pc, starts, names)] += count
        report["regions"] = {k: round(100 * v / total, 1) for k, v in regions.most_common()}
        report["functions"] = [
            [name, round(100 * count / total, 1)] for name, count in functions.most_common(args.top)
        ]
    (output / "result.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
