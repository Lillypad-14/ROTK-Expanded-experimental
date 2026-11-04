"""Animations as data: frame lists built from any enemy clip's frames, edits of them and drawings.

hacks/content/anims/<hero slug>.json holds a hero's anims. anim_studio.py is the command line around
this file (browse clips, render and review, lint, assign poses); docs/wip-docs/animation-studio.md
is the guide. A hero's "body" pose (heroes.json) names an anim as "@name".

    {"enemy": "WitchKing",
     "anims": {
        "slash": {"speed": 1, "frames": [
            {"src": "Attack:0-3"},
            {"src": "Attack:4", "hold": 2, "rotate": -20, "pivot": [6, -40]},
            {"src": "Attack:5", "hit": true, "trail": {"steps": 2, "color": 1},
             "draw": [{"arc": [0, -36, 34], "from": -60, "to": 40, "width": 3, "color": 15}]},
            {"src": "Attack:6-8"}]}}}

Coordinates are pixels from the sprite origin (the actor's feet): x right, y down. Every frame exists
in 5 facings: 0 up (the back view, walking away) through 4 down (the front view, toward the camera);
the game mirrors 1..3 for the other side. Enemy clips and players use the same order. A frame's keys apply
to every facing unless "facings" overrides them for one.

Colors (draw, trail, remap, smear edge): a palette index 1..15 or "#rrggbb", any color. A sprite piece
that uses a "#rrggbb" color gets its own 15 colors, drawn by the PC host (rotk-recompiled
docs/HOST_PALETTES.md, piece palettes); hero_data.py tags its frame set "HPAL". Canvas pixel values:
0 empty, 1..15 the palette, 16..255 extra colors (appended to the palette list passed to build).

Frame keys (one of src, png or tween is required):
    src       "Clip:i", "Clip:i-j" (one frame per index, j < i plays backwards) or "Enemy/Clip:i"
    png       a drawing: anims/<slug>/<name>_f<facing>.png, else <name>.png for all facings.
              192x192, origin at pixel (96, 128), transparent (or palette index 0) is empty.
              Any colors (exact: palette entries where they match, extra colors otherwise).
    tween     {"from": "Clip:i", "to": "Clip:j", "t": 0.5, "dir": "short" | "cw" | "ccw", "body": "Clip:k"}:
              an in-between. The nearer frame's body (or "body"), with the first frame's blade turned
              about its hilt part of the way (t) toward the second's. Smooths big pose jumps.
    tip       [x, y] or [tipX, tipY, hiltX, hiltY]: the blade on this frame, where tracking misses it
    like      "Clip:i" whose hurtbox and hotspots a png frame takes (default: the anim's first src)
    hold      steps the frame shows (default 1): hold * speed ticks (a tick is 2 screen frames)
    patch     [{"from": "Clip:i", "rect": [x0, y0, x1, y1], "at": [dx, dy], "flip": b, "rotate": deg}]
              copy a region of another frame (same facing) onto this one: kitbash arms and weapons
    erase     [[x0, y0, x1, y1], ...] clear regions (before the moves below)
    dx, dy    move the whole frame (the same way in every facing: a side step when he faces down)
    lunge     n: move the frame n px the way the facing looks (down, diagonal, side, up): strikes
    flip      mirror left to right
    scale     nearest-neighbor scale around the pivot
    rotate    degrees clockwise around the pivot
    pivot     [x, y] for scale and rotate (default [0, 0], the feet)
    remap     {"from index": to color} palette swaps
    over      ["Clip:i" or {"src": "Clip:i", "dx": n, "dy": n, "flip": b}, ...] drawn on top
    trail     {"steps": n, "color": i or [i, ...]}: afterimages of the previous n frames under this
              one, nearest first, in the given palette colors
    draw      shapes on top (or under with "under": true), each with a "color":
              {"arc": [cx, cy, r], "from": deg, "to": deg, "width": w, "taper": t}  (0 deg right,
                90 down. taper true: thin ends, "in": thin to thick toward "to", the shape of a
                smear behind a blade, "out": the reverse, false: even)
              {"line": [x0, y0, x1, y1], "width": w}
              {"rect": [x0, y0, x1, y1]}  {"ellipse": [x0, y0, x1, y1], "fill": b}
              {"pixels": [[x, y], ...]}  {"spark": [x, y] or "tip" or "Clip:i", "size": n}  (impact flash)
              {"smear": {"from": "prev", "to": "tip", "dir": "short"}, "width": 7, "color": 5, "edge": 1}
                a swing smear between two blade tips around the hand: "prev" (the last frame), "tip"
                (this frame), a "Clip:i" or [x, y]. edge: the bright leading edge color, 0 for none.
    hit       true (the clip's hitbox), "auto" (a box around what swings: pixels new since the last
              frame, outside the hurtbox), false (none) or [xMin, xMax, yMin, yMax]. Once any
              frame of an anim sets hit, only frames that set it hit. Otherwise each frame keeps
              its source frame's own. A held frame hits on its first step only.
    hurt      [xMin, xMax, yMin, yMax]: the hurtbox (where the hero can be hit)
    hotspot   [[x, y], ...]: spawn points (projectiles, effects)
    facings   {"2": {...}}: keys that replace the frame's own in that facing (null removes one, e.g.
              {"tween": null, "src": "Attack:9"} where the source clip moves differently in that facing)

File keys: "styles" ({"name": [draw items without a shape]}: a draw item {"smear": {...}, "style": "name",
"scale": 1.5} becomes one item per layer, its shape on each, widths and sizes times scale), "enemy", "anims", "tips" ({"Clip:i": [x, y] or [x, y, hiltX, hiltY], or {"2": [...]} per
facing}: blade positions where tracking misses; anim_studio.py tips shows the tracking).

Anim keys: "frames", "speed" (frameDuration: ticks per step, default the first src
clip's), "enemy" (source enemy, default the file's), "note". Frames with only src, hold, dx, dy, hit,
hurt and hotspot keep the clip's compressed tiles. The rest are drawn and re-encoded (lz77).
"""

from __future__ import annotations

from dataclasses import dataclass, field
import json
import math
from pathlib import Path
import re
import struct
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

from rotkit.compression import bios, lz77  # noqa: E402
from rotkit.spritegfx import (  # noqa: E402
    OBJ_SIZES,
    SPRITE_FRAME_LZ77,
    SPRITE_FRAME_RL,
    TILE_BYTES,
    Frame,
    FrameSet,
    Hotspot,
    OamTemplate,
    TriggerBox,
    decode_frame_tiles,
    parse_frame_set,
)

ROM_BASE = 0x08000000
FACINGS = 5
# Facing 0 walks up, away from the camera (the back view), 4 walks down (the front view). Measured in
# game: a player's facing byte (actor+0xf8) is up 0, up-right 1, right 2, down-right 3, down 4, and the
# Witch-king's walk plays right with enemy facing N in record N.
FACING_NAMES = ("up", "up-side", "side", "down-side", "down")
CANVAS = 192
ORIGIN = (96, 128)
HURTBOX, HITBOX = 0, 1
MAX_OAM = 31
GBA_FPS = 59.7275
# The game ticks at 30 Hz (every other screen frame) and an animation step lasts frameDuration ticks.
TICK_FRAMES = 2


def screen_frames(speed: int) -> int:
    """Screen frames (60 per second) one animation step of this speed lasts."""
    return speed * TICK_FRAMES
RECORD_KEYS = {"src", "hold", "dx", "dy", "hit", "hurt", "hotspot", "tip", "lunge"}
# The way each facing looks (0 up .. 4 down, the side ones facing right): "lunge" moves along it.
FORWARD = ((0, -1), (0.7, -0.7), (1, 0), (0.7, 0.7), (0, 1))
PIXEL_KEYS = {"png", "tween", "sword", "like", "patch", "erase", "flip", "scale", "rotate", "pivot", "remap", "over", "trail",
              "draw"}
FRAME_KEYS = RECORD_KEYS | PIXEL_KEYS | {"facings"}
ANIM_KEYS = {"frames", "speed", "enemy", "note"}
TWEEN_KEYS = {"from", "to", "t", "dir", "body"}
SMEAR_KEYS = {"from", "to", "dir"}
OBJ_EDGES = (64, 32, 16, 8)
NO_BOX = TriggerBox(0, 0, 0, 0, 0, 0)


class AnimError(ValueError):
    pass


def slug(hero_name: str) -> str:
    return re.sub(r"[^A-Za-z0-9]", "", hero_name).lower()


def anims_path(hero_name: str) -> Path:
    return ROOT / "hacks/content/anims" / f"{slug(hero_name)}.json"


def load(hero_name: str) -> dict:
    path = anims_path(hero_name)
    if not path.exists():
        return {"enemy": None, "anims": {}}
    try:
        data = json.loads(path.read_text())
    except json.JSONDecodeError as error:
        raise AnimError(f"{path.name}: not valid JSON: {error}") from error
    unknown = set(data) - {"enemy", "anims", "note", "tips", "styles"}
    if unknown:
        raise AnimError(f"{path.name}: unknown keys {sorted(unknown)} (enemy, anims, tips, note)")
    data.setdefault("anims", {})
    return data


def save(hero_name: str, data: dict) -> Path:
    path = anims_path(hero_name)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(dumps(data) + "\n", encoding="utf-8", newline="\n")
    return path


def dumps(data) -> str:
    """JSON with one frame per line, so diffs and edits stay readable."""
    if isinstance(data, dict) and "frames" in data and isinstance(data["frames"], list):
        rest = {k: v for k, v in data.items() if k != "frames"}
        head = json.dumps(rest)[1:-1]
        frames = ",\n".join("      " + json.dumps(f) for f in data["frames"])
        return "{" + (head + ", " if head else "") + '"frames": [\n' + frames + "\n    ]}"
    if isinstance(data, dict) and "anims" in data:
        top = {k: v for k, v in data.items() if k != "anims"}
        anims = ",\n".join(f"    {json.dumps(name)}: {dumps(anim)}" for name, anim in data["anims"].items())
        head = json.dumps(top)[1:-1]
        return "{" + (head + ",\n " if head else "") + ' "anims": {\n' + anims + "\n  }\n}"
    return json.dumps(data)


def palette_rgb(root: Path, rom: bytes, palette_id: int) -> list[tuple[int, int, int]]:
    """ObjPalettes[palette_id] as 16 RGB colors (the palette a hero's sprites draw with)."""
    from rotkit.png import bgr555_to_rgb

    table = next(int(line.split()[0], 16) for line in (root / "config/data.cfg").read_text().splitlines()
                 if len(line.split()) > 1 and line.split()[1] == "ObjPalettes")
    pointer = struct.unpack_from("<I", rom, table - ROM_BASE + palette_id * 4)[0]
    return [bgr555_to_rgb(c) for c in struct.unpack_from("<16H", rom, pointer - ROM_BASE)]


# ---------------------------------------------------------------------------------------------
# Source frames.


@dataclass
class SourceFrame:
    """One frame of a stock clip in one facing: its record and decoded 4bpp tiles."""

    clip: str
    index: int
    frame: Frame
    frame_set: FrameSet
    tiles: bytes
    chunk: bytes  # the compressed tile stream as stored
    duration: int


class Library:
    """Reads stock enemy clips (hero_clips.Sources) frame by frame, cached."""

    def __init__(self, sources, rom: bytes):
        self.sources = sources
        self.rom = rom
        self._sets = {}
        self._frames = {}

    def clip_facings(self, enemy: str, clip: str):
        key = (enemy, clip)
        if key not in self._sets:
            try:
                symbol = self.sources.symbol(enemy, clip)
            except ValueError as error:
                raise AnimError(str(error)) from error
            self._sets[key] = [(facing, parse_frame_set(facing.frames, ROM_BASE))
                               for facing in self.sources.facings(symbol)]
        return self._sets[key]

    def clips(self, enemy: str) -> list[str]:
        prefix = f"EnemyAnimationBank_EnemyType{enemy}_"
        return sorted(n[len(prefix):] for n in self.sources.banks if n.startswith(prefix))

    def enemies(self) -> list[str]:
        return sorted({n.split("_")[1][len("EnemyType"):] for n in self.sources.banks
                       if n.startswith("EnemyAnimationBank_EnemyType")})

    def frame(self, ref: str, default_enemy: str, facing: int) -> SourceFrame:
        enemy, clip, index = parse_ref(ref, default_enemy)
        key = (enemy, clip, index, facing)
        if key in self._frames:
            return self._frames[key]
        stock, frame_set = self.clip_facings(enemy, clip)[facing]
        if not 0 <= index < frame_set.frame_count:
            raise AnimError(f"{ref}: {enemy}/{clip} has frames 0..{frame_set.frame_count - 1}")
        frame = frame_set.frames[index]
        data = self.rom[stock.tiles_address - ROM_BASE:]
        tiles = decode_frame_tiles(data, frame)
        if frame.flags & SPRITE_FRAME_LZ77:
            _, end = lz77.decode(data, frame.tile_offset)
            chunk = data[frame.tile_offset:end]
        elif frame.flags & SPRITE_FRAME_RL:
            chunk = (0x30 | (len(tiles) << 8)).to_bytes(4, "little") + bios.rl_encode(tiles)
        else:
            chunk = data[frame.tile_offset:frame.tile_offset + frame.tile_bytes]
        source = SourceFrame(f"{enemy}/{clip}", index, frame, frame_set, tiles, bytes(chunk), stock.duration)
        self._frames[key] = source
        return source

    def blade(self, ref: str, default_enemy: str, facing: int, tips: dict | None = None) -> "Blade | None":
        """The blade in a clip frame: detected, or the file's "tips" entry for it."""
        enemy, clip, index = parse_ref(ref, default_enemy)
        key = ("blade", enemy, clip, index, facing)
        if key not in self._frames:
            self._frames[key] = find_blade(self.pixels(ref, default_enemy, facing))
        found = self._frames[key]
        override = tip_override(tips, ref, default_enemy, facing)
        if override is not None:
            tip, hilt = override
            return Blade(tip, hilt or (found.hilt if found else (0, -30)), found.points if found else [], 99, 1.0)
        return found

    def pixels(self, ref: str, default_enemy: str, facing: int) -> bytearray:
        source = self.frame(ref, default_enemy, facing)
        canvas = blank()
        draw_frame(canvas, source.frame, source.tiles)
        return canvas


def parse_ref(ref: str, default_enemy: str | None) -> tuple[str, str, int]:
    match = re.fullmatch(r"(?:(\w+)/)?(\w+):(\d+)", ref or "")
    if not match:
        raise AnimError(f"{ref!r}: a frame is \"Clip:index\" or \"Enemy/Clip:index\"")
    enemy = match.group(1) or default_enemy
    if not enemy:
        raise AnimError(f"{ref}: name the enemy (\"Enemy/Clip:index\") or set the file's \"enemy\"")
    return enemy, match.group(2), int(match.group(3))


def expand(entries: list[dict], where: str) -> list[tuple[int, dict]]:
    """Frame entries with "Clip:i-j" ranges expanded, each with its entry index."""
    out = []
    for n, entry in enumerate(entries):
        if not isinstance(entry, dict):
            raise AnimError(f"{where} frame {n}: a frame is an object like {{\"src\": \"Attack:3\"}}")
        if "rig" in entry:
            out += [(n, generated) for generated in expand_rig(entry["rig"], f"{where} frame {n} rig")]
            continue
        match = re.fullmatch(r"((?:\w+/)?\w+):(\d+)-(\d+)", str(entry.get("src", "")))
        if match:
            first, last = int(match.group(2)), int(match.group(3))
            step = 1 if last >= first else -1
            out += [(n, {**entry, "src": f"{match.group(1)}:{i}"}) for i in range(first, last + step, step)]
        else:
            out.append((n, entry))
    return out


# ---------------------------------------------------------------------------------------------
# Pixels: frames as indexed 128x128 canvases with the sprite origin at ORIGIN.


MAX_COLORS = 256  # canvas pixel values: 0 empty, 1..15 the hero's palette, 16.. extra colors


def ink(value, palette_rgb: list | None, where: str) -> int:
    """A canvas color: a palette index 1..15, or "#rrggbb" (any color, PC only: the pieces that use one
    get their own palette). Extra colors are appended to palette_rgb, which must be a list."""
    if isinstance(value, int) and not isinstance(value, bool) and 1 <= value <= 15:
        return value
    if isinstance(value, str) and re.fullmatch(r"#[0-9a-fA-F]{6}", value):
        if palette_rgb is None:
            raise AnimError(f"{where}: no palette to add {value} to")
        return extra_color(palette_rgb, tuple(int(value[i:i + 2], 16) for i in (1, 3, 5)))
    raise AnimError(f"{where}: a color is a palette index 1..15 (anim_studio.py palette) or \"#rrggbb\"")


def extra_color(palette_rgb: list, rgb: tuple[int, int, int]) -> int:
    """The canvas value of an exact color: a base palette entry or an extra color (added if new)."""
    rgb = tuple(c & 0xF8 for c in rgb)  # what the game can show (RGB555)
    for n in range(1, len(palette_rgb)):
        if tuple(c & 0xF8 for c in palette_rgb[n]) == rgb:
            return n
    if len(palette_rgb) >= MAX_COLORS:
        return min(range(1, MAX_COLORS), key=lambda n: sum((p - q) ** 2 for p, q in zip(palette_rgb[n], rgb)))
    palette_rgb.append(rgb)
    return len(palette_rgb) - 1


def blank() -> bytearray:
    return bytearray(CANVAS * CANVAS)


def draw_frame(canvas: bytearray, frame: Frame, tiles: bytes, piece_ids: list | None = None) -> int:
    """Blit a frame's OBJs onto the canvas (piece_ids: each OBJ's own colors, see encode_pixels).
    Returns how many pixels fell off it."""
    clipped = 0
    for k, obj in enumerate(frame.oam):
        own = piece_ids[k] if piece_ids and k < len(piece_ids) else None
        ow, oh = obj.size
        tile = obj.tile_offset
        for ty in range(oh // 8):
            for tx in range(ow // 8):
                src = tile * TILE_BYTES
                tile += 1
                if src + TILE_BYTES > len(tiles):
                    continue
                for row in range(8):
                    y = ORIGIN[1] + obj.y + ty * 8 + row
                    for col in range(8):
                        byte = tiles[src + row * 4 + col // 2]
                        value = byte >> 4 if col & 1 else byte & 0xF
                        if not value:
                            continue
                        x = ORIGIN[0] + obj.x + tx * 8 + col
                        if 0 <= x < CANVAS and 0 <= y < CANVAS:
                            canvas[y * CANVAS + x] = own[value - 1] if own else value
                        else:
                            clipped += 1
    return clipped


def at(x: float, y: float) -> int | None:
    """The canvas index of an origin-relative point, None off the canvas."""
    cx, cy = int(math.floor(x)) + ORIGIN[0], int(math.floor(y)) + ORIGIN[1]
    return cy * CANVAS + cx if 0 <= cx < CANVAS and 0 <= cy < CANVAS else None


def transform(canvas: bytes, dx: float = 0, dy: float = 0, flip: bool = False, scale: float = 1,
              rotate: float = 0, pivot=(0, 0), remap: dict[int, int] | None = None) -> bytearray:
    """Move, mirror, scale and rotate a canvas (nearest neighbor, inverse mapped)."""
    if not (dx or dy or flip or scale != 1 or rotate or remap):
        return bytearray(canvas)
    remap = remap or {}
    out = blank()
    px, py = pivot
    angle = math.radians(rotate)
    cos, sin = math.cos(angle), math.sin(angle)
    ox, oy = ORIGIN
    for i, _ in enumerate(out):
        # The output pixel's center, origin-relative, mapped back to the source.
        x = i % CANVAS - ox + 0.5 - dx
        y = i // CANVAS - oy + 0.5 - dy
        x, y = x - px, y - py
        x, y = x * cos + y * sin, -x * sin + y * cos
        x, y = x / scale + px, y / scale + py
        if flip:
            x = -x
        index = at(x, y)
        if index is not None and canvas[index]:
            value = canvas[index]
            out[i] = remap.get(value, value)
    return out


def overlay(canvas: bytearray, layer: bytes, color: int | None = None, under: bool = False) -> None:
    for i, value in enumerate(layer):
        if value and (not under or not canvas[i]):
            canvas[i] = color if color is not None else value


def rect_mask(canvas: bytes, rect) -> bytearray:
    x0, y0, x1, y1 = rect
    out = blank()
    for y in range(min(y0, y1), max(y0, y1)):
        for x in range(min(x0, x1), max(x0, x1)):
            index = at(x, y)
            if index is not None:
                out[index] = canvas[index]
    return out


def erase(canvas: bytearray, rect) -> None:
    x0, y0, x1, y1 = rect
    for y in range(min(y0, y1), max(y0, y1)):
        for x in range(min(x0, x1), max(x0, x1)):
            index = at(x, y)
            if index is not None:
                canvas[index] = 0


def plot(layer: bytearray, x: float, y: float, color: int, width: float) -> None:
    radius = max(0.0, (width - 1) / 2)
    r = int(math.ceil(radius))
    for oy in range(-r, r + 1):
        for ox in range(-r, r + 1):
            if ox * ox + oy * oy <= radius * radius + 0.25:
                index = at(x + ox, y + oy)
                if index is not None:
                    layer[index] = color


def draw_shape(shape: dict, where: str, context: dict | None = None) -> bytearray:
    """One "draw" item on its own layer. context resolves "tip" and the smear's blade positions."""
    context = context or {}
    keys = set(shape) - {"color", "width", "under", "from", "to", "fill", "taper", "size", "edge", "dir"}
    kinds = keys & {"arc", "line", "rect", "ellipse", "pixels", "spark", "smear"}
    if len(kinds) != 1 or keys - kinds:
        raise AnimError(f"{where}: a draw item is one of arc, line, rect, ellipse, pixels, spark, smear "
                        f"(plus color, width, from, to, taper, fill, size, edge, dir, under), got {sorted(shape)}")
    color = ink(shape.get("color"), context.get("palette"), where)
    width = shape.get("width", 1)
    layer = blank()
    kind = kinds.pop()
    value = shape[kind]
    if kind == "arc":
        cx, cy, radius = value
        start, end = shape.get("from", 0), shape.get("to", 90)
        steps = max(8, int(abs(end - start) * radius / 20))
        for k in range(steps + 1):
            a = math.radians(start + (end - start) * k / steps)
            mode, f = shape.get("taper", True), k / steps
            # true: thickest mid-sweep. "in": thin to thick (a smear led by the blade). "out": the reverse.
            taper = {True: math.sin(math.pi * f), "in": f ** 0.7, "out": (1 - f) ** 0.7}.get(mode, 1)
            plot(layer, cx + radius * math.cos(a), cy + radius * math.sin(a), color, max(1, width * taper))
    elif kind == "line":
        x0, y0, x1, y1 = value
        steps = max(1, int(max(abs(x1 - x0), abs(y1 - y0))))
        for k in range(steps + 1):
            plot(layer, x0 + (x1 - x0) * k / steps, y0 + (y1 - y0) * k / steps, color, width)
    elif kind == "smear":
        smear(layer, value if isinstance(value, dict) else {}, shape, where, context)
    elif kind == "spark":
        # An impact flash: 8 rays, the diagonals shorter, with a gap at the center.
        cx, cy = resolve_point(value, context, where)
        size = shape.get("size", 6)
        for k in range(8):
            a = math.radians(k * 45 + 22.5 * (k % 2 == 0))
            reach = size if k % 2 == 0 else size * 0.6
            for r in range(2, int(reach) + 1):
                plot(layer, cx + r * math.cos(a), cy + r * math.sin(a), color, width)
    elif kind == "rect":
        x0, y0, x1, y1 = value
        for y in range(min(y0, y1), max(y0, y1)):
            for x in range(min(x0, x1), max(x0, x1)):
                index = at(x, y)
                if index is not None:
                    layer[index] = color
    elif kind == "ellipse":
        x0, y0, x1, y1 = value
        cx, cy, rx, ry = (x0 + x1) / 2, (y0 + y1) / 2, abs(x1 - x0) / 2, abs(y1 - y0) / 2
        for y in range(int(min(y0, y1)), int(max(y0, y1)) + 1):
            for x in range(int(min(x0, x1)), int(max(x0, x1)) + 1):
                d = ((x + 0.5 - cx) / max(rx, 0.5)) ** 2 + ((y + 0.5 - cy) / max(ry, 0.5)) ** 2
                if d <= 1 and (shape.get("fill", True) or d >= (1 - 2 / max(rx, ry, 1))):
                    index = at(x, y)
                    if index is not None:
                        layer[index] = color
    else:
        for x, y in value:
            index = at(x, y)
            if index is not None:
                layer[index] = color
    return layer


def read_png(path: Path, palette_rgb: list[tuple[int, int, int]]) -> tuple[bytearray, int]:
    """A PNG drawing on the canvas in its own colors (palette entries where they match, extra colors
    otherwise). Returns pixels and how many opaque pixels had to be snapped (past MAX_COLORS)."""
    from PIL import Image

    image = Image.open(path)
    if image.size != (CANVAS, CANVAS):
        raise AnimError(f"{path.name}: drawings are {CANVAS}x{CANVAS} with the origin at pixel {ORIGIN}")
    rgba = image.convert("RGBA")
    pixels = blank()
    inexact = 0
    cache = {}
    for i, (r, g, b, a) in enumerate(rgba.getdata()):
        if a < 128:
            continue
        key = (r, g, b)
        if key not in cache:
            best = extra_color(palette_rgb, key)
            cache[key] = (best, tuple(c & 0xF8 for c in palette_rgb[best]) != tuple(c & 0xF8 for c in key))
        index, off = cache[key]
        pixels[i] = index
        inexact += off
    return pixels, inexact


def image(pixels: bytes, palette_rgb: list[tuple[int, int, int]], scale: int = 1):
    from PIL import Image

    out = Image.frombytes("P", (CANVAS, CANVAS), bytes(pixels))
    out.putpalette([c for rgb in palette_rgb for c in rgb])
    out.info["transparency"] = 0
    out = out.convert("RGBA")
    return out.resize((CANVAS * scale, CANVAS * scale), Image.NEAREST) if scale != 1 else out


# ---------------------------------------------------------------------------------------------
# Encoding: pixels to OBJs, tiles and a frame record.


def split_edge(length: int) -> list[int]:
    parts = []
    while length > 0:
        part = next(e for e in OBJ_EDGES if e <= length) if length >= 8 else 8
        parts.append(part)
        length -= part
    return parts


def piece_colors(values: list[int], palette_rgb) -> tuple[dict[int, int], tuple[int, ...] | None]:
    """A piece's canvas values to 4bpp indexes. Palette-only pieces keep theirs (None). A piece with
    extra colors gets its own 15: the most used, the rest snapped to the nearest of them."""
    used = [v for v in values if v]
    if not used or max(used) <= 15:
        return {v: v for v in set(values)}, None
    counts: dict[int, int] = {}
    for v in used:
        counts[v] = counts.get(v, 0) + 1
    kept = sorted(counts, key=lambda v: -counts[v])[:15]
    mapping = {0: 0}
    for v in counts:
        near = v if v in kept else min(kept, key=lambda k: sum((p - q) ** 2
                                                               for p, q in zip(palette_rgb[k], palette_rgb[v])))
        mapping[v] = kept.index(near) + 1
    return mapping, tuple(kept)


def encode_pixels(pixels: bytes, palette_rgb=None, piece_ids: list | None = None
                  ) -> tuple[list[OamTemplate], bytes, int, int]:
    """The OBJs covering the drawn pixels (empty ones dropped), their 4bpp tiles, and the cell size.
    piece_ids, when given, receives each OBJ's own colors (canvas values for indexes 1..n) or None."""
    xs = [i % CANVAS for i, v in enumerate(pixels) if v]
    ys = [i // CANVAS for i, v in enumerate(pixels) if v]
    if not xs:
        return [OamTemplate(-4, -8, 0, 0, 0)], bytes(TILE_BYTES), 8, 8
    left, top = min(xs) & ~7, min(ys) & ~7
    right, bottom = max(xs) + 1, max(ys) + 1
    width, height = (right - left + 7) & ~7, (bottom - top + 7) & ~7
    oam, tiles = [], bytearray()

    def blocks(x: int, y: int, w: int, h: int):
        """OBJ-sized blocks covering w x h: an OBJ is square or 2:1, so halve the long side."""
        if (w, h) in OBJ_SIZES.values():
            yield x, y, w, h
        elif w > h:
            yield from blocks(x, y, w // 2, h)
            yield from blocks(x + w // 2, y, w // 2, h)
        else:
            yield from blocks(x, y, w, h // 2)
            yield from blocks(x, y + h // 2, w, h // 2)

    oy = 0
    for row_height in split_edge(height):
        ox = 0
        for column_width in split_edge(width):
            for bx, by, pw, ph in blocks(ox, oy, column_width, row_height):
                values = [pixels[(top + by + y) * CANVAS + left + bx + x]
                          if left + bx + x < CANVAS and top + by + y < CANVAS else 0
                          for y in range(ph) for x in range(pw)]
                mapping, own = piece_colors(values, palette_rgb)
                block = bytearray()
                for ty in range(ph // 8):
                    for tx in range(pw // 8):
                        for row in range(8):
                            y = top + by + ty * 8 + row
                            for col in range(0, 8, 2):
                                x = left + bx + tx * 8 + col
                                lo = mapping[pixels[y * CANVAS + x]] if x < CANVAS and y < CANVAS else 0
                                hi = mapping[pixels[y * CANVAS + x + 1]] if x + 1 < CANVAS and y < CANVAS else 0
                                block.append(lo | (hi << 4))
                if any(block):
                    shape, size = next(k for k, v in OBJ_SIZES.items() if v == (pw, ph))
                    oam.append(OamTemplate(left + bx - ORIGIN[0], top + by - ORIGIN[1], size, shape,
                                           len(tiles) // TILE_BYTES))
                    tiles += block
                    if piece_ids is not None:
                        piece_ids.append(own)
            ox += column_width
        oy += row_height
    if len(oam) > MAX_OAM:
        raise AnimError(f"the frame needs {len(oam)} OBJs, the most a frame holds is {MAX_OAM}")
    return oam, bytes(tiles), width, height


def pack_frame(flags: int, pad: int, width: int, height: int, tile_offset: int, tile_bytes: int,
               hotspots, boxes, oam) -> bytes:
    data = bytearray(struct.pack("<BBBBHH", (flags & 0xE0) | len(oam), pad, width & 0xFF, height & 0xFF,
                                 tile_offset, tile_bytes))
    for spot in hotspots:
        data += struct.pack("<bb", spot.x, spot.y)
    for box in boxes:
        data += struct.pack("<bbbbBB", box.x_min, box.x_max, box.y_min, box.y_max, box.enable, box.pad)
    for obj in oam:
        data += struct.pack("<I", obj.pack())
    return bytes(data)


def clamp8(value: float) -> int:
    return max(-128, min(127, int(round(value))))


def move_point(x: float, y: float, entry: dict) -> tuple[float, float]:
    """Where an origin-relative point of the source lands after the frame's moves."""
    px, py = entry.get("pivot", (0, 0))
    if entry.get("flip"):
        x = -x
    scale, angle = entry.get("scale", 1), math.radians(entry.get("rotate", 0))
    x, y = (x - px) * scale, (y - py) * scale
    x, y = x * math.cos(angle) - y * math.sin(angle), x * math.sin(angle) + y * math.cos(angle)
    return x + px + entry.get("dx", 0), y + py + entry.get("dy", 0)


def move_box(box: TriggerBox, entry: dict) -> TriggerBox:
    corners = [move_point(x, y, entry) for x in (box.x_min, box.x_max) for y in (box.y_min, box.y_max)]
    xs, ys = [c[0] for c in corners], [c[1] for c in corners]
    return TriggerBox(clamp8(min(xs)), clamp8(max(xs)), clamp8(min(ys)), clamp8(max(ys)), box.enable, box.pad)


# ---------------------------------------------------------------------------------------------
# Building one animation.


@dataclass
class Step:
    """One displayed step of a built animation."""

    entry: int  # index in the anim's "frames" list
    frame: int  # index among the anim's frames once "Clip:i-j" ranges are expanded
    source: str  # "Clip:i" or "png name"
    first: bool  # the first step of a held frame
    hit: tuple[int, int, int, int] | None
    hurt: tuple[int, int, int, int] | None
    hotspots: list[tuple[int, int]]
    tip: tuple[int, int] | None = None  # the blade tip (tracked or set), origin-relative
    hilt: tuple[int, int] | None = None


@dataclass
class Built:
    """One facing of a built animation."""

    frames: bytes  # SpriteFrameSetHeader, offsets and records
    tiles: bytes  # the tile data the records point into
    duration: int
    steps: list[Step] = field(default_factory=list)
    canvases: list[bytes] = field(default_factory=list)  # each step's pixels
    # Per record, each OBJ's own colors (canvas values for its indexes 1..n), or None for the hero's
    # palette. The PC host draws them (rotk-recompiled docs/HOST_PALETTES.md, piece palettes).
    piece_ids: list[list] = field(default_factory=list)
    palette: list = field(default_factory=list)  # the canvas colors: the hero's 16 and the extras

    def piece_palettes(self) -> list[list[bytes | None]] | None:
        """Per record, each OBJ's 16 colors as BGR555 (or None), or None without extra colors."""
        if not any(own for record in self.piece_ids for own in record):
            return None

        def bgr555(rgb):
            return (rgb[0] >> 3) | ((rgb[1] >> 3) << 5) | ((rgb[2] >> 3) << 10)

        return [[struct.pack("<16H", 0, *[bgr555(self.palette[v]) for v in own], *[0] * (15 - len(own)))
                 if own else None for own in record] for record in self.piece_ids]

    @property
    def hits(self) -> list[int]:
        return [i for i, s in enumerate(self.steps) if s.hit]


def check_entry(entry: dict, where: str) -> None:
    unknown = set(entry) - FRAME_KEYS
    if unknown:
        raise AnimError(f"{where}: unknown keys {sorted(unknown)}. Frame keys: {', '.join(sorted(FRAME_KEYS))}")
    if sum(k in entry for k in ("src", "png", "tween")) != 1:
        raise AnimError(f"{where}: needs exactly one of src (a clip frame), png (a drawing) or tween (an in-between)")
    tween = entry.get("tween")
    if tween is not None and (not isinstance(tween, dict) or set(tween) - TWEEN_KEYS or "from" not in tween
                              or "to" not in tween or not 0 < tween.get("t", 0.5) < 1
                              or tween.get("dir", "short") not in ("short", "cw", "ccw")):
        raise AnimError(f"{where}: tween is {{\"from\": \"Clip:i\", \"to\": \"Clip:j\", \"t\": 0..1, "
                        "\"dir\": \"short\" | \"cw\" | \"ccw\"}")
    if "tip" in entry and not (isinstance(entry["tip"], list) and len(entry["tip"]) in (2, 4)):
        raise AnimError(f"{where}: tip is [x, y] or [tipX, tipY, hiltX, hiltY]")
    hold = entry.get("hold", 1)
    if not isinstance(hold, int) or not 1 <= hold <= 60:
        raise AnimError(f"{where}: hold must be 1..60 steps")
    for key in ("hit", "hurt"):
        value = entry.get(key)
        if value is not None and not (key == "hit" and (isinstance(value, bool) or value == "auto")) and not (
                isinstance(value, list) and len(value) == 4 and value[0] <= value[1] and value[2] <= value[3]):
            raise AnimError(f"{where}: {key} is {'true, false, \"auto\" or ' if key == 'hit' else ''}[xMin, xMax, yMin, yMax]")
    if not 0.25 <= entry.get("scale", 1) <= 4:
        raise AnimError(f"{where}: scale must be 0.25..4")
    trail = entry.get("trail")
    if trail is not None and (not isinstance(trail, dict) or not 1 <= trail.get("steps", 0) <= 6
                              or "color" not in trail):
        raise AnimError(f"{where}: trail is {{\"steps\": 1..6, \"color\": index or [indexes]}}")
    if "facings" in entry:
        if not isinstance(entry["facings"], dict) or not set(entry["facings"]) <= {str(f) for f in range(FACINGS)}:
            raise AnimError(f"{where}: facings is {{\"0\".. \"4\": {{keys for that facing}}}}")


def build(lib: Library, hero_name: str, anim_name: str, anims: dict, palette_rgb, facing: int,
          warnings: list[str] | None = None, compress: bool = True) -> Built:
    """One facing of an animation: its frame set and tile data. compress False skips lz77 (previews)."""
    warnings = warnings if warnings is not None else []
    if anim_name not in anims["anims"]:
        known = ", ".join(sorted(anims["anims"])) or "none yet"
        raise AnimError(f"{hero_name}: no anim {anim_name!r} in {anims_path(hero_name).name} (has {known})")
    anim = anims["anims"][anim_name]
    where = f"@{anim_name}"
    if set(anim) - ANIM_KEYS:
        raise AnimError(f"{where}: unknown keys {sorted(set(anim) - ANIM_KEYS)} ({', '.join(sorted(ANIM_KEYS))})")
    enemy = anim.get("enemy", anims.get("enemy"))
    if not anim.get("frames"):
        raise AnimError(f"{where}: no frames")
    entries = []
    for n, entry in expand(anim["frames"], where):
        check_entry(entry, f"{where} frame {n}")
        override = entry.get("facings", {}).get(str(facing), {})
        merged = {k: v for k, v in {**entry, **override}.items() if k != "facings" and v is not None}
        if "lunge" in merged:
            fx, fy = FORWARD[facing]
            lunge = merged.pop("lunge")
            merged["dx"] = merged.get("dx", 0) + round(fx * lunge)
            merged["dy"] = merged.get("dy", 0) + round(fy * lunge)
        check_entry(merged, f"{where} frame {n} facing {facing}")
        entries.append((n, merged))

    base_ref = next((e["src"] if "src" in e else tween_near(e) for _, e in entries if "src" in e or "tween" in e),
                    None) or next((e["like"] for _, e in entries if "like" in e), None)
    if base_ref is None:
        raise AnimError(f"{where}: an anim of drawings only needs \"like\" on a frame (whose boxes it takes)")
    try:
        base = lib.frame(base_ref, enemy, facing)
    except AnimError as error:
        raise AnimError(f"{where}: {error}") from None
    speed = anim.get("speed", base.duration)
    if not isinstance(speed, int) or not 0 < speed < 32:
        raise AnimError(f"{where}: speed must be 1..31 (0 freezes the pose)")

    # Record layout: the most hotspots and boxes any source clip of the anim has.
    sources = []
    for n, e in entries:
        try:
            ref = e["src"] if "src" in e else tween_near(e) if "tween" in e else e.get("like", base_ref)
            sources.append(lib.frame(ref, enemy, facing))
        except AnimError as error:
            raise AnimError(f"{where} frame {n}: {error}") from None
    hotspot_count = max([s.frame_set.hotspot_count for s in sources] +
                        [len(e.get("hotspot", [])) for _, e in entries])
    box_count = max(s.frame_set.trigger_box_count for s in sources)
    if any(e.get("hit") not in (None, False) for _, e in entries):
        box_count = max(box_count, HITBOX + 1)
    if any("hurt" in e for _, e in entries):
        box_count = max(box_count, HURTBOX + 1)
    base_hurt = next((s.frame.boxes[HURTBOX] for s in sources if s.frame.boxes), None)

    # Hits are explicit once any frame names one: the clip's own hit frames would add stray hits.
    explicit_hits = any("hit" in e for _, e in entries)

    def records_of(source: SourceFrame, entry: dict):
        hotspots = [Hotspot(*map(clamp8, move_point(h.x, h.y, entry))) for h in source.frame.hotspots]
        if "hotspot" in entry:
            hotspots = [Hotspot(clamp8(x), clamp8(y)) for x, y in entry["hotspot"]]
        hotspots += [Hotspot(0, 0)] * (hotspot_count - len(hotspots))
        boxes = list(source.frame.boxes)
        if not boxes and box_count and base_hurt is not None:
            boxes = [base_hurt]
        boxes = [move_box(b, entry) if b.enable or i == HURTBOX else b for i, b in enumerate(boxes)]
        boxes += [NO_BOX] * (box_count - len(boxes))
        if "hurt" in entry:
            boxes[HURTBOX] = TriggerBox(*[clamp8(v) for v in entry["hurt"]], 1, 0)
        hit = entry.get("hit", False if explicit_hits else None)
        if box_count > HITBOX and hit is not None and hit != "auto":
            if hit is False:
                boxes[HITBOX] = NO_BOX
            elif hit is True:
                if not boxes[HITBOX].enable:
                    donor = next((f.boxes[HITBOX] for f in source.frame_set.frames
                                  if len(f.boxes) > HITBOX and f.boxes[HITBOX].enable), None)
                    if donor is None:
                        raise AnimError(f"{where}: {source.clip} has no hitbox to copy, give hit a box "
                                        "[xMin, xMax, yMin, yMax]")
                    boxes[HITBOX] = move_box(donor, entry)
            else:
                boxes[HITBOX] = TriggerBox(*[clamp8(v) for v in hit], 1, 0)
        return hotspots, boxes

    tips = anims.get("tips")
    made: dict[int, Blade] = {}  # the blades tweens placed, by frame entry

    def base_pixels(n: int, entry: dict, source: SourceFrame) -> bytearray:
        if "tween" in entry:
            try:
                pixels, made[n] = tween_pixels(lib, entry["tween"], enemy, facing, tips, f"{where} frame {n}")
            except AnimError as error:
                # No blade to turn in this facing: the in-between shows its body frame as it is.
                warnings.append(f"{error} Facing {facing} shows {tween_near(entry)} as it is.")
                return lib.pixels(tween_near(entry), enemy, facing)
            return pixels
        if "sword" in entry and "png" in entry:
            pixels, inexact = read_png(png_path(hero_name, entry["png"], facing), palette_rgb)
            sword = dict(entry["sword"])
            if "grip" not in sword:
                mark = hand_mark(hero_name, entry["png"], facing)
                if mark is not None:
                    sword["grip"] = mark
            pixels, made[n] = draw_sword(lib, pixels, sword, enemy, facing, tips, f"{where} frame {n}")
            return pixels
        if "sword" in entry:
            pixels, made[n] = sword_pixels(lib, entry["src"], entry["sword"], enemy, facing, tips,
                                           f"{where} frame {n}")
            return pixels
        if "png" in entry:
            pixels, inexact = read_png(png_path(hero_name, entry["png"], facing), palette_rgb)
            if inexact:
                warnings.append(f"{where} frame {n} facing {facing}: {inexact} pixels snapped to the palette")
            return pixels
        pixels = blank()
        clipped = draw_frame(pixels, source.frame, source.tiles)
        if clipped:
            warnings.append(f"{where} frame {n}: {clipped} pixels fall off the {CANVAS}x{CANVAS} canvas")
        return pixels

    tiles = bytearray()
    records, steps, canvases, finals, plains, blades, record_ids = [], [], [], [], [], [], []
    for k, ((n, entry), source) in enumerate(zip(entries, sources)):
        at_where = f"{where} frame {n}"
        hotspots, boxes = records_of(source, entry)
        if set(entry) <= RECORD_KEYS:
            dx, dy = entry.get("dx", 0), entry.get("dy", 0)
            oam = [OamTemplate(o.x + dx, o.y + dy, o.obj_size, o.obj_shape, o.tile_offset) for o in source.frame.oam]
            chunk, flags, tile_bytes = source.chunk, source.frame.flags, source.frame.tile_bytes
            width, height = source.frame.width, source.frame.height
            pixels = transform(base_pixels(n, entry, source), dx, dy)
            plain = pixels
            own_ids = [None] * len(oam)
            here = blade_of(plain, entry, made.get(n))
        else:
            pixels = base_pixels(n, entry, source)
            for i, patch in enumerate(entry.get("patch", [])):
                if not isinstance(patch, dict) or set(patch) - {"from", "rect", "at", "flip", "rotate"} or \
                        "from" not in patch or len(patch.get("rect", [])) != 4:
                    raise AnimError(f"{at_where} patch {i}: {{\"from\": \"Clip:i\", \"rect\": [x0, y0, x1, y1], "
                                    "\"at\": [dx, dy], \"flip\": b, \"rotate\": deg}")
                x0, y0, x1, y1 = patch["rect"]
                piece = rect_mask(lib.pixels(patch["from"], enemy, facing), patch["rect"])
                mx, my = patch.get("at", (0, 0))
                piece = transform(piece, mx, my, patch.get("flip", False), 1, patch.get("rotate", 0),
                                  ((x0 + x1) / 2, (y0 + y1) / 2))
                overlay(pixels, piece)
            for rect in entry.get("erase", []):
                if len(rect) != 4:
                    raise AnimError(f"{at_where}: erase takes [x0, y0, x1, y1] rectangles")
                erase(pixels, rect)
            pixels = transform(pixels, entry.get("dx", 0), entry.get("dy", 0), entry.get("flip", False),
                               entry.get("scale", 1), entry.get("rotate", 0), entry.get("pivot", (0, 0)),
                               {int(a): ink(b, palette_rgb, f"{at_where} remap")
                                for a, b in entry.get("remap", {}).items()})
            for item in entry.get("over", []):
                item = {"src": item} if isinstance(item, str) else item
                layer = lib.pixels(item["src"], enemy, facing)
                overlay(pixels, transform(layer, item.get("dx", 0), item.get("dy", 0), item.get("flip", False)))
            plain = bytearray(pixels)
            trail = entry.get("trail")
            if trail:
                colors = trail["color"] if isinstance(trail["color"], list) else [trail["color"]]
                colors = [ink(c, palette_rgb, f"{at_where} trail") for c in colors]
                for back in range(1, trail["steps"] + 1):
                    if k - back >= 0:
                        overlay(pixels, finals[k - back], colors[min(back - 1, len(colors) - 1)], under=True)
            # Tracked before the draws: a smear is long and thin too.
            here = blade_of(plain, entry, made.get(n))
            context = {"tip": here, "prev": blades[-1] if blades else None, "lib": lib,
                       "enemy": enemy, "facing": facing, "tips": tips, "palette": palette_rgb}
            for i, shape in enumerate(styled(entry.get("draw", []), anims.get("styles") or {}, at_where)):
                try:
                    layer = draw_shape(shape, f"{at_where} draw {i}", context)
                except AnimError as error:
                    # A blade hidden in this facing (behind the body): skip the smear or spark here.
                    if "no blade found" not in str(error):
                        raise
                    warnings.append(f"{error} Facing {facing} skips it.")
                    continue
                overlay(pixels, layer, under=shape.get("under", False))
                overlay(plain, layer, under=shape.get("under", False))
            own_ids: list = []
            oam, raw, width, height = encode_pixels(pixels, palette_rgb, own_ids)
            if compress:
                chunk, flags = lz77.encode(raw), SPRITE_FRAME_LZ77
            else:
                chunk, flags = raw, 0
            tile_bytes = len(raw)
        if entry.get("hit") == "auto":
            box = auto_hitbox(plain, plains[-1] if plains else None, boxes[HURTBOX] if boxes else None)
            if box is None:
                raise AnimError(f"{where} frame {n} facing {facing}: hit \"auto\" found no new pixels outside the "
                                "hurtbox (nothing swings here): give hit a box [xMin, xMax, yMin, yMax]")
            boxes[HITBOX] = box
        finals.append(bytes(pixels))
        plains.append(bytes(plain))
        blades.append(here)
        offset = len(tiles)
        tiles += chunk + bytes(-len(chunk) % 4)
        if compress and len(tiles) > 0xFFFF:
            raise AnimError(f"{where} facing {facing}: tile data passes 64 KB at frame {n}, split the anim")
        if "src" in entry:
            label = entry["src"]
        elif "png" in entry:
            label = f"png {entry['png']}"
        else:
            label = f"tween {entry['tween']['from']}>{entry['tween']['to']} {entry['tween'].get('t', 0.5)}"
        found = blades[-1]
        for step in range(entry.get("hold", 1)):
            step_boxes = list(boxes)
            if step and box_count > HITBOX and boxes[HITBOX].enable:
                step_boxes[HITBOX] = NO_BOX
            hit = step_boxes[HITBOX] if box_count > HITBOX and step_boxes[HITBOX].enable else None
            hurt = step_boxes[HURTBOX] if box_count > HURTBOX and step_boxes[HURTBOX].enable else None
            steps.append(Step(n, k, label, step == 0, hit and (hit.x_min, hit.x_max, hit.y_min, hit.y_max),
                              hurt and (hurt.x_min, hurt.x_max, hurt.y_min, hurt.y_max),
                              [(h.x, h.y) for h in hotspots], found and found.tip, found and found.hilt))
            canvases.append(finals[-1])
            records.append((pack_frame(flags, source.frame.field_0x1, width, height, offset, tile_bytes,
                                       hotspots, step_boxes, oam), tile_bytes))
            record_ids.append(own_ids)

    head = base.frame_set
    box = head.movement_collision_box
    data = bytearray(struct.pack("<BB4bHHBB", head.width, head.height, box.x_min, box.x_max, box.y_min, box.y_max,
                                 len(records), max(size for _, size in records), hotspot_count, box_count))
    offset = 2 * len(records)
    for record, _ in records:
        data += struct.pack("<H", offset)
        offset += len(record)
    for record, _ in records:
        data += record
    return Built(bytes(data), bytes(tiles), speed, steps, canvases, record_ids, palette_rgb)


def auto_hitbox(pixels: bytes, previous: bytes | None, hurt: TriggerBox | None) -> TriggerBox | None:
    """A hitbox around what swings in this frame: its pixels that the previous frame lacks (the blade,
    a drawn smear) outside the hurtbox, padded by 2."""
    near = bytearray(len(pixels))
    if previous is not None:
        # Within 2 px of the previous frame's pixels is the body shifting, not a swing.
        for i, value in enumerate(previous):
            if value:
                x, y = i % CANVAS, i // CANVAS
                for oy in range(max(0, y - 2), min(CANVAS, y + 3)):
                    near[oy * CANVAS + max(0, x - 2):oy * CANVAS + min(CANVAS, x + 3)] = b"" * (min(CANVAS, x + 3) - max(0, x - 2))
    xs, ys = [], []
    for i, value in enumerate(pixels):
        if not value or near[i]:
            continue
        x, y = i % CANVAS - ORIGIN[0], i // CANVAS - ORIGIN[1]
        if hurt is not None and hurt.enable and hurt.x_min <= x <= hurt.x_max and hurt.y_min <= y <= hurt.y_max:
            continue
        xs.append(x)
        ys.append(y)
    if len(xs) < 12:
        return None
    # Trim stray pixels: the central 90 percent each way.
    xs.sort()
    ys.sort()
    cut = len(xs) // 20
    x0, x1, y0, y1 = xs[cut], xs[-1 - cut], ys[cut], ys[-1 - cut]
    return TriggerBox(clamp8(x0 - 2), clamp8(x1 + 2), clamp8(y0 - 2), clamp8(y1 + 2), 1, 0)


# ---------------------------------------------------------------------------------------------
# Blades: tracked in frames, so smears, sparks and tweens find the sword without coordinates.


@dataclass
class Blade:
    """A straight, thin part of a frame: the weapon. Origin-relative."""

    tip: tuple[int, int]  # the end farther from the body
    hilt: tuple[int, int]
    points: list[tuple[int, int]]
    length: float
    confidence: float  # 0..1: how blade-like the best candidate was


def find_blade(pixels: bytes) -> Blade | None:
    """The longest straight thin structure: thick parts (body, cape) are removed with a 3x3 opening,
    the rest is grouped (gaps up to 2 px) and each group scored by length over width."""
    filled = {(i % CANVAS, i // CANVAS) for i, v in enumerate(pixels) if v}
    if not filled:
        return None
    around = [(dx, dy) for dx in (-1, 0, 1) for dy in (-1, 0, 1) if dx or dy]
    eroded = {p for p in filled if all((p[0] + dx, p[1] + dy) in filled for dx, dy in around)}
    opened = set(eroded)
    for x, y in eroded:
        opened.update((x + dx, y + dy) for dx, dy in around)
    thin = filled - opened
    cx = sum(x for x, _ in filled) / len(filled)
    cy = sum(y for _, y in filled) / len(filled)
    seen, best = set(), None
    for start in thin:
        if start in seen:
            continue
        stack, group = [start], []
        seen.add(start)
        while stack:
            x, y = stack.pop()
            group.append((x, y))
            for dx in range(-2, 3):
                for dy in range(-2, 3):
                    q = (x + dx, y + dy)
                    if q in thin and q not in seen:
                        seen.add(q)
                        stack.append(q)
        if len(group) < 6:
            continue
        mx = sum(x for x, _ in group) / len(group)
        my = sum(y for _, y in group) / len(group)
        sxx = sum((x - mx) ** 2 for x, _ in group) / len(group)
        syy = sum((y - my) ** 2 for _, y in group) / len(group)
        sxy = sum((x - mx) * (y - my) for x, y in group) / len(group)
        half, root = (sxx + syy) / 2, math.sqrt(max(0.0, ((sxx - syy) / 2) ** 2 + sxy * sxy))
        length, width = math.sqrt(12 * (half + root)), math.sqrt(12 * max(0.0, half - root))
        score = length / (1 + width)
        if best is None or score > best[0]:
            tip = max(group, key=lambda p: (p[0] - cx) ** 2 + (p[1] - cy) ** 2)
            hilt = max(group, key=lambda p: (p[0] - tip[0]) ** 2 + (p[1] - tip[1]) ** 2)
            best = (score, tip, hilt, group, length)
    if best is None or best[4] < 8:
        return None
    score, tip, hilt, group, length = best

    def rel(p):
        return p[0] - ORIGIN[0], p[1] - ORIGIN[1]

    return Blade(rel(tip), rel(hilt), [rel(p) for p in group], length, min(1.0, score / 12))


def tip_override(tips: dict | None, ref: str, default_enemy: str, facing: int):
    """The file's "tips" entry for a frame: {"Clip:i": [x, y] or [x, y, hiltX, hiltY] or {"facing": ...}}."""
    if not tips:
        return None
    enemy, clip, index = parse_ref(ref, default_enemy)
    for key in (f"{enemy}/{clip}:{index}", f"{clip}:{index}"):
        if key in tips:
            value = tips[key]
            if isinstance(value, dict):
                value = value.get(str(facing))
            if value:
                return tuple(value[:2]), tuple(value[2:4]) if len(value) == 4 else None
    return None


def blade_of(pixels: bytes, entry: dict, made: "Blade | None" = None) -> Blade | None:
    """A built frame's blade: its "tip" key, the blade a tween placed, else tracked in its pixels."""
    if made is not None and "tip" not in entry:
        return made
    found = find_blade(pixels)
    if "tip" in entry:
        tip = tuple(entry["tip"][:2])
        hilt = tuple(entry["tip"][2:4]) if len(entry["tip"]) == 4 else (found.hilt if found else (0, -30))
        return Blade(tip, hilt, found.points if found else [], 99, 1.0)
    return found


def blade_at(value, context: dict, where: str) -> Blade:
    """"tip" (this frame's blade), "prev" (the last frame's) or "Clip:i" (that frame's)."""
    if value in ("tip", "prev"):
        blade = context.get(value)
    else:
        blade = context["lib"].blade(value, context["enemy"], context["facing"], context.get("tips"))
    if blade is None:
        raise AnimError(f"{where}: no blade found for {value!r}: set \"tip\" on that frame, a \"tips\" entry, "
                        "or give [x, y] (anim_studio.py tips shows what is tracked)")
    return blade


def resolve_point(value, context: dict, where: str) -> tuple[float, float]:
    """[x, y], or a blade's tip ("tip", "prev", "Clip:i")."""
    if isinstance(value, list) and len(value) == 2:
        return tuple(value)
    return blade_at(value, context, where).tip


def sweep(a0: float, a1: float, direction: str) -> float:
    """Degrees from a0 to a1: the short way, or clockwise (positive) or counterclockwise."""
    delta = (a1 - a0) % 360
    if direction == "cw":
        return delta
    if direction == "ccw":
        return delta - 360
    return delta if delta <= 180 else delta - 360


def smear(layer: bytearray, spec: dict, shape: dict, where: str, context: dict) -> None:
    """A swing smear from one blade tip to another, around the point the blade turned about."""
    if set(spec) - SMEAR_KEYS:
        raise AnimError(f"{where}: smear is {{\"from\": \"prev\" | \"Clip:i\" | [x, y], \"to\": \"tip\" | "
                        "\"Clip:i\" | [x, y], \"dir\": \"short\" | \"cw\" | \"ccw\"}")
    points, hilts = [], []
    for value in (spec.get("from", "prev"), spec.get("to", "tip")):
        if isinstance(value, list):
            points.append(tuple(value))
        else:
            blade = blade_at(value, context, where)
            points.append(blade.tip)
            hilts.append(blade.hilt)
    (x0, y0), (x1, y1) = points
    if math.hypot(x1 - x0, y1 - y0) < 4:
        return
    # The center: on the perpendicular bisector of the two tips, nearest the hilts (the hand).
    hx = sum(h[0] for h in hilts) / len(hilts) if hilts else (x0 + x1) / 2
    hy = sum(h[1] for h in hilts) / len(hilts) if hilts else min(y0, y1) + 20
    mx, my = (x0 + x1) / 2, (y0 + y1) / 2
    nx, ny = -(y1 - y0), x1 - x0
    norm = math.hypot(nx, ny)
    nx, ny = nx / norm, ny / norm
    along = (hx - mx) * nx + (hy - my) * ny
    cx, cy = mx + nx * along, my + ny * along
    radius = math.hypot(x0 - cx, y0 - cy)
    a0 = math.degrees(math.atan2(y0 - cy, x0 - cx))
    a1 = a0 + sweep(a0, math.degrees(math.atan2(y1 - cy, x1 - cx)), spec.get("dir", "short"))
    taper = shape.get("taper", "in")
    body = {"arc": [cx, cy, radius], "from": a0, "to": a1, "width": shape.get("width", 7), "taper": taper,
            "color": shape.get("color")}
    overlay(layer, draw_shape(body, where, context))
    edge = shape.get("edge", 1)
    if edge:
        lead = {"arc": [cx, cy, radius + 1], "from": a0 + (a1 - a0) * 0.55, "to": a1, "width": 2, "taper": taper,
                "color": edge}
        overlay(layer, draw_shape(lead, where, context))


def styled(draws: list, styles: dict, where: str) -> list:
    """Draw items with "style" expanded: one item per layer of the file's "styles" entry, the item's
    shape (smear, arc, ...) and its other keys on each layer, widths times "scale" (default 1)."""
    out = []
    for i, item in enumerate(draws):
        if "style" not in item:
            out.append(item)
            continue
        name = item["style"]
        if name not in styles:
            raise AnimError(f"{where} draw {i}: no style {name!r} in the file's \"styles\" ({', '.join(sorted(styles)) or 'none'})")
        scale = item.get("scale", 1)
        own = {k: v for k, v in item.items() if k not in ("style", "scale")}
        for layer in styles[name]:
            merged = {**layer, **own}
            if "width" in layer and "width" not in own:
                merged["width"] = max(1, round(layer["width"] * scale))
            if "size" in layer and "size" not in own:
                merged["size"] = max(1, round(layer["size"] * scale))
            out.append(merged)
    return out


RIG_KEYS = {"ticks", "body", "angle", "lunge", "sword", "hand", "hit", "smear", "flash", "trail", "facings",
            "length", "like", "arm", "shoulder", "erase", "lean"}


def keyed(keys: list, tick: int, where: str) -> float:
    """A value at a tick from [[tick, value] or [tick, value, ease], ...]: ease "in" speeds up toward
    that key, "out" slows into it, "inout" both, default even."""
    if not keys or not all(isinstance(k, list) and len(k) in (2, 3) for k in keys):
        raise AnimError(f"{where}: keys are [[tick, value], [tick, value, \"in\" | \"out\" | \"inout\"], ...]")
    keys = sorted(keys, key=lambda k: k[0])
    if tick <= keys[0][0]:
        return keys[0][1]
    for a, b in zip(keys, keys[1:]):
        if a[0] <= tick <= b[0]:
            t = (tick - a[0]) / max(1, b[0] - a[0])
            ease = b[2] if len(b) == 3 else None
            t = {"in": t * t, "out": 1 - (1 - t) ** 2, "inout": t * t * (3 - 2 * t)}.get(ease, t)
            return a[1] + (b[1] - a[1]) * t
    return keys[-1][1]


def expand_rig(rig: dict, where: str) -> list[dict]:
    """One frame entry per tick from a rig: body frames switched at their ticks, the sword drawn at an
    angle eased between keys every tick, the lunge eased every tick (no stops), hit and smear ticks.
    "facings": {"N": {rig keys}} replaces keys for one facing (e.g. the angles facing down)."""
    if not isinstance(rig, dict) or set(rig) - RIG_KEYS or not {"ticks", "body", "angle"} <= set(rig) or \
            not ({"sword", "arm"} & set(rig)):
        raise AnimError(f"{where}: a rig has ticks, body [[tick, \"Clip:i\"], ...], angle [[tick, deg], ...], "
                        f"sword \"Clip:i\" (the frame its blade is taken from), optional {sorted(RIG_KEYS)}")

    def at_tick(spec: dict, tick: int) -> dict:
        body = max((k for k in spec["body"] if k[0] <= tick), key=lambda k: k[0], default=spec["body"][0])
        angle = keyed(spec["angle"], tick, where)
        if "arm" in spec:
            # A puppet: the arm and sword cut from one frame (rect), turned about the shoulder (pivot)
            # and set on the body's shoulder, over a body frame with its own arm erased.
            arm = spec["arm"]
            x0, y0, x1, y1 = arm["rect"]
            cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
            px, py = arm["pivot"]
            turn = angle - arm.get("angle", 0)
            shoulder = spec.get("shoulder", [[0, px, py]])
            sx = keyed([[k[0], k[1]] + k[3:] for k in shoulder], tick, where)
            sy = keyed([[k[0], k[2]] + k[3:] for k in shoulder], tick, where)
            r = math.radians(turn)
            # Turning about the rect center c, then moving by at = s - c + R(c - p), turns about p onto s.
            ax = sx - cx + (cx - px) * math.cos(r) - (cy - py) * math.sin(r)
            ay = sy - cy + (cx - px) * math.sin(r) + (cy - py) * math.cos(r)
            entry = {"src": body[1], "erase": spec.get("erase", []),
                     "patch": [{"from": arm["from"], "rect": [x0, y0, x1, y1], "at": [round(ax), round(ay)],
                                "rotate": round(turn, 1)}]}
            if "lean" in spec:
                entry.update({"rotate": round(keyed(spec["lean"], tick, where), 1), "pivot": [0, 0]})
            if "lunge" in spec:
                entry["lunge"] = round(keyed(spec["lunge"], tick, where))
            if tick in spec.get("hit", []):
                entry["hit"] = True
            draws = []
            for key, shape in (("smear", {"smear": {"from": "prev", "to": "tip"}}), ("flash", {"spark": "tip"})):
                fx = spec.get(key)
                if fx and tick in fx.get("on", []):
                    draws.append({**shape, **{k: v for k, v in fx.items() if k != "on"}})
            if draws:
                entry["draw"] = draws
            trail = spec.get("trail")
            if trail and tick in trail.get("on", []):
                entry["trail"] = {k: v for k, v in trail.items() if k != "on"}
            return entry
        entry = {"sword": {"from": spec["sword"], "angle": round(angle, 1)}}
        if str(body[1]).startswith("png:"):
            # New art (anim_studio.py slice): a drawn body without a sword, hand marked in the drawing.
            entry.update({"png": body[1][4:], "like": spec.get("like", spec["sword"])})
        else:
            entry["src"] = body[1]
        if "hand" in spec:
            # [[tick, x, y], ...] keys (eased like the others, they win over tracking) or one [x, y] fallback.
            hand = spec["hand"]
            if hand and isinstance(hand[0], list):
                xs = [[k[0], k[1]] + k[3:] for k in hand]
                ys = [[k[0], k[2]] + k[3:] for k in hand]
                entry["sword"]["grip"] = [round(keyed(xs, tick, where)), round(keyed(ys, tick, where))]
            else:
                entry["sword"]["hand"] = hand
        if "length" in spec:
            entry["sword"]["length"] = spec["length"]
        if "lunge" in spec:
            entry["lunge"] = round(keyed(spec["lunge"], tick, where))
        if tick in spec.get("hit", []):
            entry["hit"] = True
        draws = []
        smear = spec.get("smear")
        if smear and tick in smear.get("on", []):
            draws.append({"smear": {"from": "prev", "to": "tip"}, **{k: v for k, v in smear.items() if k != "on"}})
        flash = spec.get("flash")
        if flash and tick in flash.get("on", []):
            draws.append({"spark": "tip", **{k: v for k, v in flash.items() if k != "on"}})
        if draws:
            entry["draw"] = draws
        trail = spec.get("trail")
        if trail and tick in trail.get("on", []):
            entry["trail"] = {k: v for k, v in trail.items() if k != "on"}
        return entry

    ticks = rig["ticks"]
    overrides = {f: {**rig, **o} for f, o in rig.get("facings", {}).items()}
    explicit_hits = "hit" in rig or any("hit" in o for o in rig.get("facings", {}).values())
    out = []
    for tick in range(ticks):
        entry = at_tick(rig, tick)
        if explicit_hits:
            entry.setdefault("hit", False)
        facings = {}
        for f, spec in overrides.items():
            other = at_tick(spec, tick)
            if explicit_hits:
                other.setdefault("hit", False)
            changed = {k: v for k, v in other.items() if entry.get(k) != v}
            changed.update({k: None for k in entry if k not in other})
            if changed:
                facings[f] = changed
        if facings:
            entry["facings"] = facings
        out.append(entry)
    return out


def sword_pixels(lib: "Library", body_ref: str, sword: dict, enemy: str, facing: int, tips: dict | None,
                 where: str) -> tuple[bytearray, Blade]:
    """A body frame with its own blade erased and the sword (the blade of sword[\"from\"]) drawn in the
    hand at sword[\"angle\"] degrees (0 right, 90 down). The hand is sword[\"grip\"], else the body
    frame's tracked hilt, else sword[\"hand\"] [x, y], else the sword frame's hilt."""
    base = lib.pixels(body_ref, enemy, facing)
    own = lib.blade(body_ref, enemy, facing, tips)
    for x, y in (own.points if own else []):
        for dx in (-1, 0, 1):
            for dy in (-1, 0, 1):
                index = at(x + dx, y + dy)
                if index is not None:
                    base[index] = 0
    if own and "grip" not in sword:
        sword = {**sword, "grip": list(own.hilt)}
    return draw_sword(lib, base, sword, enemy, facing, tips, where)


def hand_mark(hero_name: str, name: str, facing: int):
    """The sword hand painted in a drawing (anim_studio.py slice records it), origin-relative, or None."""
    path = anims_path(hero_name).with_suffix("") / "hands.json"
    if not path.exists():
        return None
    marks = json.loads(path.read_text())
    return marks.get(f"{name}_f{facing}") or marks.get(name)


def draw_sword(lib: "Library", base: bytearray, sword: dict, enemy: str, facing: int, tips: dict | None,
               where: str) -> tuple[bytearray, Blade]:
    """The sword (the blade of sword[\"from\"]) drawn on base at sword[\"angle\"] with its hilt at
    sword[\"grip\"] (else sword[\"hand\"], else where it sits in its own frame)."""
    if not isinstance(sword, dict) or "from" not in sword or "angle" not in sword:
        raise AnimError(f"{where}: sword is {{\"from\": \"Clip:i\", \"angle\": deg, \"hand\": [x, y], \"length\": 1}}")
    blade = lib.blade(sword["from"], enemy, facing, tips)
    if blade is None or not blade.points:
        raise AnimError(f"{where}: no blade found in {sword['from']} facing {facing} for the sword (anim_studio.py "
                        "tips; pick a frame where the blade shows)")
    hx, hy = sword.get("grip") or sword.get("hand") or blade.hilt
    source = lib.pixels(sword["from"], enemy, facing)
    angle_from = math.atan2(blade.tip[1] - blade.hilt[1], blade.tip[0] - blade.hilt[0])
    turn = math.radians(sword["angle"]) - angle_from
    stretch = sword.get("length", 1)
    cos, sin = math.cos(turn), math.sin(turn)
    for x, y in blade.points:
        value = source[at(x, y)]
        if not value:
            continue
        ux, uy = (x - blade.hilt[0]) * stretch, (y - blade.hilt[1]) * stretch
        for sx, sy in ((0, 0), (0.5, 0), (0, 0.5), (0.5, 0.5)):
            index = at(hx + (ux + sx) * cos - (uy + sy) * sin, hy + (ux + sx) * sin + (uy + sy) * cos)
            if index is not None:
                base[index] = value
    ux, uy = (blade.tip[0] - blade.hilt[0]) * stretch, (blade.tip[1] - blade.hilt[1]) * stretch
    tip = (round(hx + ux * cos - uy * sin), round(hy + ux * sin + uy * cos))
    return base, Blade(tip, (round(hx), round(hy)), [], blade.length * stretch, 1.0)


def tween_near(entry: dict) -> str:
    tween = entry["tween"]
    return tween.get("body", tween["from"] if tween.get("t", 0.5) < 0.5 else tween["to"])


def tween_pixels(lib: "Library", tween: dict, enemy: str, facing: int, tips: dict | None,
                 where: str) -> tuple[bytearray, Blade]:
    """An in-between of two frames: the nearer frame's body with its blade removed, and the first
    frame's blade turned about its hilt part of the way toward the second frame's blade."""
    t = tween.get("t", 0.5)
    first, second = tween["from"], tween["to"]
    blade_a = lib.blade(first, enemy, facing, tips)
    blade_b = lib.blade(second, enemy, facing, tips)
    if blade_a is None or blade_b is None or not blade_a.points:
        missing = first if blade_a is None or not blade_a.points else second
        raise AnimError(f"{where}: tween needs a tracked blade in {missing} (anim_studio.py tips; a \"tips\" "
                        "entry fixes its tip and hilt)")
    body = tween.get("body", first if t < 0.5 else second)
    near_blade = lib.blade(body, enemy, facing, tips)
    base = lib.pixels(body, enemy, facing)
    for x, y in (near_blade.points if near_blade else []):
        for dx in (-1, 0, 1):
            for dy in (-1, 0, 1):
                index = at(x + dx, y + dy)
                if index is not None:
                    base[index] = 0
    angle_a = math.degrees(math.atan2(blade_a.tip[1] - blade_a.hilt[1], blade_a.tip[0] - blade_a.hilt[0]))
    angle_b = math.degrees(math.atan2(blade_b.tip[1] - blade_b.hilt[1], blade_b.tip[0] - blade_b.hilt[0]))
    turn = math.radians(sweep(angle_a, angle_b, tween.get("dir", "short")) * t)
    stretch = 1 + (blade_b.length / max(blade_a.length, 1) - 1) * t
    hx = blade_a.hilt[0] + (blade_b.hilt[0] - blade_a.hilt[0]) * t
    hy = blade_a.hilt[1] + (blade_b.hilt[1] - blade_a.hilt[1]) * t
    source = lib.pixels(first, enemy, facing)
    cos, sin = math.cos(turn), math.sin(turn)
    for x, y in blade_a.points:
        value = source[at(x, y)]
        if not value:
            continue
        ux, uy = (x - blade_a.hilt[0]) * stretch, (y - blade_a.hilt[1]) * stretch
        # Each source pixel covers its rotated footprint, so the turned blade has no holes.
        for sx, sy in ((0, 0), (0.5, 0), (0, 0.5), (0.5, 0.5)):
            index = at(hx + (ux + sx) * cos - (uy + sy) * sin, hy + (ux + sx) * sin + (uy + sy) * cos)
            if index is not None:
                base[index] = value
    ux, uy = (blade_a.tip[0] - blade_a.hilt[0]) * stretch, (blade_a.tip[1] - blade_a.hilt[1]) * stretch
    tip = (round(hx + ux * cos - uy * sin), round(hy + ux * sin + uy * cos))
    return base, Blade(tip, (round(hx), round(hy)), [], blade_a.length * stretch, 1.0)


def png_path(hero_name: str, name: str, facing: int) -> Path:
    folder = anims_path(hero_name).with_suffix("")
    for candidate in (folder / f"{name}_f{facing}.png", folder / f"{name}.png"):
        if candidate.exists():
            return candidate
    raise AnimError(f"no drawing {folder.relative_to(ROOT).as_posix()}/{name}_f{facing}.png or {name}.png")


def decoded_canvases(built: Built) -> list[bytearray]:
    """Each step's pixels as the game will draw them: decoded from the built bytes."""
    frame_set = parse_frame_set(built.frames, ROM_BASE)
    out = []
    for n, frame in enumerate(frame_set.frames):
        pixels = blank()
        draw_frame(pixels, frame, decode_frame_tiles(built.tiles, frame),
                   built.piece_ids[n] if n < len(built.piece_ids) else None)
        out.append(pixels)
    return out


def render_built(built: Built, palette_rgb, scale: int = 1) -> list:
    """Each step of a built facing as an RGBA image on the canvas, decoded from the built bytes."""
    return [image(pixels, palette_rgb, scale) for pixels in decoded_canvases(built)]
