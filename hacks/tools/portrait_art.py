"""Make a hero's dialog portrait (56x64, 16 colors) in the game's own style from any picture.

The stock portraits stand their subject on one backdrop: flat teal above row 24, a gold line
on row 24, then a teal/dark checker. Palette slot 0 is the teal, slot 8 the gold and slot 15
the subject's darkest color (the checker's dark squares). This tool keys the subject out of
its source, shrinks it to 56x64, quantizes it to the 13 free slots and stands it on that
backdrop.

    python hacks/tools/portrait_art.py SOURCE Name [--crop L T R B] [--key red] [--preview]

SOURCE is any image; an RGBA PNG keeps its own alpha as the cut-out. --key red drops a
red/orange backdrop (fire, the Eye) from a photo. --crop is in source pixels, ideally 7:8.
Writes hacks/content/portraits/<Name>.png; heroes.json `"portrait": "<Name>"` then uses it
(hero_registry.py encodes every PNG in that folder as DialogPortrait<Name>BgAsset).
"""

import argparse
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "hacks/content/portraits"
W, H = 56, 64
TEAL = (0, 107, 140)
GOLD = (199, 140, 0)
LINE_Y = 24
FREE_SLOTS = [1, 2, 3, 4, 5, 6, 7, 9, 10, 11, 12, 13, 14, 15]


def key_red(image: Image.Image) -> Image.Image:
    """Alpha that drops saturated red/orange/yellow (fire) reachable from the picture's edge,
    so warm colors inside the figure (skin, cloth) stay."""
    rgba = image.convert("RGBA").resize((W * 6, H * 6), Image.LANCZOS)
    px = rgba.load()
    fire = Image.new("L", rgba.size, 0)
    fp = fire.load()
    for y in range(rgba.height):
        for x in range(rgba.width):
            r, g, b, _ = px[x, y]
            if (r > 150 and r > g * 1.35 and r > b * 2.2) or (r > 170 and b < r * 0.5):
                fp[x, y] = 255
    fire = fire.filter(ImageFilter.MedianFilter(5))
    # Mark (128) the fire connected to the border, then cut only that.
    for x in range(fire.width):
        for y in (0, fire.height - 1):
            if fire.getpixel((x, y)) == 255:
                ImageDraw.floodfill(fire, (x, y), 128)
    for y in range(fire.height):
        for x in (0, fire.width - 1):
            if fire.getpixel((x, y)) == 255:
                ImageDraw.floodfill(fire, (x, y), 128)
    alpha = fire.point(lambda v: 0 if v == 128 else 255).filter(ImageFilter.MedianFilter(5))
    rgba.putalpha(alpha)
    return rgba


def compose(subject: Image.Image) -> Image.Image:
    """An RGBA picture (any size, 7:8) -> the 56x64 indexed portrait on the stock backdrop."""
    small = subject.convert("RGBA").resize((W, H), Image.LANCZOS)
    mask = [a >= 128 for a in small.getchannel("A").get_flattened_data()]
    rgb = small.convert("RGB")
    # Quantize the subject alone to 14 colors (13 free slots + the darkest, shared with the checker).
    pixels = [p for p, m in zip(rgb.get_flattened_data(), mask) if m]
    # Weight saturated pixels (eyes, blood, cloth) so small accents keep a palette slot.
    pixels += [p for p in pixels if max(p) - min(p) > 70] * 12
    if not pixels:
        raise SystemExit("the cut-out is empty")
    strip = Image.new("RGB", (len(pixels), 1))
    strip.putdata(pixels)
    quant = strip.quantize(colors=len(FREE_SLOTS), method=Image.Quantize.MEDIANCUT)
    colors = [tuple(quant.getpalette()[i * 3:i * 3 + 3]) for i in range(len(FREE_SLOTS))]
    # Brightest first, so the darkest lands in slot 15 like the stock art.
    order = sorted(range(len(colors)), key=lambda i: -sum(colors[i]))
    palette = [TEAL] + [(0, 0, 0)] * 15
    palette[8] = GOLD
    for slot, i in zip(FREE_SLOTS, order):
        palette[slot] = colors[i]
    dark = palette[15]
    # Dither the subject onto its own slots only.
    pal_image = Image.new("P", (1, 1))
    flat = []
    for slot in range(16):
        flat += palette[slot] if slot in FREE_SLOTS else palette[15]
    pal_image.putpalette(flat + [0] * (768 - len(flat)))
    dithered = rgb.quantize(palette=pal_image, dither=Image.Dither.FLOYDSTEINBERG)
    out = Image.new("P", (W, H))
    out.putpalette([c for color in palette for c in color])
    data = []
    for i, (index, keep) in enumerate(zip(dithered.get_flattened_data(), mask)):
        x, y = i % W, i // W
        if keep:
            data.append(15 if index in (0, 8) else index)
        elif y < LINE_Y:
            data.append(0)
        elif y == LINE_Y:
            data.append(8)
        elif y == LINE_Y + 1:
            data.append(15)
        else:
            data.append(15 if (x + y) % 2 == 0 else 0)
    out.putdata(data)
    assert dark == palette[15]
    return out


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("source")
    parser.add_argument("name", help="portrait name, letters only (heroes.json \"portrait\")")
    parser.add_argument("--crop", nargs=4, type=int, metavar=("L", "T", "R", "B"))
    parser.add_argument("--key", choices=["red"], help="drop a red/orange backdrop")
    parser.add_argument("--preview", help="also write a 4x RGB preview here")
    args = parser.parse_args()
    if not args.name.isalpha():
        raise SystemExit("name: letters only")
    image = Image.open(args.source)
    if args.crop:
        image = image.crop(tuple(args.crop))
    image = key_red(image) if args.key == "red" else image.convert("RGBA")
    portrait = compose(image)
    OUT.mkdir(parents=True, exist_ok=True)
    portrait.save(OUT / f"{args.name}.png")
    print(f"wrote {OUT / (args.name + '.png')}")
    if args.preview:
        portrait.convert("RGB").resize((W * 4, H * 4), Image.NEAREST).save(args.preview)


if __name__ == "__main__":
    main()
