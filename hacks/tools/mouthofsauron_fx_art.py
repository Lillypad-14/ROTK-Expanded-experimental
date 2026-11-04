"""Draw the Mouth of Sauron's skill sprites into hacks/content/fx/ (skill_fx.json assets, palette id 10).

    uv run python hacks/tools/mouthofsauron_fx_art.py [--preview OUT.png]

whisper  a green forked whisper curling out from him (The Lie cast)
lied     green whisper-wisps coiling round the deceived foe's head (The Lie, on the foe)
runes    Black Speech glyphs burning in a ring round a foe (Black Speech, each foe)
mithril  the mithril shirt held up high, gleaming (The Mithril Coat)
hooves   hoofbeat dust kicked up behind his steed (Ride Them Down trail)
tribute  gold motes drawn in to him from a kill (Tribute)
"""

from __future__ import annotations

import argparse
import math

from fx_draw import Art

PALETTE = {
    "outline": (16, 10, 16),
    "night_d": (34, 22, 44),
    "night": (82, 52, 104),
    "red_d": (110, 20, 22),
    "red": (196, 46, 34),
    "ember": (252, 150, 56),
    "mithril_d": (138, 150, 172),
    "mithril": (206, 216, 234),
    "mithril_w": (252, 254, 255),
    "green_d": (54, 96, 50),
    "green": (118, 184, 86),
    "green_l": (196, 240, 150),
    "gold": (224, 170, 48),
    "gold_l": (255, 236, 150),
    "dust": (150, 136, 118),
}

art = Art("mouthofsauron", PALETTE, seed=31)


def whisper(frame):
    """A curling green ribbon with a forked tip reaches out from his mouth."""
    cv = art.canvas(48, 32)
    length = (10, 20, 30, 36, 36, 36)[frame]
    for x in range(0, length):
        y = 16 + 4 * math.sin(x * 0.35 - frame)
        fade = frame >= 4 and x < (frame - 3) * 14
        if not fade:
            cv.put(6 + x, y, "green_l" if x > length - 5 else "green")
            cv.put(6 + x, y + 1, "green_d")
    tip_y = 16 + 4 * math.sin((length - 1) * 0.35 - frame)
    if frame < 5:
        cv.line(6 + length, tip_y, 9 + length, tip_y - 2, "green_l", 1)
        cv.line(6 + length, tip_y, 9 + length, tip_y + 2, "green_l", 1)
    return cv.image()


def lied(frame):
    """Green wisps coil round the foe's head: its mind is turned."""
    cv = art.canvas(32, 48)
    cx, cy = 16, 14
    for k in range(3):
        a0 = frame * 50 + k * 120
        for s in range(10):
            a = math.radians(a0 + s * 14)
            x, y = cx + 10 * math.cos(a), cy + 4 * math.sin(a) - s * 0.2
            cv.dot(x, y, 1.2 if s < 5 else 0.8, "green_l" if s < 3 else "green" if s < 7 else "green_d")
    if frame % 2 == 0:
        cv.put(cx - 3, cy + 1, "green_l")
        cv.put(cx + 3, cy + 1, "green_l")
    return cv.image()


GLYPHS = [((0, 0), (0, 4)), ((0, 0), (3, 0)), ((0, 2), (3, 4)), ((3, 0), (0, 4)), ((1, 0), (1, 4)), ((0, 4), (3, 4))]


def runes(frame):
    """Glyphs of the Black Speech flare up in a ring round the foe, red-hot, then gutter out."""
    cv = art.canvas(48, 40)
    cx, cy = 24, 30
    hot = "ember" if frame < 2 else "red" if frame < 4 else "red_d"
    lift = frame * 1.5
    for i in range(6):
        a = math.radians(i * 60 + frame * 12)
        gx, gy = cx + 16 * math.cos(a) - 2, cy + 7 * math.sin(a) - 4 - lift
        for (x0, y0), (x1, y1) in (GLYPHS[i], GLYPHS[(i + 2) % 6]):
            cv.line(gx + x0, gy + y0, gx + x1, gy + y1, hot, 1)
    cv.ellipse(cx, cy, 18, 7, "night" if frame < 4 else "night_d", fill=False, width=1)
    return cv.image()


def mithril(frame):
    """The little mail shirt is lifted high and flashes."""
    cv = art.canvas(32, 64)
    cx, top = 16, 6 + (8, 3, 0, 0, 0, 0)[frame]
    for y in range(top, top + 16):
        half = 6 if y < top + 4 else 5
        if y < top + 7:
            half += 4  # sleeves
        for x in range(cx - half, cx + half + 1):
            mesh = (x + y) % 2
            cv.put(x, y, "mithril" if mesh else "mithril_d")
    for x in range(cx - 2, cx + 3):
        cv.put(x, top, None)
        cv.put(x, top + 1, None)
    if frame in (2, 3, 4):
        gx = cx - 4 + (frame - 2) * 4
        gy = top + 4 + (frame - 2) * 3
        for d in range(-3, 4):
            cv.put(gx + d, gy, "mithril_w")
            cv.put(gx, gy + d, "mithril_w")
    if frame == 3:
        for i in range(8):
            a = math.radians(i * 45)
            cv.line(cx + 12 * math.cos(a), top + 8 + 10 * math.sin(a), cx + 16 * math.cos(a),
                    top + 8 + 13 * math.sin(a), "mithril_w", 1)
    cv.outline("outline", inside={"mithril", "mithril_d"})
    return cv.image()


def hooves(frame):
    """Clods and dust thrown up where the steed's hooves strike."""
    cv = art.canvas(24, 24)
    cx, cy = 12, 18
    for i in range(5):
        a = math.radians(200 + i * 35)
        d = 2 + frame * 3
        cv.dot(cx + d * math.cos(a), cy + d * 0.6 * math.sin(a) - frame, 2 - frame * 0.4, "dust")
    if frame == 0:
        cv.ellipse(cx, cy + 1, 3, 1.2, "night_d")
    return cv.image()


def tribute(frame):
    """Gold motes stream in and up to him, a coin glints."""
    cv = art.canvas(32, 56)
    cx = 16
    for k in range(6):
        a = math.radians(k * 60 + frame * 25)
        r = 14 - frame * 2.2
        y = 44 - frame * 5 - k * 2
        x = cx + r * math.cos(a)
        if 0 < y < 56:
            cv.dot(x, y, 1.1, "gold_l" if k % 2 else "gold")
    if frame >= 3:
        y = 18 - (frame - 3) * 3
        cv.ellipse(cx, y, 2.5 - (frame % 2), 2.5, "gold")
        cv.put(cx - 1, y - 1, "gold_l")
    return cv.image()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--preview")
    args = parser.parse_args()
    art.sheet("whisper", [whisper(f) for f in range(6)])
    art.sheet("lied", [lied(f) for f in range(6)])
    art.sheet("runes", [runes(f) for f in range(6)])
    art.sheet("mithril", [mithril(f) for f in range(6)])
    art.sheet("hooves", [hooves(f) for f in range(4)])
    art.sheet("tribute", [tribute(f) for f in range(6)])
    art.save_palette()
    if args.preview:
        art.preview(args.preview)


if __name__ == "__main__":
    main()
