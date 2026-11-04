"""Draw the Witch-king's skill sprites into hacks/content/fx/ (skill_fx.json assets, palette id 6).

    uv run python hacks/tools/witchking_fx_art.py [--preview OUT.png]

shard     a pale Morgul shard spinning in flight (Morgul Shard, Morgul Sorcery travel)
shardhit  the shard bursts into green splinters and mist
breath    a rolling cloud of black breath (Black Breath)
breathhit green wisps curling up a foe it touches
shriek    a jagged wave of the Nazgul cry (Cry of the Nazgul, Host of Angmar)
crown     the iron crown forming over his helm (Iron Crown, No Living Man)
spikes    iron spikes bursting from a foe that struck him (Iron Crown's rebound)
terror    shadow hands reaching out around him (Lord of Terror, each pulse)
malice    dark flames rising around him (Undying Malice, Endless Night)
"""

from __future__ import annotations

import argparse
import math

from fx_draw import Art

PALETTE = {
    "outline": (16, 10, 22),
    "shadow_d": (30, 20, 42),
    "shadow": (62, 44, 82),
    "night": (110, 84, 150),
    "night_l": (176, 156, 220),
    "morgul_d": (60, 100, 56),
    "morgul": (132, 196, 112),
    "morgul_l": (212, 248, 188),
    "iron_d": (62, 64, 78),
    "iron": (128, 132, 150),
    "iron_l": (212, 216, 230),
    "blood": (176, 32, 40),
    "white": (248, 248, 255),
    "gold": (232, 196, 80),
}

art = Art("witchking", PALETTE, seed=9)
rng = art.rng


def shard(frame):
    """A thin pale blade shard tumbling, with a green glow trail."""
    cv = art.canvas(24, 16)
    a = math.radians(frame * 45)
    cx, cy = 14, 8
    for k in range(1, 4):
        cv.dot(cx - k * 3, cy + (k % 2), 1.4 - k * 0.3, "morgul" if k == 1 else "morgul_d")
    dx, dy = 5 * math.cos(a), 3 * math.sin(a)
    cv.line(cx - dx, cy - dy, cx + dx, cy + dy, "morgul_l", 2)
    cv.line(cx - dx * 0.6, cy - dy * 0.6, cx + dx * 0.6, cy + dy * 0.6, "white", 1)
    cv.outline("morgul_d", inside={"morgul_l", "white"})
    return cv.image()


def shardhit(frame):
    """Splinters fly out from the hit, green mist blooms and fades."""
    cv = art.canvas(32, 32)
    cx, cy = 16, 18
    r = 3 + frame * 3
    if frame <= 3:
        cv.ellipse(cx, cy, r * 0.8, r * 0.6, "morgul_d" if frame else "morgul")
        cv.ellipse(cx, cy - 1, r * 0.5, r * 0.35, "morgul" if frame < 2 else "morgul_d")
    for i in range(7):
        a = math.radians(i * 51 + 10)
        d = 2 + frame * 3.5
        x, y = cx + d * math.cos(a), cy + d * 0.8 * math.sin(a)
        cv.line(x, y, x + 2 * math.cos(a), y + 2 * math.sin(a), "morgul_l" if frame < 3 else "morgul", 1)
    if frame == 0:
        cv.dot(cx, cy, 2, "white")
    return cv.image()


def breath(frame):
    """Black breath rolls out from him and spreads along the ground, green at the edges."""
    cv = art.canvas(80, 48)
    cx, cy = 40, 30
    r = (8, 16, 24, 30, 34, 36)[frame]
    for i in range(9):
        a = math.radians(i * 40 + frame * 9)
        d = r * 0.7
        x, y = cx + d * math.cos(a), cy + d * 0.45 * math.sin(a) - frame
        pr = r * 0.32 + (i % 3)
        cv.dot(x, y, pr, "shadow_d" if frame < 4 else "shadow")
    for i in range(9):
        a = math.radians(i * 40 + frame * 9)
        d = r * 0.7
        x, y = cx + d * math.cos(a), cy + d * 0.45 * math.sin(a) - frame
        pr = r * 0.32 + (i % 3)
        cv.dot(x - pr * 0.3, y - pr * 0.4, pr * 0.35, "shadow" if frame < 4 else "night")
        if frame >= 1:
            cv.put(x + pr * 0.6, y - pr * 0.2, "morgul" if frame < 4 else "morgul_d")
    if frame < 4:
        cv.dot(cx, cy - frame, r * 0.35, "shadow_d")
    if frame == 5:
        for y in range(cv.h):
            for x in range(cv.w):
                if cv.px[y][x] is not None and (x + y) % 2:
                    cv.px[y][x] = None
    return cv.image()


def breathhit(frame):
    """Green wisps curl up around the foe."""
    cv = art.canvas(24, 40)
    for k, (x0, phase) in enumerate(((7, 0.0), (16, 1.7), (12, 3.1))):
        top = 34 - frame * 7 - k * 3
        for y in range(top, min(38, top + 12)):
            x = x0 + 2.5 * math.sin(y * 0.5 + phase + frame)
            t = (y - top) / 12
            cv.put(x, y, "morgul_l" if t < 0.3 else "morgul" if t < 0.7 else "morgul_d")
    return cv.image()


def shriek(frame):
    """The cry: jagged rings ripple out, a pale streak of sound in each."""
    cv = art.canvas(80, 56)
    cx, cy = 40, 28
    for ring in range(3):
        r = 6 + frame * 7 - ring * 9
        if r <= 3:
            continue
        color = "white" if ring == 0 and frame < 3 else "night_l" if ring < 2 else "night"
        points = []
        for i in range(25):
            a = math.radians(i * 15)
            jag = 3 if i % 2 else -2
            points.append((cx + (r + jag) * math.cos(a), cy + (r + jag) * 0.6 * math.sin(a)))
        width = 2 if ring == 0 and frame < 4 else 1
        for (x0, y0), (x1, y1) in zip(points, points[1:]):
            cv.line(x0, y0, x1, y1, color, width)
    return cv.image()


def crown(frame):
    """Spikes of an iron crown grow up over his helm and glint."""
    cv = art.canvas(32, 64)
    grow =(0.2, 0.5, 0.8, 1.0, 1.0, 1.0)[frame]
    cx, base = 16, 18
    cv.line(cx - 10, base, cx + 10, base, "iron_d", 3)
    cv.line(cx - 10, base - 1, cx + 10, base - 1, "iron", 1)
    for i, h in enumerate((7, 10, 14, 10, 7)):
        x = cx - 8 + i * 4
        top = base - h * grow
        cv.line(x, base - 1, x, top, "iron", 2)
        cv.put(x - 0.5, top, "iron_l")
    if frame in (4, 5):
        gx, gy = cx + (2 if frame == 4 else -3), base - 14 * grow + 2
        for d in range(-2, 3):
            cv.put(gx + d, gy, "white")
            cv.put(gx, gy + d, "white")
    cv.outline("outline", inside={"iron", "iron_d", "iron_l"})
    return cv.image()


def spikes(frame):
    """Iron spikes jab out of the foe that struck him."""
    cv = art.canvas(32, 32)
    cx, cy = 16, 20
    length = (4, 9, 12, 8, 4)[frame]
    for i in range(6):
        a = math.radians(i * 60 - 90 + 15)
        x, y = cx + length * math.cos(a), cy + length * 0.8 * math.sin(a)
        cv.line(cx + 2 * math.cos(a), cy + 2 * math.sin(a), x, y, "iron" if frame < 3 else "iron_d", 2)
        cv.put(x, y, "iron_l")
    if frame <= 1:
        cv.dot(cx, cy, 2, "blood")
    cv.outline("outline", inside={"iron", "iron_d"})
    return cv.image()


def terror(frame):
    """Shadow hands claw out of the ground in a ring around him, then sink."""
    cv = art.canvas(80, 48)
    cx, cy, rx, ry = 40, 30, 30, 13
    rise = (3, 8, 12, 10, 5)[frame]
    cv.ellipse(cx, cy, rx + 2, ry + 1, "shadow", fill=False, width=1)
    for i in range(8):
        a = math.radians(i * 45 + 22)
        x, y = cx + rx * math.cos(a), cy + ry * math.sin(a)
        lean = 2 * math.cos(a)
        cv.line(x, y, x + lean, y - rise, "shadow_d", 3)
        cv.line(x - 1, y, x + lean - 1, y - rise, "shadow", 1)
        hx, hy = x + lean, y - rise
        cv.ellipse(hx, hy - 1, 2.5, 2, "shadow_d")
        for f in (-3, -1, 1, 3):
            cv.line(hx + f * 0.7, hy - 2, hx + f, hy - 6, "night" if frame < 3 else "shadow", 1)
    return cv.image()


def malice(frame):
    """Dark violet flames lick up around him and rise away."""
    cv = art.canvas(40, 56)
    for k, (x0, h0) in enumerate(((10, 26), (20, 40), (30, 28), (15, 34), (26, 22))):
        h = h0 * (0.4 + 0.15 * frame) if frame < 4 else h0 * (1.0 - (frame - 4) * 0.35)
        base = 52 - k % 2 * 2 - (frame - 3) * 3 * (frame > 3)
        for y in range(int(base - h), int(base)):
            t = (base - y) / max(h, 1)
            w = 3.2 * (1 - t) + 0.5
            x = x0 + 1.5 * math.sin(y * 0.4 + frame + k)
            for dx in range(-int(w), int(w) + 1):
                cv.put(x + dx, y, "night_l" if t > 0.75 else "night" if abs(dx) < w * 0.5 else "shadow")
    return cv.image()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--preview")
    args = parser.parse_args()
    art.sheet("shard", [shard(f) for f in range(4)])
    art.sheet("shardhit", [shardhit(f) for f in range(5)])
    art.sheet("breath", [breath(f) for f in range(6)])
    art.sheet("breathhit", [breathhit(f) for f in range(5)])
    art.sheet("shriek", [shriek(f) for f in range(6)])
    art.sheet("crown", [crown(f) for f in range(6)])
    art.sheet("spikes", [spikes(f) for f in range(5)])
    art.sheet("terror", [terror(f) for f in range(5)])
    art.sheet("malice", [malice(f) for f in range(6)])
    art.save_palette()
    if args.preview:
        art.preview(args.preview)


if __name__ == "__main__":
    main()
