"""Draw the Warg's skill sprites into hacks/content/fx/ (skill_fx.json assets, palette id 8).

    uv run python hacks/tools/warg_fx_art.py [--preview OUT.png]

claws    three claw rakes tearing across the foe (Pounce, Hunt Down landings)
dust     a puff of dust where he springs or lands
howl     a howl rolling out in pale waves (Howl)
chill    cold dread dripping off a howled foe
fangs    jaws snapping shut on the throat, blood spurts (Go for the Throat)
bloodfang his fangs drip red (Rend)
frenzy   a ragged red aura flaring up around him (Feeding Frenzy)
"""

from __future__ import annotations

import argparse
import math

from fx_draw import Art

PALETTE = {
    "outline": (20, 16, 16),
    "fur_d": (60, 52, 48),
    "fur": (112, 100, 90),
    "blood_d": (100, 14, 14),
    "blood": (182, 30, 26),
    "blood_l": (238, 96, 70),
    "bone_d": (168, 156, 128),
    "bone": (232, 224, 200),
    "pale": (176, 190, 216),
    "pale_l": (236, 242, 255),
    "ember": (244, 128, 40),
    "dust_d": (110, 98, 84),
    "dust": (160, 146, 126),
    "dust_l": (214, 202, 180),
}

art = Art("warg", PALETTE, seed=17)


def claws(frame):
    """Three parallel rakes tear down across the foe, then bleed and fade."""
    cv = art.canvas(32, 40)
    reach = (0.35, 0.75, 1.0, 1.0, 1.0)[frame]
    for k in range(3):
        x0, y0 = 7 + k * 6, 6
        x1, y1 = x0 + 8 * reach, y0 + 24 * reach
        if frame <= 1:
            cv.line(x0, y0, x1, y1, "pale_l", 2)
        elif frame <= 3:
            cv.line(x0, y0, x1, y1, "blood", 2)
            cv.line(x0 + 1, y0, x1 + 1, y1, "blood_l", 1)
            if frame == 3:
                cv.line(x1, y1, x1, y1 + 3, "blood", 1)
        else:
            cv.line(x0 + 2, y0 + 4, x1 - 1, y1 - 2, "blood_d", 1)
    return cv.image()


def dust(frame):
    """A kicked-up puff of dust spreads low and settles."""
    cv = art.canvas(48, 24)
    cx, cy = 24, 18
    r = 5 + frame * 4
    for i in range(7):
        a = math.radians(180 + i * 30)
        x, y = cx + r * math.cos(a), cy + r * 0.35 * math.sin(a) - frame * 0.6
        pr = 3.2 - frame * 0.45
        cv.dot(x, y, pr, "dust" if frame < 3 else "dust_l")
        cv.dot(x - 1, y - 1, pr * 0.45, "dust_l")
    if frame >= 4:
        for y in range(cv.h):
            for x in range(cv.w):
                if cv.px[y][x] is not None and (x + y) % 2:
                    cv.px[y][x] = None
    return cv.image()


def howl(frame):
    """Arcs of a howl roll outward in front and around, pale and fading."""
    cv = art.canvas(80, 56)
    cx, cy = 40, 26
    for ring in range(3):
        r = 6 + frame * 6 - ring * 7
        if r <= 3:
            continue
        color = "pale_l" if frame < 2 else "pale" if frame < 4 else "fur"
        for arc in (-1, 1):
            base = 0 if arc > 0 else 180
            cv.arc(cx, cy, r, base - 50, base + 50, 2 if ring == 0 else 1, color, taper=True)
    if frame <= 1:
        cv.dot(cx, cy, 2, "pale_l")
    return cv.image()


def chill(frame):
    """Pale droplets of dread trickle down from above the foe's head."""
    cv = art.canvas(16, 48)
    for k, x in enumerate((4, 8, 12)):
        y = 8 + frame * 5 + k * 3
        if y < 44:
            cv.line(x, y, x, y + 4, "pale", 1)
            cv.dot(x, y + 5, 1.2, "pale_l")
    if frame == 0:
        cv.line(3, 6, 13, 6, "pale_l", 1)
    return cv.image()


def fangs(frame):
    """Upper and lower fangs bite together on the throat, blood sprays out."""
    cv = art.canvas(40, 48)
    cx, cy = 20, 22
    gap = (12, 7, 1, 0, 0)[frame]
    for side in (-1, 1):
        y = cy + side * (gap // 2)
        # The jaw: a solid wedge, snout pointing right, gum line along y.
        for yy in range(0, 7):
            half = 13 - yy * 1.2
            row = y + side * yy
            for xx in range(int(cx - half), int(cx + half) + 1):
                cv.put(xx, row, "fur" if yy < 2 else "fur_d")
        for xx in range(cx - 12, cx + 13):
            cv.put(xx, y, "blood_d")
        for i in range(6):
            x = cx - 10 + i * 4
            length = 5 if i in (0, 5) else 3
            for d in range(length):
                w = (length - d) / length
                cv.put(x, y - side * d, "bone")
                if w > 0.5:
                    cv.put(x + 1, y - side * d, "bone_d")
    if frame >= 2:
        for i in range(7):
            a = math.radians(200 + i * 22)
            d = 4 + (frame - 2) * 6
            cv.dot(cx + d * math.cos(a), cy + d * 0.7 * math.sin(a), 1.5, "blood" if i % 2 else "blood_l")
    return cv.image()


def bloodfang(frame):
    """Two fangs gleam red over him and drip."""
    cv = art.canvas(32, 56)
    cx, top = 16, 8
    glow = "blood_l" if frame in (1, 2) else "blood"
    for dx in (-5, 5):
        cv.line(cx + dx, top, cx + dx * 0.8, top + 12, "bone", 3)
        cv.line(cx + dx * 0.8, top + 6, cx + dx * 0.8, top + 12, glow, 2)
        drip = top + 13 + frame * 3
        cv.line(cx + dx * 0.8, top + 12, cx + dx * 0.8, drip, "blood", 1)
        cv.put(cx + dx * 0.8, drip + 1, "blood_l")
    cv.outline("outline", inside={"bone"})
    return cv.image()


def frenzy(frame):
    """A ragged red-orange aura flares up off his back and tears away."""
    cv = art.canvas(48, 56)
    cx, base = 24, 50
    for k in range(7):
        x0 = cx - 15 + k * 5
        h = (14 + 8 * math.sin(k * 1.7 + frame)) * (0.5 + 0.12 * frame if frame < 4 else 1.0 - (frame - 4) * 0.3)
        for y in range(int(base - h), base):
            t = (base - y) / max(h, 1)
            x = x0 + 2 * math.sin(y * 0.35 + k + frame)
            w = 2.5 * (1 - t) + 0.5
            for dx in range(-int(w), int(w) + 1):
                cv.put(x + dx, y, "blood_l" if t > 0.7 else "ember" if abs(dx) < w * 0.5 and t > 0.3 else "blood")
    return cv.image()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--preview")
    args = parser.parse_args()
    art.sheet("claws", [claws(f) for f in range(5)])
    art.sheet("dust", [dust(f) for f in range(5)])
    art.sheet("howl", [howl(f) for f in range(6)])
    art.sheet("chill", [chill(f) for f in range(6)])
    art.sheet("fangs", [fangs(f) for f in range(5)])
    art.sheet("bloodfang", [bloodfang(f) for f in range(5)])
    art.sheet("frenzy", [frenzy(f) for f in range(6)])
    art.save_palette()
    if args.preview:
        art.preview(args.preview)


if __name__ == "__main__":
    main()
