"""Painted dialog portraits for heroes with no stock face: drawn at 8x, then reduced by
portrait_art.compose onto the stock backdrop.

    python hacks/tools/portrait_paint.py [Warg|Spider ...] [--preview DIR]
"""

import argparse
import math
import random
from pathlib import Path

from PIL import Image, ImageChops, ImageDraw, ImageFilter

import portrait_art

S = 8
W, H = portrait_art.W * S, portrait_art.H * S


def layer() -> tuple[Image.Image, ImageDraw.ImageDraw]:
    image = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    return image, ImageDraw.Draw(image)


def pts(points, scale=S):
    return [(x * scale, y * scale) for x, y in points]


def shade(base: Image.Image, light: tuple[float, float], strength: float) -> Image.Image:
    """Darken `base` away from `light` (portrait cells), inside its own alpha."""
    lx, ly = light[0] * S, light[1] * S
    grad = Image.new("L", (W, H))
    reach = math.hypot(W, H) * 0.75
    grad.putdata([int(255 * min(1.0, math.hypot(x - lx, y - ly) / reach) * strength)
                  for y in range(H) for x in range(W)])
    dark = Image.new("RGBA", (W, H), (0, 0, 0, 255))
    dark.putalpha(ImageChops.multiply(grad, base.getchannel("A")))
    return Image.alpha_composite(base, dark)


def fur(base: Image.Image, colors, count: int, length: float, angle: float, seed: int, spread=0.6):
    """Short strokes clipped to `base`'s alpha."""
    rng = random.Random(seed)
    strokes, draw = layer()
    alpha = base.getchannel("A")
    for _ in range(count):
        x, y = rng.uniform(0, W), rng.uniform(0, H)
        if alpha.getpixel((int(x), int(y))) < 128:
            continue
        a = angle + rng.uniform(-spread, spread)
        n = length * S * rng.uniform(0.5, 1.2)
        draw.line([(x, y), (x + math.cos(a) * n, y + math.sin(a) * n)], fill=rng.choice(colors), width=S // 2)
    strokes.putalpha(ImageChops.multiply(strokes.getchannel("A"), alpha))
    return Image.alpha_composite(base, strokes)


def glow(image: Image.Image, center, radius, color) -> Image.Image:
    halo, draw = layer()
    cx, cy = center[0] * S, center[1] * S
    draw.ellipse([cx - radius * S, cy - radius * S, cx + radius * S, cy + radius * S], fill=color)
    halo = halo.filter(ImageFilter.GaussianBlur(radius * S * 0.5))
    halo.putalpha(ImageChops.multiply(halo.getchannel("A"), image.getchannel("A")))
    return Image.alpha_composite(image, halo)


def warg() -> Image.Image:
    """A snarling warg's head, three-quarter view facing left, shoulders below."""
    body, d = layer()
    # Shoulders and ruff.
    d.polygon(pts([(8, 64), (14, 44), (26, 36), (44, 34), (56, 40), (56, 64)]), fill=(72, 62, 54, 255))
    # Skull, cheek and brow.
    d.ellipse([22 * S, 12 * S, 50 * S, 42 * S], fill=(104, 90, 76, 255))
    # Ears, laid back.
    d.polygon(pts([(34, 16), (42, 1), (46, 18)]), fill=(66, 56, 48, 255))
    d.polygon(pts([(40, 18), (52, 5), (51, 24)]), fill=(52, 44, 38, 255))
    d.polygon(pts([(37, 14), (42, 5), (44, 16)]), fill=(96, 62, 58, 255))
    # Muzzle reaching left and slightly down.
    d.polygon(pts([(28, 18), (6, 25), (3, 29), (6, 33), (26, 34), (34, 30)]), fill=(116, 100, 84, 255))
    # Lower jaw, open.
    d.polygon(pts([(28, 36), (8, 40), (7, 43), (12, 44), (32, 41)]), fill=(70, 60, 52, 255))
    body = fur(body, [(120, 106, 90, 255), (42, 36, 32, 255), (100, 88, 74, 255)], 2600, 2.6, math.pi * 0.85, 1)
    body = fur(body, [(30, 26, 24, 255), (130, 116, 98, 255)], 900, 3.5, math.pi * 0.6, 2, 0.9)
    mi, m = layer()
    # Mouth: dark gums, tongue, fangs.
    m.polygon(pts([(26, 33), (7, 33.5), (8, 40), (28, 37)]), fill=(120, 22, 28, 255))
    m.polygon(pts([(22, 36), (11, 37.5), (14, 39.5), (24, 38)]), fill=(196, 80, 84, 255))
    for x, y, h in ((8, 33.5, 3.4), (12, 33.6, 2.2), (16, 33.7, 2.0), (20, 33.8, 1.8), (24, 33.9, 1.5)):
        m.polygon(pts([(x - 1, y), (x + 1, y), (x, y + h)]), fill=(236, 226, 196, 255))
    for x, y, h in ((9.5, 40.2, 3.0), (14, 39.6, 2.0), (18, 39.0, 1.8), (22, 38.4, 1.5)):
        m.polygon(pts([(x - 1, y), (x + 1, y), (x, y - h)]), fill=(216, 206, 176, 255))
    # Nose.
    m.ellipse([2.5 * S, 25.5 * S, 7 * S, 29.5 * S], fill=(18, 16, 16, 255))
    m.ellipse([3.5 * S, 26 * S, 5 * S, 27 * S], fill=(120, 120, 120, 255))
    # Snarl wrinkles.
    for i in range(4):
        y = 22 + i * 1.6
        m.line(pts([(14 + i, y), (20 + i, y - 1.2), (25 + i, y + 0.4)]), fill=(34, 28, 26, 255), width=S // 2)
    # Brow ridge over the eye.
    m.line(pts([(26, 19.5), (31, 18), (36, 20)]), fill=(28, 24, 22, 255), width=S)
    body = Image.alpha_composite(body, mi)
    body = shade(body, (10, 6), 0.5)
    ei, e = layer()
    e.polygon(pts([(26, 21.8), (31, 19.4), (35.5, 21.6), (31, 23.8)]), fill=(255, 204, 40, 255))
    e.ellipse([30.2 * S, 19.8 * S, 31.8 * S, 23.2 * S], fill=(10, 8, 6, 255))
    body = glow(Image.alpha_composite(body, ei), (31, 21.5), 4, (255, 170, 30, 110))
    # Rim light on the brow, ears and muzzle top.
    ri, r = layer()
    r.line(pts([(6, 25), (20, 20.5), (28, 17.5), (36, 13), (42, 2)]), fill=(196, 182, 150, 200), width=S // 2)
    return Image.alpha_composite(body, ri)


def spider() -> Image.Image:
    """A great spider front-on: eye cluster, dripping fangs, legs reaching past the frame."""
    out, d = layer()
    rng = random.Random(7)
    # Abdomen looming behind, banded.
    d.ellipse([10 * S, -6 * S, 50 * S, 26 * S], fill=(92, 76, 56, 255))
    for i, y in enumerate((4, 10, 16)):
        d.arc([13 * S, (y - 6) * S, 47 * S, (y + 10) * S], 200, 340, fill=(170, 132, 64, 255), width=S // 2)
    # Legs: two segments each, knee high, foot past the frame.
    legs = [((22, 34), (8, 14), (-4, 30)), ((21, 37), (4, 26), (-6, 52)), ((22, 40), (6, 44), (-2, 66)),
            ((24, 42), (14, 54), (8, 70))]
    leg_layer, ld = layer()
    for hip, knee, foot in legs:
        for side in (1, -1):
            def mx(p):
                return (56 - p[0], p[1]) if side == -1 else p
            h, k, f = mx(hip), mx(knee), mx(foot)
            ld.line(pts([h, k]), fill=(72, 60, 46, 255), width=int(3.2 * S))
            ld.line(pts([k, f]), fill=(62, 52, 40, 255), width=int(2.4 * S))
            ld.ellipse([(k[0] - 1.8) * S, (k[1] - 1.8) * S, (k[0] + 1.8) * S, (k[1] + 1.8) * S], fill=(140, 110, 62, 255))
    leg_layer = fur(leg_layer, [(140, 118, 80, 255), (30, 24, 20, 255)], 1800, 1.6, -math.pi / 2, 3, 1.4)
    out = Image.alpha_composite(out, leg_layer)
    # Cephalothorax.
    body, b = layer()
    b.ellipse([16 * S, 22 * S, 40 * S, 50 * S], fill=(100, 84, 62, 255))
    body = fur(body, [(160, 134, 88, 255), (40, 32, 24, 255), (120, 100, 70, 255)], 2200, 1.8, -math.pi / 2, 4, 1.2)
    body = shade(body, (22, 18), 0.45)
    out = Image.alpha_composite(out, body)
    # Chelicerae and fangs.
    fi, f = layer()
    for side in (-1, 1):
        cx = 28 + side * 3.6
        f.ellipse([(cx - 3.6) * S, 41 * S, (cx + 3.6) * S, 52 * S], fill=(80, 64, 50, 255))
        f.polygon(pts([(cx - 1.6, 50), (cx + 1.6, 50), (cx - side * 1.8, 59)]), fill=(196, 180, 140, 255))
        f.ellipse([(cx - side * 1.8 - 0.8) * S, 59.5 * S, (cx - side * 1.8 + 0.8) * S, 61.5 * S],
                  fill=(150, 220, 120, 255))
    out = Image.alpha_composite(out, fi)
    # Eyes: two great ones, two smaller, a row of little ones.
    ei, e = layer()
    eyes = [(24, 33, 3.4), (32, 33, 3.4), (20.5, 29, 1.8), (35.5, 29, 1.8),
            (25.5, 28, 1.4), (30.5, 28, 1.4), (22, 37.5, 1.2), (34, 37.5, 1.2)]
    for x, y, r in eyes:
        e.ellipse([(x - r) * S, (y - r) * S, (x + r) * S, (y + r) * S], fill=(8, 10, 12, 255))
        e.ellipse([(x - r * 0.7) * S, (y - r * 0.7) * S, (x + r * 0.4) * S, (y + r * 0.2) * S],
                  fill=(40, 90, 70, 255))
        e.ellipse([(x - r * 0.55) * S, (y - r * 0.6) * S, (x - r * 0.1) * S, (y - r * 0.15) * S],
                  fill=(210, 240, 220, 255))
    out = Image.alpha_composite(out, ei)
    out = glow(out, (28, 33), 7, (60, 160, 110, 60))
    del rng
    return out


PAINTERS = {"Warg": warg, "Spider": spider}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("names", nargs="*", default=list(PAINTERS))
    parser.add_argument("--preview", help="directory for 4x previews")
    args = parser.parse_args()
    portrait_art.OUT.mkdir(parents=True, exist_ok=True)
    for name in args.names:
        portrait = portrait_art.compose(PAINTERS[name]())
        portrait.save(portrait_art.OUT / f"{name}.png")
        if args.preview:
            portrait.convert("RGB").resize((portrait_art.W * 4, portrait_art.H * 4), Image.NEAREST).save(
                Path(args.preview) / f"{name.lower()}4x.png")
        print(f"wrote {portrait_art.OUT / (name + '.png')}")


if __name__ == "__main__":
    main()
