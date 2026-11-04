"""anim_kit.py: built animations draw exactly what they describe, with the boxes and timing asked for."""

from __future__ import annotations

from pathlib import Path

import pytest

import anim_kit
import hero_clips
from rotkit.spritegfx import OBJ_SIZES, parse_frame_set

ROOT = Path(__file__).resolve().parents[2]
ROM = ROOT / "rotk.gba"
pytestmark = pytest.mark.skipif(not ROM.exists(), reason="needs the base ROM (rotk.gba)")
ENEMY = "WitchKing"
CLIP_FRAMES = 17  # WitchKing/Attack


@pytest.fixture(scope="module")
def kit():
    rom = ROM.read_bytes()
    lib = anim_kit.Library(hero_clips.Sources(ROOT, rom), rom)
    return lib, anim_kit.palette_rgb(ROOT, rom, 78)


def build(kit, frames, facing=2, compress=True, **anim):
    lib, palette = kit
    anims = {"enemy": ENEMY, "anims": {"t": {"frames": frames, **anim}}}
    return anim_kit.build(lib, "Test", "t", anims, palette, facing, compress=compress)


@pytest.mark.parametrize("extra", [{}, {"scale": 1}], ids=["verbatim", "re-encoded"])
def test_copy_of_a_clip_draws_the_clip(kit, extra):
    lib, _ = kit
    for facing in range(anim_kit.FACINGS):
        built = build(kit, [{"src": f"Attack:0-{CLIP_FRAMES - 1}", **extra}], facing)
        stock = [lib.pixels(f"Attack:{i}", ENEMY, facing) for i in range(CLIP_FRAMES)]
        assert anim_kit.decoded_canvases(built) == stock
        _, frame_set = lib.clip_facings(ENEMY, "Attack")[facing]
        assert [f.boxes for f in parse_frame_set(built.frames, anim_kit.ROM_BASE).frames] == \
            [f.boxes for f in frame_set.frames]


def test_preview_build_matches_game_build(kit):
    frames = [{"src": "Attack:5", "rotate": 15, "draw": [{"arc": [0, -30, 30], "from": -60, "to": 40, "color": 3}]}]
    assert anim_kit.decoded_canvases(build(kit, frames)) == anim_kit.decoded_canvases(build(kit, frames, compress=False))


def test_ranges_and_reverse(kit):
    built = build(kit, [{"src": "Attack:2-4"}, {"src": "Attack:4-2"}])
    assert [s.source for s in built.steps] == ["Attack:2", "Attack:3", "Attack:4", "Attack:4", "Attack:3", "Attack:2"]
    assert [s.frame for s in built.steps] == list(range(6))


def test_hold_repeats_and_hits_once(kit):
    built = build(kit, [{"src": "Attack:4"}, {"src": "Attack:5", "hold": 3, "hit": True}, {"src": "Attack:6"}])
    assert len(built.steps) == 5
    assert built.hits == [1]
    assert len(set(built.canvases[1:4])) == 1


def test_explicit_hits_drop_the_clips_own(kit):
    # Attack:5 is the clip's hit frame. Naming a hit elsewhere leaves only that one.
    built = build(kit, [{"src": "Attack:4-6"}, {"src": "Attack:12", "hit": [0, 20, -40, 0]}])
    assert built.hits == [3]
    assert built.steps[3].hit == (0, 20, -40, 0)
    assert build(kit, [{"src": "Attack:4-6"}]).hits == [1]


def test_auto_hitbox_follows_the_swing(kit):
    # Attack:12 swings down to the left in the side facing.
    built = build(kit, [{"src": "Attack:11"}, {"src": "Attack:12", "hit": "auto"}])
    x_min, x_max, _, _ = built.steps[1].hit
    assert x_min < -20 and x_max < 0


def test_moves_carry_boxes(kit):
    plain = build(kit, [{"src": "Attack:5"}])
    moved = build(kit, [{"src": "Attack:5", "dx": 7, "dy": -3}])
    assert [v + d for v, d in zip(plain.steps[0].hit, (7, 7, -3, -3))] == list(moved.steps[0].hit)
    flipped = build(kit, [{"src": "Attack:5", "flip": True}])
    x0, x1, y0, y1 = plain.steps[0].hit
    assert flipped.steps[0].hit == (-x1, -x0, y0, y1)


def test_encoder_uses_real_obj_shapes():
    pixels = anim_kit.blank()
    for y in range(-60, 4):
        for x in range(-30, 41):
            pixels[anim_kit.at(x, y)] = 5
    oam, tiles, _, _ = anim_kit.encode_pixels(pixels)
    assert all(o.size in OBJ_SIZES.values() for o in oam) and len(oam) <= anim_kit.MAX_OAM
    assert len(tiles) == sum(o.size[0] * o.size[1] // 2 for o in oam)


def test_errors_name_the_frame(kit):
    with pytest.raises(anim_kit.AnimError, match="frame 1"):
        build(kit, [{"src": "Attack:1"}, {"src": "Attack:99"}])
    with pytest.raises(anim_kit.AnimError, match="unknown keys"):
        build(kit, [{"src": "Attack:1", "rotation": 5}])


def test_blade_tracking_finds_the_raised_sword(kit):
    lib, _ = kit
    blade = lib.blade("Attack:8", ENEMY, 2)
    assert blade is not None and blade.tip[1] < -55 and blade.length > 20
    assert lib.blade("Attack:8", ENEMY, 2, {"Attack:8": [1, 2]}).tip == (1, 2)


def test_tween_turns_the_blade_between_its_frames(kit):
    built = build(kit, [{"src": "Attack:9"}, {"tween": {"from": "Attack:9", "to": "Attack:6", "t": 0.5, "dir": "cw"}},
                        {"src": "Attack:6"}])
    a, mid, b = (s.tip for s in built.steps)
    # The in-between tip lies between the two ends (it travels clockwise over the top).
    assert min(a[0], b[0]) <= mid[0] <= max(a[0], b[0])
    assert built.canvases[1] not in (built.canvases[0], built.canvases[2])


def test_smear_and_spark_draw_at_the_blade(kit):
    plain = build(kit, [{"src": "Attack:8"}, {"src": "Attack:6"}])
    drawn = build(kit, [{"src": "Attack:8"}, {"src": "Attack:6", "draw": [
        {"smear": {"from": "prev", "to": "tip"}, "width": 6, "color": 3}, {"spark": "tip", "color": 1}]}])
    added = sum(1 for a, b in zip(plain.canvases[1], drawn.canvases[1]) if a != b)
    assert added > 60
    x, y = drawn.steps[1].tip
    near = [drawn.canvases[1][anim_kit.at(x + dx, y + dy)] for dx in range(-8, 9) for dy in range(-8, 9)
            if anim_kit.at(x + dx, y + dy) is not None]
    assert near.count(1) >= 6


def test_facing_order():
    # Facing 0 walks up (back view), 4 walks down (front view), measured in game: lunge must follow.
    assert anim_kit.FACING_NAMES[0] == "up" and anim_kit.FACING_NAMES[4] == "down"
    assert anim_kit.FORWARD[0][1] < 0 and anim_kit.FORWARD[4][1] > 0 and anim_kit.FORWARD[2] == (1, 0)


def test_rig_expands_one_entry_per_tick():
    rig = {"ticks": 6, "sword": "Attack:6", "body": [[0, "Attack:2"], [3, "Attack:6"]],
           "angle": [[0, -90], [5, 0, "in"]], "lunge": [[0, 0], [5, 5]], "hit": [3],
           "facings": {"4": {"angle": [[0, -90], [5, 90]]}}}
    entries = anim_kit.expand_rig(rig, "test")
    assert len(entries) == 6
    assert [e["src"] for e in entries] == ["Attack:2"] * 3 + ["Attack:6"] * 3
    angles = [e["sword"]["angle"] for e in entries]
    assert angles[0] == -90 and angles[-1] == 0 and angles == sorted(angles)
    assert angles[1] - angles[0] < angles[5] - angles[4]  # "in": the strike speeds up
    assert [e["hit"] for e in entries] == [False, False, False, True, False, False]
    assert entries[5]["facings"]["4"]["sword"]["angle"] == 90
