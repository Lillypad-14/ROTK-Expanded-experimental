"""Small pixel-art kit for skill sprite sheets (hacks/content/fx/, skill_fx.json assets).

A hero's art script defines a palette (at most 15 named colors), draws each frame on a Canvas and
writes sheets plus one palette strip:

    from fx_draw import Canvas, Art
    art = Art("corsair", {"steel": (200, 208, 216), ...})
    cv = art.canvas(32, 32)
    cv.arc(16, 16, 12, -60, 60, 2, "steel")
    art.sheet("slash", [cv.image(), ...])
    art.save_palette()

Every sheet then shares the palette strip fx/<hero>_palette.png, so a hero's effects use one OBJ
palette slot. preview() writes an enlarged montage of every sheet for review.
"""

from __future__ import annotations

import math
import random
from pathlib import Path

from PIL import Image, ImageDraw

OUT_DIR = Path(__file__).resolve().parents[1] / "content/fx"


class Canvas:
    def __init__(self, art: "Art", w: int, h: int):
        self.art, self.w, self.h = art, w, h
        self.px = [[None] * w for _ in range(h)]

    def put(self, x, y, c):
        x, y = round(x), round(y)
        if c is not None and 0 <= x < self.w and 0 <= y < self.h:
            self.px[y][x] = c

    def get(self, x, y):
        x, y = round(x), round(y)
        return self.px[y][x] if 0 <= x < self.w and 0 <= y < self.h else None

    def dot(self, x, y, r, c):
        """A filled disc of radius r."""
        for yy in range(math.floor(y - r), math.ceil(y + r) + 1):
            for xx in range(math.floor(x - r), math.ceil(x + r) + 1):
                if (xx - x) ** 2 + (yy - y) ** 2 <= r * r + 0.25:
                    self.put(xx, yy, c)

    def ellipse(self, cx, cy, rx, ry, c, fill=True, width=1.0):
        for yy in range(math.floor(cy - ry) - 1, math.ceil(cy + ry) + 2):
            for xx in range(math.floor(cx - rx) - 1, math.ceil(cx + rx) + 2):
                d = ((xx - cx) / max(rx, 0.5)) ** 2 + ((yy - cy) / max(ry, 0.5)) ** 2
                if fill and d <= 1:
                    self.put(xx, yy, c)
                elif not fill and abs(math.sqrt(d) - 1) * min(rx, ry) <= width / 2:
                    self.put(xx, yy, c)

    def line(self, x0, y0, x1, y1, c, width=1.0):
        steps = max(1, int(math.hypot(x1 - x0, y1 - y0) * 2))
        for i in range(steps + 1):
            t = i / steps
            x, y = x0 + (x1 - x0) * t, y0 + (y1 - y0) * t
            if width <= 1:
                self.put(x, y, c)
            else:
                self.dot(x, y, width / 2, c)

    def arc(self, cx, cy, r, a0, a1, width, c, taper=False):
        """An arc from angle a0 to a1 (degrees, 0 right, 90 down). taper thins both ends."""
        steps = max(4, int(abs(a1 - a0) * r / 20))
        for i in range(steps + 1):
            t = i / steps
            a = math.radians(a0 + (a1 - a0) * t)
            w = width * (math.sin(math.pi * t) if taper else 1)
            self.dot(cx + r * math.cos(a), cy + r * math.sin(a), max(0.5, w / 2), c)

    def outline(self, c, inside=None):
        """Draw c on empty pixels next to drawn ones (a dark rim). inside limits it to those colors."""
        rim = []
        for y in range(self.h):
            for x in range(self.w):
                if self.px[y][x] is None:
                    for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                        n = self.get(x + dx, y + dy)
                        if n is not None and n != c and (inside is None or n in inside):
                            rim.append((x, y))
                            break
        for x, y in rim:
            self.px[y][x] = c

    def image(self) -> Image.Image:
        img = Image.new("RGBA", (self.w, self.h))
        for y in range(self.h):
            for x in range(self.w):
                c = self.px[y][x]
                if c is not None:
                    img.putpixel((x, y), (*self.art.palette[c], 255))
        return img


class Art:
    def __init__(self, hero: str, palette: dict[str, tuple[int, int, int]], seed: int = 1):
        assert len(palette) <= 15, f"{hero}: {len(palette)} colors, at most 15"
        self.hero, self.palette = hero, palette
        self.rng = random.Random(seed)
        self.sheets: list[tuple[str, list[Image.Image]]] = []

    def canvas(self, w, h) -> Canvas:
        return Canvas(self, w, h)

    def sheet(self, name, frames):
        w, h = frames[0].size
        img = Image.new("RGBA", (w * len(frames), h))
        for i, frame in enumerate(frames):
            img.paste(frame, (i * w, 0))
        OUT_DIR.mkdir(parents=True, exist_ok=True)
        img.save(OUT_DIR / f"{self.hero}_{name}.png")
        self.sheets.append((name, frames))

    def save_palette(self):
        strip = Image.new("RGBA", (len(self.palette), 1))
        for i, rgb in enumerate(self.palette.values()):
            strip.putpixel((i, 0), (*rgb, 255))
        strip.save(OUT_DIR / f"{self.hero}_palette.png")

    def preview(self, path, scale=4):
        """Every sheet, one row each, frames side by side on a dark floor color, enlarged."""
        rows = [(n, f) for n, f in self.sheets]
        width = max(sum(fr.width + 4 for fr in f) for _, f in rows) + 90
        height = sum(max(fr.height for fr in f) + 6 for _, f in rows)
        img = Image.new("RGBA", (width, height), (92, 88, 96, 255))
        draw = ImageDraw.Draw(img)
        y = 0
        for name, frames in rows:
            draw.text((2, y + 2), name, fill=(255, 255, 0, 255))
            x = 90
            for fr in frames:
                img.alpha_composite(fr, (x, y))
                x += fr.width + 4
            y += max(fr.height for fr in frames) + 6
        img.resize((img.width * scale, img.height * scale), Image.NEAREST).save(path)
