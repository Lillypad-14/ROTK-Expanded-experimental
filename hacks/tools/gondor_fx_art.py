"""Draw the Guard of Gondor's skill effect sheets (shield wall, beacon blaze, horn) as PNGs.

    uv run python hacks/tools/gondor_fx_art.py

Writes hacks/content/fx/gondor_*.png. Every sheet uses only the colors of gondor_palette.png, so
all of them share one OBJ palette slot (skill_fx.json "palette", id 3). Rerun after changing a
drawing, then rebuild.
"""

import math
import random
from pathlib import Path

from PIL import Image

OUT_DIR = Path(__file__).resolve().parents[1] / "content" / "fx"

PALETTE = {
    "out": (24, 24, 32),
    "steel_d": (80, 88, 104),
    "steel_l": (176, 184, 200),
    "white": (248, 248, 248),
    "face": (32, 36, 56),
    "dust": (184, 172, 152),
    "f_dark": (136, 24, 16),
    "f_red": (216, 56, 24),
    "f_orange": (248, 136, 32),
    "f_yellow": (252, 212, 72),
    "f_pale": (255, 246, 192),
    "ivory_d": (168, 140, 96),
    "ivory_m": (224, 204, 160),
    "ivory_l": (248, 238, 212),
}

# Shield wall: shields stand on an ellipse around the hero's feet.
WALL_W, WALL_H = 64, 48
WALL_CX, WALL_CY = 32, 32
WALL_RX, WALL_RY = 24, 10
WALL_SHIELDS = 10
WALL_DROP = (-16, -8, -3, 0, 0)

BLAZE_W, BLAZE_H = 48, 64
BLAZE_HEIGHTS = (6, 22, 40, 54, 50, 56, 42, 26)

HORN_W, HORN_H = 48, 64


class Canvas:
    def __init__(self, w, h):
        self.w, self.h = w, h
        self.px = [[None] * w for _ in range(h)]

    def put(self, x, y, c):
        x, y = round(x), round(y)
        if 0 <= x < self.w and 0 <= y < self.h:
            self.px[y][x] = c

    def image(self):
        img = Image.new("RGBA", (self.w, self.h))
        for y in range(self.h):
            for x in range(self.w):
                c = self.px[y][x]
                if c is not None:
                    img.putpixel((x, y), (*PALETTE[c], 255))
        return img


def shield(cv, x, bottom, front):
    """A Gondor tower shield 9 px wide and 13 tall standing at (x, bottom). Front: the face with
    the White Tree and its stars. Back: the dark inside with a steel boss."""
    x, bottom = round(x), round(bottom)
    left, top = x - 4, bottom - 12
    for yy in range(top, bottom + 1):
        # Rounded top corners, a pointed bottom.
        inset = 1 if yy == top else 0
        if yy >= bottom - 2:
            inset = yy - (bottom - 3)
        for xx in range(left + inset, left + 9 - inset):
            edge = xx in (left + inset, left + 8 - inset) or yy in (top, bottom)
            cv.put(xx, yy, "out" if edge and yy == bottom else "steel_l" if edge else "face" if front else "steel_d")
    if front:
        # The White Tree: trunk, two branch pairs, three stars above.
        for yy in range(top + 5, bottom - 2):
            cv.put(x, yy, "white")
        for dx, dy in ((-1, 5), (1, 5), (-2, 4), (2, 4), (-1, 7), (1, 7)):
            cv.put(x + dx, top + dy, "white")
        for dx in (-2, 0, 2):
            cv.put(x + dx, top + 2, "steel_l")
    else:
        cv.put(x, top + 6, "steel_l")
        cv.put(x, top + 5, "white")


def shield_wall(frame, front):
    """Shields drop onto the ring and slam down. The back half draws behind the hero, the front
    half in front of him."""
    cv = Canvas(WALL_W, WALL_H)
    drop = WALL_DROP[frame]
    spots = []
    for k in range(WALL_SHIELDS):
        angle = 2 * math.pi * (k + 0.5) / WALL_SHIELDS
        x = WALL_CX + WALL_RX * math.cos(angle)
        y = WALL_CY + WALL_RY * math.sin(angle)
        if (math.sin(angle) > 0) == front:
            spots.append((y, x))
    for y, x in sorted(spots):
        shield(cv, x, y + drop, front)
        if frame == 3:
            for dx in (-6, -5, 5, 6):
                cv.put(x + dx, y - 1, "dust")
            for dx in (-4, 4):
                cv.put(x + dx, y - 2, "dust")
    return cv.image()


def flame_color(t, d):
    """t: height 0 (base) .. 1 (tip), d: distance from the center line 0 .. 1."""
    heat = (1 - t) * 0.8 + (1 - d) * 0.6
    if heat > 1.15:
        return "f_pale"
    if heat > 0.9:
        return "f_yellow"
    if heat > 0.65:
        return "f_orange"
    if heat > 0.35:
        return "f_red"
    return "f_dark"


def blaze(frame):
    """A beacon fire roaring up behind the hero, then dying to smoke."""
    rng = random.Random(40 + frame)
    cv = Canvas(BLAZE_W, BLAZE_H)
    base = BLAZE_H - 6
    height = BLAZE_HEIGHTS[frame]
    width = 14 if frame > 1 else 8
    phase = frame * 1.7
    for yy in range(height):
        t = yy / max(height, 1)
        half = width * (1 - t) ** 0.6 + 1
        sway = math.sin(t * 5 + phase) * 3 * t
        for xx in range(-int(half) - 1, int(half) + 2):
            d = abs(xx - sway) / half
            # Ragged tongues near the edge and the tip.
            if d <= 1 and not (d > 0.6 and rng.random() < 0.35 * t + 0.15):
                cv.put(BLAZE_W / 2 + xx, base - yy, flame_color(t, d))
    # Embers flying up.
    for _ in range(4 + frame):
        cv.put(BLAZE_W / 2 + rng.randint(-14, 14), base - rng.randint(height // 2, height + 6), "f_yellow")
    if frame >= len(BLAZE_HEIGHTS) - 2:
        for _ in range(26):
            cv.put(BLAZE_W / 2 + rng.randint(-8, 8), base - height - rng.randint(0, 6), "steel_d")
    # Logs of the beacon.
    for xx in range(-9, 10):
        cv.put(BLAZE_W / 2 + xx, base + 1, "out")
        cv.put(BLAZE_W / 2 + xx, base + 2 - abs(xx) // 5, "ivory_d")
    return cv.image()


def horn(frame):
    """The Horn of Gondor raised above the hero's head, sound waves spreading from its bell."""
    cv = Canvas(HORN_W, HORN_H)
    lift = (8, 3, 0, 0, 0, 0, 0)[frame]
    cy = 14 + lift
    # A curved horn: mouthpiece at the left, bell at the right.
    for i in range(22):
        t = i / 21
        x = 9 + i
        y = cy + 4 * math.sin(t * math.pi) * -0.6 + (1 - t) * 2
        r = 1 + t * 2.6
        for yy in range(int(y - r), int(y + r) + 1):
            shade = "ivory_l" if yy < y - r / 3 else "ivory_m" if yy < y + r / 2 else "ivory_d"
            cv.put(x, yy, shade)
        cv.put(x, y - r - 1, "out")
        cv.put(x, y + r + 1, "out")
        if i in (4, 11, 17):
            for yy in range(int(y - r), int(y + r) + 1):
                cv.put(x, yy, "steel_l")
    for yy in range(cy - 6, cy + 5):
        cv.put(31, yy, "out")
    # Sound waves from the bell.
    for k in range(3):
        age = frame - 2 - k
        if 0 <= age <= 3:
            rad = 4 + age * 4
            for a in range(-50, 51, 6):
                ang = math.radians(a)
                cv.put(32 + rad * math.cos(ang), cy + rad * math.sin(ang) * 1.3, "white" if age < 3 else "steel_l")
    return cv.image()


PALANTIR_LIFT = (8, 3, 0, 0, 0, 0, 0, 0)
PALANTIR_GLOW = (0, 1, 2, 3, 4, 3, 4, 2)


def palantir(frame):
    """The seeing-stone held up over the hero's head: dark glass waking to a burning glow."""
    cv = Canvas(HORN_W, HORN_H)
    cx, cy, r = 24, 14 + PALANTIR_LIFT[frame], 7
    glow = PALANTIR_GLOW[frame]
    inner = ("face", "f_dark", "f_red", "f_orange", "f_yellow")
    for yy in range(cy - r, cy + r + 1):
        for xx in range(cx - r, cx + r + 1):
            d = math.hypot(xx - cx, yy - cy)
            if d > r + 0.3:
                continue
            if d > r - 0.8:
                cv.put(xx, yy, "out")
                continue
            # A swirl of fire inside the glass, brighter toward the middle as the glow rises.
            swirl = math.sin(math.atan2(yy - cy, xx - cx) * 2 + d * 0.9 + frame) * 0.5 + 0.5
            heat = glow * (1 - d / r) * 1.4 + swirl * glow * 0.35
            cv.put(xx, yy, inner[min(4, int(heat))] if heat >= 1 else "face")
    cv.put(cx - 3, cy - 4, "white")
    cv.put(cx - 4, cy - 3, "white")
    # Light spilling out once it burns.
    if glow >= 3:
        for a in range(0, 360, 30):
            ang = math.radians(a + frame * 7)
            for k in (r + 3, r + 5):
                if k == r + 5 and glow < 4:
                    continue
                cv.put(cx + k * math.cos(ang), cy + k * math.sin(ang), "f_orange" if k == r + 3 else "f_yellow")
    return cv.image()


EYE_OPEN = (0.15, 0.45, 0.8, 1.0, 1.0, 0.9)


def eye(frame):
    """The Eye opening above a marked foe's head: a lidless slit of fire."""
    cv = Canvas(32, 48)
    cx, cy = 16, 8
    openness = EYE_OPEN[frame]
    half_h = 5 * openness
    for yy in range(int(cy - 6), int(cy + 7)):
        for xx in range(cx - 12, cx + 13):
            dx, dy = (xx - cx) / 12, (yy - cy) / max(half_h, 0.6)
            inside = dy * dy + dx * dx * 0.9 <= 1 and abs(dy) <= 1 - dx * dx * 0.7
            if not inside:
                continue
            d = math.hypot(dx, dy)
            color = "f_yellow" if d < 0.35 else "f_orange" if d < 0.7 else "f_red"
            if abs(xx - cx) <= (1 if openness > 0.5 else 0) and openness > 0.3:
                color = "out"
            cv.put(xx, yy, color)
    # Lid edges and licking flames.
    for xx in range(cx - 13, cx + 14):
        t = (xx - cx) / 13
        edge = half_h * (1 - t * t) + 1
        cv.put(xx, cy - edge, "f_dark")
        cv.put(xx, cy + edge, "f_dark")
    if openness >= 0.8:
        for k in range(-3, 4):
            flick = (frame + k) % 3
            cv.put(cx + k * 3, cy - half_h - 2 - flick, "f_orange")
            cv.put(cx + k * 3, cy - half_h - 3 - flick, "f_yellow")
    return cv.image()


WEB_GROW = (0.3, 0.6, 0.85, 1.0, 1.0)


def web(frame):
    """A spider's web spun flat on the ground: spokes and rings, spinning out from the middle."""
    cv = Canvas(48, 32)
    cx, cy, rx, ry = 24, 20, 20 * WEB_GROW[frame], 9 * WEB_GROW[frame]
    for k in range(10):
        angle = 2 * math.pi * k / 10 + 0.15
        for t in range(0, 101, 4):
            cv.put(cx + rx * math.cos(angle) * t / 100, cy + ry * math.sin(angle) * t / 100, "steel_l")
    for ring in (0.35, 0.65, 0.95):
        for step in range(0, 360, 6):
            angle = math.radians(step)
            cv.put(cx + rx * ring * math.cos(angle), cy + ry * ring * math.sin(angle), "white")
    cv.put(cx, cy, "white")
    return cv.image()


COCOON_WRAP = (0.3, 0.55, 0.8, 1.0, 1.0)


def cocoon(frame):
    """A foe wrapped in silk from the feet up."""
    cv = Canvas(24, 40)
    cx, cy, rx, ry = 12, 22, 7, 13
    top = cy + ry - 2 * ry * COCOON_WRAP[frame]
    for yy in range(int(cy - ry), int(cy + ry) + 1):
        if yy < top:
            continue
        for xx in range(int(cx - rx), int(cx + rx) + 1):
            dx, dy = (xx - cx) / rx, (yy - cy) / ry
            if dx * dx + dy * dy > 1:
                continue
            edge = dx * dx + dy * dy > 0.7
            wrap = (yy - (xx - cx) * 0.6) % 5 < 1
            cv.put(xx, yy, "steel_d" if wrap else "steel_l" if edge else "white")
    return cv.image()


def sheet(name, frames):
    w, h = frames[0].size
    img = Image.new("RGBA", (w * len(frames), h))
    for i, frame in enumerate(frames):
        img.paste(frame, (i * w, 0))
    img.save(OUT_DIR / f"gondor_{name}.png")


def main():
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    strip = Image.new("RGBA", (len(PALETTE), 1))
    for i, rgb in enumerate(PALETTE.values()):
        strip.putpixel((i, 0), (*rgb, 255))
    strip.save(OUT_DIR / "gondor_palette.png")
    sheet("wall_back", [shield_wall(f, False) for f in range(len(WALL_DROP))])
    sheet("wall_front", [shield_wall(f, True) for f in range(len(WALL_DROP))])
    sheet("blaze", [blaze(f) for f in range(len(BLAZE_HEIGHTS))])
    sheet("horn", [horn(f) for f in range(7)])
    sheet("palantir", [palantir(f) for f in range(len(PALANTIR_GLOW))])
    sheet("eye", [eye(f) for f in range(len(EYE_OPEN))])
    sheet("web", [web(f) for f in range(len(WEB_GROW))])
    sheet("cocoon", [cocoon(f) for f in range(len(COCOON_WRAP))])


if __name__ == "__main__":
    main()
