"""Cold-boot the opt-in arena in mGBA and save a debugger-driven scenario report."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import socket
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def script(elf: Path, fixture: int, recipe: int | None, level: int, ticks: int) -> str:
    lines = [
        "set pagination off",
        "set confirm off",
        "set remote noack-packet off",
        "set remote interrupt-on-connect on",
        "set remote binary-download-packet off",
        "set tcp auto-retry on",
        "set tcp connect-timeout 30",
        f"file {json.dumps(elf.as_posix())}",
        "target remote 127.0.0.1:2345",
        "break testArena_checkpoint",
        "continue",
        "if g_TestArena->ready != 1",
        '  printf "Arena failed to initialize: %u\\n", g_TestArena->ready',
        "  quit 1",
        "end",
    ]

    def command(kind: int, argument: int) -> None:
        lines.extend(
            [
                f"set g_TestArena->command = {kind}",
                f"set g_TestArena->argument = {argument}",
                f"set g_TestArena->level = {level}",
                "set g_TestArena->cosmetic = 0",
                "set g_TestArena->request = g_TestArena->request + 1",
                "continue",
                "if g_TestArena->acknowledged != g_TestArena->request || g_TestArena->result != 1",
                '  printf "Arena command failed\\n"',
                "  print *g_TestArena",
                "  quit 1",
                "end",
            ]
        )

    command(1, fixture)
    if recipe is not None:
        command(2, recipe)
    lines.extend(
        [
            f"set $arenaEnd = g_TestArena->ticks + {ticks}",
            "condition 1 g_TestArena->ticks >= $arenaEnd",
            "continue",
            'printf "ARENA_JSON {\\"ready\\":%u,\\"ticks\\":%u,\\"hits\\":%u,\\"kills\\":%u,\\"damage\\":%u,\\"playerHp\\":%d,\\"playerSpirit\\":%d,\\"targetHp\\":[%d,%d,%d]}\\n", '
            "g_TestArena->ready, g_TestArena->ticks, g_TestArena->hits, g_TestArena->kills, "
            "g_TestArena->damage, g_TestArena->playerHp, g_TestArena->playerSpirit, "
            "g_TestArena->targetHp[0], g_TestArena->targetHp[1], g_TestArena->targetHp[2]",
            "disconnect",
            "quit",
        ]
    )
    return "\n".join(lines) + "\n"


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--emulator", type=Path, required=True, help="mGBA desktop executable"
    )
    parser.add_argument("--gdb", default=shutil.which("arm-none-eabi-gdb"))
    parser.add_argument("--fixture", type=int, choices=range(3), default=0)
    parser.add_argument("--recipe", type=int)
    parser.add_argument("--level", type=int, choices=range(1, 6), default=1)
    parser.add_argument("--ticks", type=int, default=90)
    parser.add_argument(
        "--engine-checks", action="store_true", help="Run the engine regression suite"
    )
    parser.add_argument(
        "--output", type=Path, required=True, help="New report directory"
    )
    args = parser.parse_args()
    if not args.gdb or not 1 <= args.ticks <= 3600:
        parser.error("GDB must be on PATH and ticks must be 1..3600")
    if args.recipe is not None and not 0 <= args.recipe < 32:
        parser.error("recipe must be 0..31")
    emulator = args.emulator.resolve(strict=True)
    rom = ROOT / "build/test_arena.gba"
    elf = ROOT / "build/test_arena.elf"
    if not rom.is_file() or not elf.is_file():
        parser.error("Run make test-arena first")
    with socket.socket() as probe:
        try:
            probe.bind(("127.0.0.1", 2345))
        except OSError:
            parser.error("GDB port 2345 is already in use")
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    # A new basename and directory keep emulator sidecars isolated from personal saves.
    test_rom = output / "arena.gba"
    shutil.copyfile(rom, test_rom)
    shutil.copyfile(elf, output / "arena.elf")
    gdb_script = output / "scenario.gdb"
    if args.engine_checks:
        from arena_engine_checks import script as engine_script

        scenario = engine_script(output / "arena.elf")
    else:
        scenario = script(
            output / "arena.elf", args.fixture, args.recipe, args.level, args.ticks
        )
    gdb_script.write_text(scenario)
    startup = None
    if os.name == "nt":
        startup = subprocess.STARTUPINFO()
        startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
        startup.wShowWindow = 0
    with (output / "emulator.log").open("w") as log:
        process = subprocess.Popen(
            [str(emulator), "-g", str(test_rom)],
            cwd=emulator.parent,
            stdout=log,
            stderr=subprocess.STDOUT,
            startupinfo=startup,
        )
        try:
            with (output / "gdb.log").open("w") as debugger_log:
                result = subprocess.run(
                    [args.gdb, "-q", "-nx", "--batch", "-x", str(gdb_script)],
                    stdout=debugger_log,
                    stderr=subprocess.STDOUT,
                    text=True,
                    timeout=120,
                )
            debugger_output = (output / "gdb.log").read_text()
            result.check_returncode()
            payload = next(
                line.removeprefix("ARENA_JSON ")
                for line in debugger_output.splitlines()
                if line.startswith("ARENA_JSON ")
            )
            report = json.loads(payload)
            report["checks"] = [
                line.removeprefix("ARENA_CHECK ")
                for line in debugger_output.splitlines()
                if line.startswith("ARENA_CHECK ")
            ]
            report.update(
                fixture=args.fixture,
                recipe=args.recipe,
                level=args.level,
                elapsedTicks=args.ticks,
                romSha256=hashlib.sha256(test_rom.read_bytes()).hexdigest(),
            )
            (output / "result.json").write_text(json.dumps(report, indent=2) + "\n")
            print(json.dumps(report, indent=2))
        finally:
            process.terminate()
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()


if __name__ == "__main__":
    main()
