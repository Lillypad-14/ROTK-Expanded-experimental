"""Draw the Corsair's skill sprites into hacks/content/fx/ (skill_fx.json assets, palette id 4).

    uv run python hacks/tools/corsair_fx_art.py [--preview OUT.png]

hook     iron grapnel biting into the foe (Grappling Hook, at the target)
rope     a twisted rope bit, laid every 12 px along the hook's line
slash    a curved cutlass arc (Cutlass Flurry, each cut)
powder   a billowing cloud of black powder (Smoke Powder)
coins    a spill of gold coins (Plunder, on each drop and the cast)
keg      a powder keg with a lit fuse (Powder Keg, held until it blows)
burst    a powder blast with flying splinters (Powder Keg, Black Sails)
cannon   a cannonball dropping in and bursting (Black Sails, each blast)
"""

from __future__ import annotations

import argparse
import math

from fx_draw import Art

PALETTE = {
    "outline": (24, 18, 22),
    "steel_d": (88, 96, 112),
    "steel": (160, 168, 184),
    "steel_l": (232, 240, 248),
    "rope_d": (112, 76, 40),
    "rope": (184, 140, 80),
    "wood": (136, 80, 32),
    "fire_d": (184, 48, 16),
    "fire": (240, 128, 32),
    "fire_l": (255, 224, 104),
    "smoke_d": (44, 40, 48),
    "smoke": (124, 118, 130),
    "smoke_l": (176, 172, 182),
    "gold": (224, 168, 40),
    "gold_l": (255, 236, 150),
}

art = Art("corsair", PALETTE, seed=7)
rng = art.rng


def hook(frame):
    """Grapnel: three prongs swing in and bite (frame 0 flying, 3 bitten)."""
    cv = art.canvas(24, 24)
    cx, cy = 12, 14
    turn = (40, 15, 0, 0)[frame]
    spread = (0.6, 0.85, 1.0, 1.0)[frame]
    cv.line(cx, cy, cx - 10, cy - 10, "rope", 1.5)
    cv.line(cx, cy, cx, cy - 7, "steel_d", 2)
    for k, base in enumerate((-55, 90, 235)):
        a = math.radians(base + turn)
        tx, ty = cx + 8 * spread * math.cos(a), cy + 6 * spread * math.sin(a)
        cv.line(cx, cy, tx, ty, "steel", 2)
        b = a + math.radians(70)
        cv.line(tx, ty, tx + 3 * math.cos(b), ty + 3 * math.sin(b), "steel_l", 1)
    cv.dot(cx, cy, 2, "steel_l")
    if frame >= 2:
        for i in range(6):
            a = math.radians(i * 60 + 15)
            r = 9 + frame
            cv.put(cx + r * math.cos(a), cy + r * 0.7 * math.sin(a), "fire_l" if frame == 2 else "steel_l")
    cv.outline("outline", inside={"steel", "steel_d", "steel_l", "rope"})
    return cv.image()


def rope(frame):
    """A twisted rope bit: strands alternate light and dark, the last frame frays."""
    cv = art.canvas(16, 8)
    length = (5, 7, 7)[frame]
    for i in range(-length, length + 1):
        twist = (i // 2) % 2
        cv.put(8 + i, 3 + twist, "rope" if frame < 2 else "rope_d")
        cv.put(8 + i, 4 + twist, "rope_d" if frame < 2 else None)
        if frame < 2 and twist == 0:
            cv.put(8 + i, 2 + twist, "outline")
            cv.put(8 + i, 5 + twist, "outline")
    return cv.image()


def slash(frame):
    """A crescent cut sweeping from upper left to lower right, then fading."""
    cv = art.canvas(40, 32)
    a0, a1 = -150, (-90, -20, 40, 40, 40)[frame]
    if frame <= 2:
        cv.arc(20, 22, 15, a0, a1, 6, "steel", taper=True)
        cv.arc(20, 22, 16, a0, a1, 3, "steel_l", taper=True)
        cv.arc(20, 22, 13, a0 + 20, a1, 2, "steel_d", taper=True)
    elif frame == 3:
        cv.arc(20, 22, 15, a0 + 40, a1, 4, "steel_d", taper=True)
        cv.arc(20, 22, 16, a0 + 60, a1, 2, "steel", taper=True)
    else:
        cv.arc(20, 22, 16, a0 + 90, a1, 2, "steel_d", taper=True)
    if frame == 2:
        tip = math.radians(a1)
        tx, ty = 20 + 16 * math.cos(tip), 22 + 16 * math.sin(tip)
        for d in range(-3, 4):
            cv.put(tx + d, ty, "steel_l")
            cv.put(tx, ty + d, "steel_l")
    return cv.image()


POWDER_R = (6, 11, 16, 20, 22, 21, 19)


def powder(frame):
    """Puffs of black powder billow out and lift; embers glint early on."""
    cv = art.canvas(64, 48)
    r = POWDER_R[frame]
    puffs = [(0, 0, 1.0), (-0.8, 0.1, 0.6), (0.8, 0.15, 0.65), (-0.45, -0.45, 0.6), (0.45, -0.5, 0.55),
             (0.0, -0.75, 0.5), (-1.0, -0.3, 0.4), (1.0, -0.35, 0.42)]
    lift = frame * 1.2
    for dx, dy, s in puffs:
        x, y, pr = 32 + dx * r, 34 + dy * r * 0.8 - lift, s * r * 0.75 + 1
        cv.dot(x, y, pr, "smoke_d" if frame < 5 else "smoke")
    for dx, dy, s in puffs:
        x, y, pr = 32 + dx * r, 34 + dy * r * 0.8 - lift, s * r * 0.75 + 1
        cv.dot(x - pr * 0.3, y - pr * 0.35, pr * 0.45, "smoke" if frame < 5 else "smoke_l")
        if frame < 4:
            cv.dot(x - pr * 0.45, y - pr * 0.5, max(0.5, pr * 0.15), "smoke_l")
    if frame < 4:
        for _ in range(8 - frame * 2):
            a = rng.uniform(0, 2 * math.pi)
            d = rng.uniform(0.3, 1.0) * r
            cv.put(32 + d * math.cos(a), 30 + d * 0.7 * math.sin(a) - lift, rng.choice(("fire", "fire_l")))
    if frame == 6:
        # Thinning out: a checker dither.
        for y in range(cv.h):
            for x in range(cv.w):
                if cv.px[y][x] is not None and (x + y) % 2:
                    cv.px[y][x] = None
    return cv.image()


COINS = [(-1.0, -1.6), (0.7, -1.9), (-0.3, -2.2), (1.2, -1.3), (-1.3, -1.0), (0.2, -1.6)]


def coins(frame):
    """Coins burst up and tumble down around the spot."""
    cv = art.canvas(32, 32)
    t = frame / 5
    for k, (vx, vy) in enumerate(COINS):
        x = 16 + vx * 9 * t
        y = 26 + (vy * 16 * t + 14 * t * t)
        edge = (frame + k) % 3 == 1
        if edge:
            cv.line(x, y - 2, x, y + 2, "gold", 1)
            cv.put(x, y - 2, "gold_l")
        else:
            cv.ellipse(x, y, 2, 2, "gold")
            cv.put(x - 1, y - 1, "gold_l")
            cv.put(x + 1, y + 1, "rope_d")
    if frame <= 1:
        cv.ellipse(16, 27, 6 + frame * 3, 2 + frame, "gold_l", fill=False)
    if frame in (2, 4):
        for sx, sy in ((8, 10), (24, 8)):
            cv.put(sx, sy, "gold_l")
            cv.put(sx - 1, sy, "gold")
            cv.put(sx + 1, sy, "gold")
            cv.put(sx, sy - 1, "gold")
            cv.put(sx, sy + 1, "gold")
    return cv.image()


SPARKS = ((2, "gold_l"), (1, "steel_l"), (2, "fire"), (3, "fire_l"))


def keg(frame):
    cv = art.canvas(24, 24)
    cx, top, bottom = 12, 8, 21
    for x in range(6, 19):
        cv.put(x, bottom + 1, "smoke_d")
    for y in range(top, bottom + 1):
        t = (y - top) / (bottom - top)
        half = 5 + round(1.5 * (1 - (2 * t - 1) ** 2))
        for x in range(cx - half, cx + half + 1):
            if x in (cx - half, cx + half) or y in (top, bottom):
                color = "outline"
            elif y in (top + 3, bottom - 3):
                color = "steel" if x < cx else "steel_d"
            else:
                color = "rope" if x < cx - 2 else "wood" if x < cx + 3 else "rope_d"
                if (x - cx) % 3 == 0:
                    color = "rope_d"
            cv.put(x, y, color)
    for x in range(cx - 4, cx + 5):
        cv.put(x, top - 1, "outline")
        cv.put(x, top, "rope")
    for dx, dy in ((1, -2), (2, -3), (3, -4), (4, -4), (5, -5)):
        cv.put(cx + dx, top + dy, "rope")
    sx, sy = cx + 6, top - 6
    size, color = SPARKS[frame]
    for d in range(-size, size + 1):
        cv.put(sx + d, sy, color)
        cv.put(sx, sy + d, color)
    cv.put(sx, sy, "steel_l")
    return cv.image()


BURST_R = (5, 11, 16, 19, 20, 19, 17)


def burst(frame, w=56, h=48, cy=34):
    """A powder blast: hot core, fire ring, splinters, then smoke."""
    cv = art.canvas(w, h)
    cx = w // 2
    r = BURST_R[frame]
    if frame <= 4:
        cv.ellipse(cx, cy - r * 0.35, r, r * 0.8, "fire_d")
        cv.ellipse(cx, cy - r * 0.4, r * 0.8, r * 0.65, "fire")
        cv.ellipse(cx, cy - r * 0.45, r * (0.55 - frame * 0.08), r * (0.45 - frame * 0.06), "fire_l")
    if frame >= 3:
        k = frame - 3
        rs = 18
        for dx, dy, s in ((0, -0.6, 0.6), (-0.6, -0.3, 0.5), (0.6, -0.35, 0.5), (0, -1.0, 0.45)):
            cv.dot(cx + dx * rs, cy - rs * 0.4 + dy * rs - k * 3, s * rs * (0.7 - k * 0.05),
                   "smoke_d" if k < 2 else "smoke")
            cv.dot(cx + dx * rs - 2, cy - rs * 0.45 + dy * rs - k * 3 - 2, s * rs * 0.3, "smoke" if k < 2 else "smoke_l")
        if k == 3:
            for y in range(cv.h):
                for x in range(cv.w):
                    if cv.px[y][x] is not None and (x + y) % 2:
                        cv.px[y][x] = None
    for i in range(10):
        a = math.radians(i * 36 + 10)
        d = 4 + frame * 3.6
        x, y = cx + d * math.cos(a), cy - 6 + d * 0.8 * math.sin(a) + frame * frame * 0.25
        if frame >= 1 and y < h:
            cv.line(x, y, x - 2 * math.cos(a), y - 2 * math.sin(a), "wood" if i % 3 else "rope", 1)
    if frame <= 2:
        cv.ellipse(cx, cy, r + 4, (r + 4) * 0.35, "fire_l" if frame == 0 else "fire", fill=False, width=1.2)
    return cv.image()


def cannon(frame):
    """A cannonball whistles down onto the spot (frames 0-3), then bursts (4-9)."""
    cv = art.canvas(48, 64)
    if frame <= 3:
        y = 6 + frame * 14
        for k in range(1, 4):
            cv.line(24, y - k * 4 - 2, 24, y - k * 4 + 2, "smoke_l" if k == 1 else "smoke", 1)
        cv.dot(24, y, 3, "outline")
        cv.dot(24, y, 2.4, "smoke_d")
        cv.put(23, y - 1, "steel")
        cv.ellipse(24, 54, 3 + frame, 1 + frame * 0.3, "smoke_d")
        return cv.image()
    blast = burst(frame - 4, 48, 64, 56)
    return blast


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--preview")
    args = parser.parse_args()
    art.sheet("hook", [hook(f) for f in range(4)])
    art.sheet("rope", [rope(f) for f in range(3)])
    art.sheet("slash", [slash(f) for f in range(5)])
    art.sheet("powder", [powder(f) for f in range(len(POWDER_R))])
    art.sheet("coins", [coins(f) for f in range(6)])
    art.sheet("keg", [keg(f) for f in range(len(SPARKS))])
    art.sheet("burst", [burst(f) for f in range(len(BURST_R))])
    art.sheet("cannon", [cannon(f) for f in range(4 + len(BURST_R))])
    art.save_palette()
    if args.preview:
        art.preview(args.preview)


if __name__ == "__main__":
    main()
