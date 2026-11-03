"""Try hero changes in the PC game in seconds, without rebuilding it.

    uv run python hacks/tools/hero_fast.py play             # open the game with heroes.json's heroes
    uv run python hacks/tools/hero_fast.py check Witch-king # headless: attack 3 times, frame sheet
    uv run python hacks/tools/hero_fast.py check Witch-king --button A  # the same, casting the skill

A hero's "body" and "sounds" (heroes.json) live in the hero data region (hero_data.py), and its
skill spec (hacks/content/heroes/<hero>.json) in the hero kits region (hero_kit.py). Code reaches
both only through fixed slots, so this rewrites the two regions in a copy of the last prepared
Expanded ROM (the kits linked against the last build's build/hack_pc.elf) and runs the last built
game on it. Anything else (stats, any C or asm) needs the full build: make hack-pc, then lab.py
prepare and build (docs/wip-docs/HANDOFF-hero-pipeline.md).

play uses its own save (build/fast/rotk.sram, first copied from the real play save), so the real
save is never touched. check needs a savestate in a level: hero_fast.py state <route.csv> <frame>
makes one (once per full build).
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import zlib

import hero_data
import hero_kit
import hero_registry

ROOT = Path(__file__).resolve().parents[2]
PORT = ROOT.parent / "rotk-recompiled"
PREPARED = PORT / ".private/expanded"
HOST = PORT / "build/host-expanded"
EXE = "ROTK-Recompiled-Expanded.exe"
FAST = ROOT / "build/fast"
PACKAGE = Path("mods/packages/rotk.pc/1.0.0")
REAL_SAVE = PREPARED / "play/rotk.sram"
BUTTONS = {"A": 0x3FE, "B": 0x3FD}
RELEASED = 0x3FF


def region_bounds(name: str = "HERO_DATA") -> tuple[int, int]:
    """A hacks/layout.cfg ROM region's first and last byte offsets in the ROM."""
    for line in (ROOT / "hacks/layout.cfg").read_text().splitlines():
        parts = line.split("#")[0].split()
        if len(parts) == 4 and parts[0] == name:
            return int(parts[2], 16) - hero_data.ROM_BASE, int(parts[3], 16) - hero_data.ROM_BASE
    sys.exit(f"No {name} region in hacks/layout.cfg")


def kits_region() -> bytes:
    """The hero kits region from the specs, linked against the last PC build's modcode."""
    elf, cflags = ROOT / "build/hack_pc.elf", ROOT / "build/modcode_cflags_pc.txt"
    if not cflags.exists():
        sys.exit("No build/modcode_cflags_pc.txt: run make hack-pc (and lab.py prepare/build) once.")
    source = FAST / "hero_spec_kits.c"
    FAST.mkdir(parents=True, exist_ok=True)
    try:
        hero_kit.generate(ROOT, hero_registry.load(ROOT, True), source)
        return hero_kit.link(ROOT, source, elf, cflags)
    except hero_kit.SpecError as error:
        sys.exit(f"Skill spec: {error}")


def patched_rom() -> bytes:
    """The prepared ROM with the hero data region rewritten from heroes.json."""
    prepared = PREPARED / "rotk.gba"
    if not prepared.exists():
        sys.exit("No prepared Expanded ROM: run the full build once (make hack-pc, lab.py prepare/build).")
    rom = bytearray(prepared.read_bytes())
    regions = [region_bounds("HERO_KITS"), region_bounds("HERO_DATA")]
    built = ROOT / "build/rotk_pc.gba"
    if built.exists():
        # Everything but the two regions must be the prepared game's, or the kits would link
        # against a modcode the recompiled game does not run.
        other, mine = bytearray(built.read_bytes()), bytearray(rom)
        for start, end in regions:
            other[start:end + 1] = mine[start:end + 1] = bytes(end + 1 - start)
        if other != mine:
            sys.exit("build/rotk_pc.gba has code changes the prepared game lacks: run lab.py prepare and build first.")
    heroes = hero_registry.load(ROOT, True)
    data, _ = hero_data.build(ROOT, heroes, (ROOT / "rotk.gba").read_bytes())
    for (start, end), region in zip(regions, (kits_region(), data)):
        if len(region) > end + 1 - start:
            sys.exit(f"Region at 0x{start + hero_data.ROM_BASE:08x} overflows by {len(region) - (end + 1 - start)} bytes")
        rom[start:end + 1] = region + bytes(end + 1 - start - len(region))
    return bytes(rom)


def link(target: Path, source: Path) -> None:
    """A directory junction (no copy of the large HD assets)."""
    if target.exists():
        return
    subprocess.run(["cmd", "/c", "mklink", "/J", str(target), str(source)], check=True, capture_output=True)


def setup(rom: bytes) -> tuple[Path, Path]:
    """build/fast: the game binaries, its mods with the ROM pin updated, and a config for the ROM."""
    FAST.mkdir(parents=True, exist_ok=True)
    (FAST / "rotk.gba").write_bytes(rom)
    sha1, crc32 = hashlib.sha1(rom).hexdigest(), f"0x{zlib.crc32(rom) & 0xFFFFFFFF:08x}"
    bin_dir = FAST / "bin"
    bin_dir.mkdir(exist_ok=True)
    for path in [HOST / EXE, *HOST.glob("*.dll")]:
        target = bin_dir / path.name
        if not target.exists() or target.stat().st_mtime < path.stat().st_mtime:
            shutil.copy2(path, target)
    package = bin_dir / PACKAGE
    package.mkdir(parents=True, exist_ok=True)
    for item in (HOST / PACKAGE).iterdir():
        if item.is_dir():
            link(package / item.name, item)
    manifest = (HOST / PACKAGE / "manifest.toml").read_text()
    (package / "manifest.toml").write_text(re.sub(r'rom_sha1 = "[0-9a-f]+"', f'rom_sha1 = "{sha1}"', manifest))
    for item in (HOST / "mods").iterdir():
        if item.is_file():
            shutil.copy2(item, bin_dir / "mods" / item.name)
    for item in HOST.iterdir():
        if item.is_dir() and item.name in ("assets",):
            link(bin_dir / item.name, item)
    config = (PREPARED / "game.toml").read_text()
    config = config.replace((PREPARED / "rotk.gba").as_posix(), (FAST / "rotk.gba").as_posix())
    config = re.sub(r'(\[rom\][^\[]*?sha1 = ")[0-9a-f]+', lambda m: m.group(1) + sha1, config, flags=re.S)
    config = re.sub(r'(\[rom\][^\[]*?crc32 = ")0x[0-9a-f]+', lambda m: m.group(1) + crc32, config, flags=re.S)
    (FAST / "game.toml").write_text(config)
    return bin_dir / EXE, FAST / "game.toml"


def run(exe: Path, config: Path, args: list[str], env: dict | None = None) -> subprocess.CompletedProcess:
    return subprocess.run([str(exe), "--config", str(config), *args], cwd=FAST, env={**os.environ, **(env or {})},
                          capture_output=True, text=True, errors="replace")


def play(_: argparse.Namespace) -> None:
    exe, config = setup(patched_rom())
    save = FAST / "rotk.sram"
    if not save.exists() and REAL_SAVE.exists():
        shutil.copy2(REAL_SAVE, save)
    print("Opening the game with heroes.json's heroes (save: build/fast/rotk.sram)")
    subprocess.Popen([str(exe), "--config", str(config), "--window", "--save-path", str(save),
                      "--state-dir", str(FAST / "states")], cwd=FAST)


def make_state(args: argparse.Namespace) -> None:
    exe, config = setup(patched_rom())
    save = FAST / "check.sram"
    shutil.copy2(args.save, save)
    result = run(exe, config, ["--no-window", "--frames", str(args.frame), "--volume", "0", "--save-path", str(save),
                               "--state-dir", str(FAST / "states"), "--dump-state", str(FAST / "check.gbas")],
                 {"GBARECOMP_INPUT_REPLAY": str(args.route.resolve())})
    if result.returncode or not (FAST / "check.gbas").exists():
        sys.exit(result.stdout[-2000:] + result.stderr[-2000:])
    (FAST / "check.frame").write_text(str(args.frame))
    print(f"Wrote {FAST / 'check.gbas'}: check runs start from frame {args.frame} of {args.route.name}")


def check(args: argparse.Namespace) -> None:
    from PIL import Image

    state = FAST / "check.gbas"
    if not state.exists():
        sys.exit("No savestate: hero_fast.py state <route.csv> <frame> (a route that ends in a level as the hero)")
    rom = patched_rom()
    exe, config = setup(rom)
    # A savestate names its ROM's SHA-1 (bytes 8..47). The hero data region is all that differs,
    # and the actors point only at its fixed slots, so the state is retargeted to this ROM.
    snapshot = bytearray(state.read_bytes())
    snapshot[8:48] = hashlib.sha1(rom).hexdigest().encode()
    state = FAST / "check-run.gbas"
    state.write_bytes(snapshot)
    frames = FAST / "frames"
    shutil.rmtree(frames, ignore_errors=True)
    frames.mkdir()
    # Presses: one, then three in a row (the melee combo), each held 6 frames.
    presses = [10, 90, 105, 120]
    key = BUTTONS[args.button]
    # The input replay counts frames from boot, so the state's frame offsets the presses.
    start = int((FAST / "check.frame").read_text())
    route = FAST / "check.csv"
    route.write_text("# gbarecomp-keyinput-v1\n0,0x3ff\n" + "".join(
        f"{start + f},0x{key:x}\n{start + f + 6},0x{RELEASED:x}\n" for f in presses))
    shutil.copy2(FAST / "check.sram", FAST / "check-run.sram")
    total = 220
    result = run(exe, config, ["--window", "--frames", str(total), "--volume", "0", "--save-path",
                               str(FAST / "check-run.sram"), "--state-dir", str(FAST / "states"),
                               "--load-state", str(state)],
                 {"GBARECOMP_INPUT_REPLAY": str(route), "GBARECOMP_FRAMEDUMP_DIR": str(frames),
                  "GBARECOMP_FRAMEDUMP_START": "0", "GBARECOMP_FRAMEDUMP_COUNT": str(total),
                  "SDL_VIDEODRIVER": "dummy", "SDL_AUDIODRIVER": "dummy", "SDL_RENDER_DRIVER": "software",
                  "GBARECOMP_DIAGNOSTICS": "0"})
    dumped = sorted(frames.glob("*.png"))
    if not dumped:
        sys.exit("The game wrote no frames:\n" + result.stdout[-2000:] + result.stderr[-2000:])
    step = max(1, args.step)
    picked = dumped[::step]
    box = tuple(args.box)
    width, height = box[2] - box[0], box[3] - box[1]
    columns = 12
    sheet = Image.new("RGB", (width * columns, height * ((len(picked) + columns - 1) // columns)))
    for k, path in enumerate(picked):
        sheet.paste(Image.open(path).crop(box), ((k % columns) * width, (k // columns) * height))
    out = ROOT / "build/preview" / f"check_{''.join(c for c in args.hero if c.isalnum())}.png"
    out.parent.mkdir(parents=True, exist_ok=True)
    sheet.resize((sheet.width * 2, sheet.height * 2), Image.NEAREST).save(out)
    print(f"Wrote {out.relative_to(ROOT)}: every {step} frames, {args.button} at {presses}")


def cards(args: argparse.Namespace) -> None:
    """Every hero select card, group by group, from a route that ends on the hero select."""
    from PIL import Image

    exe, config = setup(patched_rom())
    lines = [line for line in args.route.read_text().splitlines()[1:] if int(line.split(",")[0]) < args.frame]
    heroes = hero_registry.load(ROOT, True)
    groups = list(dict.fromkeys(hero.get("group", "Heroes") for hero in heroes))
    counts = [sum(hero.get("group", "Heroes") == group for hero in heroes) for group in groups]
    # Right moves to the next card, R to the next group; each capture waits for the move to draw.
    presses, shots, at = [], [], args.frame + 20
    for group, count in enumerate(counts):
        for card in range(count):
            shots.append(at)
            if card + 1 < count:
                presses.append((at + 4, 0x3EF))
                at += 24
        if group + 1 < len(counts):
            presses.append((at + 4, 0x2FF))
            at += 30
    route = FAST / "cards.csv"
    route.write_text("# gbarecomp-keyinput-v1\n" + "\n".join(lines) + "\n" + "".join(
        f"{f},0x{key:x}\n{f + 6},0x{RELEASED:x}\n" for f, key in presses))
    frames = FAST / "frames"
    shutil.rmtree(frames, ignore_errors=True)
    frames.mkdir()
    save = FAST / "cards.sram"
    shutil.copy2(args.save, save)
    run(exe, config, ["--no-window", "--frames", str(at + 2), "--volume", "0", "--save-path", str(save),
                      "--state-dir", str(FAST / "states")],
        {"GBARECOMP_INPUT_REPLAY": str(route), "GBARECOMP_FRAMEDUMP_DIR": str(frames),
         "GBARECOMP_FRAMEDUMP_START": str(args.frame), "GBARECOMP_FRAMEDUMP_COUNT": str(at + 2 - args.frame)})
    columns = 4
    sheet = Image.new("RGB", (240 * columns, 160 * ((len(shots) + columns - 1) // columns)))
    for k, frame in enumerate(shots):
        path = frames / f"f_{frame:06d}.png"
        if path.exists():
            sheet.paste(Image.open(path), ((k % columns) * 240, (k // columns) * 160))
    out = ROOT / "build/preview/hero_select.png"
    sheet.resize((sheet.width * 2, sheet.height * 2), Image.NEAREST).save(out)
    print(f"Wrote {out.relative_to(ROOT)}: {len(shots)} cards in {len(groups)} groups")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    commands.add_parser("play", help="open the game with heroes.json's heroes").set_defaults(func=play)
    state = commands.add_parser("state", help="make the savestate check starts from")
    state.add_argument("route", type=Path, help="input route (gbarecomp-keyinput-v1 csv) into a level")
    state.add_argument("frame", type=int, help="frame to save at")
    state.add_argument("--save", type=Path, required=True, help="save file the route starts from")
    state.set_defaults(func=make_state)
    check_parser = commands.add_parser("check", help="attack from the savestate and write a frame sheet")
    check_parser.add_argument("hero", help="name for the output file")
    check_parser.add_argument("--button", choices=sorted(BUTTONS), default="B", help="B attacks, A casts the skill")
    check_parser.add_argument("--step", type=int, default=3, help="sheet every N frames")
    check_parser.add_argument("--box", type=int, nargs=4, default=(40, 20, 150, 130), help="crop x0 y0 x1 y1")
    check_parser.set_defaults(func=check)
    card_parser = commands.add_parser("cards", help="screenshot every hero select card")
    card_parser.add_argument("--route", type=Path, default=ROOT / "hacks/content/routes/hero_select.csv",
                             help="a route that reaches the hero select")
    card_parser.add_argument("--frame", type=int, default=1130, help="frame the route is on the hero select")
    card_parser.add_argument("--save", type=Path, default=ROOT / "build/route_start.sram")
    card_parser.set_defaults(func=cards)
    args = parser.parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
