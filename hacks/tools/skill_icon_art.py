"""Turns skill icon art into hack-ready skill tree icons.

slice: cut icons out of an upscaled skill screen mockup (3x3 passive grid on the left, 3/2/1
active slots on the right) or an icon sheet into 32x24 PNGs.
build: convert a folder of those PNGs into a C file (plus .h) of tile icon assets, and with
--hd-out the PC port's HD pack of the full-resolution crops that slice --hd saved.
hd: write that HD pack for icons already compiled into a C file.
preview: show a folder's PNGs next to how build draws them in game.

    uv run python hacks/tools/skill_icon_art.py slice MOCKUP.png OUT_DIR --names a,b,...
    uv run python hacks/tools/skill_icon_art.py slice SHEET.png OUT_DIR --grid 13,206:94,294:5.69
    uv run python hacks/tools/skill_icon_art.py build PNG_DIR OUT.c --prefix SamIcon
    uv run python hacks/tools/skill_icon_art.py hd PNG_DIR OUT.c HD_DIR --prefix SamIcon
    uv run python hacks/tools/skill_icon_art.py preview PNG_DIR OUT.png

The frame always comes from the stock icon frame, so only the circle interior is taken from
the art.

The default color style writes full-color RGB PNGs, a sharpened downscale of the art. build
then splits a folder of them into at most six color groups and fits eight colors to each group.
The game draws the icon with the frame's colors 0..7 plus its group's colors 8..15
(SkillExpansionIconColors). The classic style writes PNGs indexed in the shared stock palette,
which the game recolors with the art ramps.
"""

import argparse
import glob
import os
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageFilter

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", "tools"))

from rotkit.carve.skill_icons import (  # noqa: E402
    ICON_HEIGHT,
    ICON_TILE_BYTES,
    ICON_WIDTH,
    SHARED_PALETTE,
    rebuild_tiles,
)
from rotkit.paths import GFX_LOCAL  # noqa: E402
from rotkit.png import (  # noqa: E402
    IndexedImage,
    bgr555_to_rgb,
    read_indexed,
    rgb_to_bgr555,
    write_indexed,
)

PALETTE_RGB = np.array([bgr555_to_rgb(c) for c in SHARED_PALETTE], float)
# Indices 1..14 are drawable. Index 15 duplicates 14.
DRAWABLE = range(1, 15)
LUMA = np.array([0.3, 0.59, 0.11])

# Slot origins in the 1858x846 mockup layout, passives then actives. Other sizes scale.
MOCKUP_WIDTH = 1858
MOCKUP_SCALE = 7.5
MOCKUP_SLOTS = [
    (55, 61), (301, 61), (547, 61),
    (55, 305), (301, 305), (547, 305),
    (55, 549), (301, 550), (547, 549),
    (1042, 62), (1282, 61), (1534, 61),
    (1164, 306), (1411, 306),
    (1287, 550),
]
SEARCH = 12
# Greys darker than this mean brightness are stone backdrop and become transparent.
BACKDROP = 140

# Color style: the frame keeps palette indices 0..7, each group fits colors 8..15.
FRAME_COLORS = 8
OWN_COLORS = 8
GROUPS = 6
# Channel weights of the color distance. Green differences show most.
WEIGHT = np.array([0.35, 0.45, 0.2])
# Unsharp mask after the downscale, which keeps edges readable at 32x24.
SHARPEN = ImageFilter.UnsharpMask(radius=1, percent=60, threshold=2)
KMEANS_STEPS = 20
# Alpha contrast of the HD icon's upscaled corner mask, which keeps its edge a few pixels wide.
HD_EDGE = 3
# HD icons are stored at most this many times the GBA size, the renderer's highest scale.
HD_MAX_SCALE = 16


def stock_frame():
    """The stock frame pixels and a mask of the pixels every stock icon shares."""
    files = sorted(glob.glob(str(GFX_LOCAL / "skills" / "icons" / "*" / "*.png")))
    if not files:
        sys.exit("no carved icons: run `make carve` first")
    icons = np.stack([np.array(Image.open(f)) for f in files])
    shared = ~(icons != icons[0]).any(0)
    return icons[0], shared


def quantize(block, backdrop=BACKDROP):
    out = np.zeros((ICON_HEIGHT, ICON_WIDTH), np.uint8)
    for y in range(ICON_HEIGHT):
        for x in range(ICON_WIDTH):
            p = block[y, x]
            if p.max() - p.min() < 28 and 35 < p.mean() < backdrop:
                continue
            dist = (((PALETTE_RGB[1:15] - p) ** 2) * LUMA).sum(1)
            out[y, x] = DRAWABLE[int(dist.argmin())]
    return out


def sample(src, x0, y0, scale):
    box = (round(x0), round(y0), round(x0 + ICON_WIDTH * scale), round(y0 + ICON_HEIGHT * scale))
    return np.array(src.crop(box).resize((ICON_WIDTH, ICON_HEIGHT), Image.BOX)).astype(float)


def crisp_sample(src, x0, y0, scale):
    """A Lanczos downscale with a light sharpen, for the color style."""
    box = (round(x0), round(y0), round(x0 + ICON_WIDTH * scale), round(y0 + ICON_HEIGHT * scale))
    image = src.crop(box).resize((ICON_WIDTH, ICON_HEIGHT), Image.LANCZOS).filter(SHARPEN)
    return np.array(image).astype(float)


def fit(src, frame, shared, x, y, scale, backdrop):
    """The offset near (x, y) whose frame best matches the stock frame."""

    def score(dx, dy):
        q = quantize(sample(src, x + dx, y + dy, scale), backdrop)
        return (q[shared] != frame[shared]).sum()

    best = min(((score(dx, dy), dx, dy) for dy in range(-SEARCH, SEARCH + 1, 3)
                for dx in range(-SEARCH, SEARCH + 1, 3)))
    _, bx, by = best
    best = min(((score(bx + dx, by + dy), bx + dx, by + dy) for dy in (-1, 0, 1)
                for dx in (-1, 0, 1)))
    return x + best[1], y + best[2]


def to_image(pixels, palette=None):
    if palette is None:
        palette = [bgr555_to_rgb(c) for c in SHARED_PALETTE]
    return IndexedImage(ICON_WIDTH, ICON_HEIGHT, bytes(pixels.flatten()), palette, 0)


def grid_slots(spec):
    """`X,X,...:Y,Y,...:SCALE` frame origins of an icon sheet, slots row by row."""
    cols, rows, scale = spec.split(":")
    return [(int(x), int(y)) for y in rows.split(",") for x in cols.split(",")], float(scale)


def snap(colors):
    """Round to the GBA's 5-bit channels."""
    return np.clip(np.round(np.asarray(colors) / 8) * 8, 0, 248)


def distances(px, colors):
    return (((px[:, None, :] - colors[None]) ** 2) * WEIGHT).sum(2)


def fit_colors(px, fixed, count=OWN_COLORS, steps=KMEANS_STEPS):
    """count colors that, together with the fixed ones, best cover px (k-means). Returns them
    and the summed error."""
    rng = np.random.default_rng(1)
    free = []
    for _ in range(count):
        centers = np.vstack([fixed, *free]) if free else fixed
        d = distances(px, centers).min(1) + 1e-6
        free.append(px[rng.choice(len(px), p=d / d.sum())])
    free = np.array(free, float)
    for _ in range(steps):
        label = distances(px, np.vstack([fixed, free])).argmin(1) - len(fixed)
        for k in range(count):
            if (label == k).any():
                free[k] = px[label == k].mean(0)
    free = snap(free)
    return free, distances(px, np.vstack([fixed, free])).min(1).sum()


def group_icons(pixels, fixed, groups=GROUPS):
    """Split icons into at most `groups` groups, greedily merging the pair that costs least."""
    clusters = [[i] for i in range(len(pixels))]
    cost = {(i,): fit_colors(pixels[i], fixed)[1] for i in range(len(pixels))}
    while len(clusters) > groups:
        best = None
        for a in range(len(clusters)):
            for b in range(a + 1, len(clusters)):
                key = tuple(sorted(clusters[a] + clusters[b]))
                if key not in cost:
                    cost[key] = fit_colors(np.concatenate([pixels[i] for i in key]), fixed)[1]
                extra = cost[key] - cost[tuple(clusters[a])] - cost[tuple(clusters[b])]
                if best is None or extra < best[0]:
                    best = (extra, a, b)
        _, a, b = best
        clusters[a] = sorted(clusters[a] + clusters[b])
        del clusters[b]
    return clusters


def color_master(block, frame, shared):
    """Full-color 32x24 icon: the art inside the ring, stock frame colors outside."""
    rgb = PALETTE_RGB[frame].copy()
    rgb[~shared] = block[~shared]
    return np.clip(rgb, 0, 255).astype(np.uint8)


def cmd_slice(args):
    src = Image.open(args.mockup).convert("RGB")
    if args.grid:
        slots, scale = grid_slots(args.grid)
        ratio = 1.0
    else:
        slots, scale = MOCKUP_SLOTS, MOCKUP_SCALE
        ratio = src.width / MOCKUP_WIDTH
    names = args.names.split(",") if args.names else [f"{i:02d}" for i in range(len(slots))]
    frame, shared = stock_frame()
    os.makedirs(args.out_dir, exist_ok=True)
    for i, (x, y) in enumerate(slots):
        if i >= len(names) or names[i] in ("", "-"):
            continue
        x, y = fit(src, frame, shared, x * ratio, y * ratio, scale * ratio, args.backdrop)
        path = os.path.join(args.out_dir, f"{names[i]}.png")
        if args.hd:
            os.makedirs(args.hd, exist_ok=True)
            box = (round(x), round(y), round(x + ICON_WIDTH * scale * ratio),
                   round(y + ICON_HEIGHT * scale * ratio))
            src.crop(box).save(os.path.join(args.hd, f"{names[i]}.png"))
            if args.hd_only:
                print(f"slot {i:2d} at ({x:.0f}, {y:.0f}) -> {args.hd}")
                continue
        if getattr(args, "style", "color") == "classic":
            pixels = quantize(sample(src, x, y, scale * ratio), args.backdrop)
            pixels[shared] = frame[shared]
            write_indexed(path, to_image(pixels))
        else:
            block = crisp_sample(src, x, y, scale * ratio)
            Image.fromarray(color_master(block, frame, shared)).save(path)
        print(f"slot {i:2d} at ({x:.0f}, {y:.0f}) -> {path}")


def pascal_name(prefix, path):
    return prefix + "".join(w.capitalize() for w in os.path.basename(path)[:-4].split("_"))


def c_bytes(name, data):
    lines = [f"const u8 {name}[{len(data):#x}] __attribute__((aligned(4))) = {{"]
    for i in range(0, len(data), 16):
        lines.append("    " + ", ".join(f"{b:#04x}" for b in data[i : i + 16]) + ",")
    return lines + ["};", ""]


def icon_asset(pixels, palette):
    tiles = rebuild_tiles(to_image(pixels, palette))
    assert len(tiles) == ICON_TILE_BYTES
    return bytes([0x10, 0x00, ICON_TILE_BYTES & 0xFF, ICON_TILE_BYTES >> 8]) + tiles


def build_color(files):
    """Icons, palette groups and per-icon indexed pixels for a folder of RGB PNGs."""
    frame, shared = stock_frame()
    interior = ~shared
    fixed = PALETTE_RGB[1:FRAME_COLORS]
    masters = [np.array(Image.open(f).convert("RGB")).astype(float) for f in files]
    pixels = [m[interior] for m in masters]
    clusters = group_icons(pixels, fixed)
    palettes, results = [], {}
    for group, members in enumerate(clusters):
        own = fit_colors(np.concatenate([pixels[i] for i in members]), fixed)[0]
        palette = np.vstack([PALETTE_RGB[:FRAME_COLORS], own])
        palettes.append(own)
        for i in members:
            out = frame.copy()
            out[interior] = distances(pixels[i], palette[1:]).argmin(1) + 1
            results[i] = (group, out, palette)
    return palettes, [results[i] for i in range(len(files))]


def hd_icon(art_path, pixels, out_path):
    """The full-resolution art with the in-game icon's transparent corners."""
    art = Image.open(art_path).convert("RGB")
    if art.width > ICON_WIDTH * HD_MAX_SCALE:
        art = art.resize((ICON_WIDTH * HD_MAX_SCALE, ICON_HEIGHT * HD_MAX_SCALE), Image.LANCZOS)
    mask = Image.fromarray(np.where(pixels != 0, 255, 0).astype(np.uint8))
    soft = np.array(mask.resize(art.size, Image.BILINEAR)).astype(float)
    art.putalpha(Image.fromarray(np.clip((soft - 128) * HD_EDGE + 128, 0, 255).astype(np.uint8)))
    art.save(out_path)


def write_hd_pack(hd_in, hd_out, files, pixels, assets):
    """PNGs plus manifest.txt lines `<png> <tiles_w> <tiles_h> <tile data hex>` for the PC port."""
    os.makedirs(hd_out, exist_ok=True)
    lines = ["# Generated by hacks/tools/skill_icon_art.py. Edit the PNGs' art, not this file."]
    for f, p, asset in zip(files, pixels, assets):
        name = os.path.basename(f)
        art = os.path.join(hd_in, name)
        if not os.path.exists(art):
            print(f"no HD art for {name}")
            continue
        hd_icon(art, p, os.path.join(hd_out, name))
        lines.append(f"{name} {ICON_WIDTH // 8} {ICON_HEIGHT // 8} {asset[4:].hex()}")
    with open(os.path.join(hd_out, "manifest.txt"), "w", newline="\n") as out:
        out.write("\n".join(lines) + "\n")
    print(f"wrote {hd_out}: {len(lines) - 1} HD icons")


def compiled_assets(c_file):
    """Icon arrays of a generated C file: name -> asset bytes."""
    assets, name, data = {}, None, []
    for line in open(c_file):
        if line.startswith("const u8 "):
            name, data = line.split()[2].split("[")[0], []
        elif name and line.startswith("};"):
            assets[name], name = bytes(data), None
        elif name:
            data += [int(v, 16) for v in line.replace(",", " ").split()]
    return assets


def asset_pixels(asset):
    """The 32x24 palette indices of an icon asset's 4bpp tiles."""
    pixels = np.zeros((ICON_HEIGHT, ICON_WIDTH), np.uint8)
    columns = ICON_WIDTH // 8
    for i, byte in enumerate(asset[4:]):
        tile, offset = divmod(i, 32)
        y = (tile // columns) * 8 + offset // 4
        x = (tile % columns) * 8 + (offset % 4) * 2
        pixels[y, x], pixels[y, x + 1] = byte & 15, byte >> 4
    return pixels


def cmd_hd(args):
    files = icon_files(args.png_dir)
    assets = compiled_assets(args.c_file)
    names = [pascal_name(args.prefix, f) for f in files]
    missing = [n for n in names if n not in assets]
    if missing:
        sys.exit(f"{args.c_file}: no {', '.join(missing)}")
    chosen = [assets[n] for n in names]
    write_hd_pack(args.hd_dir, args.hd_dir, files, [asset_pixels(a) for a in chosen], chosen)


def icon_files(png_dir):
    files = sorted(glob.glob(os.path.join(png_dir, "*.png")))
    if not files:
        sys.exit(f"{png_dir}: no PNGs")
    return files


def cmd_build(args):
    files = icon_files(args.png_dir)
    color = Image.open(files[0]).mode != "P"
    if any((Image.open(f).mode != "P") != color for f in files):
        sys.exit(f"{args.png_dir}: mix of color and indexed PNGs")
    lines = [
        "// Generated by hacks/tools/skill_icon_art.py build. Edit the PNGs, not this file.",
        "",
        '#include "patches/skill_expansion/skill_expansion.h"' if color else '#include "types.h"',
        "",
    ]
    names = [pascal_name(args.prefix, f) for f in files]
    if args.hd_out and not color:
        sys.exit("--hd-out needs full-color icons")
    if color:
        palettes, results = build_color(files)
        assets = []
        for name, (_, pixels, palette) in zip(names, results):
            rgb = [tuple(int(v) for v in c) for c in palette]
            assets.append(icon_asset(pixels, rgb))
            lines += c_bytes(name, assets[-1])
        if args.hd_out:
            write_hd_pack(args.hd_in or args.hd_out, args.hd_out, files,
                          [r[1] for r in results], assets)
        lines.append(f"static const u16 {args.prefix}Palettes[][SKILL_EXPANSION_ICON_OWN_COLOR_COUNT]"
                     " = {")
        for own in palettes:
            lines.append("    {" + ", ".join(f"{rgb_to_bgr555(tuple(int(v) for v in c)):#06x}"
                                             for c in own) + "},")
        lines += ["};", "", f"const SkillExpansionIconColors {args.prefix}Colors[] = {{"]
        for name, (group, _, _) in zip(names, results):
            lines.append(f"    {{{name}, {args.prefix}Palettes[{group}], {group}}},")
        lines += ["    {NULL, NULL, 0},", "};", ""]
    else:
        for f, name in zip(files, names):
            image = read_indexed(f)
            if (image.width, image.height) != (ICON_WIDTH, ICON_HEIGHT):
                sys.exit(f"{f}: icons are {ICON_WIDTH}x{ICON_HEIGHT}")
            lines += c_bytes(name, icon_asset(np.frombuffer(image.pixels, np.uint8),
                                              image.palette))
    with open(args.out, "w", newline="\n") as out:
        out.write("\n".join(lines))
    header = [
        "#pragma once",
        "",
        "// Generated by hacks/tools/skill_icon_art.py build. "
        + ("Skill tree icons with their own colors." if color else "Raw tile skill tree icons."),
        "",
        '#include "patches/skill_expansion/skill_expansion.h"' if color else '#include "types.h"',
        "",
        *[f"extern const u8 {n}[{ICON_TILE_BYTES + 4:#x}];" for n in names],
    ]
    if color:
        header += ["", "// Colors of every icon above, ended by a NULL icon.",
                   f"extern const SkillExpansionIconColors {args.prefix}Colors[];"]
    with open(os.path.splitext(args.out)[0] + ".h", "w", newline="\n") as out:
        out.write("\n".join(header + [""]))
    print(f"wrote {args.out} and its .h: {len(names)} icons"
          + (f" in {len(palettes)} color groups" if color else ""))


def cmd_preview(args):
    """Each PNG next to its in-game look, scaled up, with group numbers for color PNGs."""
    files = icon_files(args.png_dir)
    color = Image.open(files[0]).mode != "P"
    game = []
    if color:
        _, results = build_color(files)
        game = [(palette[pixels].astype(np.uint8), group) for group, pixels, palette in results]
    else:
        for f in files:
            image = read_indexed(f)
            px = np.frombuffer(image.pixels, np.uint8).reshape(ICON_HEIGHT, ICON_WIDTH)
            game.append((PALETTE_RGB[px].astype(np.uint8), None))
    s, cols = args.scale, 5
    w, h = ICON_WIDTH * s * 2 + 12, ICON_HEIGHT * s + 22
    rows = (len(files) + cols - 1) // cols
    sheet = Image.new("RGB", (w * cols, h * rows), (36, 36, 44))
    draw = ImageDraw.Draw(sheet)
    for i, (f, (rgb, group)) in enumerate(zip(files, game)):
        x, y = (i % cols) * w, (i // cols) * h
        master = Image.open(f).convert("RGB").resize((ICON_WIDTH * s, ICON_HEIGHT * s),
                                                     Image.NEAREST)
        sheet.paste(master, (x + 4, y + 4))
        sheet.paste(Image.fromarray(rgb).resize((ICON_WIDTH * s, ICON_HEIGHT * s), Image.NEAREST),
                    (x + 8 + ICON_WIDTH * s, y + 4))
        label = os.path.basename(f)[:-4] + ("" if group is None else f"  group {group}")
        draw.text((x + 4, y + ICON_HEIGHT * s + 6), label, fill=(220, 220, 220))
    sheet.save(args.out)
    print(f"wrote {args.out}: PNG on the left, in game on the right")


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = parser.add_subparsers(required=True)
    p = sub.add_parser("slice")
    p.add_argument("mockup")
    p.add_argument("out_dir")
    p.add_argument("--names", help="comma list, one per slot in order; '-' skips a slot. Write "
                   "--names=-,a,b when the list starts with '-'.")
    p.add_argument("--grid", help="icon sheet instead of a mockup: X,X,..:Y,Y,..:SCALE")
    p.add_argument("--backdrop", type=float, default=BACKDROP,
                   help="frame alignment, and in the classic style greys darker than this "
                   "become transparent (lower it for grey art)")
    p.add_argument("--style", choices=("color", "classic"), default="color",
                   help="color: full-color PNGs drawn with their own palettes. classic: PNGs "
                   "in the shared stock palette, recolored by the art ramps.")
    p.add_argument("--hd", help="also save each icon's full-resolution crop to this folder")
    p.add_argument("--hd-only", action="store_true", help="with --hd, leave the 32x24 PNGs alone")
    p.set_defaults(func=cmd_slice)
    p = sub.add_parser("build")
    p.add_argument("png_dir")
    p.add_argument("out")
    p.add_argument("--prefix", required=True)
    p.add_argument("--hd-out", help="write the PC port's HD icon pack (PNGs and manifest.txt) here")
    p.add_argument("--hd-in", help="full-resolution crops from slice --hd (default: --hd-out, "
                   "whose PNGs keep their art under the alpha)")
    p.set_defaults(func=cmd_build)
    p = sub.add_parser("hd", help="(re)write the HD pack from the icons already compiled into C_FILE")
    p.add_argument("png_dir")
    p.add_argument("c_file")
    p.add_argument("hd_dir", help="full-resolution crops, rewritten in place with the manifest")
    p.add_argument("--prefix", required=True)
    p.set_defaults(func=cmd_hd)
    p = sub.add_parser("preview")
    p.add_argument("png_dir")
    p.add_argument("out")
    p.add_argument("--scale", type=int, default=4)
    p.set_defaults(func=cmd_preview)
    args = parser.parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
