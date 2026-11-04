"""Draw the Uruk-hai's skill sprites into hacks/content/fx/ (skill_fx.json assets, palette id 7).

    uv run python hacks/tools/urukhai_fx_art.py [--preview OUT.png]

pitburst  the pit-mud erupts, clods fly (Whelps of the Pit, under each whelp)
mudhands  muddy hands burst up and clutch the foe, then drag it down (Mud Tomb)
jaws      great jaws snap shut on the foe (Meat's Back on the Menu)
bloodpit  his blood pours down into a swirling pit (Blood for the Pits)
mudcoat   mud flies up and cakes onto him (Hide of the Pits)
whitehand the White Hand burns onto him, red-glowing (The White Hand)
"""

from __future__ import annotations

import argparse
import math

from fx_draw import Art

PALETTE = {
    "outline": (20, 14, 10),
    "shadow": (38, 30, 36),
    "mud_d": (54, 38, 24),
    "mud": (98, 72, 44),
    "mud_l": (152, 118, 78),
    "slime": (108, 138, 58),
    "slime_l": (172, 204, 92),
    "blood_d": (100, 14, 14),
    "blood": (178, 30, 26),
    "blood_l": (236, 98, 70),
    "bone_d": (164, 150, 118),
    "bone": (226, 216, 186),
    "white": (250, 248, 240),
}

art = Art("urukhai", PALETTE, seed=12)
rng = art.rng


def pitburst(frame):
    """The ground splits, a mud dome heaves up and bursts, clods arc out and fall."""
    cv = art.canvas(48, 40)
    cx, cy = 24, 32
    pool = (6, 10, 14, 16, 16, 15)[frame]
    cv.ellipse(cx, cy, pool, pool * 0.35, "mud_d")
    cv.ellipse(cx, cy - 1, pool * 0.7, pool * 0.22, "mud")
    if frame <= 1:
        dome = 4 + frame * 4
        cv.ellipse(cx, cy - dome * 0.5, dome * 0.9, dome * 0.6, "mud")
        cv.put(cx - 2, cy - dome, "mud_l")
    if frame >= 2:
        t = (frame - 1) / 4
        for i in range(8):
            a = math.radians(200 + i * 20)
            x = cx + math.cos(a) * 16 * t
            y = cy - 4 + math.sin(a) * 18 * t + 30 * t * t
            cv.dot(x, y, 1.5, "mud" if i % 2 else "mud_l")
        for i in range(3):
            cv.ellipse(cx - 6 + i * 6, cy - 2, 1.5, 1, "slime_l" if frame < 4 else "slime")
    return cv.image()


def hand(cv, x, base, rise, lean, color, light):
    cv.line(x, base, x + lean, base - rise, color, 3)
    cv.line(x - 1, base, x + lean - 1, base - rise, light, 1)
    hx, hy = x + lean, base - rise
    cv.ellipse(hx, hy, 2.5, 2, color)
    for f in (-2.5, -1, 0.5, 2):
        cv.line(hx + f, hy - 1, hx + f * 1.4, hy - 5, color, 1)


def mudhands(frame):
    """Hands claw up out of a mud pool, clutch inward, then drag down."""
    cv = art.canvas(48, 48)
    cx, cy = 24, 40
    rise = (4, 12, 18, 16, 14, 8, 2)[frame]
    grip = (0, 0, 2, 5, 6, 6, 4)[frame]
    cv.ellipse(cx, cy, 15, 5, "mud_d")
    cv.ellipse(cx, cy - 1, 11, 3, "mud")
    for i, (dx, lean) in enumerate(((-10, 1), (10, -1), (-5, 2), (5, -2))):
        side = -1 if dx < 0 else 1
        hand(cv, cx + dx, cy - 1 + (i // 2), rise - i // 2 * 3, lean - side * grip, "mud", "mud_l")
    if frame >= 3:
        for i in range(4):
            x = cx - 6 + i * 4
            cv.line(x, cy - 2, x, cy - 2 - (frame - 2) * 2, "mud_d", 1)
    if 2 <= frame <= 4:
        cv.put(cx - 7, cy - rise - 6, "slime_l")
        cv.put(cx + 8, cy - rise - 4, "slime_l")
    return cv.image()


def jaws(frame):
    """Toothed jaws gape over the foe and snap shut, blood spurts."""
    cv = art.canvas(40, 48)
    cx, cy = 20, 22
    gap = (14, 10, 3, 0, 0)[frame]
    for side in (-1, 1):
        y_edge = cy + side * gap / 2
        cv.ellipse(cx, y_edge + side * 4, 14, 5, "shadow")
        cv.ellipse(cx, y_edge + side * 3, 12, 3.5, "blood_d")
        for i in range(7):
            x = cx - 12 + i * 4
            cv.line(x, y_edge, x + 1, y_edge - side * 4, "bone", 1)
            cv.put(x + 1, y_edge - side * 4, "bone_d")
    if frame >= 3:
        for i in range(6):
            a = math.radians(-160 + i * 28)
            d = 6 + (frame - 3) * 7
            cv.dot(cx + d * math.cos(a), cy + d * 0.6 * math.sin(a), 1.4, "blood" if i % 2 else "blood_l")
    cv.outline("outline", inside={"shadow", "blood_d"})
    return cv.image()


def bloodpit(frame):
    """Blood streams down off him into a dark pool that swirls into the mud."""
    cv = art.canvas(48, 56)
    cx, base = 24, 50
    pool = (4, 8, 12, 15, 16, 14)[frame]
    cv.ellipse(cx, base, pool + 3, (pool + 3) * 0.35, "mud_d")
    cv.ellipse(cx, base, pool, pool * 0.33, "blood_d")
    for k in range(3):
        a0 = frame * 50 + k * 120
        cv.arc(cx, base, pool * 0.6, a0, a0 + 80, 1, "blood")
    if frame <= 4:
        for i, x in enumerate((cx - 6, cx + 1, cx + 7)):
            top = 14 + i * 3 + frame * 4
            cv.line(x, top, x + (i - 1), base - 2, "blood", 2 if frame < 3 else 1)
            cv.put(x, top, "blood_l")
    return cv.image()


def mudcoat(frame):
    """Clods fly up from the ground and splat onto his body, which ends caked and dripping."""
    cv = art.canvas(40, 56)
    cx, base = 20, 52
    body = [(cx - 5, base - 9), (cx + 4, base - 12), (cx - 1, base - 17), (cx + 6, base - 22), (cx - 6, base - 24),
            (cx + 1, base - 28), (cx - 3, base - 34), (cx + 5, base - 36), (cx, base - 41)]
    for i, (bx, by) in enumerate(body):
        t = min(1.0, max(0.0, frame / 3 - i * 0.04))
        sx, sy = cx + (i - 4) * 5, base
        x, y = sx + (bx - sx) * t, sy + (by - sy) * t - 10 * math.sin(math.pi * t)
        if t < 1:
            cv.dot(x, y, 1.6, "mud_l")
        else:
            cv.ellipse(bx, by, 3 + (i % 2), 2 + (i % 3) * 0.5, "mud")
            cv.ellipse(bx - 1, by - 1, 1.2, 0.8, "mud_l")
            cv.put(bx + 2, by + 1, "mud_d")
            if frame >= 4:
                cv.line(bx, by + 2, bx, by + 2 + (frame - 3), "mud_d", 1)
    cv.ellipse(cx, base, 10, 3, "mud_d")
    return cv.image()


def whitehand(frame):
    """A white hand print burns into being above him, glows red, then smoulders away."""
    cv = art.canvas(32, 64)
    cx, cy = 16, 18
    scale = (0.5, 0.8, 1.0, 1.0, 1.0, 1.0)[frame]
    palm = "white" if frame < 4 else "bone_d"
    glow = "blood_l" if frame in (2, 3) else "blood"
    if frame >= 1:
        cv.ellipse(cx, cy + 3 * scale, 9 * scale, 9 * scale, glow if frame < 5 else "blood_d")
    cv.ellipse(cx, cy + 3 * scale, 5 * scale, 5 * scale, palm)
    for i, (dx, h) in enumerate(((-4, 7), (-1.5, 9), (1.5, 9), (4, 7))):
        cv.line(cx + dx * scale, cy, cx + dx * scale * 1.1, cy - h * scale, palm, 2)
    cv.line(cx - 5 * scale, cy + 4 * scale, cx - 9 * scale, cy + 0 * scale, palm, 2)
    if frame >= 4:
        for i in range(3):
            cv.put(cx - 4 + i * 4, cy - 12 - (frame - 4) * 3, "shadow")
    return cv.image()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--preview")
    args = parser.parse_args()
    art.sheet("pitburst", [pitburst(f) for f in range(6)])
    art.sheet("mudhands", [mudhands(f) for f in range(7)])
    art.sheet("jaws", [jaws(f) for f in range(5)])
    art.sheet("bloodpit", [bloodpit(f) for f in range(6)])
    art.sheet("mudcoat", [mudcoat(f) for f in range(6)])
    art.sheet("whitehand", [whitehand(f) for f in range(6)])
    art.save_palette()
    if args.preview:
        art.preview(args.preview)


if __name__ == "__main__":
    main()
