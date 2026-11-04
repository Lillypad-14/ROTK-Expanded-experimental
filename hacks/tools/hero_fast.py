"""Try hero changes in the PC game in seconds, without rebuilding it.

    uv run python hacks/tools/hero_fast.py play             # open the game with heroes.json's heroes
    uv run python hacks/tools/hero_fast.py push             # send anim/body/sound edits into that game
    uv run python hacks/tools/hero_fast.py combo Witch-king # headless: which combo stage each press reached
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
    print("Opening the game with heroes.json's heroes (save: build/fast/rotk.sram). "
          "hero_fast.py push sends later anim and heroes.json body/sound edits into it.")
    subprocess.Popen([str(exe), "--config", str(config), "--no-launcher", "--window", "--save-path", str(save),
                      "--state-dir", str(FAST / "states"), "--tcp-observe", str(PUSH_PORT)], cwd=FAST)


PUSH_PORT = 19855  # play's debug port (gbarecomp TCP observe mode), which push writes through


def push(_: argparse.Namespace) -> None:
    """Send the hero data region's changes into the game play opened, without restarting it."""
    import socket

    shown = FAST / "rotk.gba"
    if not shown.exists():
        sys.exit("No game from hero_fast.py play to push to")
    old, new = shown.read_bytes(), patched_rom()
    start, end = region_bounds("HERO_DATA")
    kits = region_bounds("HERO_KITS")
    if old[kits[0]:kits[1] + 1] != new[kits[0]:kits[1] + 1]:
        print("Skill kits changed: they are code, which push cannot swap. Restart with hero_fast.py play for those.")
    ranges, i = [], start
    while i <= end:
        if old[i] != new[i]:
            j = i
            while j <= end and (old[j] != new[j] or j - i < 64 and new[j:j + 16] != old[j:j + 16]):
                j += 1
            ranges.append((i, j))
            i = j
        else:
            i += 1
    if not ranges:
        print("Nothing changed in the hero data since the game started")
        return
    chunk = 3500  # bytes per write_mem: a command line holds 8192 characters
    sent = 0
    try:
        with socket.create_connection(("127.0.0.1", PUSH_PORT), timeout=5) as connection:
            reader = connection.makefile("r")
            for a, b in ranges:
                for at in range(a, b, chunk):
                    data = new[at:min(b, at + chunk)]
                    connection.sendall((json.dumps({"cmd": "write_mem", "addr": hero_data.ROM_BASE + at,
                                                    "hex": data.hex()}) + "\n").encode())
                    reply = json.loads(reader.readline())
                    if not reply.get("ok"):
                        sys.exit(f"The game refused the write at 0x{hero_data.ROM_BASE + at:08x}: {reply}")
                    sent += len(data)
    except OSError as error:
        sys.exit(f"No game listening on port {PUSH_PORT} ({error}): start it with hero_fast.py play")
    shown.write_bytes(new[:start] + new[start:end + 1] + old[end + 1:])
    print(f"Pushed {sent} bytes in {len(ranges)} ranges. Poses already playing switch at their next start "
          "(attack again, or walk). A crash right after a push: push while idle.")


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


PLAYER_0 = 0x03003C00  # g_PlayerGlobals.players[0]
# Player field offsets (include/player.h, measured with clang --target=armv4t-none-eabi).
PLAYER_SKILLS_PURCHASED, PLAYER_SKILLS, PLAYER_SPIRIT, PLAYER_SELECTED = 0x16A, 0x170, 0x17E, 0x190


def state_ram(snapshot: bytes, address: int) -> int:
    """The savestate byte offset of an EWRAM or IWRAM address (BUS0 chunk: EWRAM, then IWRAM)."""
    at = 52
    while at < len(snapshot):
        tag, size = snapshot[at:at + 4], int.from_bytes(snapshot[at + 4:at + 8], "little")
        if tag == b"BUS0":
            base = at + 8 if address < 0x03000000 else at + 8 + 0x40000
            return base + (address & (0x3FFFF if address < 0x03000000 else 0x7FFF))
        at += 8 + size
    sys.exit("The savestate has no BUS0 chunk")


def give_skills(snapshot: bytearray, row: int) -> None:
    """Player 1 owns every active row at level 5, has 999 Spirit and has row selected (A casts it)."""
    player = state_ram(snapshot, PLAYER_0)
    snapshot[player + PLAYER_SKILLS_PURCHASED:player + PLAYER_SKILLS_PURCHASED + 6] = bytes([5] * 6)
    snapshot[player + PLAYER_SKILLS:player + PLAYER_SKILLS + 6] = bytes([5] * 6)
    snapshot[player + PLAYER_SPIRIT:player + PLAYER_SPIRIT + 2] = (999).to_bytes(2, "little")
    snapshot[player + PLAYER_SELECTED] = row


PC_INFO_MAGIC = b"RPC1"  # the PC host's info block in host RAM (rotk-recompiled src/main.cpp)
PC_INFO_STRESS_SPAWN = 0x20


def spawn_foes(snapshot: bytearray, count: int) -> None:
    """Ask the mod's debug lab to spawn count orc archers around the hero (and keep him alive). The
    host writes this once at boot, so a loaded savestate needs it in its own copy of host RAM."""
    at = snapshot.find(PC_INFO_MAGIC, state_ram(snapshot, 0x03000000) + 0x8000)
    if at < 0:
        sys.exit("The savestate has no PC info block (not a PC build state?)")
    snapshot[at + PC_INFO_STRESS_SPAWN:at + PC_INFO_STRESS_SPAWN + 4] = count.to_bytes(4, "little")


def capture(rom: bytes, key: int, presses: list[int], total: int, skill: int | None,
            foes: int = 0, face: str | None = None) -> tuple[list[Path], object]:
    """Run total frames from the check savestate, pressing key at presses (held 6 frames); with skill,
    the player owns every row at level 5 and has row skill selected. face taps that direction first
    (frames 2..5) to turn the hero. Returns the dumped frames."""
    state = FAST / "check.gbas"
    if not state.exists():
        sys.exit("No savestate: hero_fast.py state <route.csv> <frame> (a route that ends in a level as the hero)")
    exe, config = setup(rom)
    # A savestate names its ROM's SHA-1 (bytes 8..47). The hero data region is all that differs,
    # and the actors point only at its fixed slots, so the state is retargeted to this ROM.
    snapshot = bytearray(state.read_bytes())
    snapshot[8:48] = hashlib.sha1(rom).hexdigest().encode()
    if skill is not None:
        give_skills(snapshot, skill)
    if foes:
        spawn_foes(snapshot, foes)
    state = FAST / "check-run.gbas"
    state.write_bytes(snapshot)
    frames = FAST / "frames"
    shutil.rmtree(frames, ignore_errors=True)
    frames.mkdir()
    # The input replay counts frames from boot, so the state's frame offsets the presses.
    start = int((FAST / "check.frame").read_text())
    route = FAST / "check.csv"
    turn = f"{start + 2},0x{FACE_KEYS[face]:x}\n{start + 5},0x{RELEASED:x}\n" if face else ""
    route.write_text("# gbarecomp-keyinput-v1\n0,0x3ff\n" + turn + "".join(
        f"{start + f},0x{key:x}\n{start + f + 6},0x{RELEASED:x}\n" for f in presses))
    shutil.copy2(FAST / "check.sram", FAST / "check-run.sram")
    result = run(exe, config, ["--window", "--frames", str(total), "--volume", "0", "--save-path",
                               str(FAST / "check-run.sram"), "--state-dir", str(FAST / "states"),
                               "--load-state", str(state)],
                 {"GBARECOMP_INPUT_REPLAY": str(route), "GBARECOMP_FRAMEDUMP_DIR": str(frames),
                  "GBARECOMP_FRAMEDUMP_START": "0", "GBARECOMP_FRAMEDUMP_COUNT": str(total),
                  "SDL_VIDEODRIVER": "dummy", "SDL_AUDIODRIVER": "dummy", "SDL_RENDER_DRIVER": "software",
                  "GBARECOMP_DIAGNOSTICS": "0", "ROTK_PC_STRESS_SPAWN": str(foes)})
    dumped = sorted(frames.glob("*.png"))
    if not dumped:
        sys.exit("The game wrote no frames:\n" + result.stdout[-2000:] + result.stderr[-2000:])
    return dumped, result


def check(args: argparse.Namespace) -> None:
    from PIL import Image

    # Presses: one, then three in a row (the melee combo), each held 6 frames.
    presses = args.presses or [10, 90, 105, 120]
    dumped, _ = capture(patched_rom(), BUTTONS[args.button], presses, args.frames or max(presses) + 70, args.skill,
                        args.foes, args.face)
    from PIL import ImageDraw

    step = max(1, args.step)
    box = tuple(args.box)
    width, height = box[2] - box[0], box[3] - box[1]
    # Every step-th frame, minus those that look like the last one kept (idle stretches).
    picked, last = [], None
    for n, path in list(enumerate(dumped))[::step]:
        crop = Image.open(path).convert("RGB").crop(box)
        if last is None or args.all_frames or crop.tobytes() != last:
            picked.append((n, crop))
            last = crop.tobytes()
    columns = 12
    sheet = Image.new("RGB", (width * columns, height * ((len(picked) + columns - 1) // columns)))
    draw = ImageDraw.Draw(sheet)
    for k, (n, crop) in enumerate(picked):
        x, y = (k % columns) * width, (k // columns) * height
        sheet.paste(crop, (x, y))
        draw.text((x + 2, y + 1), f"f{n}", fill=(255, 255, 0))
    out = ROOT / "build/preview" / f"check_{''.join(c for c in args.hero if c.isalnum())}.png"
    out.parent.mkdir(parents=True, exist_ok=True)
    sheet.resize((sheet.width * 2, sheet.height * 2), Image.NEAREST).save(out)
    print(f"Wrote {out.relative_to(ROOT)}: {len(picked)} frames (every {step}, unchanged ones skipped, labels are "
          f"game frames after the savestate), {args.button} at {presses}")
    report_colors(args.hero, dumped)


SKILL_ROWS = 6
# Direction keys (active low) for --face.
FACE_KEYS = {"right": 0x3EF, "left": 0x3DF, "up": 0x3BF, "down": 0x37F}
# Rows of the screen the HUD covers (the cast's Spirit cost changes it): ignored when cropping.
HUD_ROWS = 24
SKILL_COLUMNS = 10
CAST_AT = 10


def skills(args: argparse.Namespace) -> None:
    """Cast each first page skill from the savestate (level 5, full Spirit) and sheet them, one row per
    skill, cropped to where anything moved: the whole kit's visuals in one image."""
    from PIL import Image, ImageChops, ImageDraw

    rom = patched_rom()
    spec = hero_kit.spec_path(ROOT, args.hero)
    names = [s["name"] for s in json.loads(spec.read_text())["skills"]] if spec.exists() else []
    rows, box = [], None
    for row in args.rows or range(SKILL_ROWS):
        dumped, _ = capture(rom, BUTTONS["A"], [CAST_AT], args.frames, row, args.foes, args.face)
        images = [Image.open(path).convert("RGB") for path in dumped]
        first = images[0]
        changed = []
        for n, image in enumerate(images):
            diff = ImageChops.difference(image, first).crop((0, 0, image.width, image.height - HUD_ROWS)).getbbox()
            if diff is not None and n >= CAST_AT:
                changed.append(n)
                box = diff if box is None else (min(box[0], diff[0]), min(box[1], diff[1]),
                                                max(box[2], diff[2]), max(box[3], diff[3]))
        # Evenly spaced frames from the cast to the last change.
        last = changed[-1] if changed else args.frames - 1
        picks = sorted({CAST_AT + round(k * (last - CAST_AT) / (SKILL_COLUMNS - 1)) for k in range(SKILL_COLUMNS)})
        label = f"{row} {names[row]}" if row < len(names) else f"row {row}"
        rows.append((label, [(n, images[n]) for n in picks if n < len(images)], not changed))
    pad = 8
    width, height = rows[0][1][0][1].size
    box = (0, 0, width, height) if box is None else (max(0, box[0] - pad), max(0, box[1] - pad),
                                                    min(width, box[2] + pad), min(height, box[3] + pad))
    cw, ch = box[2] - box[0], box[3] - box[1]
    label_h = 12
    sheet = Image.new("RGB", (cw * SKILL_COLUMNS, (ch + label_h) * len(rows)), (24, 24, 32))
    draw = ImageDraw.Draw(sheet)
    for r, (label, picked, quiet) in enumerate(rows):
        y = r * (ch + label_h)
        draw.text((2, y), label + ("   (nothing moved: no cast? check Spirit, target, level)" if quiet else ""),
                  fill=(255, 96, 96) if quiet else (255, 255, 0))
        for k, (n, image) in enumerate(picked):
            sheet.paste(image.crop(box), (k * cw, y + label_h))
            draw.text((k * cw + 2, y + label_h + 1), f"f{n}", fill=(255, 255, 255))
    out = ROOT / "build/preview" / f"skills_{''.join(c for c in args.hero if c.isalnum())}.png"
    out.parent.mkdir(parents=True, exist_ok=True)
    sheet.save(out)
    print(f"Wrote {out.relative_to(ROOT)}: one row per skill, cast at f{CAST_AT}, crop {box}")


DIRECTIONS = (("up", 0x3BF), ("up-right", 0x3AF), ("right", 0x3EF), ("down-right", 0x36F), ("down", 0x37F))
FACING_BYTE = 0xF8  # the player actor's facing: up 0, up-right 1, right 2, down-right 3, down 4


def directions(args: argparse.Namespace) -> None:
    """Face each of the 5 directions, attack, and sheet one row per direction, labelled with the
    game's own facing byte: wrong-way poses show without guessing from pictures."""
    from PIL import Image, ImageDraw

    rom = patched_rom()
    exe, config, state, start = start_from_state(rom)
    gap = 90
    events = []
    for k, (_, key) in enumerate(DIRECTIONS):
        t = 10 + k * gap
        events += [(t, key), (t + 4, RELEASED), (t + 10, BUTTONS["B"]), (t + 14, RELEASED)]
    route = FAST / "directions.csv"
    route.write_text("# gbarecomp-keyinput-v1\n0,0x3ff\n" + "".join(f"{start + t},0x{key:x}\n" for t, key in events))
    frames = FAST / "frames"
    shutil.rmtree(frames, ignore_errors=True)
    frames.mkdir()
    total = 10 + len(DIRECTIONS) * gap
    env = {"GBARECOMP_INPUT_REPLAY": str(route), "GBARECOMP_FRAMEDUMP_DIR": str(frames),
           "GBARECOMP_FRAMEDUMP_START": "0", "GBARECOMP_FRAMEDUMP_COUNT": str(total),
           "SDL_VIDEODRIVER": "dummy", "SDL_AUDIODRIVER": "dummy", "SDL_RENDER_DRIVER": "software",
           "GBARECOMP_DIAGNOSTICS": "0"}
    cache = FAST / "combo_actor.txt"
    actor = int(cache.read_text(), 16) if cache.exists() else None
    trace = FAST / "directions_trace.jsonl"
    if actor is not None:
        env.update({"GBARECOMP_WRAM_TRACE": str(trace), "GBARECOMP_WRAM_TRACE_LO": hex(actor + FACING_BYTE),
                    "GBARECOMP_WRAM_TRACE_HI": hex(actor + FACING_BYTE)})
    run(exe, config, ["--window", "--frames", str(total), "--volume", "0", "--save-path",
                      str(FAST / "check-run.sram"), "--state-dir", str(FAST / "states"), "--load-state", str(state)], env)
    facing_at = {}
    if actor is not None and trace.exists():
        for line in trace.read_text().splitlines():
            record = json.loads(line)
            # A windowed run traces once per game tick (30 Hz) but dumps 60 frames a second.
            facing_at[record["f"] * 2] = int(record["val"], 16)
    dumped = sorted(frames.glob("*.png"))
    if not dumped:
        sys.exit("The game wrote no frames")
    box = tuple(args.box)
    width, height = box[2] - box[0], box[3] - box[1]
    picks = list(range(10, 58, args.step))
    sheet = Image.new("RGB", (width * len(picks), (height + 12) * len(DIRECTIONS)), (30, 30, 36))
    draw = ImageDraw.Draw(sheet)
    for row, (name, _) in enumerate(DIRECTIONS):
        t0 = 10 + row * gap
        known = [v for f, v in sorted(facing_at.items()) if f <= t0 + 10]
        label = f"{name} (game facing {known[-1]})" if known else name
        draw.text((2, row * (height + 12)), label, fill=(255, 255, 0))
        for col, offset in enumerate(picks):
            index = t0 + offset
            if index < len(dumped):
                crop = Image.open(dumped[index]).convert("RGB").crop(box)
                sheet.paste(crop, (col * width, row * (height + 12) + 12))
    out = ROOT / "build/preview" / f"directions_{''.join(c for c in args.hero if c.isalnum())}.png"
    out.parent.mkdir(parents=True, exist_ok=True)
    sheet.resize((sheet.width * 2, sheet.height * 2), Image.NEAREST).save(out)
    print(f"Wrote {out.relative_to(ROOT)}: one row per direction (tap, then B), every {args.step} frames from the "
          "press. Each row should show the same swing seen from that side.")


def report_colors(hero_name: str, frames: list[Path]) -> None:
    """Whether the hero's "#rrggbb" anim colors (piece palettes, drawn by the PC host) reach the screen."""
    from PIL import Image
    import anim_kit

    path = anim_kit.anims_path(hero_name)
    if not path.exists():
        return
    wanted = {tuple(int(h[i:i + 2], 16) & 0xF8 for i in (1, 3, 5))
              for h in re.findall(r'"(#[0-9a-fA-F]{6})"', path.read_text())}
    if not wanted:
        return
    shown, most = 0, 0
    for frame in frames:
        count = sum(1 for rgb in Image.open(frame).convert("RGB").getdata()
                    if tuple(c & 0xF8 for c in rgb) in wanted)
        shown += count > 0
        most = max(most, count)
    if shown:
        print(f"Anim colors: {len(wanted)} \"#rrggbb\" colors in {path.name}, on screen in {shown} of {len(frames)} "
              f"frames (up to {most} px)")
    else:
        print(f"Anim colors: none of the {len(wanted)} \"#rrggbb\" colors of {path.name} reached the screen. Run "
              "with ROTK_PC_PIECE_PALETTE_LOG=1 (rotk-recompiled docs/HOST_PALETTES.md)")


# The player actor's melee bytes (attack state 0x08024602): combo stage, frame index, attack-started flag.
COMBO_STAGE, FRAME_INDEX, ATTACK_STARTED, POSE = 0x9C, 0x22, 0xA0, 0x95
STAGES = {1: "swing 1", 2: "recover 1", 4: "swing 2", 8: "recover 2", 0x10: "finisher", 0x20: "locked"}
HOST_RAM = (0x12000000, 0x123FFFFF)  # where the PC build keeps actors (traced to find the hero)


def start_from_state(rom: bytes) -> tuple[Path, Path, Path, int]:
    """Prepare a run from the check savestate: exe, config, retargeted state, savestate frame."""
    state = FAST / "check.gbas"
    if not state.exists():
        sys.exit("No savestate: hero_fast.py state <route.csv> <frame> (a route that ends in a level as the hero)")
    exe, config = setup(rom)
    snapshot = bytearray(state.read_bytes())
    snapshot[8:48] = hashlib.sha1(rom).hexdigest().encode()
    run_state = FAST / "check-run.gbas"
    run_state.write_bytes(snapshot)
    shutil.copy2(FAST / "check.sram", FAST / "check-run.sram")
    return exe, config, run_state, int((FAST / "check.frame").read_text())


def traced_run(rom: bytes, presses: list[int], frames: int, lo: int, hi: int) -> dict[int, list[tuple[int, int]]]:
    """Press B at the given frames (held 4) and return {address: [(frame, new value), ...]} in [lo, hi]."""
    exe, config, state, start = start_from_state(rom)
    route = FAST / "combo.csv"
    route.write_text("# gbarecomp-keyinput-v1\n0,0x3ff\n" + "".join(
        f"{start + f},0x{BUTTONS['B']:x}\n{start + f + 4},0x{RELEASED:x}\n" for f in presses))
    trace = FAST / "combo_trace.jsonl"
    result = run(exe, config, ["--no-window", "--frames", str(frames), "--volume", "0", "--save-path",
                               str(FAST / "check-run.sram"), "--state-dir", str(FAST / "states"),
                               "--load-state", str(state)],
                 {"GBARECOMP_INPUT_REPLAY": str(route), "GBARECOMP_WRAM_TRACE": str(trace),
                  "GBARECOMP_WRAM_TRACE_LO": hex(lo), "GBARECOMP_WRAM_TRACE_HI": hex(hi),
                  "GBARECOMP_DIAGNOSTICS": "0", "SDL_AUDIODRIVER": "dummy"})
    if result.returncode or not trace.exists():
        sys.exit("The game run failed:\n" + result.stdout[-1500:] + result.stderr[-1500:])
    changes: dict[int, list[tuple[int, int]]] = {}
    for line in trace.read_text().splitlines():
        record = json.loads(line)
        changes.setdefault(int(record["adr"], 16), []).append((record["f"], int(record["val"], 16)))
    return changes


def find_actor(changes: dict) -> int | None:
    """The actor whose combo stage byte stepped through melee stages, from a host RAM trace."""
    for address, values in changes.items():
        seen = {v for _, v in values}
        actor = address - COMBO_STAGE
        if seen <= set(STAGES) | {0} and len(seen & {1, 2, 4, 8, 0x10}) >= 2 and                 all(actor + offset in changes for offset in (ATTACK_STARTED, FRAME_INDEX, POSE)):
            return actor
    return None


def combo_timeline(changes: dict, actor: int, presses: list[int], frames: int) -> list[str]:
    """One line per press and per combo stage change, with how each press was used."""
    def series(offset):
        return dict(changes.get(actor + offset, []))

    stage_at, started_at = series(COMBO_STAGE), series(ATTACK_STARTED)
    events, stage, attacking, entered = [], None, False, 0
    pending = sorted(presses)
    lines = []
    for f in range(frames):
        while pending and pending[0] <= f:
            events.append(pending.pop(0))
            lines.append(f"f{events[-1]:>4}  press B   (during {stage_name(stage, attacking)})")
        if f in stage_at and stage_at[f] != stage:
            previous, stage = stage, stage_at[f]
            note = ""
            if stage == 1 and previous in (2, 8, 0x10):
                note = "  << combo restarted"
            elif previous is not None and stage in (4, 0x10) and previous in (1, 4):
                note = "  (buffered press: recovery skipped)"
            elif previous is not None:
                note = f"  ({stage_name(previous, True)} lasted {f - entered} frames)"
            lines.append(f"f{f:>4}  {STAGES.get(stage, hex(stage)):<9}{note}")
            entered = f
            attacking = True
        if f in started_at and started_at[f] == 0 and attacking:
            attacking = False
            lines.append(f"f{f:>4}  idle      ({stage_name(stage, True)} lasted {f - entered} frames: "
                         "the combo is over, the next press starts swing 1)")
    return lines


def stage_name(stage: int | None, attacking: bool) -> str:
    return STAGES.get(stage, "idle") if attacking and stage else "idle"


def combo(args: argparse.Namespace) -> None:
    """Mash pattern in, combo stages out: does every press chain?"""
    rom = patched_rom()
    cache = FAST / "combo_actor.txt"
    actor = int(cache.read_text(), 16) if cache.exists() and cache.read_text().strip() else None
    changes = None
    if actor is not None:
        changes = traced_run(rom, args.presses, args.frames, actor, actor + 0xFF)
        if find_actor(changes) != actor:
            actor = None
    if actor is None:
        print("Finding the hero actor in host RAM (once per savestate, about 20 s)...")
        changes = traced_run(rom, args.presses, args.frames, *HOST_RAM)
        actor = find_actor(changes)
        if actor is None:
            sys.exit("No actor went through the melee stages: do the presses start an attack? (the savestate's hero "
                     "must be in a level, free to act)")
        cache.write_text(hex(actor))
    print(f"{args.hero}: B at {args.presses} (game frames after the savestate, 60 per second)")
    for line in combo_timeline(changes, actor, args.presses, args.frames):
        print("  " + line)


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
    commands.add_parser("push", help="send anim and body/sound edits into the open game (no restart)").set_defaults(
        func=push)
    state = commands.add_parser("state", help="make the savestate check starts from")
    state.add_argument("route", type=Path, help="input route (gbarecomp-keyinput-v1 csv) into a level")
    state.add_argument("frame", type=int, help="frame to save at")
    state.add_argument("--save", type=Path, required=True, help="save file the route starts from")
    state.set_defaults(func=make_state)
    check_parser = commands.add_parser("check", help="attack from the savestate and write a frame sheet")
    check_parser.add_argument("hero", help="name for the output file")
    check_parser.add_argument("--button", choices=sorted(BUTTONS), default="B", help="B attacks, A casts the skill")
    check_parser.add_argument("--step", type=int, default=3, help="sheet every N frames")
    check_parser.add_argument("--presses", type=lambda s: [int(v) for v in s.split(",")],
                              help="frames after the savestate to press the button, e.g. 10,18,26 (default 10,90,105,120)")
    check_parser.add_argument("--box", type=int, nargs=4, default=(40, 20, 150, 130), help="crop x0 y0 x1 y1")
    check_parser.add_argument("--frames", type=int, help="frames to run (default: 70 past the last press)")
    check_parser.add_argument("--all-frames", action="store_true", help="keep frames that look like the last one")
    check_parser.add_argument("--foes", type=int, default=0, help="spawn this many orc archers around the hero first")
    check_parser.add_argument("--face", choices=sorted(FACE_KEYS), help="turn the hero this way first")
    check_parser.add_argument("--skill", type=int, choices=range(6),
                              help="with --button A: own every skill at level 5, 999 Spirit, cast this row (0..5)")
    check_parser.set_defaults(func=check)
    skills_parser = commands.add_parser("skills", help="cast every first page skill, one sheet row each")
    skills_parser.add_argument("hero", help="hero name (its spec names the rows); the savestate's hero casts")
    skills_parser.add_argument("--rows", type=lambda v: [int(x) for x in v.split(",")], help="only these rows, e.g. 1,5")
    skills_parser.add_argument("--frames", type=int, default=100, help="frames per cast (default 100)")
    skills_parser.add_argument("--face", choices=sorted(FACE_KEYS), help="turn the hero this way before each cast")
    skills_parser.add_argument("--foes", type=int, default=0,
                               help="spawn this many orc archers around the hero first (ROTK_PC_STRESS_SPAWN; it also keeps the hero alive)")
    skills_parser.set_defaults(func=skills)
    directions_parser = commands.add_parser("directions", help="attack facing each of the 5 directions, one row each")
    directions_parser.add_argument("hero", help="name for the output file")
    directions_parser.add_argument("--step", type=int, default=4, help="sheet every N frames")
    directions_parser.add_argument("--box", type=int, nargs=4, default=(50, 20, 190, 140), help="crop x0 y0 x1 y1")
    directions_parser.set_defaults(func=directions)
    combo_parser = commands.add_parser("combo", help="press B on a schedule and print the combo stages it reached")
    combo_parser.add_argument("hero", help="name for the report")
    combo_parser.add_argument("--presses", type=lambda v: [int(x) for x in v.split(",")], default=[10, 30, 50, 70],
                              help="frames after the savestate to press B (default 10,30,50,70: one press every 1/3 s)")
    combo_parser.add_argument("--frames", type=int, default=160, help="frames to run")
    combo_parser.set_defaults(func=combo)
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
