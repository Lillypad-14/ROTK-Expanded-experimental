"""Measure the stock heroes' melee swings: the reference for making new attack animations.

    uv run python hacks/tools/vanilla_study.py                 # every hero, side and down facings
    uv run python hacks/tools/vanilla_study.py --sheets        # also build/anim/vanilla_<hero>.png

Stock heroes draw a pose as up to 8 parts (body, arms, weapon...) animated together (bank
SpriteAnimation *[8 parts][7 poses]). This composites the parts per frame and prints, per combo
piece: steps, distinct drawings, held steps, body motion per step and sprite height. (Blade tracking
is unreliable on multi-part heroes, so the weapon arc is not reported.) Notes on what the numbers
mean: docs/wip-docs/vanilla-swings.md.
"""

from __future__ import annotations

import argparse
import math
import struct
import sys

import anim_kit as K
import anim_studio as A
import hero_clips
from rotkit.spritegfx import parse_frame_set

HEROES = {"Frodo": 0, "Legolas": 1, "Aragorn": 2, "Gandalf": 3, "Eowyn": 4, "Gimli": 5, "Sam": 6, "Smeagol": 7}
PIECES = ((0, "swing 1"), (1, "recover 1"), (2, "swing 2"), (3, "recover 2"), (4, "finisher"))
GREYS = [(255, 0, 255)] + [(40 + 13 * i, 40 + 13 * i, 46 + 13 * i) for i in range(15)]


def u32(rom: bytes, address: int) -> int:
    return struct.unpack_from("<I", rom, address - K.ROM_BASE)[0]


def parts(rom: bytes, hero_id: int, pose: int, facing: int):
    """[(part, duration, [canvas per frame])] of a melee pose."""
    bank = u32(rom, A.HERO_MELEE_BANKS + 4 * hero_id)
    out = []
    for part in range(8):
        animation = u32(rom, bank + part * 0x1C + pose * 4)
        if not 0x08000000 <= animation < 0x0A000000:
            continue
        tiles, header, _, duration = struct.unpack_from("<IIIH", rom, animation + 16 * facing - K.ROM_BASE)
        if not 0x08000000 <= header < 0x0A000000:
            continue
        frame_set = parse_frame_set(hero_clips.frame_set_bytes(rom, header), K.ROM_BASE)
        canvases = []
        for frame in frame_set.frames:
            pixels = K.blank()
            K.draw_frame(pixels, frame, K.decode_frame_tiles(rom[tiles - K.ROM_BASE:], frame))
            canvases.append(pixels)
        if any(any(c) for c in canvases):
            out.append((part, duration, canvases))
    return out


def timeline(found) -> list[bytes]:
    """The composite drawing shown at each tick (parts may step at different speeds)."""
    ticks = max(d * len(c) for _, d, c in found)
    frames = []
    for tick in range(ticks):
        pixels = K.blank()
        for _, duration, canvases in found:
            K.overlay(pixels, canvases[min(tick // duration, len(canvases) - 1)])
        frames.append(bytes(pixels))
    return frames


def centroid(pixels: bytes):
    points = [i for i, v in enumerate(pixels) if v]
    if not points:
        return None
    return sum(p % K.CANVAS for p in points) / len(points), sum(p // K.CANVAS for p in points) / len(points)


def height(pixels: bytes) -> int:
    rows = [i // K.CANVAS for i, v in enumerate(pixels) if v]
    return max(rows) - min(rows) + 1 if rows else 0


def weapon_angles(found) -> list[float | None]:
    """The weapon part's blade angle per tick (degrees, about its hilt), None where no blade shows."""
    best, best_count = None, -1
    for part, duration, canvases in found:
        count = sum(K.find_blade(c) is not None for c in canvases)
        if part and count > best_count:
            best, best_count = (duration, canvases), count
    if best is None:
        return []
    duration, canvases = best
    angles = []
    for canvas in canvases:
        blade = K.find_blade(canvas)
        angle = None
        if blade is not None:
            angle = math.degrees(math.atan2(blade.tip[1] - blade.hilt[1], blade.tip[0] - blade.hilt[0]))
        angles.extend([angle] * duration)
    return angles


def study(rom: bytes, hero_id: int, pose: int, facing: int) -> dict:
    found = parts(rom, hero_id, pose, facing)
    frames = timeline(found)
    body = timeline([p for p in found if p[0] == 0]) if any(p[0] == 0 for p in found) else frames
    held = sum(a == b for a, b in zip(frames, frames[1:]))
    moves = []
    for a, b in zip(body, body[1:]):
        ca, cb = centroid(a), centroid(b)
        moves.append(math.dist(ca, cb) if ca and cb else 0.0)
    angles = weapon_angles(found)
    steps = []
    for a, b in zip(angles, angles[1:]):
        if a is not None and b is not None:
            d = (b - a + 180) % 360 - 180
            steps.append(abs(d))
    fastest = max(range(len(steps)), key=lambda i: steps[i]) + 1 if steps else None
    return {"ticks": len(frames), "frames": len(set(frames)), "held": held, "parts": [p for p, _, _ in found],
            "move_mean": sum(moves) / max(1, len(moves)), "move_max": max(moves, default=0),
            "arc": sum(steps), "arc_step_max": max(steps, default=0), "strike_tick": fastest,
            "height": max(height(f) for f in frames), "timeline": frames}


def sheet(rows: list[tuple[str, list[bytes]]], path) -> None:
    from PIL import Image, ImageDraw

    crop = (36, 36, 156, 156)
    w, h = crop[2] - crop[0], crop[3] - crop[1]
    columns = max(len(frames) for _, frames in rows)
    out = Image.new("RGBA", (w * columns, (h + 12) * len(rows)), (150, 156, 170, 255))
    draw = ImageDraw.Draw(out)
    for r, (label, frames) in enumerate(rows):
        draw.text((2, r * (h + 12)), label, fill=(20, 20, 20, 255))
        for c, frame in enumerate(frames):
            out.alpha_composite(K.image(frame, GREYS).crop(crop), (c * w, r * (h + 12) + 12))
    out = out.resize((out.width * 2, out.height * 2), Image.NEAREST)
    out.save(path)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("heroes", nargs="*", help="default every stock hero")
    parser.add_argument("--facings", type=int, nargs="+", default=[2, 4], help="2 side, 4 down (front)")
    parser.add_argument("--sheets", action="store_true", help="write build/anim/vanilla_<hero>.png (side facing)")
    args = parser.parse_args()
    rom = (A.ROOT / "rotk.gba").read_bytes()
    names = args.heroes or list(HEROES)
    print("piece: ticks (screen frames = ticks x 2) | distinct drawings/held ticks | body px per tick mean/max | "
          "height px | parts")
    for name in names:
        hero_id = HEROES[name]
        rows = []
        for facing in args.facings:
            print(f"{name} facing {facing} ({K.FACING_NAMES[facing]}):")
            for pose, label in PIECES:
                s = study(rom, hero_id, pose, facing)
                print(f"  {label:<10} {s['ticks']:>3} | {s['frames']:>2}/{s['held']:<2} | "
                      f"{s['move_mean']:4.1f}/{s['move_max']:4.1f} | {s['height']:>3} | {s['parts']}")
                if facing == 2 and pose in (0, 2, 4):
                    rows.append((f"{name} {label}", s["timeline"]))
        if args.sheets and rows:
            path = A.ROOT / "build/anim" / f"vanilla_{name.lower()}.png"
            path.parent.mkdir(parents=True, exist_ok=True)
            sheet(rows, path)
            print(f"  wrote {path.relative_to(A.ROOT)}")


if __name__ == "__main__":
    main()
