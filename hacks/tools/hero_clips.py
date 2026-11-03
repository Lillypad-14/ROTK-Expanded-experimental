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
no hit. A "melee" list of 7 poses replaces "swings".

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
        return [Facing(t, self.addresses[t], frame_set_bytes(self.rom, self.addresses[f]), d)
                for t, f, d in self.banks[symbol]]


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
    return Facing(facing.tiles, facing.tiles_address, join_frames(header, picked), facing.duration)


def make_clip(sources: Sources, enemy: str, entry, label: str) -> Clip:
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
    banks = {}
    for bank in ("move", "cast"):
        poses = body[bank]
        if len(poses) != POSES:
            raise ValueError(f"{hero['name']}: body.{bank} needs {POSES} poses ({', '.join(POSE_NAMES[bank])})")
        banks[bank] = dedupe([make_clip(sources, enemy, p, f"{symbol}{bank.title()}{n}") for n, p in enumerate(poses)], poses)
    if "melee" in body:
        if len(body["melee"]) != POSES:
            raise ValueError(f"{hero['name']}: body.melee needs {POSES} poses")
        banks["melee"] = dedupe([make_clip(sources, enemy, p, f"{symbol}Melee{n}") for n, p in enumerate(body["melee"])],
                                body["melee"])
    else:
        swings, recovers = [], []
        for n, swing in enumerate(body["swings"]):
            swing = {"clip": "Attack", **({"clip": swing} if isinstance(swing, str) else swing)}
            clip = make_clip(sources, enemy, swing, f"{symbol}Swing{n}")
            count = len(split_frames(clip.facings[0].frames, 0)[1])
            start = max(0, count - RECOVERY_FRAMES)
            recover = Clip(f"{symbol}Recover{n}", [cut(f, start, count - 1, False) for f in clip.facings], False)
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
