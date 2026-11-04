"""Draw the Orc Captain's skill sprites into hacks/content/fx/ (skill_fx.json assets, palette id 5).

    uv run python hacks/tools/orccaptain_fx_art.py [--preview OUT.png]

whip     a length of whip cord, laid every 16 px along Whip Crack's line
crack    the snap at a struck foe
boot     an iron-shod boot stamping in, then dust (Boot, on each foe)
shout    jagged war-cry lines bursting out (Over Here, Maggots!)
angry    a red "!" over a taunted foe's head
banner   a Mordor banner thrust up and flapping (Onward!)
spit     he spits and steams, red motes rise (Second Wind)
reap     a blood arc sweeping all around him (Leave None Alive)
gash     a red cut on each foe the cleave catches
"""

from __future__ import annotations

import argparse
import math

from fx_draw import Art

PALETTE = {
    "outline": (22, 14, 12),
    "leather_d": (72, 44, 28),
    "leather": (124, 80, 46),
    "leather_l": (184, 132, 80),
    "blood_d": (104, 14, 14),
    "blood": (184, 30, 24),
    "blood_l": (236, 92, 64),
    "dust_d": (104, 92, 78),
    "dust": (156, 142, 122),
    "dust_l": (212, 200, 178),
    "iron": (88, 90, 102),
    "iron_l": (172, 174, 188),
    "ember": (244, 124, 36),
    "green": (132, 176, 72),
    "white": (250, 246, 232),
}

art = Art("orccaptain", PALETTE, seed=3)
rng = art.rng


def whip(frame):
    """A wavy cord: it lashes out (frame 0), lies taut (1), then whips away thin (2)."""
    cv = art.canvas(16, 16)
    amp = (3, 1.5, 0.8)[frame]
    for x in range(16):
        y = 8 + amp * math.sin((x + frame * 3) * 0.7)
        if frame < 2:
            cv.put(x, y - 1, "dust_l" if x % 4 == 0 else "leather_l")
            cv.put(x, y, "leather_l")
            cv.put(x, y + 1, "leather")
            cv.put(x, y + 2, "outline")
        elif x % 2 == 0:
            cv.put(x, y, "leather_d")
    return cv.image()


def crack(frame):
    """The snap: a white star with a curl of cord, then sparks."""
    cv = art.canvas(24, 24)
    cx, cy = 12, 12
    if frame <= 1:
        r = 4 + frame * 4
        for i in range(8):
            a = math.radians(i * 45 + 22 * frame)
            length = r if i % 2 == 0 else r * 0.55
            cv.line(cx, cy, cx + length * math.cos(a), cy + length * math.sin(a), "white", 1)
        cv.dot(cx, cy, 2 - frame * 0.5, "white")
        cv.arc(cx - 3, cy + 2, 4, 100, 330, 1.5, "leather")
    else:
        for i in range(6):
            a = math.radians(i * 60 + frame * 17)
            d = 6 + frame * 3
            cv.put(cx + d * math.cos(a), cy + d * math.sin(a), "ember" if frame == 2 else "dust_l")
    return cv.image()


def boot(frame):
    """An iron-shod boot stamps down onto the foe, then a dust ring spreads."""
    cv = art.canvas(32, 40)
    if frame <= 2:
        y = (6, 14, 18)[frame]
        x = 14
        cv.ellipse(x + 2, y + 10, 8, 4, "leather_d")
        cv.ellipse(x + 1, y + 9, 7, 3, "leather")
        cv.line(x - 3, y - 6, x - 3, y + 8, "leather", 4)
        cv.line(x - 4, y - 6, x - 4, y + 6, "leather_l", 1)
        cv.line(x - 6, y + 13, x + 10, y + 13, "iron", 2)
        cv.line(x - 6, y + 12, x + 10, y + 12, "iron_l", 1)
        if frame < 2:
            for k in range(3):
                cv.line(x - 12 + k * 3, y - 4 + k * 5, x - 8 + k * 3, y - 4 + k * 5, "white", 1)
        cv.outline("outline", inside={"leather", "leather_d", "leather_l", "iron", "iron_l"})
    if frame >= 2:
        k = frame - 2
        cv.ellipse(16, 31, 6 + k * 4, 2 + k * 1.3, "dust" if k < 2 else "dust_l", fill=False, width=2 - k * 0.4)
        for i in range(6):
            a = math.radians(180 + i * 36)
            d = 5 + k * 4
            cv.dot(16 + d * math.cos(a), 30 + d * 0.5 * math.sin(a) - k, 1.6 - k * 0.3, "dust_l" if i % 2 else "dust")
    return cv.image()


def shout(frame):
    """Jagged war-cry lines burst out from his mouth."""
    cv = art.canvas(64, 48)
    cx, cy = 32, 16
    for ring in range(2):
        r = 6 + frame * 6 - ring * 8
        if r <= 2:
            continue
        for i in range(10):
            a0 = math.radians(i * 36 + ring * 18)
            a1 = a0 + math.radians(14)
            x0, y0 = cx + r * math.cos(a0), cy + r * 0.7 * math.sin(a0)
            x1, y1 = cx + (r + 5) * math.cos(a1), cy + (r + 5) * 0.7 * math.sin(a1)
            color = "white" if frame < 2 else "ember" if frame < 4 else "blood"
            cv.line(x0, y0, x1, y1, color, 1.5 if frame < 3 else 1)
    return cv.image()


def angry(frame):
    """A red "!" pops above a foe's head and wobbles."""
    cv = art.canvas(16, 48)
    y = (8, 4, 5, 5)[frame]
    dx = 2 + (0, 0, -1, 1)[frame]
    cv.line(6 + dx, y, 6 + dx, y + 7, "blood", 3)
    cv.line(6 + dx, y, 6 + dx, y + 6, "blood_l", 1)
    cv.dot(6 + dx, y + 11, 1.5, "blood")
    cv.outline("outline")
    return cv.image()


def banner(frame):
    """A Mordor banner thrust up beside him, its cloth flapping."""
    cv = art.canvas(32, 48)
    rise = (16, 6, 0, 0, 0, 0)[frame]
    pole_x, top = 10, 4 + rise
    cv.line(pole_x, top, pole_x, 44, "leather_d", 2)
    cv.line(pole_x - 3, top + 1, pole_x + 3, top + 1, "iron_l", 1)
    wave = frame * 1.3
    for x in range(1, 18):
        y0 = top + 3 + 2 * math.sin(x * 0.45 + wave)
        for y in range(round(y0), round(y0 + 14 - x * 0.25)):
            edge = y == round(y0) or x == 17
            cv.put(pole_x + x, y, "blood_d" if edge else "blood")
        if 6 <= x <= 10:
            eye_y = round(y0 + 6)
            cv.put(pole_x + x, eye_y, "ember")
            if x == 8:
                cv.put(pole_x + x, eye_y - 1, "outline")
                cv.put(pole_x + x, eye_y + 1, "ember")
    cv.outline("outline", inside={"blood", "blood_d", "leather_d"})
    return cv.image()


def spit(frame):
    """A gob of spit, a huff of steam, red motes of grit rising."""
    cv = art.canvas(32, 40)
    k = frame
    for i, (dx, sp) in enumerate(((-6, 1.0), (5, 1.3), (0, 0.8), (-2, 1.5), (7, 0.9))):
        y = 34 - (k * 5 + i * 3) * sp
        if 0 <= y < 40:
            cv.line(16 + dx, y - 1, 16 + dx, y + 1, "blood_l", 1)
            cv.line(16 + dx - 1, y, 16 + dx + 1, y, "blood_l", 1)
    if k <= 3:
        cv.dot(16 + k * 3, 10 + k * k, 1.5, "green")
        for j in range(3):
            cv.dot(14 - j * 3 - k, 8 - k * 2 - j * 2, 2.2 - j * 0.5, "dust_l" if k < 2 else "dust")
    return cv.image()


def reap(frame):
    """A blood arc sweeps all the way around him, then spatters."""
    cv = art.canvas(72, 48)
    cx, cy, rx, ry = 36, 30, 28, 13
    sweep = (90, 200, 300, 360, 360, 360)[frame]
    start = -120
    for i in range(0, sweep, 6):
        a = math.radians(start + i)
        w = 7 if frame < 3 else 5 if frame < 5 else 2.5
        fade = i / max(sweep, 1)
        color = "blood_l" if fade > 0.85 and frame < 4 else "blood" if fade > 0.3 or frame < 2 else "blood_d"
        cv.dot(cx + rx * math.cos(a), cy + ry * math.sin(a), w / 2 * (0.4 + fade * 0.6), color)
    if frame >= 3:
        for i in range(12):
            a = math.radians(i * 30 + 7)
            d = 1 + (frame - 3) * 0.12
            cv.dot(cx + rx * d * math.cos(a), cy + ry * d * math.sin(a) + (frame - 3) * 2, 1, "blood_d")
    return cv.image()


def gash(frame):
    """A red cut slashes across the foe, drips, fades."""
    cv = art.canvas(24, 24)
    if frame == 0:
        cv.line(4, 6, 20, 18, "white", 2)
    elif frame <= 2:
        cv.line(4, 6, 20, 18, "blood", 2.5)
        cv.line(5, 6, 19, 17, "blood_l", 1)
        for x, y in ((9, 10), (14, 14)):
            cv.line(x, y + 1, x, y + 2 + frame * 2, "blood", 1)
    else:
        cv.line(6, 8, 18, 17, "blood_d", 1.5)
    return cv.image()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--preview")
    args = parser.parse_args()
    art.sheet("whip", [whip(f) for f in range(3)])
    art.sheet("crack", [crack(f) for f in range(4)])
    art.sheet("boot", [boot(f) for f in range(6)])
    art.sheet("shout", [shout(f) for f in range(6)])
    art.sheet("angry", [angry(f) for f in range(4)])
    art.sheet("banner", [banner(f) for f in range(6)])
    art.sheet("spit", [spit(f) for f in range(6)])
    art.sheet("reap", [reap(f) for f in range(6)])
    art.sheet("gash", [gash(f) for f in range(4)])
    art.save_palette()
    if args.preview:
        art.preview(args.preview)


if __name__ == "__main__":
    main()
