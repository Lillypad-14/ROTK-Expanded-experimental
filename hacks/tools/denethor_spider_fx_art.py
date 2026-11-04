"""Draw Denethor's (palette id 11) and the Spider's (palette id 12) extra skill sprites into
hacks/content/fx/ (skill_fx.json assets). Both already use the Gondor art (palantir, beacon, webs).

    uv run python hacks/tools/denethor_spider_fx_art.py [--preview-denethor OUT.png] [--preview-spider OUT.png]

denethor_tower    the White Tower rising pale behind him (Lord of the White Tower)
denethor_madness  crimson cracks of madness spidering out around him (Madness)
denethor_despair  red life-motes draining off him into flame (Despair)
spider_fangs      dripping venom fangs (Venom Bite)
spider_silk       a strand of silk, laid along Reel In's line
spider_spit       a glob of web spat onto the foe (Reel In, on the foe)
spider_scuttle    dust flicked up by eight scuttling legs (Skitter)
"""

from __future__ import annotations

import argparse
import math

from fx_draw import Art

DENETHOR = {
    "outline": (16, 14, 18),
    "stone_d": (120, 126, 140),
    "stone": (188, 194, 206),
    "stone_l": (236, 240, 248),
    "white": (255, 255, 255),
    "blood_d": (96, 12, 16),
    "blood": (176, 28, 30),
    "blood_l": (236, 92, 70),
    "ember": (248, 140, 40),
    "gold": (250, 214, 96),
    "night": (40, 26, 48),
}

SPIDER = {
    "outline": (14, 12, 12),
    "silk_d": (150, 150, 160),
    "silk": (210, 210, 218),
    "silk_w": (250, 250, 255),
    "venom_d": (46, 96, 30),
    "venom": (110, 190, 54),
    "venom_l": (200, 250, 120),
    "fang_d": (60, 50, 44),
    "fang": (150, 136, 118),
    "dust": (150, 138, 120),
}

den = Art("denethor", DENETHOR, seed=41)
spi = Art("spider", SPIDER, seed=43)


def tower(frame):
    """A pale tower of Ecthelion rises up behind him and gleams at the peak."""
    cv = den.canvas(32, 72)
    rise = (0.3, 0.6, 0.85, 1.0, 1.0, 1.0)[frame]
    base, height = 68, 60 * rise
    top = base - height
    for y in range(int(top), base):
        t = (y - top) / max(height, 1)
        half = 3 + 4 * t
        for x in range(int(16 - half), int(16 + half) + 1):
            cv.put(x, y, "stone_l" if x < 16 - half + 2 else "stone" if x < 16 + half - 1 else "stone_d")
    cv.line(16, top - 6 * rise, 16, top, "stone_l", 1)
    for y in range(int(top) + 10, base - 4, 12):
        cv.line(13, y, 19, y, "stone_d", 1)
    if frame >= 3:
        gx, gy = 16, top - 6
        size = (3, 4, 2)[frame - 3]
        for d in range(-size, size + 1):
            cv.put(gx + d, gy, "white")
            cv.put(gx, gy + d, "white")
    if frame == 5:
        for y in range(cv.h):
            for x in range(cv.w):
                if cv.px[y][x] is not None and (x + y) % 2:
                    cv.px[y][x] = None
    return cv.image()


def madness(frame):
    """Crimson cracks spider out across the ground from him and glow."""
    cv = den.canvas(64, 40)
    cx, cy = 32, 28
    reach = (6, 12, 18, 22, 24, 24)[frame]
    for i in range(7):
        a = math.radians(i * 51 + 10)
        x, y = cx, cy
        for s in range(int(reach / 3)):
            a2 = a + math.sin(s * 1.9 + i) * 0.5
            nx, ny = x + 3 * math.cos(a2), y + 1.6 * math.sin(a2)
            cv.line(x, y, nx, ny, "blood" if frame < 4 else "blood_d", 1)
            x, y = nx, ny
        if frame in (2, 3, 4):
            cv.put(x, y, "blood_l")
    if frame <= 3:
        cv.ellipse(cx, cy, 3 + frame, 1.5 + frame * 0.5, "blood_l" if frame < 2 else "blood")
    return cv.image()


def despair(frame):
    """Red motes of his life drain upward and catch fire."""
    cv = den.canvas(32, 64)
    for k in range(7):
        x = 16 + 9 * math.sin(k * 2.1)
        y = 54 - frame * 6 - k * 3
        if 0 < y < 64:
            hot = y < 30
            cv.dot(x * (1 - frame * 0.08) + 16 * frame * 0.08, y, 1.3, "ember" if hot else "blood_l")
            if hot:
                cv.put(x, y - 2, "gold")
    return cv.image()


def fangs(frame):
    """Two curved fangs gleam over her and drip green venom."""
    cv = spi.canvas(32, 56)
    cx, top = 16, 8
    for side in (-1, 1):
        cv.arc(cx + side * 3, top, 8, 90 - side * 70, 90 + side * 10, 2.5, "fang")
        cv.arc(cx + side * 3, top, 8, 90 - side * 70, 90 - side * 30, 1, "fang_d")
        tip_x = cx + side * 3 + 8 * math.cos(math.radians(90 + side * 10))
        tip_y = top + 8 * math.sin(math.radians(90 + side * 10))
        drip = frame * 4
        cv.line(tip_x, tip_y, tip_x, tip_y + drip, "venom", 1)
        cv.dot(tip_x, tip_y + drip + 1, 1.3, "venom_l")
    return cv.image()


def silk(frame):
    """A taut strand of silk, glinting, then sagging away."""
    cv = spi.canvas(16, 8)
    for x in range(16):
        y = 4 + (0 if frame < 2 else math.sin(x * 0.4) * frame * 0.5)
        c = "silk_w" if (x + frame * 3) % 7 == 0 else "silk" if frame < 2 else "silk_d"
        cv.put(x, y, c)
        cv.put(x, y + 1, "silk_d" if frame < 2 else None)
    return cv.image()


def spit(frame):
    """A glob of web splats onto the foe and its strands spread."""
    cv = spi.canvas(32, 32)
    cx, cy = 16, 18
    r = (2, 5, 8, 10, 10)[frame]
    cv.dot(cx, cy, max(1.5, 4 - frame), "silk_w")
    for i in range(8):
        a = math.radians(i * 45 + 10)
        cv.line(cx, cy, cx + r * math.cos(a), cy + r * 0.8 * math.sin(a), "silk" if frame < 3 else "silk_d", 1)
    if frame >= 2:
        cv.ellipse(cx, cy, r * 0.6, r * 0.5, "silk_d", fill=False, width=1)
    return cv.image()


def scuttle(frame):
    """Little puffs of dust flicked up in pairs by her legs."""
    cv = spi.canvas(40, 24)
    for k in range(4):
        for side in (-1, 1):
            x = 20 + side * (6 + k * 3) + frame * side
            y = 18 - k % 2 - frame
            cv.dot(x, y, 1.5 - frame * 0.3, "dust")
    return cv.image()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--preview-denethor")
    parser.add_argument("--preview-spider")
    args = parser.parse_args()
    den.sheet("tower", [tower(f) for f in range(6)])
    den.sheet("madness", [madness(f) for f in range(6)])
    den.sheet("despair", [despair(f) for f in range(7)])
    den.save_palette()
    spi.sheet("fangs", [fangs(f) for f in range(5)])
    spi.sheet("silk", [silk(f) for f in range(3)])
    spi.sheet("spit", [spit(f) for f in range(5)])
    spi.sheet("scuttle", [scuttle(f) for f in range(4)])
    spi.save_palette()
    if args.preview_denethor:
        den.preview(args.preview_denethor)
    if args.preview_spider:
        spi.preview(args.preview_spider)


if __name__ == "__main__":
    main()
