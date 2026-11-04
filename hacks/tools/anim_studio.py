"""Animation studio: browse, build, review and assign hero animations in seconds (PC edition).

Animations live in hacks/content/anims/<hero>.json (format: anim_kit.py, guide:
docs/wip-docs/animation-studio.md). Every command prints what it found and writes PNGs to
build/anim/ that show the result, so an agent can look and adjust without opening the game.

Browse what exists:
    anim_studio.py enemies                          every enemy with clips
    anim_studio.py clips WitchKing                  its clips: frames, timing, hit frames, boxes
    anim_studio.py sheet WitchKing Attack Stagger   numbered frames with origin, boxes, hotspots
    anim_studio.py palette Witch-king               the 16 colors (indexes for draw, remap)

Make and review:
    anim_studio.py new Witch-king slash Attack:0-16 --speed 1    start an anim from frames
    anim_studio.py show Witch-king slash            frames, onion skin, timeline, frame data
    anim_studio.py show Witch-king slash --vs WitchKing/Attack   side by side with any clip or anim
    anim_studio.py show Witch-king slash --all-facings --gif
    anim_studio.py lint Witch-king                  every anim and pose: errors and jank
    anim_studio.py verify Witch-king                lint + combo timing + the real game (chaining, colors): text
    anim_studio.py stock                            stock heroes' combo timing (the targets)
    anim_studio.py tips Witch-king Attack           where the blade is tracked (smears, tweens use it)
    anim_studio.py combo Witch-king                 the melee chain as the game plays it, with a GIF
    anim_studio.py export Witch-king Attack:5 --name lunge       a frame as PNGs to draw over

Use in the game:
    anim_studio.py assign Witch-king swings @slash @slash2 @finisher
    anim_studio.py assign Witch-king move.hurt @flinch
    anim_studio.py assign Witch-king cast @cast       (all 7 cast poses; cast.2 for one)
    anim_studio.py game Witch-king                  the real PC game, headless (hero_fast.py check)
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import subprocess
import sys

import anim_kit
from anim_kit import CANVAS, FACING_NAMES, FACINGS, GBA_FPS, ORIGIN, AnimError
import hero_clips
import hero_registry

ROOT = anim_kit.ROOT
OUT = ROOT / "build/anim"
HEROES = ROOT / "hacks/content/heroes.json"
MOVE_POSES = ("knocked-down", "death", "getting-up", "hurt", "idle", "knocked-back", "walk")
BG = (40, 38, 46, 255)
CELL_BG = (128, 134, 148, 255)
TEXT = (230, 230, 230, 255)
DIM = (150, 150, 160, 255)
HIT = (255, 60, 60, 255)
HURT = (80, 220, 120, 255)
SPOT = (255, 220, 60, 255)


class Studio:
    def __init__(self):
        self.rom = (ROOT / "rotk.gba").read_bytes()
        self.sources = hero_clips.Sources(ROOT, self.rom)
        self.lib = anim_kit.Library(self.sources, self.rom)
        self.heroes = hero_registry.load(ROOT, True)

    def hero(self, name: str) -> dict:
        for hero in self.heroes:
            if str(hero["id"]) == name or (hero.get("name") or "").lower() == name.lower() or \
                    anim_kit.slug(hero.get("name") or "") == anim_kit.slug(name):
                if "body" not in hero:
                    sys.exit(f"{hero.get('name', name)} has no enemy body (heroes.json \"body\"): the studio edits "
                             "heroes made from enemies (new_hero.py)")
                return hero
        names = ", ".join(h["name"] for h in self.heroes if h.get("name") and "body" in h)
        sys.exit(f"No hero {name!r} with a body. Heroes with bodies: {names}")

    def palette(self, hero: dict):
        """The hero's colors: the 16 of its palette, then the extra "#rrggbb" colors its anims add."""
        if not hasattr(self, "_palettes"):
            self._palettes = {}
        return self._palettes.setdefault(hero["name"], anim_kit.palette_rgb(ROOT, self.rom, hero["palette"]))

    def anims(self, hero: dict) -> dict:
        data = anim_kit.load(hero["name"])
        if not data.get("enemy"):
            data["enemy"] = hero["body"]["enemy"]
        return data

    def build(self, hero: dict, target: str, facing: int, warnings: list[str], anims: dict | None = None):
        """An @anim, or a stock clip ("Clip" or "Enemy/Clip", all frames) as a Built."""
        anims = anims or self.anims(hero)
        if target.startswith("@"):
            return anim_kit.build(self.lib, hero["name"], target[1:], anims, self.palette(hero), facing, warnings,
                                  compress=False)
        enemy, _, clip = target.rpartition("/")
        enemy = enemy or anims["enemy"]
        count = self.lib.clip_facings(enemy, clip)[facing][1].frame_count
        temp = {"enemy": enemy, "anims": {"clip": {"frames": [{"src": f"{enemy}/{clip}:0-{count - 1}"}]}}}
        return anim_kit.build(self.lib, hero["name"], "clip", temp, self.palette(hero), facing, warnings,
                              compress=False)


# ---------------------------------------------------------------------------------------------
# Drawing helpers.


def font(size: int = 10):
    from PIL import ImageFont

    for name in ("consola.ttf", "DejaVuSansMono.ttf", "cour.ttf"):
        try:
            return ImageFont.truetype(name, size)
        except OSError:
            pass
    return ImageFont.load_default()


def content_box(canvases: list[bytes], boxes: list[tuple] = (), margin: int = 6) -> tuple[int, int, int, int]:
    """The canvas rectangle (pixel coordinates) holding every drawn pixel and box, plus a margin."""
    xs, ys = [ORIGIN[0]], [ORIGIN[1]]
    for pixels in canvases:
        for i, v in enumerate(pixels):
            if v:
                xs.append(i % CANVAS)
                ys.append(i // CANVAS)
    for box in boxes:
        if box:
            xs += [box[0] + ORIGIN[0], box[1] + ORIGIN[0]]
            ys += [box[2] + ORIGIN[1], box[3] + ORIGIN[1]]
    return (max(0, min(xs) - margin), max(0, min(ys) - margin),
            min(CANVAS, max(xs) + margin + 1), min(CANVAS, max(ys) + margin + 1))


def stats(pixels: bytes) -> dict | None:
    """Origin-relative bounds, feet line and horizontal center of a frame's pixels."""
    xs = [i % CANVAS - ORIGIN[0] for i, v in enumerate(pixels) if v]
    ys = [i // CANVAS - ORIGIN[1] for i, v in enumerate(pixels) if v]
    if not xs:
        return None
    edge = any(pixels[i] for i in range(CANVAS)) or any(pixels[(CANVAS - 1) * CANVAS + i] for i in range(CANVAS)) or \
        any(pixels[r * CANVAS] or pixels[r * CANVAS + CANVAS - 1] for r in range(CANVAS))
    return {"bounds": (min(xs), min(ys), max(xs), max(ys)), "feet": max(ys), "cx": sum(xs) / len(xs),
            "count": len(xs), "edge": edge}


TIP = (60, 200, 255, 255)


def cell(pixels: bytes, palette, crop, scale: int, step=None, onion: list[bytes] | None = None, path=None):
    """One frame cropped and scaled, with its boxes, hotspots, blade tip and the origin marked.
    path: blade tips to join on an onion skin, numbered by frame."""
    from PIL import Image, ImageDraw

    out = Image.new("RGBA", (CANVAS, CANVAS), CELL_BG)
    if onion:
        for k, layer in enumerate(onion):
            ghost = anim_kit.image(layer, palette)
            alpha = ghost.getchannel("A").point(lambda a, k=k: int(a * (0.12 + 0.5 * k / max(1, len(onion) - 1))))
            ghost.putalpha(alpha)
            out.alpha_composite(ghost)
    if pixels is not None:
        out.alpha_composite(anim_kit.image(pixels, palette))
    out = out.crop(crop).resize(((crop[2] - crop[0]) * scale, (crop[3] - crop[1]) * scale), Image.NEAREST)
    draw = ImageDraw.Draw(out)

    def point(x, y):
        return (x + ORIGIN[0] - crop[0]) * scale, (y + ORIGIN[1] - crop[1]) * scale

    ox, oy = point(0, 0)
    draw.line((ox - 4, oy, ox + 4, oy), fill=DIM)
    draw.line((ox, oy - 4, ox, oy + 4), fill=DIM)
    if step is not None:
        if step.hurt:
            draw.rectangle((*point(step.hurt[0], step.hurt[2]), *point(step.hurt[1], step.hurt[3])), outline=HURT)
        if step.hit:
            draw.rectangle((*point(step.hit[0], step.hit[2]), *point(step.hit[1], step.hit[3])), outline=HIT, width=2)
        for x, y in step.hotspots:
            px, py = point(x, y)
            draw.ellipse((px - 2, py - 2, px + 2, py + 2), outline=SPOT)
        if step.tip:
            px, py = point(*step.tip)
            draw.line((px - 3, py - 3, px + 3, py + 3), fill=TIP)
            draw.line((px - 3, py + 3, px + 3, py - 3), fill=TIP)
    if path:
        marks = [(n, point(*tip)) for n, tip in path if tip]
        for (_, a), (_, b) in zip(marks, marks[1:]):
            draw.line((*a, *b), fill=TIP, width=1)
        for n, (px, py) in marks:
            draw.ellipse((px - 2, py - 2, px + 2, py + 2), fill=TIP)
            draw.text((px + 3, py - 5), str(n), fill=TIP, font=font(9))
    return out


def frame_data(built) -> dict:
    """Timing of a built facing in game frames: total, and the first hit's startup, active, recovery."""
    per = anim_kit.screen_frames(built.duration)
    total = len(built.steps) * per
    hits = built.hits
    data = {"per_step": per, "steps": len(built.steps), "total": total, "ms": round(total * 1000 / GBA_FPS)}
    if hits:
        first = hits[0]
        last = first
        while last + 1 < len(built.steps) and built.steps[last + 1].hit:
            last += 1
        data.update(startup=first * per, active=(last - first + 1) * per, recovery=total - (last + 1) * per,
                    hit_steps=hits)
    return data


def describe_timing(data: dict) -> str:
    text = f"{data['steps']} steps x {data['per_step']}f = {data['total']} frames ({data['ms']} ms)"
    if "startup" in data:
        text += f", hit: startup {data['startup']}f, active {data['active']}f, recovery {data['recovery']}f"
        if len(data["hit_steps"]) > 1:
            text += f", {len(data['hit_steps'])} hit steps {data['hit_steps']}"
    else:
        text += ", no hit"
    return text


BAND = (-18, 4)  # the rows near the feet (origin-relative y) the slide check aligns
SLIDE = 8


def band_mask(pixels: bytes) -> set[tuple[int, int]]:
    return {(i % CANVAS - ORIGIN[0], i // CANVAS - ORIGIN[1]) for i in
            range((ORIGIN[1] + BAND[0]) * CANVAS, (ORIGIN[1] + BAND[1]) * CANVAS) if pixels[i]}


def slide(previous: bytes, current: bytes) -> tuple[int, int] | None:
    """How far the lower body moved since the previous frame: the shift that best lines up the rows
    near the feet, when it lines them up clearly better than no shift. None when it stays put."""
    a, b = band_mask(previous), band_mask(current)
    if len(a) < 20 or len(b) < 20:
        return None
    still = len(a & b)
    best, shift = still, (0, 0)
    for dy in range(-3, 4):
        for dx in range(-SLIDE, SLIDE + 1):
            score = sum((x - dx, y - dy) in a for x, y in b)
            if score > best:
                best, shift = score, (dx, dy)
    if shift != (0, 0) and best > still * 1.3 + 8 and (abs(shift[0]) >= 2 or abs(shift[1]) >= 2):
        return shift
    return None


def spacing(built) -> tuple[str, list[str]]:
    """How far the blade tip travels between frames, and the jumps that read as a pop."""
    entries = [s for s in built.steps if s.first]
    moves = []
    for a, b in zip(entries, entries[1:]):
        if a.tip and b.tip:
            moves.append((b, round(((b.tip[0] - a.tip[0]) ** 2 + (b.tip[1] - a.tip[1]) ** 2) ** 0.5)))
        else:
            moves.append((b, None))
    known = sorted(d for _, d in moves if d)
    if not known:
        return "  blade: not tracked (anim_studio.py tips)", []
    median = known[len(known) // 2]
    issues = []
    for (step, distance), previous in zip(moves, entries):
        if distance and distance > max(14, 2.5 * median):
            source = previous.source if not previous.source.startswith(("png", "tween")) else "<previous frame>"
            target = step.source if not step.source.startswith(("png", "tween")) else "<this frame>"
            issues.append(f"#{step.frame} ({step.source}): the blade tip jumps {distance} px (typical {median}). "
                          f"An in-between before it smooths it: {{\"tween\": {{\"from\": \"{source}\", "
                          f"\"to\": \"{target}\", \"t\": 0.5}}}}")
    text = "  blade tip travel per frame: " + " ".join("-" if d is None else str(d) for _, d in moves)
    return text, issues


def entry_report(built) -> tuple[list[str], list[str]]:
    """Per frame: source, hold, start time, bounds and hitbox. And jank: slides, edge clipping."""
    lines, issues = [], []
    per = anim_kit.screen_frames(built.duration)
    start = 0
    entries = [(i, s) for i, s in enumerate(built.steps) if s.first]
    for i, (index, step) in enumerate(entries):
        hold = (entries[i + 1][0] if i + 1 < len(entries) else len(built.steps)) - index
        info = stats(built.canvases[index])
        flags = []
        if step.hit:
            flags.append(f"HIT {list(step.hit)}")
        name = f"#{step.frame} (entry {step.entry}, {step.source})"
        if info and info["edge"]:
            flags.append("touches canvas edge")
            issues.append(f"{name} touches the canvas edge: pixels may be cut off")
        if i:
            moved = slide(built.canvases[entries[i - 1][0]], built.canvases[index])
            if moved:
                flags.append(f"slides {moved[0]:+d},{moved[1]:+d}")
                issues.append(f"{name}: the body slides {moved[0]:+d},{moved[1]:+d} px from the last frame "
                              f"(\"dx\": {-moved[0]}, \"dy\": {-moved[1]} on this frame holds it in place)")
        bounds = info["bounds"] if info else "empty"
        tip = f"tip {list(step.tip)}" if step.tip else "tip -"
        lines.append(f"  #{step.frame:<3} entry {step.entry:<3} {step.source:<22} x{hold:<2} t={start:>3}f  "
                     f"bounds {bounds}  {tip}" + (f"  {'; '.join(flags)}" if flags else ""))
        start += hold * per
    text, jumps = spacing(built)
    return lines + [text], issues + jumps


def strip(title: str, built, palette, scale: int, crop, show_onion: bool = True):
    """One facing: a title, one cell per frame entry (hold, time, hit marked), an onion skin, a timeline."""
    from PIL import Image, ImageDraw

    entries = [(i, s) for i, s in enumerate(built.steps) if s.first]
    cells = []
    per = anim_kit.screen_frames(built.duration)
    time = 0
    for k, (index, step) in enumerate(entries):
        end = entries[k + 1][0] if k + 1 < len(entries) else len(built.steps)
        hold = end - index
        image = cell(built.canvases[index], palette, crop, scale, step)
        label = f"#{step.frame} {step.source}" + (f" e{step.entry}" if step.entry != step.frame else "")
        sub = f"t{time} x{hold}" + (" HIT" if any(s.hit for s in built.steps[index:end]) else "")
        cells.append((image, label, sub, bool(step.hit)))
        time += hold * per
    if show_onion:
        onion = cell(None, palette, crop, scale, None, [built.canvases[i] for i, _ in entries],
                     [(s.frame, s.tip) for _, s in entries])
        cells.append((onion, "onion + blade path", "cyan: tip per frame", False))
    width = max(image.width for image, *_ in cells)
    height = cells[0][0].height
    label_font = font(10)
    columns = min(len(cells), max(1, 1800 // (width + 4)))
    rows = (len(cells) + columns - 1) // columns
    cell_h = height + 26
    timeline_h = 22
    out = Image.new("RGBA", (max(columns * (width + 4) + 4, 420), 18 + rows * cell_h + timeline_h + 6), BG)
    draw = ImageDraw.Draw(out)
    draw.text((4, 3), title, fill=TEXT, font=label_font)
    for n, (image, label, sub, hit) in enumerate(cells):
        x = 4 + (n % columns) * (width + 4)
        y = 18 + (n // columns) * cell_h
        out.alpha_composite(image, (x, y + 12))
        if hit:
            draw.rectangle((x - 1, y + 11, x + image.width, y + 12 + image.height), outline=HIT)
        draw.text((x, y), label[:max(8, width // 6)], fill=TEXT, font=label_font)
        draw.text((x, y + 12 + image.height + 1), sub, fill=HIT if "HIT" in sub else DIM, font=label_font)
    # Timeline: one block per step, width proportional to game frames, hit steps red.
    y = 18 + rows * cell_h + 2
    total = len(built.steps) * per
    span = out.width - 8
    for i, step in enumerate(built.steps):
        x0 = 4 + span * i * per // total
        x1 = 4 + span * (i + 1) * per // total - 1
        color = HIT if step.hit else ((110, 110, 130, 255) if step.first else (80, 80, 96, 255))
        draw.rectangle((x0, y, x1, y + 10), fill=color)
    draw.text((4, y + 11), f"0f", fill=DIM, font=label_font)
    draw.text((out.width - 40, y + 11), f"{total}f", fill=DIM, font=label_font)
    return out


def stack(images, path: Path) -> Path:
    from PIL import Image

    width = max(i.width for i in images)
    out = Image.new("RGBA", (width, sum(i.height for i in images) + 4 * len(images)), BG)
    y = 0
    for image in images:
        out.alpha_composite(image, (0, y))
        y += image.height + 4
    path.parent.mkdir(parents=True, exist_ok=True)
    out.save(path)
    return path


def facings_of(args) -> list[int]:
    return list(range(FACINGS)) if getattr(args, "all_facings", False) else [args.facing]


# ---------------------------------------------------------------------------------------------
# Commands.


def cmd_enemies(studio: Studio, args) -> None:
    for enemy in studio.lib.enemies():
        clips = studio.lib.clips(enemy)
        print(f"{enemy:<26} {len(clips):>2} clips: {', '.join(clips)}")


def cmd_clips(studio: Studio, args) -> None:
    clips = studio.lib.clips(args.enemy)
    if not clips:
        sys.exit(f"No clips for {args.enemy!r} (anim_studio.py enemies lists them)")
    print(f"{args.enemy}: clip, frames, speed (ticks per step), length, hit frames, hotspots, boxes, "
          f"bounds (facing {args.facing})")
    for clip in clips:
        stock, frame_set = studio.lib.clip_facings(args.enemy, clip)[args.facing]
        hits = [i for i, f in enumerate(frame_set.frames) if len(f.boxes) > 1 and f.boxes[1].enable]
        total = frame_set.frame_count * anim_kit.screen_frames(stock.duration)
        from rotkit.spritegfx import frame_bounds
        print(f"  {clip:<14} {frame_set.frame_count:>3} frames  speed {stock.duration:<2} {total:>4}f "
              f"({round(total * 1000 / GBA_FPS):>4} ms)  hits {hits or '-'}  hotspots {frame_set.hotspot_count}  "
              f"boxes {frame_set.trigger_box_count}  bounds {frame_bounds(frame_set.frames)}")


def cmd_sheet(studio: Studio, args) -> None:
    hero_palette = None
    if args.hero:
        hero_palette = studio.palette(studio.hero(args.hero))
    palette = hero_palette or palette_for_enemy(studio, args.enemy)
    images = []
    warnings: list[str] = []
    for clip in args.clips or studio.lib.clips(args.enemy):
        for facing in facings_of(args):
            fake = {"name": args.hero or "sheet", "palette": 0, "body": {"enemy": args.enemy}}
            count = studio.lib.clip_facings(args.enemy, clip)[facing][1].frame_count
            temp = {"enemy": args.enemy, "anims": {"c": {"frames": [{"src": f"{clip}:0-{count - 1}"}]}}}
            built = anim_kit.build(studio.lib, fake["name"], "c", temp, palette, facing, warnings, compress=False)
            crop = content_box(built.canvases, [s.hit for s in built.steps])
            title = f"{args.enemy}/{clip} facing {facing} ({FACING_NAMES[facing]}): {describe_timing(frame_data(built))}"
            images.append(strip(title, built, palette, args.scale, crop, show_onion=False))
    name = f"sheet_{args.enemy}" + ("_" + "_".join(args.clips) if args.clips else "")
    path = stack(images, OUT / f"{name}.png")
    print(f"Wrote {path.relative_to(ROOT)}: labels are the \"Clip:index\" a frame's src takes")


def palette_for_enemy(studio: Studio, enemy: str):
    import new_hero

    unit_type, _ = new_hero.find(enemy)
    return anim_kit.palette_rgb(ROOT, studio.rom, studio.rom[new_hero.ENEMY_PALETTES - anim_kit.ROM_BASE + unit_type])


def cmd_palette(studio: Studio, args) -> None:
    from PIL import Image, ImageDraw

    hero = studio.hero(args.hero)
    palette = studio.palette(hero)
    out = Image.new("RGBA", (16 * 40, 56), BG)
    draw = ImageDraw.Draw(out)
    for i, rgb in enumerate(palette):
        draw.rectangle((i * 40 + 2, 2, i * 40 + 38, 38), fill=(*rgb, 255) if i else CELL_BG, outline=DIM)
        draw.text((i * 40 + 12, 42), str(i), fill=TEXT, font=font(11))
        print(f"  {i:>2}  #{rgb[0]:02x}{rgb[1]:02x}{rgb[2]:02x}" + ("  (transparent)" if i == 0 else ""))
    path = OUT / f"palette_{anim_kit.slug(hero['name'])}.png"
    path.parent.mkdir(parents=True, exist_ok=True)
    out.save(path)
    print(f"Wrote {path.relative_to(ROOT)}: draw and remap take these indexes")


def cmd_new(studio: Studio, args) -> None:
    hero = studio.hero(args.hero)
    data = anim_kit.load(hero["name"])
    data.setdefault("enemy", hero["body"]["enemy"])
    if data.get("enemy") is None:
        data["enemy"] = hero["body"]["enemy"]
    name = args.anim.lstrip("@")
    if name in data["anims"] and not args.force:
        sys.exit(f"@{name} exists: --force replaces it, or edit {anim_kit.anims_path(hero['name']).relative_to(ROOT)}")
    anim = {"frames": [{"src": ref} for ref in args.frames]}
    if args.speed is not None:
        anim = {"speed": args.speed, **anim}
    data["anims"][name] = anim
    warnings: list[str] = []
    anim_kit.build(studio.lib, hero["name"], name, data, studio.palette(hero), 2, warnings, compress=False)
    path = anim_kit.save(hero["name"], data)
    print(f"@{name} in {path.relative_to(ROOT)}. Next: anim_studio.py show {hero['name']} {name}")


def cmd_show(studio: Studio, args) -> None:
    hero = studio.hero(args.hero)
    palette = studio.palette(hero)
    anims = studio.anims(hero)
    targets = [t if t.startswith("@") or "/" in t or t in studio.lib.clips(anims["enemy"]) else "@" + t
               for t in (args.anims or [f"@{n}" for n in anims["anims"]])]
    if not targets:
        sys.exit(f"{hero['name']} has no anims yet: anim_studio.py new {hero['name']} <name> <Clip:i-j>")
    images = []
    for target in targets:
        group = [target] + ([args.vs] if args.vs else [])
        built_by = {}
        warnings: list[str] = []
        try:
            for facing in facings_of(args):
                for t in group:
                    built_by[(t, facing)] = studio.build(hero, t, facing, warnings, anims)
        except AnimError as error:
            print(f"ERROR {target}: {error}")
            continue
        main = built_by[(target, facings_of(args)[0])]
        print(f"{target} facing {facings_of(args)[0]}: {describe_timing(frame_data(main))}")
        lines, issues = entry_report(main)
        print("\n".join(lines))
        for warning in dict.fromkeys(warnings):
            print(f"  warning: {warning}")
        for issue in issues:
            print(f"  check: {issue}")
        for facing in facings_of(args):
            # One crop for the whole group, so compared anims share a scale and position.
            group_built = [built_by[(t, facing)] for t in group]
            crop = content_box([c for b in group_built for c in b.canvases],
                               [s.hit for b in group_built for s in b.steps])
            for t, built in zip(group, group_built):
                title = f"{t} facing {facing} ({FACING_NAMES[facing]}): {describe_timing(frame_data(built))}"
                images.append(strip(title, built, palette, args.scale, crop))
        if args.gif:
            write_gifs(hero, target, built_by, group, facings_of(args), palette, args.scale)
    if images:
        name = "_".join(t.lstrip("@").replace("/", "-") for t in targets[:4]) + (f"_vs_{args.vs.lstrip('@').replace('/', '-')}" if args.vs else "")
        path = stack(images, OUT / f"{anim_kit.slug(hero['name'])}_{name}.png")
        print(f"Wrote {path.relative_to(ROOT)}: red box hitbox, green hurtbox, yellow hotspots, + the origin (feet)")


def write_gifs(hero, target, built_by, group, facings, palette, scale) -> None:
    from PIL import Image

    for facing in facings:
        builts = [built_by[(t, facing)] for t in group]
        crop = content_box([c for b in builts for c in b.canvases])
        # Real time: one GIF frame per game frame would be 16.7 ms; GIF delays are 10 ms units, so
        # each step is shown for its game frames rounded to 10 ms.
        length = max(len(b.steps) * anim_kit.screen_frames(b.duration) for b in builts)
        frames = []
        for t in range(0, length, 1):
            row = Image.new("RGBA", ((crop[2] - crop[0]) * scale * len(builts), (crop[3] - crop[1]) * scale), BG)
            for k, b in enumerate(builts):
                step = min(t // anim_kit.screen_frames(b.duration), len(b.steps) - 1)
                row.alpha_composite(cell(b.canvases[step], palette, crop, scale, b.steps[step]),
                                    (k * (crop[2] - crop[0]) * scale, 0))
            frames.append(row)
        path = OUT / f"{anim_kit.slug(hero['name'])}_{target.lstrip('@').replace('/', '-')}_f{facing}.gif"
        frames[0].save(path, save_all=True, append_images=frames[1:], duration=round(1000 / GBA_FPS), loop=0,
                       disposal=2)
        print(f"Wrote {path.relative_to(ROOT)} (real speed)")


def cmd_tips(studio: Studio, args) -> None:
    """Where the blade is tracked in a clip's frames, to check before smears and tweens use it."""
    hero = studio.hero(args.hero)
    anims = studio.anims(hero)
    palette = studio.palette(hero)
    images = []
    for facing in facings_of(args):
        count = studio.lib.clip_facings(anims["enemy"], args.clip)[facing][1].frame_count
        temp = {"enemy": anims["enemy"], "tips": anims.get("tips"),
                "anims": {"c": {"frames": [{"src": f"{args.clip}:0-{count - 1}"}]}}}
        built = anim_kit.build(studio.lib, hero["name"], "c", temp, palette, facing, [], compress=False)
        print(f"{args.clip} facing {facing} ({FACING_NAMES[facing]}): frame: tip, hilt, confidence")
        for i in range(count):
            blade = studio.lib.blade(f"{args.clip}:{i}", anims["enemy"], facing, anims.get("tips"))
            text = (f"tip {list(blade.tip)} hilt {list(blade.hilt)} confidence {blade.confidence:.2f}"
                    if blade else "none found")
            print(f"  {args.clip}:{i:<3} {text}")
        crop = content_box(built.canvases)
        images.append(strip(f"{args.clip} facing {facing}: cyan x = tracked blade tip", built, palette, args.scale,
                            crop))
    path = stack(images, OUT / f"tips_{anim_kit.slug(hero['name'])}_{args.clip}.png")
    print(f"Wrote {path.relative_to(ROOT)}. Wrong tips: add \"tips\": {{\"{args.clip}:i\": [x, y] or "
          f"{{\"facing\": [x, y]}}}} to {anim_kit.anims_path(hero['name']).name} (or [x, y, hiltX, hiltY])")


COMBO = (("move", 4, "idle"), ("melee", 0, "swing 1"), ("melee", 1, "recover 1"), ("melee", 2, "swing 2"),
         ("melee", 3, "recover 2"), ("melee", 4, "finisher"), ("move", 4, "idle"))
# The game chains a press during a swing (buffered) or during recovery frames 0..CHAIN_FRAMES - 1;
# later presses restart the combo at swing 1 (docs/wip-docs/animation-studio.md, Combo feel).
CHAIN_FRAMES = 5
TICK = anim_kit.TICK_FRAMES
MIN_RECOVERY = 10  # screen frames: shorter recoveries eat late presses (stock: 10 to 16)
HERO_MELEE_BANKS = 0x082828B8  # SpriteAnimation *[7] per stock hero id
STOCK_HEROES = {"Frodo": 0, "Aragorn": 2, "Gandalf": 3, "Eowyn": 4, "Gimli": 5, "Sam": 6}


def stock_melee(rom: bytes, hero_id: int, facing: int) -> list:
    """A stock hero's melee poses 0..4 (swing 1, recovery, swing 2, recovery, finisher) as Builts."""
    import struct

    def u32(address):
        return struct.unpack_from("<I", rom, address - anim_kit.ROM_BASE)[0]

    bank = u32(HERO_MELEE_BANKS + 4 * hero_id)
    poses = []
    for pose in range(5):
        tiles, header, _, duration = struct.unpack_from("<IIIH", rom, u32(bank + 4 * pose) + 16 * facing
                                                        - anim_kit.ROM_BASE)
        poses.append(facing_built(hero_clips.Facing("stock", tiles, hero_clips.frame_set_bytes(rom, header),
                                                    duration), rom))
    return poses


SMOOTH_JUMP = 6  # body px per tick: stock swings move 1 to 2 on average, at most 3 to 8 (vanilla-swings.md)


def smoothness(built) -> list[str]:
    """Stutter and pops against the stock swings: held ticks before the hit (stock: none) and body
    jumps (the centroid of palette pixels, so \"#rrggbb\" effects do not count)."""
    def centroid(pixels):
        points = [i for i, v in enumerate(pixels) if 0 < v < 16]
        if not points:
            return None
        return (sum(p % CANVAS for p in points) / len(points), sum(p // CANVAS for p in points) / len(points))

    hits = built.hits
    end = hits[0] if hits else len(built.canvases)
    held = sum(a == b for a, b in zip(built.canvases[:end], built.canvases[1:end]))
    issues = []
    if held:
        issues.append(f"{held} held ticks before the hit (stock swings: none; a rig moves something every tick)")
    centers = [centroid(c) for c in built.canvases]
    for k, (a, b) in enumerate(zip(centers, centers[1:]), 1):
        if a and b and ((a[0] - b[0]) ** 2 + (a[1] - b[1]) ** 2) ** 0.5 > SMOOTH_JUMP:
            issues.append(f"#{k}: the body jumps {((a[0] - b[0]) ** 2 + (a[1] - b[1]) ** 2) ** 0.5:.0f} px in one "
                          f"tick (stock: at most 3 to 8)")
    return issues


def combo_checks(melee: list) -> list[str]:
    """Combo feel problems in melee poses 0..4 (Builts): recoveries that eat presses or cannot chain."""
    problems = []
    for pose, label in ((1, "recover 1"), (3, "recover 2")):
        built = melee[pose]
        data = frame_data(built)
        total = data["total"] - data["per_step"] + TICK  # the game ends a pose one tick into its last frame
        if total < MIN_RECOVERY:
            problems.append(f"{label} shows {total} frames in game: presses after it restart the combo at swing 1. "
                            f"Aim for {MIN_RECOVERY} to 16 (heroes.json body \"recoveryTicks\": 8)")
        if len(built.steps) > CHAIN_FRAMES:
            problems.append(f"{label} has {len(built.steps)} frames: presses on frames {CHAIN_FRAMES}+ do not chain")
    for pose, label in ((0, "swing 1"), (2, "swing 2")):
        total = frame_data(melee[pose])["total"]
        if total < 14:
            problems.append(f"{label} lasts {total} frames: a short swing leaves little time to buffer the next "
                            "press (stock swings: 18 to 30)")
    return problems


def facing_built(facing, rom: bytes, palette: list | None = None):
    """A built hero pose facing (hero_clips.Facing) as a Built: decoded frames, hits, tracked tips.
    palette (the studio's list) receives the colors of pieces with their own palettes."""
    import struct
    from rotkit.spritegfx import parse_frame_set

    def own_ids(n: int):
        if not facing.palettes or palette is None or n >= len(facing.palettes):
            return None
        ids = []
        for colors in facing.palettes[n]:
            if colors is None:
                ids.append(None)
                continue
            values = struct.unpack("<16H", colors)[1:]
            ids.append(tuple(anim_kit.extra_color(palette, ((c & 31) << 3, ((c >> 5) & 31) << 3, ((c >> 10) & 31) << 3))
                             for c in values))
        return ids

    tiles = facing.tiles_data if facing.tiles_data is not None else rom[facing.tiles_address - anim_kit.ROM_BASE:]
    frame_set = parse_frame_set(facing.frames, anim_kit.ROM_BASE)
    steps, canvases = [], []
    for n, frame in enumerate(frame_set.frames):
        pixels = anim_kit.blank()
        anim_kit.draw_frame(pixels, frame, anim_kit.decode_frame_tiles(tiles, frame), own_ids(n))
        box = lambda i: frame.boxes[i] if len(frame.boxes) > i and frame.boxes[i].enable else None
        hit, hurt = box(anim_kit.HITBOX), box(anim_kit.HURTBOX)
        blade = anim_kit.find_blade(pixels)
        steps.append(anim_kit.Step(n, n, f"{n}", True, hit and (hit.x_min, hit.x_max, hit.y_min, hit.y_max),
                                   hurt and (hurt.x_min, hurt.x_max, hurt.y_min, hurt.y_max), [],
                                   blade and blade.tip, blade and blade.hilt))
        canvases.append(bytes(pixels))
    return anim_kit.Built(facing.frames, bytes(tiles[:0]), facing.duration, steps, canvases)


def cmd_stock(studio: Studio, args) -> None:
    """Every stock hero's melee combo timing: the targets a new hero's combo is compared with."""
    labels = ("swing 1", "recover 1", "swing 2", "recover 2", "finisher")
    print(f"Stock melee combos, facing {args.facing}, screen frames (60/s). A recovery shows one tick less in game.")
    print("  " + f"{'hero':<9}" + "".join(f"{label:>11}" for label in labels))
    for name, hero_id in STOCK_HEROES.items():
        poses = stock_melee(studio.rom, hero_id, args.facing)
        print("  " + f"{name:<9}" + "".join(f"{frame_data(p)['total']:>11}" for p in poses))


def cmd_verify(studio: Studio, args) -> None:
    """Everything about a hero's melee in one go, as text: lint, combo timing, the real game chaining a
    relaxed and a mashed rhythm, and its \"#rrggbb\" colors on screen."""
    hero = studio.hero(args.hero)
    name = hero["name"]
    tools = ROOT / "hacks/tools"

    def run(*command: str) -> str:
        result = subprocess.run([sys.executable, "-W", "ignore", *command], cwd=ROOT, capture_output=True, text=True)
        return result.stdout + result.stderr

    results = []
    lint = run(str(tools / "anim_studio.py"), "lint", name)
    lines = [line.strip() for line in lint.splitlines() if line.startswith(("ERROR", "look", "      "))]
    errors = sum(line.startswith("ERROR") for line in lines)
    results.append(("FAIL" if errors else "LOOK" if lines else "PASS", "lint", lines))
    combo = run(str(tools / "anim_studio.py"), "combo", name)
    checks = [line.strip() for line in combo.splitlines() if "check:" in line]
    versus = next((line.strip() for line in combo.splitlines() if " vs " in line), "")
    results.append(("LOOK" if checks else "PASS", "combo timing", [versus] + checks))
    if not args.static:
        for rhythm, presses in (("relaxed", "10,32,54"), ("mash", ",".join(str(f) for f in range(10, 80, 6)))):
            out = run(str(tools / "hero_fast.py"), "combo", name, "--presses", presses, "--frames", "150")
            stages = [line.strip() for line in out.splitlines() if line.strip().startswith("f") and "press B" not in line]
            reached = any("finisher" in line for line in stages)
            restarted = any("combo restarted" in line for line in stages)
            status = "PASS" if reached and not (restarted and rhythm == "relaxed") else "FAIL"
            results.append((status, f"in game, {rhythm} presses ({presses if rhythm == 'relaxed' else 'every 6'})",
                            [] if status == "PASS" else stages or out.strip().splitlines()[-3:]))
        check = run(str(tools / "hero_fast.py"), "check", name, "--presses", "10")
        colors = next((line.strip() for line in check.splitlines() if line.startswith("Anim colors")), None)
        if colors:
            results.append(("FAIL" if "none of" in colors else "PASS", "colors on screen", [colors]))
        sheet = next((line.split(":")[0].replace("Wrote ", "") for line in check.splitlines()
                      if line.startswith("Wrote")), None)
        if sheet:
            results.append(("INFO", f"in-game sheet (look only if something fails): {sheet}", []))
    for status, what, details in results:
        print(f"{status:<5}{what}")
        for line in details:
            print(f"       {line}")
    failed = sum(status == "FAIL" for status, _, _ in results)
    print(f"{failed} failed, {sum(s == 'LOOK' for s, _, _ in results)} to look at")


def cmd_combo(studio: Studio, args) -> None:
    """The melee chain as the game plays it: idle, swing 1, recovery, swing 2, recovery, finisher."""
    from PIL import Image

    hero = studio.hero(args.hero)
    palette = studio.palette(hero)
    banks = hero_clips.body_banks(ROOT, studio.rom, hero, studio.sources)
    images, gif_frames = [], []
    for facing in facings_of(args):
        parts = [(label, facing_built(banks[bank][pose].facings[facing], studio.rom, palette))
                 for bank, pose, label in COMBO]
        crop = content_box([c for _, b in parts for c in b.canvases])
        total = 0
        print(f"combo facing {facing} ({FACING_NAMES[facing]}):")
        for label, built in parts:
            data = frame_data(built)
            total += data["total"]
            print(f"  {label:<10} {describe_timing(data)}")
            images.append(strip(f"{label} facing {facing}: {describe_timing(data)}", built, palette, args.scale, crop,
                                show_onion=False))
            if facing == facings_of(args)[0]:
                for step, pixels in zip(built.steps, built.canvases):
                    for _ in range(anim_kit.screen_frames(built.duration)):
                        gif_frames.append(cell(pixels, palette, crop, args.scale, step))
        print(f"  whole chain {total} frames ({round(total * 1000 / GBA_FPS)} ms), idle to idle")
        mine = [built for (bank, _, _), (_, built) in zip(COMBO, parts) if bank == "melee"]
        stock = stock_melee(studio.rom, STOCK_HEROES[args.vs], facing)
        print(f"  vs {args.vs}: " + ", ".join(
            f"{label} {frame_data(m)['total']}/{frame_data(o)['total']}"
            for label, m, o in zip(("swing 1", "recover 1", "swing 2", "recover 2", "finisher"), mine, stock))
            + " frames (this hero/stock)")
        for problem in combo_checks(mine):
            print(f"  check: {problem}")
    path = stack(images, OUT / f"combo_{anim_kit.slug(hero['name'])}.png")
    gif = OUT / f"combo_{anim_kit.slug(hero['name'])}_f{facings_of(args)[0]}.gif"
    gif_frames[0].save(gif, save_all=True, append_images=gif_frames[1:], duration=round(1000 / GBA_FPS), loop=0,
                       disposal=2)
    print(f"Wrote {path.relative_to(ROOT)} and {gif.relative_to(ROOT)} (real speed). The game starts the next swing "
          "when attack is pressed again during a recovery.")


def body_targets(hero: dict) -> list[tuple[str, object]]:
    body = hero["body"]
    out = [(f"move.{MOVE_POSES[i]}", p) for i, p in enumerate(body.get("move", []))]
    out += [(f"swing.{i}", p) for i, p in enumerate(body.get("swings", []))]
    out += [(f"melee.{i}", p) for i, p in enumerate(body.get("melee", []))]
    out += [(f"cast.{i}", p) for i, p in enumerate(body.get("cast", []))]
    return out


def cmd_lint(studio: Studio, args) -> None:
    hero = studio.hero(args.hero)
    anims = studio.anims(hero)
    errors = issues = 0
    for name in anims["anims"]:
        warnings: list[str] = []
        problems = []
        try:
            builds = [anim_kit.build(studio.lib, hero["name"], name, anims, studio.palette(hero), f, warnings)
                      for f in range(FACINGS)]
        except AnimError as error:
            print(f"ERROR @{name}: {error}")
            errors += 1
            continue
        timings = {json.dumps(frame_data(b)) for b in builds}
        if len(timings) > 1:
            problems.append("facings differ in timing or hit steps (facings overrides?)")
        for facing, built in enumerate(builds):
            _, found = entry_report(built)
            problems += [f"facing {facing}: {p}" for p in found]
            if built.hits:
                problems += [f"facing {facing}: {p}" for p in smoothness(built)]
        size = sum(len(b.frames) + len(b.tiles) for b in builds)
        print(f"{'ok  ' if not problems and not warnings else 'look'} @{name}: {describe_timing(frame_data(builds[2]))}"
              f", {size // 1024} KB")
        for line in list(dict.fromkeys(warnings)) + problems:
            print(f"      {line}")
        issues += bool(problems or warnings)
    used = set()
    for slot, pose in body_targets(hero):
        ref = pose if isinstance(pose, str) else (pose.get("anim") if isinstance(pose, dict) else None)
        if isinstance(ref, str) and ref.startswith("@"):
            used.add(ref[1:])
            if ref[1:] not in anims["anims"]:
                print(f"ERROR {slot} uses {ref}, which {anim_kit.anims_path(hero['name']).name} lacks")
                errors += 1
        if slot.startswith("swing.") or slot.startswith("melee.0") or slot.startswith("melee.2") or \
                slot.startswith("melee.4"):
            try:
                built = studio.build(hero, ref, 2, [], anims) if isinstance(ref, str) and ref.startswith("@") else None
            except AnimError:
                built = None
            if built is not None and not built.hits:
                print(f"look {slot} ({ref}): an attack pose with no hit frame never deals damage")
                issues += 1
    if "body" in hero:
        banks = hero_clips.body_banks(ROOT, studio.rom, hero, studio.sources)
        melee = [facing_built(banks["melee"][pose].facings[2], studio.rom) for pose in range(5)]
        for problem in combo_checks(melee):
            print(f"look combo: {problem}")
            issues += 1
    for name in anims["anims"]:
        if name not in used:
            print(f"note @{name} is not used by any pose (anim_studio.py assign)")
    print(f"{errors} errors, {issues} to look at")
    if errors:
        sys.exit(1)


def cmd_export(studio: Studio, args) -> None:
    hero = studio.hero(args.hero)
    palette = studio.palette(hero)
    anims = studio.anims(hero)
    folder = anim_kit.anims_path(hero["name"]).with_suffix("")
    folder.mkdir(parents=True, exist_ok=True)
    name = args.name or args.frame.replace("@", "").replace(":", "_").replace("/", "_")
    for facing in facings_of(args) if not args.all_facings else range(FACINGS):
        if args.frame.startswith("@"):
            anim, _, step = args.frame[1:].partition(":")
            built = anim_kit.build(studio.lib, hero["name"], anim, anims, palette, facing, [], compress=False)
            pixels = built.canvases[int(step or 0)]
        else:
            pixels = studio.lib.pixels(args.frame, anims["enemy"], facing)
        path = folder / f"{name}_f{facing}.png"
        anim_kit.image(pixels, palette).save(path)
        print(f"Wrote {path.relative_to(ROOT)} ({CANVAS}x{CANVAS}, origin at pixel {ORIGIN})")
    print(f"Use it as {{\"png\": \"{name}\", \"like\": \"<Clip:i>\"}}. One {name}.png serves every facing.")


SHEET_CELL = (16, 16, 176, 176)  # the canvas part a sheet cell shows: 160x160, the feet at (80, 112)
SHEET_BG = (32, 32, 255)  # sheet background, keyed out by slice: a blue no hero art should use
GUIDE = (255, 128, 255)  # the faint guide pose, keyed out by slice
GUIDE_MARK = (255, 0, 255)  # feet cross and ground line, keyed out by slice
HAND_MARK = (0, 255, 255)  # paint the sword hand in this color: slice records it and removes it


def sheet_folder(hero: dict) -> Path:
    return anim_kit.anims_path(hero["name"]).with_suffix("") / "sheets"


def cmd_template(studio: Studio, args) -> None:
    """A drawing sheet: one cell per pose (columns) and facing (rows), each with a faint guide pose and
    the feet marked, plus a JSON sidecar slice reads. Draw over it at any size."""
    from PIL import Image, ImageDraw

    hero = studio.hero(args.hero)
    anims = studio.anims(hero)
    poses = args.poses.split(",")
    guides = args.guides.split(",") if args.guides else []
    facings = [int(f) for f in args.facings.split(",")]
    scale = args.scale
    x0, y0, x1, y1 = SHEET_CELL
    cw, ch = (x1 - x0) * scale, (y1 - y0) * scale
    sheet = Image.new("RGB", (cw * len(poses), ch * len(facings)), SHEET_BG)
    draw = ImageDraw.Draw(sheet)
    for row, facing in enumerate(facings):
        for col, pose in enumerate(poses):
            ox, oy = col * cw, row * ch
            if col < len(guides) and guides[col]:
                pixels = studio.lib.pixels(guides[col], anims["enemy"], facing)
                for i, v in enumerate(pixels):
                    x, y = i % CANVAS, i // CANVAS
                    if v and x0 <= x < x1 and y0 <= y < y1:
                        draw.rectangle([ox + (x - x0) * scale, oy + (y - y0) * scale,
                                        ox + (x - x0 + 1) * scale - 1, oy + (y - y0 + 1) * scale - 1], fill=GUIDE)
            fx, fy = ox + (ORIGIN[0] - x0) * scale, oy + (ORIGIN[1] - y0) * scale
            draw.line([ox, fy, ox + cw - 1, fy], fill=GUIDE_MARK)
            draw.line([fx - 4 * scale, fy - 4 * scale, fx + 4 * scale, fy + 4 * scale], fill=GUIDE_MARK)
            draw.line([fx - 4 * scale, fy + 4 * scale, fx + 4 * scale, fy - 4 * scale], fill=GUIDE_MARK)
            draw.rectangle([ox, oy, ox + cw - 1, oy + ch - 1], outline=GUIDE_MARK)
            draw.text((ox + 3, oy + 2), f"{pose} f{facing} ({FACING_NAMES[facing]})", fill=GUIDE_MARK)
    folder = sheet_folder(hero)
    folder.mkdir(parents=True, exist_ok=True)
    path = folder / f"{args.name}_template.png"
    sheet.save(path)
    (folder / f"{args.name}.json").write_text(json.dumps(
        {"poses": poses, "facings": facings, "cell": list(SHEET_CELL), "scale": scale, "size": list(sheet.size),
         "background": list(SHEET_BG), "keyed": [list(GUIDE), list(GUIDE_MARK)], "hand": list(HAND_MARK)}, indent=1))
    print(f"Wrote {path.relative_to(ROOT)}: {len(poses)} poses x {len(facings)} facings, cells {cw}x{ch} "
          f"(feet at the pink cross). Draw the body WITHOUT the sword, paint the sword hand with one cyan "
          f"(#00ffff) dot, keep or erase the pinks. Then: anim_studio.py slice {hero['name']} {args.name} <drawn.png>")


def cmd_slice(studio: Studio, args) -> None:
    """A drawn sheet (any size, the template's layout) into canvas PNGs <name>_<pose>_f<facing>.png and
    the sword hands into hands.json, ready for {\"png\": ...} frames and rig bodies \"png:...\"."""
    from PIL import Image

    hero = studio.hero(args.hero)
    folder = sheet_folder(hero)
    layout = json.loads((folder / f"{args.name}.json").read_text())
    drawn = Image.open(args.sheet).convert("RGBA")
    factor = drawn.width / layout["size"][0]
    if abs(drawn.height / layout["size"][1] - factor) > 0.02:
        sys.exit(f"{args.sheet}: {drawn.size} is not the template's shape {layout['size']} at any scale")
    x0, y0, x1, y1 = layout["cell"]
    cw, ch = x1 - x0, y1 - y0
    keyed = [tuple(layout["background"])] + [tuple(c) for c in layout["keyed"]]
    hand = tuple(layout["hand"])
    near = lambda a, b, tol: sum((p - q) ** 2 for p, q in zip(a, b)) <= tol * tol
    # Key at full size (background, guides; the hand dot kept apart), then scale down: scaling first would
    # blend the guides into stray colors.
    full = drawn.load()
    hand_layer = Image.new("L", drawn.size, 0)
    hand_pixels = hand_layer.load()
    for y in range(drawn.height):
        for x in range(drawn.width):
            r, g, b, a = full[x, y]
            if a < 128 or any(near((r, g, b), k, args.tolerance) for k in keyed):
                full[x, y] = (0, 0, 0, 0)
            elif near((r, g, b), hand, 60):
                hand_pixels[x, y] = 255
                full[x, y] = (0, 0, 0, 0)
    size = (cw * len(layout["poses"]), ch * len(layout["facings"]))
    whole = abs(factor - round(factor)) < 0.01 and round(factor) >= 1
    small = drawn.resize(size, Image.NEAREST if whole else Image.BOX)
    hands = hand_layer.resize(size, Image.BOX)
    marks_path = anim_kit.anims_path(hero["name"]).with_suffix("") / "hands.json"
    marks = json.loads(marks_path.read_text()) if marks_path.exists() else {}
    for row, facing in enumerate(layout["facings"]):
        for col, pose in enumerate(layout["poses"]):
            cell = small.crop((col * cw, row * ch, (col + 1) * cw, (row + 1) * ch))
            canvas = Image.new("RGBA", (CANVAS, CANVAS), (0, 0, 0, 0))
            hand_cell = hands.crop((col * cw, row * ch, (col + 1) * cw, (row + 1) * ch)).load()
            hand_points, opaque = [], 0
            out = cell.load()
            for y in range(ch):
                for x in range(cw):
                    r, g, b, a = out[x, y]
                    if hand_cell[x, y] >= 64:
                        hand_points.append((x, y))
                    if a < 128:
                        out[x, y] = (0, 0, 0, 0)
                    else:
                        out[x, y] = (round(r * 255 / a), round(g * 255 / a), round(b * 255 / a), 255) \
                            if a < 255 else (r, g, b, 255)
                        opaque += 1
            canvas.paste(cell, (x0, y0))
            name = f"{args.name}_{pose}"
            target = anim_kit.anims_path(hero["name"]).with_suffix("") / f"{name}_f{facing}.png"
            target.parent.mkdir(parents=True, exist_ok=True)
            canvas.save(target)
            note = ""
            if hand_points:
                hx = round(sum(p[0] for p in hand_points) / len(hand_points)) + x0 - ORIGIN[0]
                hy = round(sum(p[1] for p in hand_points) / len(hand_points)) + y0 - ORIGIN[1]
                marks[f"{name}_f{facing}"] = [hx, hy]
                note = f", hand at [{hx}, {hy}]"
            print(f"  {target.name}: {opaque} px{note}{'  (empty: not drawn yet?)' if not opaque else ''}")
    marks_path.write_text(json.dumps(marks, indent=1))
    print(f"Use a pose in a rig body as \"png:{args.name}_<pose>\" (hand from the cyan dot), or as a frame "
          f"{{\"png\": \"{args.name}_<pose>\", \"like\": \"Attack:0\"}}.")


def cmd_assign(studio: Studio, args) -> None:
    hero = studio.hero(args.hero)
    heroes = json.loads(HEROES.read_text())
    target = next(h for h in heroes if h.get("name") == hero["name"])
    body = target["body"]
    poses = [p if not p.startswith("@") or p[1:] in studio.anims(hero)["anims"] else None for p in args.poses]
    if None in poses:
        missing = [p for p, ok in zip(args.poses, poses) if ok is None]
        sys.exit(f"No anim {', '.join(missing)} in {anim_kit.anims_path(hero['name']).name}")
    slot = args.slot
    if slot == "swings":
        body["swings"] = list(args.poses)
    elif slot == "cast":
        body["cast"] = [args.poses[0]] * 7
    else:
        bank, _, which = slot.partition(".")
        names = {"move": MOVE_POSES}.get(bank, ())
        index = names.index(which) if which in names else int(which) if which.isdigit() else None
        if bank not in ("move", "swing", "melee", "cast") or index is None:
            sys.exit("Slots: swings (the combo, one pose per swing), swing.N, cast, cast.N (skill row N+1), "
                     f"melee.N, move.<{'|'.join(MOVE_POSES)}>")
        key = "swings" if bank == "swing" else bank
        if key not in body:
            sys.exit(f"{hero['name']} has no body.{key}")
        if index >= len(body[key]):
            if key == "swings" and index == len(body[key]):
                body[key].append(args.poses[0])
            else:
                sys.exit(f"body.{key} has {len(body[key])} poses")
        else:
            body[key][index] = args.poses[0]
    hero_clips.body_banks(ROOT, studio.rom, {**hero, "body": body}, studio.sources)
    HEROES.write_text("[\n" + ",\n".join("  " + json.dumps(h) for h in heroes) + "\n]\n", encoding="utf-8",
                      newline="\n")
    print(f"{hero['name']} {slot} = {' '.join(args.poses)}. Try it: anim_studio.py game {hero['name']}")


def cmd_game(studio: Studio, args) -> None:
    hero = studio.hero(args.hero)
    command = [sys.executable, str(ROOT / "hacks/tools/hero_fast.py"), "check", hero["name"], "--button", args.button,
               "--step", str(args.step), "--box", *map(str, args.box)]
    if args.combo:
        # Mash attack so each swing chains into the next (the game chains on a press during a recovery).
        command += ["--presses", ",".join(str(10 + 8 * k) for k in range(12))]
    result = subprocess.run(command, cwd=ROOT)
    if result.returncode:
        sys.exit(result.returncode)
    # A readable strip of the first press: the dumped frames from just before it until it settles.
    from PIL import Image, ImageDraw

    frames = sorted((ROOT / "build/fast/frames").glob("*.png"))
    first, last = args.window
    picked = frames[10 + first:10 + last:args.step]
    if not picked:
        return
    x0, y0, x1, y1 = args.box
    cells = [Image.open(path).convert("RGB").crop((x0, y0, x1, y1)) for path in picked]
    columns = 8
    w, h = (x1 - x0) * 2, (y1 - y0) * 2
    sheet = Image.new("RGB", (w * columns, (h + 12) * ((len(cells) + columns - 1) // columns)), BG[:3])
    draw = ImageDraw.Draw(sheet)
    for n, image in enumerate(cells):
        x, y = (n % columns) * w, (n // columns) * (h + 12)
        sheet.paste(image.resize((w, h), Image.NEAREST), (x, y + 12))
        draw.text((x + 2, y), f"+{first + n * args.step}f", fill=TEXT[:3], font=font(10))
    path = OUT / f"game_{anim_kit.slug(hero['name'])}.png"
    sheet.save(path)
    print(f"Wrote {path.relative_to(ROOT)}: game frames after pressing {args.button}, every {args.step}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="command", required=True)

    def add(name, func, help_text):
        p = sub.add_parser(name, help=help_text)
        p.set_defaults(func=func)
        return p

    add("enemies", cmd_enemies, "every enemy with clips")
    p = add("clips", cmd_clips, "an enemy's clips with timing and hit frames")
    p.add_argument("enemy")
    p.add_argument("--facing", type=int, default=2)
    p = add("sheet", cmd_sheet, "numbered frames of an enemy's clips")
    p.add_argument("enemy")
    p.add_argument("clips", nargs="*", help="clips (default all)")
    p.add_argument("--hero", help="draw in this hero's palette")
    p.add_argument("--facing", type=int, default=2)
    p.add_argument("--all-facings", action="store_true")
    p.add_argument("--scale", type=int, default=2)
    p = add("palette", cmd_palette, "a hero's 16 colors")
    p.add_argument("hero")
    p = add("new", cmd_new, "start an anim from frames")
    p.add_argument("hero")
    p.add_argument("anim")
    p.add_argument("frames", nargs="+", help="Clip:i or Clip:i-j, one frame entry each")
    p.add_argument("--speed", type=int)
    p.add_argument("--force", action="store_true")
    p = add("show", cmd_show, "render anims: frames, onion skin, timeline, frame data")
    p.add_argument("hero")
    p.add_argument("anims", nargs="*", help="@anim, Clip or Enemy/Clip (default every anim)")
    p.add_argument("--vs", help="compare with this @anim, Clip or Enemy/Clip")
    p.add_argument("--facing", type=int, default=2)
    p.add_argument("--all-facings", action="store_true")
    p.add_argument("--scale", type=int, default=2)
    p.add_argument("--gif", action="store_true", help="also real-speed GIFs")
    p = add("tips", cmd_tips, "where the blade is tracked in a clip's frames")
    p.add_argument("hero")
    p.add_argument("clip")
    p.add_argument("--facing", type=int, default=2)
    p.add_argument("--all-facings", action="store_true")
    p.add_argument("--scale", type=int, default=2)
    p = add("combo", cmd_combo, "the melee chain as the game plays it, with a real-speed GIF")
    p.add_argument("hero")
    p.add_argument("--vs", choices=sorted(STOCK_HEROES), default="Aragorn", help="stock hero to compare timing with")
    p.add_argument("--facing", type=int, default=2)
    p.add_argument("--all-facings", action="store_true")
    p.add_argument("--scale", type=int, default=2)
    p = add("stock", cmd_stock, "every stock hero's melee combo timing (targets)")
    p.add_argument("--facing", type=int, default=2)
    p = add("verify", cmd_verify, "lint, combo timing, the real game's chaining and colors, as one text report")
    p.add_argument("hero")
    p.add_argument("--static", action="store_true", help="skip the game runs (1 s instead of about 40)")
    p = add("lint", cmd_lint, "check every anim and pose")
    p.add_argument("hero")
    p = add("export", cmd_export, "a frame as canvas PNGs to draw over")
    p.add_argument("hero")
    p.add_argument("frame", help="Clip:i, Enemy/Clip:i or @anim:step")
    p.add_argument("--name")
    p.add_argument("--facing", type=int, default=2)
    p.add_argument("--all-facings", action="store_true")
    p = add("template", cmd_template, "a drawing sheet for new art: poses x facings, guide pose, feet marked")
    p.add_argument("hero")
    p.add_argument("name", help="sheet name, e.g. swings")
    p.add_argument("--poses", required=True, help="comma list, e.g. ready,windup,strike,impact,follow")
    p.add_argument("--guides", help="comma list of Clip:i shown faintly per pose column, e.g. Attack:0,Attack:2")
    p.add_argument("--facings", default="0,1,2,3,4", help="rows (0 up/back .. 4 down/front)")
    p.add_argument("--scale", type=int, default=2)
    p = add("slice", cmd_slice, "a drawn template sheet into frame PNGs and sword hand marks")
    p.add_argument("hero")
    p.add_argument("name")
    p.add_argument("sheet", type=Path, help="the drawn sheet (any size with the template's shape)")
    p.add_argument("--tolerance", type=int, default=40, help="how close a color must be to a keyed one")
    p = add("assign", cmd_assign, "use anims or clips for a hero's poses (heroes.json)")
    p.add_argument("hero")
    p.add_argument("slot", help="swings, swing.N, cast, cast.N, melee.N, move.<pose>")
    p.add_argument("poses", nargs="+", help="@anim or a clip name")
    p = add("game", cmd_game, "run the hero in the real PC game, headless (hero_fast.py check)")
    p.add_argument("hero")
    p.add_argument("--button", choices=["A", "B"], default="B", help="B attacks, A casts the selected skill")
    p.add_argument("--step", type=int, default=2, help="sheet every N game frames")
    p.add_argument("--box", type=int, nargs=4, default=(20, 0, 220, 150), help="screen crop x0 y0 x1 y1")
    p.add_argument("--window", type=int, nargs=2, default=(0, 48), help="game frames after the press to show")
    p.add_argument("--combo", action="store_true", help="mash attack: the whole melee chain (use --window 0 96)")
    args = parser.parse_args()
    try:
        args.func(Studio(), args)
    except AnimError as error:
        sys.exit(f"ERROR: {error}")


if __name__ == "__main__":
    main()
