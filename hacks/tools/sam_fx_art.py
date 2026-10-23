"""Draw Samwise's garden effect sheets (plants, stew pot, pan, rope, potatoes) as PNGs.

    uv run python hacks/tools/sam_fx_art.py

Writes hacks/content/fx/sam_*.png. Every sheet uses only the colors of sam_palette.png, so
all of them share one OBJ palette slot (skill_fx.json "palette"). Rerun after changing a
drawing, then rebuild. Hand-drawn sheets in the same palette can replace any of these.
"""

import math
import random
from pathlib import Path

from PIL import Image

OUT_DIR = Path(__file__).resolve().parents[1] / "content" / "fx"

PALETTE = {
    "out": (40, 24, 16),
    "soil_d": (88, 56, 32),
    "soil_m": (128, 88, 48),
    "soil_l": (168, 128, 80),
    "leaf_d": (32, 96, 40),
    "leaf_m": (64, 152, 56),
    "leaf_l": (136, 208, 88),
    "spud_d": (176, 128, 64),
    "spud_l": (224, 184, 112),
    "cream": (248, 240, 216),
    "red": (208, 40, 48),
    "fire": (248, 168, 40),
    "iron_d": (64, 64, 72),
    "iron_l": (160, 168, 176),
    "thorn": (96, 48, 64),
}


class Canvas:
    def __init__(self, w, h):
        self.w, self.h = w, h
        self.px = [[None] * w for _ in range(h)]

    def put(self, x, y, c):
        x, y = round(x), round(y)
        if 0 <= x < self.w and 0 <= y < self.h:
            self.px[y][x] = c

    def get(self, x, y):
        return self.px[y][x] if 0 <= x < self.w and 0 <= y < self.h else None

    def ellipse(self, cx, cy, rx, ry, c, shade=None):
        """Filled ellipse. shade(dx, dy) may return a color per pixel."""
        for y in range(math.floor(cy - ry), math.ceil(cy + ry) + 1):
            for x in range(math.floor(cx - rx), math.ceil(cx + rx) + 1):
                dx, dy = (x - cx) / max(rx, 0.5), (y - cy) / max(ry, 0.5)
                if dx * dx + dy * dy <= 1.0:
                    self.put(x, y, shade(dx, dy) if shade else c)

    def ring(self, cx, cy, rx, ry, color_at, width=1.0):
        steps = int(8 * (rx + ry)) + 16
        for i in range(steps):
            a = 2 * math.pi * i / steps
            for k in range(int(width * 2)):
                r = 1 - k * 0.5 / max(rx, 1)
                self.put(cx + math.cos(a) * rx * r, cy + math.sin(a) * ry * r, color_at(a))

    def line(self, x0, y0, x1, y1, c, width=1):
        steps = int(max(abs(x1 - x0), abs(y1 - y0)) * 2) + 1
        for i in range(steps + 1):
            t = i / steps
            x, y = x0 + (x1 - x0) * t, y0 + (y1 - y0) * t
            self.put(x, y, c)
            if width > 1:
                self.put(x + 1, y, c)

    def curve(self, points, c, width=1):
        for (x0, y0), (x1, y1) in zip(points, points[1:]):
            self.line(x0, y0, x1, y1, c, width)

    def leaf(self, x, y, angle, length, width, light="leaf_l", mid="leaf_m", dark="leaf_d"):
        """A pointed leaf from (x, y) along angle (degrees, 0 = right, 90 = up)."""
        a = math.radians(angle)
        ux, uy = math.cos(a), -math.sin(a)
        nx, ny = -uy, ux
        # The side facing up gets the light shade.
        up_side = 1 if ny < 0 else -1
        for i in range(int(length * 3) + 1):
            t = i / (length * 3)
            half = width * math.sin(math.pi * min(t * 1.15, 1.0)) ** 0.7
            for j in range(-int(half * 3), int(half * 3) + 1):
                s = j / 3
                px, py = x + ux * t * length + nx * s, y + uy * t * length + ny * s
                side = s * up_side
                col = light if side > half * 0.35 else dark if side < -half * 0.2 else mid
                self.put(px, py, col)
        for i in range(int(length * 2)):
            t = i / (length * 2) * 0.8
            self.put(x + ux * t * length, y + uy * t * length, dark)

    def outline(self, color="out"):
        edge = []
        for y in range(self.h):
            for x in range(self.w):
                if self.px[y][x] is None and any(
                    self.get(x + dx, y + dy) not in (None, color)
                    for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))
                ):
                    edge.append((x, y))
        for x, y in edge:
            self.px[y][x] = color
        return self


def mound(cv, cx, cy, rx, ry, seed=0):
    rnd = random.Random(seed)

    def shade(dx, dy):
        if dy < -0.45:
            return "soil_l"
        return "soil_d" if dy > 0.4 else "soil_m"

    cv.ellipse(cx, cy, rx, ry, None, shade)
    for _ in range(int(rx)):
        cv.put(cx + rnd.uniform(-rx * 0.7, rx * 0.7), cy + rnd.uniform(-ry * 0.2, ry * 0.5), "soil_d")


def spud(cv, cx, cy, rx=3, ry=2, glint=False):
    cv.ellipse(cx, cy, rx, ry, None, lambda dx, dy: "spud_l" if dy < 0.2 and dx < 0.4 else "spud_d")
    cv.put(cx + 1, cy, "soil_m")
    cv.put(cx - 1, cy + 1, "soil_m")
    if glint:
        cv.put(cx - 1, cy - 1, "cream")


def sparkle(cv, x, y, size=2, color="cream"):
    cv.put(x, y, color)
    for k in range(1, size + 1):
        for dx, dy in ((k, 0), (-k, 0), (0, k), (0, -k)):
            cv.put(x + dx, y + dy, color if k < size else "fire")


def flower(cv, x, y):
    for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
        cv.put(x + dx, y + dy, "cream")
    cv.put(x, y, "fire")


def sheet(name, frames):
    w, h = frames[0].w, frames[0].h
    img = Image.new("RGBA", (w * len(frames), h), (0, 0, 0, 0))
    for k, cv in enumerate(frames):
        for y in range(h):
            for x in range(w):
                c = cv.px[y][x]
                if c is not None:
                    img.putpixel((k * w + x, y), (*PALETTE[c], 255))
    img.save(OUT_DIR / f"sam_{name}.png")
    print(f"sam_{name}.png: {len(frames)} frames {w}x{h}")


# Loops end on the rest pose they start with, so a respawn overlapping the old copy is seamless.
SWAY = (0, 1, 0, -1, 0)


# Tater plant stages. Loops: rest, lean right, rest, lean left, rest.

def tater_sprout(sway):
    cv = Canvas(32, 24)
    mound(cv, 16, 20, 8, 3, 1)
    top = (16 + sway, 12)
    cv.curve([(16, 19), (16, 16), top], "leaf_d")
    cv.leaf(top[0], top[1] + 1, 35 + sway * 6, 6, 2.2)
    cv.leaf(top[0], top[1] + 1, 145 + sway * 6, 6, 2.2)
    cv.leaf(top[0], top[1], 90 + sway * 8, 3, 1.2)
    return cv.outline()


def potato_bush(cv, cx, base, sway, scale, flowers=False):
    stems = [(-62, 0.75), (-30, 1.0), (0, 1.1), (28, 0.95), (58, 0.75)]
    for i, (lean, reach) in enumerate(stems):
        a = math.radians(90 - lean - sway * 4)
        tip = (cx + math.cos(a) * 12 * reach * scale, base - math.sin(a) * 12 * reach * scale)
        cv.line(cx, base, tip[0], tip[1], "leaf_d")
        # Compound potato leaf: leaflet pairs along the stem and one at the tip.
        for t in (0.45, 0.75):
            sx, sy = cx + (tip[0] - cx) * t, base + (tip[1] - base) * t
            ang = 90 - lean - sway * 4
            cv.leaf(sx, sy, ang + 55, 4.5 * scale, 1.7 * scale)
            cv.leaf(sx, sy, ang - 55, 4.5 * scale, 1.7 * scale)
        cv.leaf(tip[0], tip[1], 90 - lean - sway * 4, 5 * scale, 2 * scale)
        if flowers and i in (1, 3):
            flower(cv, tip[0] + math.cos(a) * 4, tip[1] - math.sin(a) * 4 - 1)
    if flowers:
        flower(cv, cx + sway, base - 15 * scale)


def tater_leafy(sway):
    cv = Canvas(32, 32)
    mound(cv, 16, 28, 9, 3, 2)
    potato_bush(cv, 16, 26, sway, 0.8)
    return cv.outline()


def tater_ripe(frame):
    sway = SWAY[frame]
    frame %= 4
    cv = Canvas(32, 32)
    mound(cv, 16, 28, 11, 3.5, 3)
    spud(cv, 8, 28, glint=frame == 1)
    spud(cv, 24, 28.5, glint=frame == 3)
    spud(cv, 15, 30, 2.5, 1.5)
    potato_bush(cv, 16, 26, sway, 0.95, flowers=True)
    return cv.outline()


def tater_sow(frame):
    """Seed lands, the soil heaves into a mound and a shoot breaks through."""
    cv = Canvas(32, 24)
    rnd = random.Random(10 + frame)
    sizes = [(3, 1.2), (6, 2.5), (8, 3), (8, 3)]
    rx, ry = sizes[frame]
    mound(cv, 16, 20, rx, ry, 1)
    if frame <= 2:
        for _ in range(6 - frame):
            a = rnd.uniform(0.2, math.pi - 0.2)
            r = 4 + frame * 4 + rnd.uniform(0, 2)
            cv.put(16 + math.cos(a) * r, 19 - math.sin(a) * r * 0.8, rnd.choice(["soil_m", "soil_l"]))
    if frame == 0:
        spud(cv, 16, 12 + 0, 1.5, 1.2)
    if frame >= 2:
        cv.line(16, 18, 16, 15 if frame == 2 else 13, "leaf_m")
    if frame == 3:
        cv.leaf(16, 14, 40, 4, 1.6)
        cv.leaf(16, 14, 140, 4, 1.6)
    return cv.outline()


# Bramble stages: arching thorny canes, dark leaves, red berries when grown.

def cane(cv, x0, y0, lean, height, reach, sway, thorns=True, arch=0.85):
    """A cane rising from (x0, y0) and arching over toward lean (-1 left, 1 right)."""
    pts = []
    for i in range(13):
        t = i / 12
        x = x0 + lean * reach * t + sway * t * t
        y = y0 - height * math.sin(math.pi * t * arch)
        pts.append((x, y))
    cv.curve(pts, "thorn")
    if thorns:
        for i in range(2, 12, 3):
            x, y = pts[i]
            cv.put(x, y - 1, "soil_l")
            cv.put(x + (1 if i % 2 else -1), y, "soil_l")
    return pts


def bramble_sprout(sway):
    cv = Canvas(32, 24)
    mound(cv, 16, 20, 7, 2.5, 4)
    pts = cane(cv, 16, 19, 1, 9, 6, sway)
    cane(cv, 15, 19, -1, 6, 5, sway)
    x, y = pts[8]
    cv.leaf(x, y, 60 + sway * 5, 4, 1.6, "leaf_m", "leaf_d", "leaf_d")
    # Curling tip.
    tx, ty = pts[-1]
    cv.put(tx + 1, ty + 1, "thorn")
    cv.put(tx + 1, ty + 2, "thorn")
    return cv.outline()


def bramble_bush(sway, width=32, height=32, humps=1, berries=False, seed=5):
    """A dark leafy dome with thorny canes curling out of it."""
    cv = Canvas(width, height)
    cx, base = width // 2, height - 4
    rnd = random.Random(seed)
    mound(cv, cx, base + 1, width * 0.36, 2.5, seed)
    span = width / 2 - 5
    centers = [cx + span * 0.75 * (2 * k / max(humps - 1, 1) - 1) if humps > 1 else cx for k in range(humps)]
    tall = height - 13
    canes = []
    # Canes behind the leaves, arching out past the dome and curling at the tip.
    for k, hx in enumerate(centers):
        for lean in (-1, 1):
            h = tall * rnd.uniform(0.75, 0.95)
            pts = []
            for i in range(15):
                t = i / 14
                x = hx + lean * (span / humps * 0.95 + 1) * t + sway * lean * t * t
                y = base - 1 - h * math.sin(math.pi * min(t * 1.1, 1.0) * 0.9)
                pts.append((x, y))
            tx, ty = pts[-1]
            pts += [(tx + lean, ty + 1), (tx + lean, ty + 2), (tx, ty + 2)]
            canes.append(pts)
            cv.curve(pts, "thorn")
    for hx in centers:
        rx, ry = span / humps + 3, tall * 0.62
        for j in range(12):
            ox = rnd.uniform(-rx * 0.75, rx * 0.75)
            top = ry * (1 - (ox / rx) ** 2)
            cv.ellipse(hx + ox, base - 3 - rnd.uniform(0, top), rnd.uniform(3, 4.5), rnd.uniform(2.5, 3.5),
                       None, lambda dx, dy: "leaf_m" if dy < -0.35 and dx < 0.3 else "leaf_d")
    # Leaf tips and highlights so the dome reads as foliage, not a blob.
    for hx in centers:
        for _ in range(7):
            x = hx + rnd.uniform(-span / humps - 2, span / humps + 2)
            y = base - rnd.uniform(4, tall * 1.05)
            if cv.get(round(x), round(y)) in ("leaf_d", "leaf_m"):
                cv.put(x, y, "leaf_l")
                cv.put(x + 1, y, "leaf_m")
    # Front canes crossing the foliage with pale thorns.
    for pts in canes[::2]:
        cv.curve(pts[4:11], "thorn")
    for pts in canes:
        for i in range(3, len(pts) - 3, 3):
            x, y = pts[i]
            cv.put(x, y - 1, "soil_l")
    if berries:
        for _ in range(12 * humps):
            x = cx + rnd.uniform(-span, span)
            y = base - rnd.uniform(5, tall)
            if cv.get(round(x), round(y)) in ("leaf_d", "leaf_m", "leaf_l"):
                cv.put(x, y, "red")
                cv.put(x + 1, y, "red")
                cv.put(x, y + 1, "thorn")
                cv.put(x + 1, y + 1, "red")
                cv.put(x, y, "cream")
    return cv.outline()


def thorn_jab(frame):
    """Thorny canes stab up around a foe's feet and sink back."""
    cv = Canvas(32, 24)
    heights = [3, 9, 12, 6, 2]
    h = heights[frame]
    for x0, lean, k in ((9, -1, 0.8), (16, 0, 1.0), (23, 1, 0.85), (12, -0.5, 0.6), (20, 0.5, 0.6)):
        top = (x0 + lean * h * 0.3, 21 - h * k)
        cv.line(x0, 21, top[0], top[1], "thorn")
        cv.put(top[0], top[1] - 1, "soil_l")
        if h > 5:
            cv.put((x0 + top[0]) / 2 + 1, (21 + top[1]) / 2, "soil_l")
    cv.ellipse(16, 21.5, 10, 1.5, "soil_d")
    if frame == 2:
        for x, y in ((14, 8), (19, 10), (11, 11)):
            cv.put(x, y, "red")
    return cv.outline()


def plant_wilt(frame):
    """A plant droops, browns and crumbles back into the soil."""
    cv = Canvas(32, 32)
    flat = [3, 3, 2.5, 2, 1.5][frame]
    mound(cv, 16, 28, 9 - frame, flat, 2)
    colors = [("leaf_m", "leaf_d", "leaf_d"), ("soil_l", "leaf_d", "soil_m"),
              ("soil_l", "soil_m", "soil_d"), ("soil_m", "soil_d", "soil_d")]
    if frame < 4:
        light, mid, dark = colors[frame]
        droop = frame * 22
        height = 12 - frame * 3
        for lean in (-50, -15, 20, 55):
            ang = 90 - lean - (droop if lean > 0 else -droop)
            a = math.radians(ang)
            tip = (16 + math.cos(a) * height, 26 - math.sin(a) * height)
            cv.line(16, 26, tip[0], tip[1], dark)
            cv.leaf(tip[0], tip[1], ang - (40 if lean > 0 else -40), 5, 1.8, light, mid, dark)
    rnd = random.Random(frame)
    if frame >= 2:
        for _ in range(4 + frame * 2):
            cv.put(16 + rnd.uniform(-10, 10), 26 + rnd.uniform(-6 + frame, 3), rnd.choice(["soil_m", "soil_l"]))
    return cv.outline()


def spud_hit(frame):
    """A potato lobbed in from the upper left bonks the foe and bounces off."""
    cv = Canvas(32, 32)
    if frame == 0:
        spud(cv, 6, 3, 3, 2)
        cv.line(1, 1, 3, 2, "cream")
        cv.line(1, 4, 3, 4, "cream")
    elif frame == 1:
        spud(cv, 14, 8, 3.5, 1.6)
        sparkle(cv, 17, 6, 3)
    elif frame == 2:
        spud(cv, 22, 3, 3, 2)
        for x, y in ((10, 6), (16, 10), (19, 7)):
            sparkle(cv, x, y, 1, "fire")
        sparkle(cv, 14, 8, 4)
    elif frame == 3:
        spud(cv, 28, 1, 2.5, 1.5)
        for a in range(3):
            ang = a * 2.1 + 0.5
            sparkle(cv, 15 + math.cos(ang) * 6, 9 + math.sin(ang) * 2, 1, "fire")
    else:
        for a in range(3):
            ang = a * 2.1 + 1.2
            cv.put(15 + math.cos(ang) * 6, 9 + math.sin(ang) * 2, "fire")
    return cv.outline()


def harvest_pop(frame):
    """A ripe potato pops out of its mound and glints."""
    cv = Canvas(32, 32)
    mound(cv, 16, 28, 9 - min(frame, 2) * 2, 3 - min(frame, 2) * 0.5, 3)
    rnd = random.Random(20 + frame)
    heights = [26, 18, 9, 6, 6]
    y = heights[frame]
    if frame < 4:
        spud(cv, 16, y, 3.5, 2.5, glint=frame >= 2)
    if frame in (1, 2):
        for _ in range(7):
            a = rnd.uniform(0.3, math.pi - 0.3)
            r = rnd.uniform(4, 9) + frame * 2
            cv.put(16 + math.cos(a) * r, 27 - math.sin(a) * r * 0.7, rnd.choice(["soil_m", "soil_l", "soil_d"]))
    if frame >= 2:
        sparkle(cv, 21, y - 3, 2 if frame < 4 else 3)
    if frame >= 3:
        sparkle(cv, 10, y + 1, 1)
    return cv.outline()


def stew_pot(frame):
    """An iron pot bubbling over a small fire, steam drifting up."""
    cv = Canvas(32, 32)
    # Logs.
    cv.line(8, 29, 23, 27, "soil_d", 2)
    cv.line(9, 27, 24, 29, "soil_m", 2)
    # Flames, flickering per frame.
    flick = [(0, 1, 0), (1, 0, 1), (0, 0, 1), (1, 1, 0)][frame]
    for i, x in enumerate((11, 16, 21)):
        h = 3 + flick[i] * 2
        cv.ellipse(x, 27 - h / 2, 1.8, h / 2 + 0.5, "red")
        cv.ellipse(x, 27 - h / 3, 1, h / 3, "fire")
    # Pot body.
    cv.ellipse(16, 19, 9, 6.5, None, lambda dx, dy: "iron_l" if dx < -0.55 and dy < 0.3 else "iron_d")
    cv.line(7, 14, 25, 14, "iron_l")
    cv.ellipse(16, 14, 8, 1.6, "soil_d")
    cv.line(9, 14, 23, 14, "spud_d")
    bubble = [(12, 13), (19, 14), (15, 13), (21, 13)][frame]
    cv.put(*bubble, "spud_l")
    cv.put(bubble[0] + 1, bubble[1], "spud_l")
    # Handles.
    cv.put(6, 16, "iron_d")
    cv.put(26, 16, "iron_d")
    cv.outline()
    # Steam on top, left unoutlined so it reads soft.
    for k in range(3):
        t = (frame + k * 4 / 3) % 4 / 4
        sx = 16 + math.sin((t + k) * 5) * 3 + (k - 1) * 3
        sy = 11 - t * 10
        size = 1 + t * 1.5
        cv.ellipse(sx, sy, size, size * 0.8, "cream")
    return cv


def frying_pan(cv, cx, cy, angle, face):
    """Pan centered at (cx, cy), handle pointing along angle. face 0..1 widens the disk."""
    a = math.radians(angle)
    hx, hy = cx + math.cos(a) * 12, cy - math.sin(a) * 12
    cv.line(cx + math.cos(a) * 5, cy - math.sin(a) * 5, hx, hy, "soil_d", 2)
    rx, ry = 6, 2 + 4 * face
    cv.ellipse(cx, cy, rx, ry, None, lambda dx, dy: "iron_l" if dx * dx + dy * dy > 0.6 else "iron_d")
    if face > 0.5:
        cv.put(cx - 2, cy - 2, "cream")
        cv.put(cx - 3, cy - 1, "iron_l")


def pan_clang(frame):
    """Sam's frying pan swings up and rings off a blocked blow."""
    cv = Canvas(32, 32)
    poses = [(10, 14, 200, 0.2), (14, 12, 230, 0.8), (16, 12, 250, 1.0), (16, 12, 250, 1.0), (15, 13, 240, 0.9)]
    x, y, ang, face = poses[frame]
    frying_pan(cv, x, y, ang, face)
    cv.outline()
    if frame == 2:
        sparkle(cv, 22, 6, 4)
        for dx, dy in ((5, -5), (7, 0), (4, 5)):
            cv.line(22 + dx * 0.6, 6 + dy * 0.6, 22 + dx, 6 + dy, "fire")
    elif frame == 3:
        sparkle(cv, 23, 5, 2)
        cv.put(27, 9, "cream")
        cv.put(19, 2, "cream")
    elif frame == 1:
        cv.line(4, 20, 8, 16, "cream")
    return cv


def rope_color(a):
    """Twisted elven rope: alternating silver and light strands."""
    k = int((a % (2 * math.pi)) / (2 * math.pi) * 28)
    return ("cream", "iron_l", "iron_l", "iron_d")[k % 4]


def rope_lasso(frame):
    """A loop of silver elven rope drops around a foe's feet and cinches tight."""
    cv = Canvas(32, 16)
    sizes = [(14, 5, 3), (12, 4, 5), (9, 3, 8), (8, 2.6, 8), (8, 2.6, 8)]
    rx, ry, cy = sizes[frame]
    cv.ring(16, cy, rx, ry, rope_color, 1.0)
    if frame >= 2:
        # Knot and trailing end.
        cv.ellipse(16 + rx, cy, 1.5, 1.5, "iron_l")
        cv.put(16 + rx, cy, "cream")
        cv.line(17 + rx, cy, 21 + rx, cy - 3, "iron_l")
    if frame == 3:
        sparkle(cv, 16 - rx, cy - 2, 1)
    return cv.outline()


def rope_knot(frame):
    """A twist of rope flicking along the throw."""
    cv = Canvas(16, 16)
    a0 = frame * 1.1
    pts = [(8 + math.cos(a0 + t) * (2 + t), 8 + math.sin(a0 + t) * (2 + t) * 0.7) for t in [i * 0.35 for i in range(14)]]
    for i, (x, y) in enumerate(pts):
        cv.put(x, y, ("cream", "iron_l", "iron_l", "iron_d")[i % 4])
    return cv.outline()


def main():
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    strip = Image.new("RGBA", (len(PALETTE), 1))
    for i, rgb in enumerate(PALETTE.values()):
        strip.putpixel((i, 0), (*rgb, 255))
    strip.save(OUT_DIR / "sam_palette.png")
    sheet("sow", [tater_sow(f) for f in range(4)])
    sheet("tater_sprout", [tater_sprout(s) for s in SWAY])
    sheet("tater_leafy", [tater_leafy(s) for s in SWAY])
    sheet("tater_ripe", [tater_ripe(f) for f in range(5)])
    sheet("bramble_sprout", [bramble_sprout(s) for s in SWAY])
    sheet("bramble_bush", [bramble_bush(s) for s in SWAY])
    sheet("bramble_hedge", [bramble_bush(s, 48, 32, 2, True, 7) for s in SWAY])
    sheet("thorn_jab", [thorn_jab(f) for f in range(5)])
    sheet("wilt", [plant_wilt(f) for f in range(5)])
    sheet("spud_hit", [spud_hit(f) for f in range(5)])
    sheet("harvest", [harvest_pop(f) for f in range(5)])
    sheet("stew_pot", [stew_pot(f % 4) for f in range(5)])
    sheet("pan_clang", [pan_clang(f) for f in range(5)])
    sheet("rope_lasso", [rope_lasso(f) for f in range(5)])
    sheet("rope_knot", [rope_knot(f) for f in range(4)])


if __name__ == "__main__":
    main()
