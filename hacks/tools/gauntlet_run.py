"""Self-driving Gauntlet runs: patch gauntletTest_config into copies of the hack ROM and read the
room lines each run prints through mGBA's debug log."""

import argparse
from concurrent.futures import ThreadPoolExecutor
import json
import os
from pathlib import Path
import struct
import subprocess
import time

from arena_sweep_all import EMULATOR_CONFIG, HERO_NAMES, LOG_WARNINGS

ROOT = Path(__file__).resolve().parents[2]
ROM_BASE = 0x08000000
CONFIG_SYMBOL = "gauntletTest_config"
ROOM_FIELDS = (
    "room",
    "mission",
    "spawned",
    "landFails",
    "purged",
    "ticks",
    "level",
    "maxHp",
    "heapRoom",
    "allocFailures",
    "bssGuardIntact",
)
ENTRY_FIELDS = ("roomType", "map", "candidate", "x", "y", "clear")
OVER_FIELDS = (
    "room",
    "kills",
    "boons",
    "rerollsLeft",
    "heapRoom",
    "allocFailures",
    "bssGuardIntact",
)


def config_offset(elf: Path, nm: str) -> int:
    symbols = subprocess.run([nm, str(elf)], capture_output=True, text=True, check=True)
    for line in symbols.stdout.splitlines():
        parts = line.split()
        if len(parts) == 3 and parts[2] == CONFIG_SYMBOL:
            return int(parts[0], 16) - ROM_BASE
    raise LookupError(f"{CONFIG_SYMBOL} not found in {elf}")


def patch_rom(
    rom: bytes,
    offset: int,
    rooms: int,
    hero: int,
    probe: bool,
    unlocks: bool = False,
    boons: bool = False,
) -> bytes:
    if struct.unpack_from("<4I", rom, offset) != (0, 0, 0, 0):
        raise ValueError("ROM test config does not hold the expected defaults")
    patched = bytearray(rom)
    struct.pack_into(
        "<4I",
        patched,
        offset,
        rooms,
        hero,
        int(probe),
        int(unlocks) | (int(boons) << 1),
    )
    return bytes(patched)


def parse_log(text: str) -> dict:
    result = dict(
        booted=False,
        rooms=[],
        over=None,
        done=False,
        stuck=[],
        errors=[],
        entries=[],
        variants=[],
        backdrops=[],
    )
    for line in text.splitlines():
        if (
            "Illegal opcode" in line
            or "Stub software interrupt" in line
            or "Unimplemented BIOS" in line
        ):
            result["errors"].append(line.strip())
            break
        _, marker, payload = line.partition("GAUNTLET_")
        if not marker:
            continue
        kind, _, values = payload.partition(" ")
        try:
            numbers = [int(value) for value in values.split()]
        except ValueError:
            continue
        if kind == "BOOT":
            result["booted"] = True
        elif kind == "ROOM" and len(numbers) == len(ROOM_FIELDS):
            result["rooms"].append(dict(zip(ROOM_FIELDS, numbers)))
        elif kind == "ENTRY" and len(numbers) == len(ENTRY_FIELDS):
            result["entries"].append(dict(zip(ENTRY_FIELDS, numbers)))
        elif kind == "BACKDROP" and len(numbers) == 4:
            result["backdrops"].append(
                dict(zip(("mission", "effect", "control", "frame"), numbers))
            )
        elif kind == "VARIANT" and len(numbers) == 2:
            result["variants"].append(dict(zip(("modifier", "rarity"), numbers)))
        elif kind == "OVER" and len(numbers) == len(OVER_FIELDS):
            result["over"] = dict(zip(OVER_FIELDS, numbers))
        elif kind == "STUCK" and numbers:
            result["stuck"].append(numbers[0])
        elif kind == "DONE":
            result["done"] = True
    return result


def run(emulator: Path, rom: bytes, offset: int, hero: int, args, output: Path) -> dict:
    run_dir = output / f"hero-{hero}-{HERO_NAMES[hero].lower()}"
    run_dir.mkdir(parents=True, exist_ok=True)
    test_rom = run_dir / "gauntlet.gba"
    test_rom.write_bytes(
        patch_rom(rom, offset, args.rooms, hero, args.probe, args.unlocks, args.boons)
    )
    log = run_dir / "mgba.log"
    log.unlink(missing_ok=True)
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
    result = parse_log("")
    try:
        while time.monotonic() - started < args.timeout:
            time.sleep(0.5)
            if log.is_file():
                result = parse_log(log.read_text(errors="replace"))
                if result["done"] or result["errors"]:
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
    result["seconds"] = round(time.monotonic() - started, 1)
    (run_dir / "result.json").write_text(json.dumps(result, indent=2) + "\n")
    return result


def report(hero: int, result: dict) -> bool:
    memory_ok = all(
        not room["landFails"] and not room["allocFailures"] and room["bssGuardIntact"]
        for room in result["rooms"]
    )
    if result["over"]:
        memory_ok &= not result["over"]["allocFailures"] and bool(
            result["over"]["bssGuardIntact"]
        )
    memory_ok &= all(entry["clear"] for entry in result.get("entries", []))
    ok = (
        result["done"]
        and not result["stuck"]
        and not result.get("errors")
        and memory_ok
    )
    print(
        f"== {HERO_NAMES[hero]} {result['seconds']}s {'ok' if ok else 'FAILED'}",
        flush=True,
    )
    if not result["booted"]:
        print("   never booted", flush=True)
    for room in result["rooms"]:
        flags = []
        if room["landFails"]:
            flags.append(f"{room['landFails']} spawns found no ground")
        if not room["bssGuardIntact"]:
            flags.append("bss guard overwritten")
        if room["allocFailures"]:
            flags.append("alloc failures")
        print(
            f"   room {room['room']:2} mission 0x{room['mission']:02x}: spawned {room['spawned']:3} "
            f"purged {room['purged']:3} ticks {room['ticks']:5} lv {room['level']:2} hp {room['maxHp']:4} "
            f"heap {room['heapRoom']} {'; '.join(flags)}",
            flush=True,
        )
    for room in result["stuck"]:
        print(f"   STUCK in room {room}", flush=True)
    if result["over"]:
        over = result["over"]
        print(
            f"   over: room {over['room']} kills {over['kills']} boons {over['boons']} "
            f"rerolls left {over['rerollsLeft']}",
            flush=True,
        )
    for error in result.get("errors", []):
        print(f"   EMULATOR ERROR: {error}", flush=True)
    if not result["done"]:
        print("   never returned to the main menu", flush=True)
    return ok


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--emulator", type=Path, required=True, help="mGBA desktop executable"
    )
    parser.add_argument("--nm", default="arm-none-eabi-nm")
    parser.add_argument("--rom", type=Path, default=ROOT / "build/rotk_hack.gba")
    parser.add_argument("--elf", type=Path, default=ROOT / "build/hack.elf")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--heroes", type=int, nargs="*", default=[2])
    parser.add_argument(
        "--rooms", type=int, default=4, help="Rooms to clear before the hero falls"
    )
    parser.add_argument(
        "--probe", action="store_true", help="Play every room map in table order"
    )
    parser.add_argument(
        "--unlocks", action="store_true", help="Start with every unlock maxed"
    )
    parser.add_argument(
        "--boons", action="store_true", help="Stress all boons and cast starting skills"
    )
    parser.add_argument("--jobs", type=int, default=5)
    parser.add_argument("--timeout", type=int, default=900, help="Seconds per run")
    args = parser.parse_args()
    rom = args.rom.read_bytes()
    offset = config_offset(args.elf, args.nm)
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        futures = {
            hero: pool.submit(
                run, args.emulator, rom, offset, hero, args, args.output.resolve()
            )
            for hero in args.heroes
        }
    results = [report(hero, futures[hero].result()) for hero in args.heroes]
    raise SystemExit(0 if all(results) else 1)


if __name__ == "__main__":
    main()
