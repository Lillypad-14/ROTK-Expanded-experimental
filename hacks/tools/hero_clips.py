"""A hero's body as data: the move, melee and cast banks of a hero made from an enemy.

heroes.json "body" (hacks/tools/new_hero.py writes one):

    "body": {"enemy": "WitchKing",
             "move": ["Stagger", "Death", "Stagger", "Stagger", {"clip": "Walk", "frames": [0, 0]},
                      "Stagger", "Walk"],
             "swings": [{"frames": [0, 7]}, {"frames": [8, 16], "hit": 12}],
             "cast": ["Attack", "Attack", "Attack", "Attack", "Attack", "Attack", "Attack"]}

A pose is a clip: a name of the enemy's clips (EnemyAnimationBank_EnemyType<enemy>_<name>), any
SpriteAnimation symbol, or a cut of one:

    {"clip": "Attack", "frames": [first, last], "hit": ..., "speed": n}

- "frames": an inclusive range of the clip's frames (default all).
- "hit": a frame index of the clip that deals the melee hit (the clip's hitbox copied there), or
  false for no hit. Default: the clip's own hitbox frames.
- "speed": frameDuration, game frames per animation frame minus 1 (default the clip's).

Move poses: knocked down, death, getting up, hurt, idle, knocked back, walk. Cast pose n is skill row
n - 1. "swings" are melee cuts (clip default "Attack"): the stock combo plays pose 0 first swing,
1 its recovery, 2 second swing, 3 its recovery, 4 finisher. The swings cycle through 0, 2 and 4,
the finisher is the last swing, and each recovery is its swing's last RECOVERY_FRAMES frames with
no hit, plus a padding repeat of the last frame (the game ends a pose one tick into its last
frame), slowed to last about RECOVERY_TICKS game ticks in all (body "recoveryTicks" overrides). The
recovery is the combo window: the game chains a press during a swing (buffered) or during the first
5 frames of its recovery, so a short recovery eats late presses (stock heroes: 8 ticks, 268 ms). A "melee" list of 7 poses replaces "swings".

hero_data.py links the bodies into the hero data region. hero_preview.py draws a body straight
from this data, without a build.
"""

from __future__ import annotations

from dataclasses import dataclass
import json
from pathlib import Path
import re
import struct

ROM_BASE = 0x08000000
FACINGS = 5
PARTS = 8
POSES = 7
RECOVERY_FRAMES = 2
RECOVERY_TICKS = 8
HEADER = 12
FRAME = 8
HOTSPOT = 2
TRIGGER_BOX = 6
OAM = 4
# CombatTriggerBoxSlot COMBAT_TRIGGER_HITBOX and the ActorTriggerBox.enable byte.
HITBOX_SLOT = 1
ENABLE = 4
BANK_NAMES = ("move", "melee", "cast")
MOVE_POSES = ("knocked down", "death", "getting up", "hurt", "idle", "knocked back", "walk")
MELEE_POSES = ("swing 1", "recover 1", "swing 2", "recover 2", "finisher", "pose 5", "pose 6")
CAST_POSES = tuple(f"pose {n}" for n in range(POSES))
POSE_NAMES = {"move": MOVE_POSES, "melee": MELEE_POSES, "cast": CAST_POSES}


@dataclass
class Facing:
    tiles: str  # tiles symbol
    tiles_address: int
    frames: bytes  # the frame set (header, offsets, frame records)
    duration: int
    # Tile data of a built anim (anim_kit.py), placed in the hero data region. None: tiles_address.
    tiles_data: bytes | None = None
    # Per frame, each OBJ's own 16 colors (BGR555) or None: anims with "#rrggbb" colors (PC host).
    palettes: list | None = None


@dataclass
class Clip:
    """One pose: a stock SpriteAnimation (symbol) or a generated cut (label, facings)."""

    label: str
    facings: list[Facing]
    stock: bool


class Sources:
    """The carved enemy clips: their symbols, addresses and frame sets."""

    def __init__(self, root: Path, rom: bytes):
        self.root = root
        self.rom = rom
        self.addresses = {}
        for line in (root / "carved/config/enemy_graphics.cfg").read_text().splitlines():
            parts = line.split()
            if len(parts) >= 2 and parts[0].startswith("0x"):
                self.addresses[parts[1]] = int(parts[0], 16)
        self.banks = {}
        for path in (root / "carved/data/enemy_graphics").glob("EnemyAnimationBanks_*.c"):
            for name, body in re.findall(r"\b(\w+)\[5\] = \{(.*?)\n\};", path.read_text(), re.S):
                rows = re.findall(r"\{ (\w+), &(\w+)\.header, NULL, (\d+) \}", body)
                if len(rows) == FACINGS:
                    self.banks[name] = [(t, f, int(d)) for t, f, d in rows]

    def symbol(self, enemy: str, clip: str) -> str:
        if clip in self.banks:
            return clip
        full = f"EnemyAnimationBank_EnemyType{enemy}_{clip}"
        if full not in self.banks:
            names = sorted(n.split("_", 2)[2] for n in self.banks if n.startswith(f"EnemyAnimationBank_EnemyType{enemy}_"))
            raise ValueError(f"No clip {clip} for {enemy}: one of {', '.join(names)}")
        return full

    def facings(self, symbol: str) -> list[Facing]:
        """The clip's 5 facings in player order (0 up .. 4 down). Clips listed in
        hacks/content/enemy_facings.json store up and down the other way round and come reversed."""
        out = [Facing(t, self.addresses[t], frame_set_bytes(self.rom, self.addresses[f]), d)
               for t, f, d in self.banks[symbol]]
        return out[::-1] if symbol in self.reversed_clips() else out

    def reversed_clips(self) -> set[str]:
        if not hasattr(self, "_reversed"):
            path = self.root / "hacks/content/enemy_facings.json"
            data = json.loads(path.read_text()) if path.exists() else {}
            self._reversed = {f"EnemyAnimationBank_EnemyType{enemy}_{clip}"
                              for enemy, clips in data.get("reversed", {}).items() for clip in clips}
        return self._reversed


def frame_set_bytes(rom: bytes, address: int) -> bytes:
    header, frames, _, _ = split_frames(rom, address - ROM_BASE)
    return join_frames(header, frames)


def split_frames(data: bytes, base: int) -> tuple[bytes, list[bytes], int, int]:
    """A frame set's header bytes, each frame record's bytes, and the hotspot and box counts."""
    header = data[base:base + HEADER]
    count = struct.unpack_from("<H", header, 6)[0]
    hotspots, boxes = header[10], header[11]
    offsets = struct.unpack_from(f"<{count}H", data, base + HEADER)
    frames = []
    for offset in offsets:
        start = base + HEADER + offset
        size = FRAME + hotspots * HOTSPOT + boxes * TRIGGER_BOX + (data[start] & 0x1F) * OAM
        frames.append(data[start:start + size])
    return header, frames, hotspots, boxes


def join_frames(header: bytes, frames: list[bytes]) -> bytes:
    data = bytearray(header)
    struct.pack_into("<H", data, 6, len(frames))
    struct.pack_into("<H", data, 8, max(struct.unpack_from("<H", f, 6)[0] for f in frames))
    at = 2 * len(frames)
    for frame in frames:
        data += struct.pack("<H", at)
        at += len(frame)
    for frame in frames:
        data += frame
    return bytes(data)


def hitbox_at(hotspots: int) -> int:
    return FRAME + hotspots * HOTSPOT + HITBOX_SLOT * TRIGGER_BOX


def hit_frames(frames: bytes) -> list[int]:
    """Frame indexes of a frame set whose hitbox is enabled."""
    _, records, hotspots, boxes = split_frames(frames, 0)
    if boxes <= HITBOX_SLOT:
        return []
    return [i for i, f in enumerate(records) if f[hitbox_at(hotspots) + ENABLE]]


def cut(facing: Facing, first: int, last: int, hit) -> Facing:
    """Frames first..last of one facing. hit: None keeps the clip's hitboxes, False removes them, a
    frame index moves the clip's hitbox to that frame."""
    header, frames, hotspots, boxes = split_frames(facing.frames, 0)
    box = hitbox_at(hotspots)
    has_hitbox = boxes > HITBOX_SLOT
    if isinstance(hit, int) and hit is not True and hit is not False:
        source = next((f for f in frames if has_hitbox and f[box + ENABLE]), None)
        if source is None:
            raise ValueError("the clip has no hitbox to copy")
    picked = []
    for index in range(first, last + 1):
        frame = bytearray(frames[index])
        if has_hitbox and hit is False:
            frame[box + ENABLE] = 0
        elif has_hitbox and isinstance(hit, int) and hit is not True:
            if index == hit:
                frame[box:box + TRIGGER_BOX] = source[box:box + TRIGGER_BOX]
            else:
                frame[box + ENABLE] = 0
        picked.append(bytes(frame))
    palettes = facing.palettes[first:last + 1] if facing.palettes else None
    return Facing(facing.tiles, facing.tiles_address, join_frames(header, picked), facing.duration, facing.tiles_data,
                  palettes)


def pad_last(facing: Facing) -> Facing:
    """The facing with its last frame repeated once (the frame the game cuts short)."""
    header, frames, _, _ = split_frames(facing.frames, 0)
    palettes = facing.palettes + facing.palettes[-1:] if facing.palettes else None
    return Facing(facing.tiles, facing.tiles_address, join_frames(header, frames + frames[-1:]), facing.duration,
                  facing.tiles_data, palettes)


class Anims:
    """A hero's anim file (anim_kit.py), for poses written "@name"."""

    def __init__(self, root: Path, rom: bytes, hero: dict, sources: Sources):
        import anim_kit

        self.kit = anim_kit
        self.hero = hero["name"]
        self.data = anim_kit.load(self.hero)
        if self.data.get("enemy") is None:
            self.data["enemy"] = hero["body"]["enemy"]
        self.lib = anim_kit.Library(sources, rom)
        self.palette = anim_kit.palette_rgb(root, rom, hero["palette"])
        self.warnings: list[str] = []

    def clip(self, name: str, label: str, speed: int | None = None) -> Clip:
        facings = []
        for facing in range(FACINGS):
            built = self.kit.build(self.lib, self.hero, name, self.data, self.palette, facing, self.warnings)
            facings.append(Facing(f"{label}Tiles", 0, built.frames, speed or built.duration, built.tiles,
                                  built.piece_palettes()))
        return Clip(label, facings, False)


def make_clip(sources: Sources, enemy: str, entry, label: str, anims: Anims | None = None) -> Clip:
    anim = entry[1:] if isinstance(entry, str) and entry.startswith("@") else None
    if isinstance(entry, dict) and "anim" in entry:
        if set(entry) - {"anim", "speed"}:
            raise ValueError(f"{label}: an anim pose takes only anim and speed")
        anim = entry["anim"].lstrip("@")
    if anim is not None:
        if anims is None:
            raise ValueError(f"{label}: @{anim} needs the hero's anim file")
        speed = entry.get("speed") if isinstance(entry, dict) else None
        return anims.clip(anim, label, speed)
    if isinstance(entry, str):
        entry = {"clip": entry}
    unknown = set(entry) - {"clip", "frames", "hit", "speed"}
    if unknown:
        raise ValueError(f"{label}: unknown keys {sorted(unknown)} (clip, frames, hit, speed)")
    symbol = sources.symbol(enemy, entry["clip"])
    facings = sources.facings(symbol)
    if set(entry) == {"clip"}:
        return Clip(symbol, facings, True)
    count = len(split_frames(facings[0].frames, 0)[1])
    first, last = entry.get("frames", [0, count - 1])
    hit = entry.get("hit")
    if not 0 <= first <= last < count:
        raise ValueError(f"{label}: frames {first}..{last} outside {entry['clip']}'s 0..{count - 1}")
    if hit is True or (hit is not None and hit is not False and not first <= hit <= last):
        raise ValueError(f"{label}: hit must be false or a frame in {first}..{last}")
    speed = entry.get("speed")
    if speed is not None and not 0 < speed < 32:
        raise ValueError(f"{label}: speed must be 1..31 (0 freezes the pose and blocks attacks)")
    out = []
    for facing in facings:
        part = cut(facing, first, last, hit)
        if speed is not None:
            part.duration = speed
        out.append(part)
    return Clip(label, out, False)


def body_banks(root: Path, rom: bytes, hero: dict, sources: Sources | None = None) -> dict[str, list[Clip]]:
    """The 7 pose clips of each bank in a hero's "body"."""
    body = hero["body"]
    sources = sources or Sources(root, rom)
    enemy = body["enemy"]
    symbol = re.sub(r"[^A-Za-z0-9]", "", hero["name"].title())
    anims = Anims(root, rom, hero, sources)
    banks = {}
    for bank in ("move", "cast"):
        poses = body[bank]
        if len(poses) != POSES:
            raise ValueError(f"{hero['name']}: body.{bank} needs {POSES} poses ({', '.join(POSE_NAMES[bank])})")
        banks[bank] = dedupe([make_clip(sources, enemy, p, f"{symbol}{bank.title()}{n}", anims) for n, p in enumerate(poses)], poses)
    if "melee" in body:
        if len(body["melee"]) != POSES:
            raise ValueError(f"{hero['name']}: body.melee needs {POSES} poses")
        banks["melee"] = dedupe([make_clip(sources, enemy, p, f"{symbol}Melee{n}", anims) for n, p in enumerate(body["melee"])],
                                body["melee"])
    else:
        swings, recovers = [], []
        for n, swing in enumerate(body["swings"]):
            if not (isinstance(swing, str) and swing.startswith("@")) and not (isinstance(swing, dict) and "anim" in swing):
                swing = {"clip": "Attack", **({"clip": swing} if isinstance(swing, str) else swing)}
            clip = make_clip(sources, enemy, swing, f"{symbol}Swing{n}", anims)
            count = len(split_frames(clip.facings[0].frames, 0)[1])
            start = max(0, count - RECOVERY_FRAMES)
            ticks = int(body.get("recoveryTicks", RECOVERY_TICKS))
            # The game ends a pose one tick into its last frame, so the last frame repeats once
            # as padding and the real frames get the whole time.
            facings = [pad_last(cut(f, start, count - 1, False)) for f in clip.facings]
            for f in facings:
                f.duration = max(f.duration, -(-ticks // (count - start)))
            recover = Clip(f"{symbol}Recover{n}", facings, False)
            swings.append(clip)
            recovers.append(recover)
        if not swings:
            raise ValueError(f"{hero['name']}: body.swings needs at least one swing")
        k = len(swings)
        banks["melee"] = [swings[0], recovers[0], swings[1 % k], recovers[1 % k], swings[-1], swings[0], swings[0]]
    return banks


def dedupe(clips: list[Clip], entries: list) -> list[Clip]:
    """Equal pose entries share one clip."""
    seen = {}
    for n, entry in enumerate(entries):
        key = json.dumps(entry, sort_keys=True)
        clips[n] = seen.setdefault(key, clips[n])
    return clips
