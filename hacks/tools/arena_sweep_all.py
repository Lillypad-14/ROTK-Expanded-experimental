"""Build the test arena once, then sweep every hero's skill rows in parallel mGBA instances."""

import argparse
from concurrent.futures import ThreadPoolExecutor
import json
import os
from pathlib import Path
import shutil
import subprocess
import time

from arena_hero_sweep import config_offset, parse_log, patch_rom

ROOT = Path(__file__).resolve().parents[2]
HERO_NAMES = (
    "Frodo",
    "Legolas",
    "Aragorn",
    "Gandalf",
    "Eowyn",
    "Gimli",
    "Sam",
    "Smeagol",
    "Ringwraith",
    "Saruman",
)
# Unthrottled, silent, and logging the game's debug prints to a file.
EMULATOR_CONFIG = (
    "audioSync=0",
    "videoSync=0",
    "fpsTarget=10000",
    "mute=1",
    "logToFile=1",
    "logToStdout=0",
)
# mGBA log level mask with only warnings, the level the arena prints its results at.
LOG_WARNINGS = 4


def run_hero(emulator: Path, rom: bytes, offset: int, hero: int, args, output: Path) -> dict:
    run_dir = output / f"hero-{hero}-{HERO_NAMES[hero].lower()}"
    run_dir.mkdir()
    # A new basename and directory keep emulator sidecars isolated from personal saves.
    test_rom = run_dir / "arena.gba"
    test_rom.write_bytes(patch_rom(rom, offset, hero, args.ticks, args.fixture))
    log = run_dir / "mgba.log"
    config = [*EMULATOR_CONFIG, f"logFile={log}"]
    startup = None
    if os.name == "nt":
        startup = subprocess.STARTUPINFO()
        startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
        startup.wShowWindow = 0
    command = [str(emulator)]
    for value in config:
        command += ["-C", value]
    process = subprocess.Popen(
        [*command, "-l", str(LOG_WARNINGS), str(test_rom)],
        cwd=emulator.parent,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        startupinfo=startup,
    )
    started = time.monotonic()
    result = dict(rows=[], done=None, failed=None)
    try:
        while time.monotonic() - started < args.timeout:
            time.sleep(0.5)
            if log.is_file():
                result = parse_log(log.read_text(errors="replace"))
                if result["done"] or result["failed"] is not None:
                    break
            if process.poll() is not None:
                break
    finally:
        process.terminate()
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()
    result["hung"] = result["done"] is None and result["failed"] is None
    result["seconds"] = round(time.monotonic() - started, 1)
    (run_dir / "result.json").write_text(json.dumps(result, indent=2) + "\n")
    return result


def report(name: str, hero: int, result: dict) -> None:
    print(f"== {name} (hero {hero}) {result['seconds']}s", flush=True)
    if result["failed"] is not None:
        print(f"   arena failed to start: {result['failed']}", flush=True)
    for row in result["rows"]:
        status = "; ".join(row["problems"]) or "ok"
        if not row["accepted"]:
            status = "not castable" if status == "ok" else status
        if row["playerDowned"]:
            status += "; player died, mission reloaded"
        print(
            f"   row {row['row']:2}: {status:30} dmg {row['damage']:5} "
            f"lag {row['lagTicks']:3} worst {row['worstTickVblanks']} "
            f"room {row['heapRoom']}",
            flush=True,
        )
    if result["hung"]:
        print(f"   HUNG after {len(result['rows'])} rows", flush=True)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--emulator", type=Path, required=True, help="mGBA desktop executable")
    parser.add_argument("--make", default=shutil.which("make") or "make")
    parser.add_argument("--nm", default=shutil.which("arm-none-eabi-nm") or "arm-none-eabi-nm")
    parser.add_argument("--heroes", type=int, nargs="*", default=list(range(len(HERO_NAMES))))
    parser.add_argument("--fixture", type=int, choices=range(3), default=1)
    parser.add_argument("--ticks", type=int, default=150, help="Ticks each cast runs")
    parser.add_argument("--jobs", type=int, default=5, help="Emulators running at once")
    parser.add_argument("--timeout", type=int, default=600, help="Seconds per hero")
    parser.add_argument("--no-build", action="store_true", help="Reuse build/test_arena.gba")
    parser.add_argument("--output", type=Path, required=True, help="New report directory")
    args = parser.parse_args()
    if not 1 <= args.ticks <= 3600 or args.jobs < 1:
        parser.error("ticks must be 1..3600 and jobs at least 1")
    emulator = args.emulator.resolve(strict=True)
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    if not args.no_build:
        build = subprocess.run(
            [args.make, "test-arena"], cwd=ROOT, capture_output=True, text=True
        )
        (output / "build.log").write_text(build.stdout + build.stderr)
        if build.returncode != 0:
            raise SystemExit(f"make test-arena failed, see {output / 'build.log'}")
    rom = (ROOT / "build/test_arena.gba").read_bytes()
    offset = config_offset(ROOT / "build/test_arena.elf", args.nm)
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        futures = {
            hero: pool.submit(run_hero, emulator, rom, offset, hero, args, output)
            for hero in args.heroes
        }
        summary = {}
        for hero, future in futures.items():
            result = future.result()
            summary[HERO_NAMES[hero]] = result
            report(HERO_NAMES[hero], hero, result)
    (output / "summary.json").write_text(json.dumps(summary, indent=2) + "\n")


if __name__ == "__main__":
    main()
