"""Write a swing's held frames out as one slightly different drawing per tick (same timing).

    uv run python hacks/tools/anim_smooth.py Witch-king thrust rising cleave
    uv run python hacks/tools/anim_smooth.py Witch-king thrust --dry-run

A "hold" shows one drawing for several ticks, which looks like a low frame rate; stock swings draw
something new every tick. For each held entry this writes `hold` entries instead:
- before the hit: the lunge pulls back 1 px and the body lifts 1 px a tick (to PULL_LIMIT), with the
  lift removed facing up (there the two cancel out),
- the hit: only the first tick hits; later ticks fade the smear one style a tick (FADE), shrink then
  drop the flash, and keep one trail,
- after the hit: the lunge steps toward the next entry's.
Run `anim_studio.py lint <hero>` after: any `held ticks before the hit` line left needs a hand tweak
(a bigger lunge on one facing). Guide: docs/wip-docs/animation-studio.md, step 3 "Smooth".
"""

from __future__ import annotations

import argparse
import copy
import json

import anim_kit as K

# Smear styles one tick fainter (None: gone). Add a hero's own styles here.
FADE = {"morgul": "morgul-fade", "morgul-fade": "morgul-faint", "morgul-faint": None}
PULL_LIMIT = -3


def fade_draw(draw: list | None, step: int) -> list | None:
    if draw is None:
        return None
    out = []
    for item in draw:
        item = dict(item)
        if "spark" in item:
            if step < 2:
                item["scale"] = round(item.get("scale", 1) * 0.6, 2)
                out.append(item)
            continue
        if "smear" in item:
            style = item.get("style")
            for _ in range(step):
                style = FADE.get(style, style)
                if style is None:
                    break
            if style is None:
                continue
            item["style"] = style
            item["scale"] = round(item.get("scale", 1) * 0.85 ** step, 2)
        out.append(item)
    return out


def tick_copy(entry: dict, i: int, n: int, before_hit: bool, next_lunge) -> dict:
    """Tick i of an entry held n ticks (the entry's own keys, or one facing's merged keys)."""
    e = copy.deepcopy(entry)
    if i == 0:
        return e
    lunge = e.get("lunge") or 0
    if before_hit:
        e["lunge"] = max(lunge - i, min(lunge, PULL_LIMIT))
        e["dy"] = max((e.get("dy") or 0) - i, PULL_LIMIT)
    elif next_lunge is not None:
        e["lunge"] = round(lunge + (next_lunge - lunge) * i / n)
    e.pop("hit", None)
    if e.get("draw") is not None:
        e["draw"] = fade_draw(e["draw"], i)
        if not e["draw"]:
            e.pop("draw")
    if e.get("trail"):
        e["trail"] = {"steps": 1, "color": e["trail"]["color"][1:] or e["trail"]["color"]}
    for key in ("lunge", "dy"):
        if e.get(key) == 0:
            e.pop(key)
    return e


def smooth(frames: list[dict]) -> list[dict]:
    hit_at = next((k for k, f in enumerate(frames)
                   if "hit" in f or any("hit" in o for o in f.get("facings", {}).values())), None)
    out = []
    for k, f in enumerate(frames):
        n = f.get("hold", 1)
        next_lunge = frames[k + 1].get("lunge", 0) if k + 1 < len(frames) else None
        before = hit_at is not None and k < hit_at
        base = {key: v for key, v in f.items() if key not in ("hold", "facings")}
        for i in range(n):
            entry = tick_copy(base, i, n, before, next_lunge)
            facings = {}
            for name, override in f.get("facings", {}).items():
                merged = tick_copy({**base, **override}, i, n, before, next_lunge)
                diff = {key: v for key, v in merged.items() if entry.get(key, "\0") != v}
                diff.update({key: None for key in entry if key not in merged})
                if diff:
                    facings[name] = diff
            if before and i and entry.get("dy"):
                facings.setdefault("0", {}).setdefault("dy", None)
            if facings:
                entry["facings"] = facings
            out.append(entry)
    return out


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("hero")
    parser.add_argument("anims", nargs="+")
    parser.add_argument("--dry-run", action="store_true", help="print the new frames, write nothing")
    args = parser.parse_args()
    data = K.load(args.hero)
    for name in args.anims:
        anim = data["anims"][name]
        ticks = sum(f.get("hold", 1) for f in anim["frames"])
        anim["frames"] = smooth(anim["frames"])
        print(f"@{name}: {ticks} ticks, {len(anim['frames'])} entries (one drawing per tick)")
        if args.dry_run:
            for f in anim["frames"]:
                print("   ", json.dumps(f)[:140])
    if not args.dry_run:
        print(f"Wrote {K.save(args.hero, data)}. Next: anim_studio.py lint {args.hero}")


if __name__ == "__main__":
    main()
