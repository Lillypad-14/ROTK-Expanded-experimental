"""Draw the Uruk Crossbowman's skill sprites into hacks/content/fx/ (skill_fx.json assets, palette id 13).

    uv run python hacks/tools/urukcrossbowman_fx_art.py [--preview OUT.png]

bolt      a steel glint left every tick along a bolt's flight: they read as a streak (Heavy Bolt, Volley)
fire      the same glint burning (Fire Bolt)
thunk     a bolt bites into a foe: splinters and a puff (each foe a bolt strikes)
pavise    a tall shield with the White Hand slams into the ground (Pavise)
flames    a foe burning (Fire Bolt, and the foes the fire leaps to)
sight     a red crosshair over him tightening, once a second he holds still (Steady Aim)
hailbolt  a bolt dropping out of the sky and thudding into the ground (Rain of Bolts)
"""

from __future__ import annotations

import argparse
import math

from fx_draw import Art

PALETTE = {
    "outline": (18, 14, 12),
    "wood_d": (70, 44, 26),
    "wood": (122, 80, 44),
    "wood_l": (176, 128, 78),
    "iron_d": (70, 72, 84),
    "iron": (140, 144, 160),
    "iron_l": (222, 226, 236),
    "white": (252, 250, 244),
    "fire_d": (176, 44, 16),
    "fire": (242, 128, 34),
    "fire_l": (255, 222, 104),
    "smoke": (110, 104, 110),
    "dust": (164, 150, 128),
    "red": (214, 40, 30),
}

art = Art("urukcrossbowman", PALETTE, seed=51)


def glint(frame, core, mid, edge):
    # Drawn high on a tall canvas: the anchor (the flight point, at foot level) sits 18 px below it.
    cv = art.canvas(8, 32)
    r = (3, 2, 1)[frame]
    for d in range(-r, r + 1):
        cv.put(4 + d, 4, mid if abs(d) < r else edge)
        cv.put(4, 4 + d, mid if abs(d) < r else edge)
    cv.put(4, 4, core)
    return cv.image()


def thunk(frame):
    """Splinters and a puff burst where the bolt bites."""
    cv = art.canvas(24, 32)
    cx, cy = 12, 18
    if frame == 0:
        for d in range(-4, 5):
            cv.put(cx + d, cy, "iron_l")
            cv.put(cx, cy + d, "iron_l")
        cv.dot(cx, cy, 1.5, "white")
    else:
        for i in range(6):
            a = math.radians(i * 60 + 20)
            d = 2 + frame * 3
            x, y = cx + d * math.cos(a), cy + d * 0.8 * math.sin(a) + frame
            cv.line(x, y, x + 1.5 * math.cos(a), y + 1.5 * math.sin(a), "wood_l" if i % 2 else "wood", 1)
        cv.dot(cx, cy - frame, 3 - frame * 0.6, "dust")
    return cv.image()


def pavise(frame):
    """A tall wooden shield drops and slams into the ground, dust kicking out."""
    cv = art.canvas(32, 48)
    drop = (14, 4, 0, 0, 0)[frame]
    left, right, top, bottom = 7, 25, 6 + -drop, 42 - drop
    top = max(0, top)
    for y in range(top, bottom + 1):
        taper = 2 if y > bottom - 6 else 0
        for x in range(left + taper, right - taper + 1):
            board = (x - left) // 6
            cv.put(x, y, ("wood_l", "wood", "wood_d")[board % 3] if x not in (left + taper, right - taper) else "outline")
    for x in range(left, right + 1):
        cv.put(x, top, "iron")
        cv.put(x, top + 1, "iron_d")
    cx, cy = 16, top + 15
    cv.ellipse(cx, cy + 2, 3, 3, "white")
    for dx in (-2.5, -1, 1, 2.5):
        cv.line(cx + dx, cy, cx + dx * 1.1, cy - 5, "white", 1)
    cv.line(cx - 3, cy + 3, cx - 6, cy, "white", 1)
    if frame >= 1:
        k = frame - 1
        for i in range(6):
            a = math.radians(180 + i * 36)
            d = 8 + k * 4
            cv.dot(16 + d * math.cos(a), 44 + 2 * math.sin(a) - k, 1.6 - k * 0.3, "dust")
    cv.outline("outline", inside={"iron", "iron_d"})
    return cv.image()


def flames(frame):
    """Flames lick up the foe and fade to smoke."""
    cv = art.canvas(24, 48)
    for k, (x0, h0) in enumerate(((7, 18), (12, 26), (17, 20))):
        h = h0 * (0.5 + 0.12 * frame if frame < 4 else 1.0 - (frame - 4) * 0.3)
        base = 44
        for y in range(int(base - h), base):
            t = (base - y) / max(h, 1)
            w = 3 * (1 - t) + 0.5
            x = x0 + 1.5 * math.sin(y * 0.5 + frame + k)
            for dx in range(-int(w), int(w) + 1):
                cv.put(x + dx, y, "fire_l" if abs(dx) < w * 0.4 and t < 0.6 else "fire" if t < 0.8 else "fire_d")
    if frame >= 4:
        cv.dot(12, 44 - 26 - (frame - 4) * 4, 2.5, "smoke")
    return cv.image()


def sight(frame):
    """A red crosshair closes in over his head and locks."""
    cv = art.canvas(16, 48)
    cx, cy = 8, 6
    r = (6, 5, 4, 3)[frame]
    for d in (-1, 1):
        cv.line(cx + d * r, cy, cx + d * (r + 2), cy, "red", 1)
        cv.line(cx, cy + d * r, cx, cy + d * (r + 2), "red", 1)
    if frame == 3:
        cv.put(cx, cy, "white")
    return cv.image()


def hailbolt(frame):
    """A bolt drops out of the sky point first, thuds into the ground, dust jumps."""
    cv = art.canvas(16, 64)
    if frame <= 3:
        tip = 8 + frame * 15
        cv.line(8, tip - 12, 8, tip, "wood", 2)
        cv.line(6, tip - 12, 10, tip - 15, "white", 1)
        cv.line(8, tip + 1, 8, tip + 3, "iron_l", 2)
        for k in range(1, 3):
            cv.line(8, tip - 14 - k * 5, 8, tip - 12 - k * 5, "iron_d" if k == 1 else "smoke", 1)
    else:
        k = frame - 4
        cv.line(8, 50, 8, 58, "wood", 1)
        cv.line(7, 50, 9, 48, "white", 1)
        for i in range(4):
            a = math.radians(180 + i * 60)
            d = 2 + k * 3
            cv.dot(8 + d * math.cos(a), 59 + d * 0.4 * math.sin(a), 1.2, "dust")
    return cv.image()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--preview")
    args = parser.parse_args()
    art.sheet("bolt", [glint(f, "white", "iron_l", "iron") for f in range(3)])
    art.sheet("fire", [glint(f, "fire_l", "fire", "fire_d") for f in range(3)])
    art.sheet("thunk", [thunk(f) for f in range(4)])
    art.sheet("pavise", [pavise(f) for f in range(5)])
    art.sheet("flames", [flames(f) for f in range(6)])
    art.sheet("sight", [sight(f) for f in range(4)])
    art.sheet("hailbolt", [hailbolt(f) for f in range(7)])
    art.save_palette()
    if args.preview:
        art.preview(args.preview)


if __name__ == "__main__":
    main()
