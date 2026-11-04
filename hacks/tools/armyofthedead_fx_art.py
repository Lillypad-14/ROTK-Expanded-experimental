"""Draw the Army of the Dead's skill sprites into hacks/content/fx/ (skill_fx.json assets, palette id 9).

    uv run python hacks/tools/armyofthedead_fx_art.py [--preview OUT.png]

wail      pale rings of the dead's wail (Terror of the Dead, The Dead Sweep the Field)
skull     a wailing ghost skull rising off a terrified foe
fog       a low bank of grave fog rolling along the ground (Grave Mist, each pulse)
soul      a soul wisp torn up out of a foe (Soul Harvest, each foe drained)
harvest   pale wisps spiralling in to him (Soul Harvest cast)
grasp     spectral hands rising to hold a foe (Grasp of the Dead)
dissolve  he breaks into drifting mist (Pass Through)
"""

from __future__ import annotations

import argparse
import math

from fx_draw import Art

PALETTE = {
    "outline": (12, 22, 22),
    "shadow": (26, 36, 40),
    "ghost_d": (40, 66, 62),
    "ghost": (88, 140, 124),
    "ghost_l": (156, 212, 188),
    "ghost_w": (222, 250, 236),
    "white": (250, 255, 252),
    "eye": (190, 255, 120),
}

art = Art("armyofthedead", PALETTE, seed=21)


def wail(frame):
    """Wavering pale rings roll out from him."""
    cv = art.canvas(80, 48)
    cx, cy = 40, 26
    for ring in range(3):
        r = 6 + frame * 6 - ring * 8
        if r <= 3:
            continue
        color = "ghost_w" if frame < 2 else "ghost_l" if frame < 4 else "ghost"
        pts = [(cx + (r + 1.5 * math.sin(i * 1.3 + frame)) * math.cos(math.radians(i * 12)),
                cy + (r + 1.5 * math.sin(i * 1.3 + frame)) * 0.55 * math.sin(math.radians(i * 12))) for i in range(31)]
        for (x0, y0), (x1, y1) in zip(pts, pts[1:]):
            cv.line(x0, y0, x1, y1, color, 1.5 if ring == 0 else 1)
    return cv.image()


def skull(frame):
    """A ghostly skull with a trailing wisp wails up off the foe."""
    cv = art.canvas(24, 56)
    cx = 12 + 2 * math.sin(frame * 1.2)
    cy = 34 - frame * 5
    for k in range(4):
        cv.dot(cx + 1.5 * math.sin(frame + k), cy + 8 + k * 3, 3 - k * 0.6, "ghost" if k < 2 else "ghost_d")
    cv.ellipse(cx, cy, 5, 5.5, "ghost_l" if frame < 4 else "ghost")
    cv.ellipse(cx, cy + 4, 3, 2, "ghost_l" if frame < 4 else "ghost")
    for ex in (-2, 2):
        cv.ellipse(cx + ex, cy - 0.5, 1.3, 1.6, "shadow")
        cv.put(cx + ex, cy - 0.5, "eye")
    cv.ellipse(cx, cy + 4, 1.5, 1.8 + (frame % 2), "shadow")
    if frame >= 5:
        for y in range(cv.h):
            for x in range(cv.w):
                if cv.px[y][x] is not None and (x + y) % 2:
                    cv.px[y][x] = None
    return cv.image()


def fog(frame):
    """A low bank of fog rolls along the ground and thins."""
    cv = art.canvas(96, 40)
    cx, cy = 48, 28
    spread = (24, 34, 40, 42, 42)[frame]
    for i in range(10):
        x = cx - spread + i * spread * 2 / 9
        y = cy + 3 * math.sin(i * 1.7 + frame)
        r = 6 + 3 * math.sin(i * 2.3) - frame * 0.4
        cv.dot(x, y, r, "ghost_d")
    for i in range(10):
        x = cx - spread + i * spread * 2 / 9
        y = cy + 3 * math.sin(i * 1.7 + frame) - 2
        r = 6 + 3 * math.sin(i * 2.3) - frame * 0.4
        cv.dot(x - 1, y - 1, r * 0.55, "ghost" if frame < 3 else "ghost_l")
    if frame >= 3:
        for y in range(cv.h):
            for x in range(cv.w):
                if cv.px[y][x] is not None and (x + y + frame) % (2 if frame == 4 else 3) == 0:
                    cv.px[y][x] = None
    return cv.image()


def soul(frame):
    """A soul wisp is torn up out of the foe and streams away upward."""
    cv = art.canvas(16, 56)
    top = 40 - frame * 7
    for y in range(max(0, top), min(56, top + 18)):
        t = (y - top) / 18
        x = 8 + 2.5 * math.sin(y * 0.45 + frame * 1.5) * t
        w = 2.5 * (1 - t) + 0.5
        for dx in range(-int(w), int(w) + 1):
            cv.put(x + dx, y, "white" if t < 0.2 else "ghost_w" if t < 0.5 else "ghost_l")
    if 0 <= top < 56:
        cv.put(7, top + 2, "eye")
        cv.put(9, top + 2, "eye")
    return cv.image()


def harvest(frame):
    """Pale wisps spiral inward to him from all around."""
    cv = art.canvas(64, 56)
    cx, cy = 32, 36
    for k in range(5):
        a0 = math.radians(k * 72 + frame * 40)
        r = 26 - frame * 4
        for s in range(8):
            a = a0 - s * 0.18
            rr = r + s * 1.5
            x, y = cx + rr * math.cos(a), cy + rr * 0.5 * math.sin(a) - 8
            cv.dot(x, y, 1.3 if s < 3 else 0.8, "ghost_w" if s == 0 else "ghost_l" if s < 4 else "ghost")
    if frame >= 4:
        cv.dot(cx, cy - 10, 3 + (frame - 4) * 2, "ghost_w")
    return cv.image()


def hand(cv, x, base, rise, lean):
    cv.line(x, base, x + lean, base - rise, "ghost", 3)
    cv.line(x - 1, base, x + lean - 1, base - rise, "ghost_l", 1)
    hx, hy = x + lean, base - rise
    cv.ellipse(hx, hy, 2.5, 2, "ghost_l")
    for f in (-2.5, -1, 0.5, 2):
        cv.line(hx + f, hy - 1, hx + f * 1.4, hy - 5, "ghost_w", 1)


def grasp(frame):
    """Pale hands rise from a cold glow and close around the foe's legs."""
    cv = art.canvas(48, 48)
    cx, cy = 24, 40
    rise = (4, 12, 18, 17, 16)[frame]
    grip = (0, 0, 2, 5, 6)[frame]
    cv.ellipse(cx, cy, 15, 5, "ghost_d")
    cv.ellipse(cx, cy, 11, 3, "ghost")
    for i, (dx, lean) in enumerate(((-10, 1), (10, -1), (-5, 2), (5, -2))):
        side = -1 if dx < 0 else 1
        hand(cv, cx + dx, cy - 1 + i // 2, rise - i // 2 * 3, lean - side * grip)
    return cv.image()


def dissolve(frame):
    """He breaks apart into drifting wisps of mist."""
    cv = art.canvas(48, 64)
    cx, base = 24, 58
    spots = [(-6, -8), (5, -12), (-2, -18), (7, -24), (-7, -27), (1, -31), (-4, -38), (5, -41), (0, -47)]
    for k, (ox, oy) in enumerate(spots):
        x0, y0 = cx + ox, base + oy
        drift = frame * (3 + k % 3)
        x, y = x0 + drift * (1 if k % 2 else -1) * 0.7, y0 - drift
        r = 4 - frame * 0.5
        if r > 0.6:
            cv.dot(x, y, r, "ghost_l" if frame < 3 else "ghost")
            cv.dot(x - 1, y - 1, r * 0.4, "ghost_w")
    return cv.image()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--preview")
    args = parser.parse_args()
    art.sheet("wail", [wail(f) for f in range(6)])
    art.sheet("skull", [skull(f) for f in range(7)])
    art.sheet("fog", [fog(f) for f in range(5)])
    art.sheet("soul", [soul(f) for f in range(7)])
    art.sheet("harvest", [harvest(f) for f in range(6)])
    art.sheet("grasp", [grasp(f) for f in range(5)])
    art.sheet("dissolve", [dissolve(f) for f in range(6)])
    art.save_palette()
    if args.preview:
        art.preview(args.preview)


if __name__ == "__main__":
    main()
