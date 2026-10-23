"""Validate effect recipes and compile PNG sprite sheets into native FX assets."""

import argparse
from html import escape
import json
from pathlib import Path
import re
import struct

from PIL import Image
from rotkit.compression import lz77
from rotkit.png import bgr555_to_rgb, rgb_to_bgr555
from rotkit.spritegfx import OBJ_SIZES, OamTemplate, pixels_to_tiles

ROOT = Path(__file__).resolve().parents[2]
SHAPES = {
    "star_bloom": 0,
    "sparkle_ring": 2,
    "diamond_ring": 3,
    "rising_streaks": 4,
    "pool_flash": 5,
    "bubble_rings": 6,
    "ground_pool": 7,
    "tendril_burst": 8,
    "checker_ring": 9,
    "smoke_rise": 10,
    "smoke_small": 11,
    "ring_sparkles": 12,
    "sky_rain": 13,
    "streak_rain": 14,
    "lightning": 15,
    "spark": 16,
    "oval_ring": 17,
    "tendril_star": 18,
    "wide_ring": 19,
    "shockwave": 20,
    "shockwave_alt": 21,
    "debris": 22,
}
COLORS = {
    "white": 2,
    "ember": 3,
    "green": 4,
    "blue": 5,
    "nazgul": 6,
    "night": 7,
    "gold": 8,
}


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def number(value: object, low: int, high: int, label: str) -> int:
    require(
        type(value) is int and low <= value <= high, f"{label}: expected {low}..{high}"
    )
    return value


def key(value: str) -> str:
    require(
        isinstance(value, str) and re.fullmatch(r"[a-z][a-z0-9_]*", value) is not None,
        "keys must use lowercase letters, digits and underscores",
    )
    return value


def fields(value: dict, allowed: set[str], label: str) -> None:
    require(isinstance(value, dict), f"{label}: expected an object")
    require(
        not (set(value) - allowed), f"{label}: unknown fields {set(value) - allowed}"
    )


def array(name: str, data: bytes) -> str:
    rows = [
        ", ".join(f"0x{byte:02x}" for byte in data[i : i + 16])
        for i in range(0, len(data), 16)
    ]
    return (
        f"static const u8 {name}[] __attribute__((aligned(4))) = {{\n    "
        + ",\n    ".join(rows)
        + "\n};\n"
    )


def compile_sheet(asset: dict, base: Path) -> dict:
    fields(
        asset,
        {"key", "id", "sheet", "width", "height", "frameTicks", "anchor", "palette"},
        "asset",
    )
    name = key(asset["key"])
    identity = number(asset["id"], 0, 31, name + " palette ID")
    width = number(asset["width"], 8, 128, name + " width")
    height = number(asset["height"], 8, 128, name + " height")
    require(
        width % 8 == 0 and height % 8 == 0,
        f"{name}: frame size must use multiples of 8",
    )
    require(width * height // 2 <= 2304, f"{name}: exceeds the FX tile buffer")
    path = (base / asset["sheet"]).resolve()
    require(
        path.is_relative_to(base.resolve()),
        f"{name}: sheet must stay inside the manifest directory",
    )
    with Image.open(path) as source:
        sheet = source.convert("RGBA")
    require(
        sheet.width % width == 0 and sheet.height % height == 0,
        f"{name}: sheet dimensions must be multiples of the frame size",
    )
    count = sheet.width // width * (sheet.height // height)
    number(count, 1, 64, name + " frame count")
    ticks = number(asset.get("frameTicks", 2), 1, 255, name + " frameTicks")
    anchor = asset.get("anchor", [width // 2, height])
    require(
        isinstance(anchor, list) and len(anchor) == 2, f"{name}: expected [x,y] anchor"
    )
    ax, ay = [number(value, -128, 128, name + " anchor") for value in anchor]
    colors = set()
    for r, g, b, alpha in sheet.get_flattened_data():
        require(alpha in (0, 255), f"{name}: partial transparency is unsupported")
        if alpha:
            colors.add(rgb_to_bgr555((r, g, b)))
    require(
        0 < len(colors) <= 15,
        f"{name}: needs 1..15 opaque GBA colors, found {len(colors)}",
    )
    order = sorted(colors)
    if "palette" in asset:
        # A palette strip shared by several sheets, so they all fit one OBJ palette slot.
        strip_path = (base / asset["palette"]).resolve()
        require(
            strip_path.is_relative_to(base.resolve()),
            f"{name}: palette must stay inside the manifest directory",
        )
        with Image.open(strip_path) as source:
            strip = source.convert("RGBA")
        order = []
        for r, g, b, alpha in strip.get_flattened_data():
            color = rgb_to_bgr555((r, g, b))
            if alpha and color not in order:
                order.append(color)
        require(len(order) <= 15, f"{name}: palette strip has more than 15 colors")
        require(colors <= set(order), f"{name}: sheet uses colors outside its palette strip")
    palette = [0] + order
    indexes = {color: index + 1 for index, color in enumerate(order)}
    palette += [0] * (16 - len(palette))
    objects = []

    def partition(x: int, y: int, w: int, h: int) -> None:
        if w == 0 or h == 0:
            return
        (shape, size), (ow, oh) = max(
            (
                (attributes, dimensions)
                for attributes, dimensions in OBJ_SIZES.items()
                if dimensions[0] <= w and dimensions[1] <= h
            ),
            key=lambda pair: pair[1][0] * pair[1][1],
        )
        offset = sum(obj.size[0] * obj.size[1] // 64 for obj in objects)
        objects.append(OamTemplate(x, y, size, shape, offset))
        partition(x + ow, y, w - ow, oh)
        partition(x, y + oh, w, h - oh)

    partition(0, 0, width, height)
    require(len(objects) <= 31, f"{name}: exceeds 31 OBJ templates per frame")
    packing = tuple(objects)
    drawing = b"".join(
        struct.pack(
            "<I",
            OamTemplate(
                obj.x - ax, obj.y - ay, obj.obj_size, obj.obj_shape, obj.tile_offset
            ).pack(),
        )
        for obj in objects
    )
    tile_bytes = width * height // 2
    require(tile_bytes <= 2304, f"{name}: exceeds the FX tile buffer")
    tiles, records, previews = bytearray(), [], []
    for index in range(count):
        x, y = (
            (index % (sheet.width // width)) * width,
            (index // (sheet.width // width)) * height,
        )
        pixels = bytes(
            indexes[rgb_to_bgr555((r, g, b))] if a else 0
            for r, g, b, a in sheet.crop(
                (x, y, x + width, y + height)
            ).get_flattened_data()
        )
        require(len(tiles) <= 65535, f"{name}: frame tile offset exceeds 16 bits")
        records.append(
            struct.pack(
                "<BBBBHH", 0x40 | len(objects), 0, width, height, len(tiles), tile_bytes
            )
            + bytes(12)
            + drawing
        )
        compressed = lz77.encode(pixels_to_tiles(pixels, packing, width, height))
        tiles += compressed + bytes(-len(compressed) % 4)
        preview = Image.new("RGBA", (width, height))
        preview.putdata([(*bgr555_to_rgb(palette[p]), 255 if p else 0) for p in pixels])
        previews.append(preview)
    # A burst ends as it enters its last frame, so a copy of the last frame (same tiles) ends it
    # after every drawn frame got its full frameTicks.
    records.append(records[-1])
    stored = count + 1
    header = struct.pack(
        "<BB4bHHBB", width, height, 0, 0, 0, 0, stored, tile_bytes, 0, 2
    )
    offsets = [stored * 2 + index * len(records[0]) for index in range(stored)]
    frames = header + struct.pack(f"<{stored}H", *offsets) + b"".join(records)
    symbol = "SkillFxAsset_" + name
    code = array(symbol + "Tiles", tiles) + array(symbol + "Frames", frames)
    code += (
        f"static const u16 {symbol}Palette[16] = {{"
        + ",".join(hex(c) for c in palette)
        + "};\n"
    )
    code += f"static const SpriteAnimation {symbol} = {{{symbol}Tiles, (const SpriteFrameSetHeader *){symbol}Frames, NULL, {ticks}}};\n"
    return dict(
        key=name,
        id=identity,
        palette=palette,
        code=code,
        previews=previews,
        ticks=ticks,
        tileBytes=tile_bytes,
        frames=count,
        romBytes=len(tiles) + len(frames) + 32,
        colors=len(colors),
    )


def generate(manifest: Path, output: Path, preview: Path) -> None:
    document = json.loads(manifest.read_text())
    fields(document, {"version", "assets", "effects"}, "manifest")
    require(document.get("version") == 1, "unsupported effect manifest version")
    assets = {}
    palette_ids = {}
    for spec in document.get("assets", []):
        asset = compile_sheet(spec, manifest.parent)
        require(
            asset["key"] not in assets
            and palette_ids.get(asset["id"], asset["palette"]) == asset["palette"],
            "duplicate asset key, or a palette ID reused with different colors",
        )
        assets[asset["key"]] = asset
        palette_ids[asset["id"]] = asset["palette"]
    effects = document["effects"]
    number(len(effects), 1, 255, "effect count")
    definitions, names, identifiers, keys = [], [], [], set()
    code = "// Generated by hacks/tools/skill_fx.py.\n" + "".join(
        a["code"] for a in assets.values()
    )
    for index, effect in enumerate(effects):
        fields(effect, {"key", "id", "name", "layers"}, "effect")
        name = key(effect["key"])
        require(name not in keys, "duplicate effect key")
        require(
            number(effect["id"], 0, 254, "effect ID") == index,
            "effect IDs must be consecutive; append new effects",
        )
        keys.add(name)
        label = effect["name"]
        require(
            isinstance(label, str)
            and 1 <= len(label) <= 20
            and label.isascii()
            and label.isprintable(),
            "effect names need 1..20 ASCII characters",
        )
        layers = effect["layers"]
        number(len(layers), 1, 4, name + " layer count")
        entries = []
        for layer in layers:
            fields(
                layer,
                {
                    "asset",
                    "shape",
                    "color",
                    "frameTicks",
                    "duration",
                    "layer",
                    "attachment",
                },
                "layer",
            )
            require(
                ("asset" in layer) != ("shape" in layer),
                f"{name}: choose asset or shape",
            )
            if "asset" in layer:
                require(layer["asset"] in assets, f"{name}: unknown sprite asset")
            default_ticks = assets[layer["asset"]]["ticks"] if "asset" in layer else 2
            ticks = number(
                layer.get("frameTicks", default_ticks), 1, 255, name + " frameTicks"
            )
            duration = number(layer.get("duration", 0), 0, 3600, name + " duration")
            require(
                layer.get("layer", "front") in ("front", "behind"), "invalid draw layer"
            )
            require(
                layer.get("attachment", "follow") in ("follow", "fixed"),
                "invalid attachment",
            )
            common = f".frameTicks={ticks}, .duration={duration}, .layer={int(layer.get('layer', 'front') == 'front')}, .fixed={int(layer.get('attachment') == 'fixed')}"
            if "asset" in layer:
                require(layer["asset"] in assets, f"{name}: unknown sprite asset")
                require(
                    duration == 0,
                    f"{name}: sprite lifetimes come from animation frames",
                )
                asset = assets[layer["asset"]]
                require(
                    "color" not in layer,
                    f"{name}: imported sprites use their sheet palette",
                )
                symbol = "SkillFxAsset_" + asset["key"]
                entries.append(
                    f"{{.animation=&{symbol}, .palette={symbol}Palette, .paletteId={asset['id']}, {common}}}"
                )
            else:
                require(
                    layer["shape"] in SHAPES and layer.get("color") in COLORS,
                    f"{name}: unknown shape or color",
                )
                entries.append(
                    f"{{.shape={SHAPES[layer['shape']]}, .color={COLORS[layer['color']]}, {common}}}"
                )
        code += (
            f"static const HackLibEffectLayer SkillFxLayers_{name}[] = {{"
            + ",".join(entries)
            + "};\n"
        )
        definitions.append(f"{{SkillFxLayers_{name}, {len(layers)}}}")
        names.append(json.dumps(label))
        identifiers.append(f"    HACKLIB_EFFECT_{name.upper()} = {index},")
    code += (
        "const HackLibVisualEffect HackLibVisualEffects[] = {"
        + ",".join(definitions)
        + "};\n"
    )
    code += (
        "const char *const HackLibVisualEffectNames[] = {" + ",".join(names) + "};\n"
    )
    output.mkdir(parents=True, exist_ok=True)
    (output / "skill_fx_assets.inc").write_text(code, newline="\n")
    (output / "skill_fx_ids.h").write_text(
        "#pragma once\nenum HackLibEffectId {\n"
        + "\n".join(identifiers)
        + f"\n    HACKLIB_VISUAL_EFFECT_COUNT = {len(effects)}\n}};\n",
        newline="\n",
    )
    preview.mkdir(parents=True, exist_ok=True)
    cards, report = [], []
    for asset in assets.values():
        frames = asset["previews"]
        enlarged = [
            frame.resize((frame.width * 3, frame.height * 3), Image.Resampling.NEAREST)
            for frame in frames
        ]
        enlarged[0].save(
            preview / (asset["key"] + ".gif"),
            save_all=True,
            append_images=enlarged[1:],
            duration=round(asset["ticks"] * 1000 / 30),
            loop=0,
            disposal=2,
        )
        strip = Image.new("RGBA", (frames[0].width * len(frames), frames[0].height))
        for i, frame in enumerate(frames):
            strip.paste(frame, (i * frame.width, 0))
        strip.save(preview / (asset["key"] + ".png"))
        report.append(
            {
                k: asset[k]
                for k in ("key", "id", "frames", "tileBytes", "romBytes", "colors")
            }
        )
        cards.append(
            f'<article><h2>{escape(asset["key"])}</h2><img src="{asset["key"]}.gif"><p>{asset["frames"]} frames; {asset["tileBytes"]}/2304 tile bytes; {asset["colors"]}/15 colors</p><a href="{asset["key"]}.png">Frame strip</a></article>'
        )
    recipes = "".join(
        f"<li>{escape(effect['name'])}: <code>{escape(json.dumps(effect['layers']))}</code></li>"
        for effect in effects
    )
    (preview / "index.html").write_text(
        '<!doctype html><meta charset="utf-8"><title>Skill effects</title><style>body{background:#182228;color:#eee;font:16px system-ui;margin:2rem}article{padding:1rem;margin:1rem 0;background:#28343b}img{image-rendering:pixelated;background:repeating-conic-gradient(#35424a 0% 25%,#46535a 0% 50%) 0/16px 16px}a{color:#9dd}</style><h1>Skill effect previews</h1><p>GBA palette colors. Animation timing approximates 30 game ticks per second. Stock shapes are previewed in-game.</p>'
        + "".join(cards)
        + "<h2>Recipes</h2><ul>"
        + recipes
        + "</ul>",
        newline="\n",
    )
    (preview / "report.json").write_text(
        json.dumps(report, indent=2) + "\n", newline="\n"
    )
    print(f"Skill FX: {len(assets)} sprite sheets, {len(effects)} effects validated")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--manifest", type=Path, default=ROOT / "hacks/content/skill_fx.json"
    )
    parser.add_argument("--output", type=Path, default=ROOT / "build/hack_include")
    parser.add_argument("--preview", type=Path, default=ROOT / "build/skill_fx_preview")
    args = parser.parse_args()
    try:
        generate(args.manifest, args.output, args.preview)
    except (ValueError, KeyError, TypeError, OSError) as error:
        raise SystemExit(f"Skill FX validation failed: {error}") from error


if __name__ == "__main__":
    main()
