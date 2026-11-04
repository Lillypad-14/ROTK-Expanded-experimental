"""Experimental, not used: rejected for the Witch-king (looked wrong next to the sprite). The default for
swings is the enemy clip cut into ranges (docs/wip-docs/animation-studio.md).

3D puppet hero animations: a posed 3D model rendered into canvas PNGs for every facing and tick.

    uv run python hacks/tools/puppet3d.py render Witch-king            # every anim in the puppet file
    uv run python hacks/tools/puppet3d.py render Witch-king swing1 --facing 2
    uv run python hacks/tools/puppet3d.py sheet Witch-king swing1      # build/anim/puppet_<anim>.png
    uv run python hacks/tools/puppet3d.py write Witch-king             # anims into anims/<slug>.json

The puppet file is hacks/content/anims/<slug>/puppet.json: "model" (body sizes, colors), "anims"
(keyed poses). Every tick is a new drawing and every facing is the same 3D motion seen from that side
(0 up/back .. 4 down/front, the side ones facing right), so no facing needs its own keys.

Character space: x his right, y up, z forward, feet at the origin, 1 unit = 1 canvas pixel before the
3/4 camera tilt. Pose keys (any subset, Catmull-Rom between keys, so motion never stops):
    t      tick
    root   [x, y, z]       body offset (z forward is a lunge, y down a crouch)
    lean   [pitch, twist]  upper body degrees about the hips (pitch + forward, twist + right shoulder forward)
    rh     [x, y, z]       right hand (the sword hand)
    sword  [x, y, z]       blade direction (normalized)
    lh     [x, y, z]       left hand, or "grip" for both hands on the hilt
    flare  n               cloak hem spread (1 normal)
    hem    [x, z]          cloak hem offset (trails motion)
    step   n               front foot forward
Anim keys: "keys", "hit" (ticks), "smear" (ticks), "burst" (ticks: a flash at the tip), "note".
"""

from __future__ import annotations

import argparse
import json
import math
from pathlib import Path
import sys

import numpy as np

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "hacks/tools"))
import anim_kit as K  # noqa: E402

CANVAS, OX, OY = 192, 96, 128
PITCH = math.radians(32)
FORWARD = {0: (0.0, -1.0), 1: (0.7071, -0.7071), 2: (1.0, 0.0), 3: (0.7071, 0.7071), 4: (0.0, 1.0)}
SS = 3  # supersampling per axis
VIEW = (-48, -96, 56, 24)  # rendered screen rect (x0, y0, x1, y1), origin-relative

# Materials
CLOAK, HOOD, VOID, CROWN, ARMOR, BLADE, HILT = range(7)


def norm(v):
    v = np.asarray(v, float)
    n = np.linalg.norm(v)
    return v / n if n > 1e-9 else v


# ---------------------------------------------------------------------------------------------
# SDF primitives (p: (N, 3))


def sd_round_cone(p, a, b, r1, r2):
    a = np.asarray(a, float)
    b = np.asarray(b, float)
    ba = b - a
    l2 = float(ba @ ba)
    if l2 < 1e-9:
        return np.linalg.norm(p - a, axis=1) - max(r1, r2)
    rr = r1 - r2
    a2 = l2 - rr * rr
    il2 = 1.0 / l2
    pa = p - a
    y = pa @ ba
    z = y - l2
    q = pa * l2 - np.outer(y, ba)
    x2 = np.einsum("ij,ij->i", q, q)
    y2 = y * y * l2
    z2 = z * z * l2
    k = math.copysign(1, rr) * rr * rr * x2
    d_b = np.sqrt(x2 + z2) * il2 - r2
    d_a = np.sqrt(x2 + y2) * il2 - r1
    d_m = (np.sqrt(np.maximum(x2 * a2 * il2, 0)) + y * rr) * il2 - r1
    return np.where(np.sign(z) * a2 * z2 > k, d_b, np.where(np.sign(y) * a2 * y2 < k, d_a, d_m))


def sd_frustum(p, a, b, ra, rb, ragged=0.0):
    """Capped cone from a (radius ra) to b (radius rb), flat ends. ragged: tattered depth at the b end."""
    a = np.asarray(a, float)
    b = np.asarray(b, float)
    rba = rb - ra
    ba = b - a
    baba = float(ba @ ba)
    pa = p - a
    papa = np.einsum("ij,ij->i", pa, pa)
    paba = (pa @ ba) / baba
    x = np.sqrt(np.maximum(papa - paba * paba * baba, 0))
    cax = np.maximum(0.0, x - np.where(paba < 0.5, ra, rb))
    cay = np.abs(paba - 0.5) - 0.5
    k = rba * rba + baba
    f = np.clip((rba * (x - ra) + paba * baba) / k, 0, 1)
    cbx = x - ra - f * rba
    cby = paba - f
    sgn = np.where((cbx < 0) & (cay < 0), -1.0, 1.0)
    d = sgn * np.sqrt(np.minimum(cax * cax + cay * cay * baba, cbx * cbx + cby * cby * baba))
    if ragged:
        # Tatters: notches cut into the hem, deeper toward the bottom.
        side = np.cross(ba, [0.0, 0.0, 1.0] if abs(ba[2]) < 0.9 * math.sqrt(baba) else [1.0, 0.0, 0.0])
        side /= np.linalg.norm(side)
        other = np.cross(ba / math.sqrt(baba), side)
        ang = np.arctan2(pa @ side, pa @ other)
        notch = 0.5 + 0.5 * np.sin(ang * 9 + 0.7) * np.sin(ang * 5 + 2.1) + 0.3 * np.sin(ang * 17)
        depth = np.clip((paba - 0.72) / 0.28, 0, 1)
        d = d + depth * np.maximum(notch, 0) * ragged
    return d


def sd_sphere(p, c, r):
    return np.linalg.norm(p - np.asarray(c, float), axis=1) - r


def sd_box(p, c, axes, half):
    """Oriented box: axes (3 unit vectors), half extents."""
    d = p - np.asarray(c, float)
    q = np.abs(np.stack([d @ np.asarray(ax, float) for ax in axes], axis=1)) - np.asarray(half, float)
    return np.linalg.norm(np.maximum(q, 0), axis=1) + np.minimum(q.max(axis=1), 0)


def sd_blade(p, hilt, d, flat, length, width, thick):
    """A blade from hilt along d, tapering to a point."""
    rel = p - np.asarray(hilt, float)
    u = rel @ d
    side = np.cross(d, flat)
    w = rel @ side
    t = rel @ flat
    taper = np.clip(1.0 - (u - (length - 6)) / 6.0, 0.15, 1.0)
    hw = width * taper
    qx = np.abs(u - length / 2) - length / 2
    qy = np.abs(w) - hw
    qz = np.abs(t) - thick
    q = np.stack([qx, qy, qz], axis=1)
    return np.linalg.norm(np.maximum(q, 0), axis=1) + np.minimum(q.max(axis=1), 0)


def smin(a, b, k):
    h = np.clip(0.5 + 0.5 * (b - a) / k, 0, 1)
    return b * (1 - h) + a * h - k * h * (1 - h)


def hash_noise(x, y, seed=0):
    v = np.sin(x * 12.9898 + y * 78.233 + seed * 37.719) * 43758.5453
    return v - np.floor(v)


# ---------------------------------------------------------------------------------------------
# Posing


def rot_x(deg):
    a = math.radians(deg)
    c, s = math.cos(a), math.sin(a)
    return np.array([[1, 0, 0], [0, c, -s], [0, s, c]])


def rot_y(deg):
    a = math.radians(deg)
    c, s = math.cos(a), math.sin(a)
    return np.array([[c, 0, s], [0, 1, 0], [-s, 0, c]])


def catmull(keys, t, name, default):
    """Catmull-Rom through the keys that set name (vectors or scalars), clamped at the ends."""
    pts = [(k["t"], np.atleast_1d(np.asarray(k[name], float))) for k in keys if name in k and k[name] != "grip"]
    if not pts:
        return np.atleast_1d(np.asarray(default, float))
    if t <= pts[0][0]:
        return pts[0][1]
    if t >= pts[-1][0]:
        return pts[-1][1]
    i = max(j for j in range(len(pts) - 1) if pts[j][0] <= t)
    t0, p1 = pts[i]
    t1, p2 = pts[i + 1]
    p0 = pts[i - 1][1] if i > 0 else p1 - (p2 - p1)
    p3 = pts[i + 2][1] if i + 2 < len(pts) else p2 + (p2 - p1)
    # Non-uniform spacing: scale tangents by segment length.
    tm0 = pts[i - 1][0] if i > 0 else t0 - (t1 - t0)
    tm3 = pts[i + 2][0] if i + 2 < len(pts) else t1 + (t1 - t0)
    u = (t - t0) / (t1 - t0)
    m1 = (p2 - p0) / (t1 - tm0) * (t1 - t0)
    m2 = (p3 - p1) / (tm3 - t0) * (t1 - t0)
    u2, u3 = u * u, u * u * u
    return (2 * u3 - 3 * u2 + 1) * p1 + (u3 - 2 * u2 + u) * m1 + (-2 * u3 + 3 * u2) * p2 + (u3 - u2) * m2


def ik(shoulder, hand, l1, l2, pole):
    """Elbow position for a two-bone arm, and the hand pulled into reach."""
    d = hand - shoulder
    dist = np.linalg.norm(d)
    reach = (l1 + l2) * 0.999
    if dist > reach:
        hand = shoulder + d / dist * reach
        d = hand - shoulder
        dist = reach
    dist = max(dist, abs(l1 - l2) + 0.01)
    dn = d / dist
    a = (l1 * l1 - l2 * l2 + dist * dist) / (2 * dist)
    h = math.sqrt(max(l1 * l1 - a * a, 0))
    pole = np.asarray(pole, float)
    perp = norm(pole - dn * (pole @ dn))
    return shoulder + dn * a + perp * h, hand


def pose_at(model, anim, t):
    keys = anim["keys"]
    root = catmull(keys, t, "root", [0, 0, 0])
    pitch, twist = catmull(keys, t, "lean", [0, 0])
    rh = catmull(keys, t, "rh", model["ready_rh"])
    sword = norm(catmull(keys, t, "sword", model["ready_sword"]))
    flare = float(catmull(keys, t, "flare", [1])[0])
    hem = catmull(keys, t, "hem", [0, 0])
    step = float(catmull(keys, t, "step", [0])[0])
    grip = any(k.get("lh") == "grip" for k in keys if k["t"] <= t + 0.01) and \
        next((k.get("lh") for k in sorted(keys, key=lambda k: -k["t"]) if k["t"] <= t + 0.01 and "lh" in k), None) == "grip"
    lh = None if grip else catmull(keys, t, "lh", model["ready_lh"])
    return {"root": root, "pitch": float(pitch), "twist": float(twist), "rh": rh, "sword": sword,
            "flare": flare, "hem": hem, "step": step, "grip": grip, "lh": lh}


def build_parts(model, pose):
    """Posed primitives in character space: [(kind, args, material)]."""
    m = model
    root = pose["root"]
    pelvis = np.array([0, m["pelvis"], 0]) + root
    upper = rot_y(-pose["twist"]) @ rot_x(-pose["pitch"])

    def up(p):
        return pelvis + upper @ (np.asarray(p, float) - np.array([0, m["pelvis"], 0]))

    parts = []
    neck = up([0, m["neck"], 0])
    head = up([0, m["head"], 0.3])
    shoulders_y = m["shoulder"]
    rs = up([m["shoulder_w"], shoulders_y, 0])
    ls = up([-m["shoulder_w"], shoulders_y, 0])
    # Cloak: from the shoulders to the hem, the hem trailing and flaring.
    hem_c = np.array([pose["hem"][0], 1.5, pose["hem"][1] - 2.5]) + np.array([root[0], 0, root[2]])
    hem_r = m["hem_r"] * pose["flare"]
    parts.append(("frustum", (up([0, shoulders_y - 1, -0.5]), hem_c, m["cloak_top_r"], hem_r, m["ragged"]), CLOAK))
    # The cape: a drape from the upper back trailing behind to the ground.
    cape_end = np.array([pose["hem"][0] * 1.4, 0.5, pose["hem"][1] * 1.5 - m["cape_trail"]]) +         np.array([root[0], 0, root[2]])
    parts.append(("frustum", (up([0, shoulders_y - 2, -2.5]), cape_end, m["cloak_top_r"] * 0.8,
                              m["cape_r"] * pose["flare"], m["ragged"] * 1.3), CLOAK))
    parts.append(("cone", (ls + (rs - ls) * 0.0, rs, m["shoulder_r"], m["shoulder_r"]), CLOAK))
    parts.append(("cone", (neck, head, 3.2, m["head_r"] * 0.9), HOOD))
    parts.append(("sphere", (head, m["head_r"]), HOOD))
    # Hood peak behind the head.
    parts.append(("sphere", (up([0, m["head"] + 1.5, -1.6]), m["head_r"] * 0.85), HOOD))
    # Crown: spikes around the top of the head.
    crown_c = up([0, m["head"] + 2.5, 0.2])
    n = m["spikes"]
    for i in range(n):
        a = 2 * math.pi * i / n
        local = np.array([math.sin(a) * m["crown_r"], 0, math.cos(a) * m["crown_r"]])
        base = crown_c + upper @ local
        front = math.cos(a)
        length = m["spike_len"] * (0.75 + 0.25 * (0.5 + 0.5 * front)) * (1.0 + 0.15 * ((i % 2) - 0.5))
        tip = base + upper @ np.array([local[0] * 0.18, length, local[2] * 0.18])
        parts.append(("cone", (base, tip, m["spike_r"], 0.15), CROWN))
    parts.append(("cone", (crown_c - upper @ np.array([0, 0.6, 0]), crown_c + upper @ np.array([0, 0.9, 0]),
                           m["crown_r"] + 0.6, m["crown_r"] + 0.6), CROWN))
    # Arms.
    rh = pose["rh"] + root
    sword = pose["sword"]
    l1, l2 = m["upper_arm"], m["forearm"]
    elbow, rh = ik(rs, rh, l1, l2, [1.0, -1.0, -0.4])
    parts.append(("cone", (rs, elbow, m["sleeve_r"], m["sleeve_r"] * 0.9), CLOAK))
    wrist = elbow + (rh - elbow) * 0.6
    parts.append(("cone", (elbow, wrist, m["sleeve_r"] * 0.9, m["sleeve_r"] * 1.15), CLOAK))
    parts.append(("cone", (wrist, rh, m["gauntlet_r"], m["gauntlet_r"] * 1.05), ARMOR))
    if pose["grip"]:
        lh_target = rh - sword * 3.2
    else:
        lh_target = pose["lh"] + root
    lelbow, lh = ik(ls, lh_target, l1, l2, [-1.0, -1.0, -0.4])
    parts.append(("cone", (ls, lelbow, m["sleeve_r"], m["sleeve_r"] * 0.9), CLOAK))
    lwrist = lelbow + (lh - lelbow) * 0.6
    parts.append(("cone", (lelbow, lwrist, m["sleeve_r"] * 0.9, m["sleeve_r"] * 1.15), CLOAK))
    parts.append(("cone", (lwrist, lh, m["gauntlet_r"], m["gauntlet_r"] * 1.05), ARMOR))
    # Legs: armored shins showing under the cloak.
    step = pose["step"]
    for side, fwd in ((1, -step * 0.4), (-1, step)):
        hip = np.array([side * 3.2, m["pelvis"] - 2, 0]) + root
        foot = np.array([side * 3.6, 1.2, fwd + 1.0]) + np.array([root[0], 0, root[2]])
        knee = (hip + foot) / 2 + np.array([0, 0, 2.0])
        parts.append(("cone", (hip, knee, 2.8, 2.4), ARMOR))
        parts.append(("cone", (knee, foot, 2.4, 1.8), ARMOR))
        parts.append(("cone", (foot, foot + np.array([0, -0.4, 3.0]), 1.6, 1.3), ARMOR))
    # Sword.
    flat = norm(np.cross(sword, [0.0, 1.0, 0.0] if abs(sword[1]) < 0.9 else [1.0, 0.0, 0.0]))
    guard_axis = norm(np.cross(sword, flat))
    parts.append(("cone", (rh - sword * 3.8, rh + sword * 2.2, 0.75, 0.75), HILT))
    parts.append(("sphere", (rh - sword * 4.3, 1.1), HILT))
    parts.append(("box", (rh + sword * 2.6, (sword, guard_axis, flat), (0.6, 4.2, 0.7)), HILT))
    parts.append(("blade", (rh + sword * 2.8, sword, flat, m["blade_len"], m["blade_w"], 0.45), BLADE))
    refs = {"hand": rh, "tip": rh + sword * (2.8 + m["blade_len"]), "sword": sword,
            "face_fwd": upper @ norm([0, -0.15, 1.0]), "face_top": m["head"] + 1.0 + root[1], "head": head}
    return parts, refs


def eval_parts(parts, p):
    d = np.full(len(p), 1e9)
    mat = np.zeros(len(p), np.int8)
    for kind, args, material in parts:
        if kind == "cone":
            di = sd_round_cone(p, *args)
        elif kind == "frustum":
            di = sd_frustum(p, *args)
        elif kind == "sphere":
            di = sd_sphere(p, *args)
        elif kind == "box":
            di = sd_box(p, *args)
        else:
            di = sd_blade(p, *args)
        if material in (CLOAK, HOOD):
            # Cloth merges softly.
            soft = (mat == CLOAK) | (mat == HOOD)
            merged = np.where(soft, smin(d, di, 1.5), np.minimum(d, di))
        else:
            merged = np.minimum(d, di)
        mat = np.where(di < d, material, mat)
        d = merged
    return d, mat


# ---------------------------------------------------------------------------------------------
# Camera and rendering


def camera(facing):
    fx, fd = FORWARD[facing]
    # Columns: character x (right), y (up), z (forward) in world (X right, Y up, D toward camera).
    m = np.array([[-fd, 0, fx], [0, 1, 0], [fx, 0, fd]], float)
    v = np.array([0, -math.sin(PITCH), -math.cos(PITCH)])
    u = np.array([0, math.cos(PITCH), -math.sin(PITCH)])
    return m, v, u


def project(point, facing):
    m, v, u = camera(facing)
    w = m @ np.asarray(point, float)
    return float(w[0]), float(-(w @ u)), float(-(w @ v))  # screen x, screen y (down), depth toward camera


def render(model, pose, facing):
    """(lum, mat, cov) at screen pixels of VIEW, and the posed reference points."""
    parts, refs = build_parts(model, pose)
    m, v, u = camera(facing)
    x0, y0, x1, y1 = VIEW
    w, h = (x1 - x0) * SS, (y1 - y0) * SS
    sx = x0 + (np.arange(w) + 0.5) / SS
    sy = y0 + (np.arange(h) + 0.5) / SS
    gx, gy = np.meshgrid(sx, sy)
    gx, gy = gx.ravel(), gy.ravel()
    far = 140.0
    origin_w = np.outer(gx, [1, 0, 0]) + np.outer(-gy, u) - far * v
    # To character space: m is orthogonal.
    mt = m.T
    origin = origin_w @ mt.T
    direction = mt @ v
    t = np.zeros(len(gx))
    alive = np.ones(len(gx), bool)
    hit = np.zeros(len(gx), bool)
    # Skip empty space quickly: start near the model's bounding sphere.
    for _ in range(90):
        idx = np.nonzero(alive)[0]
        if not len(idx):
            break
        p = origin[idx] + np.outer(t[idx], direction)
        d, _ = eval_parts(parts, p)
        t[idx] += d * 0.9
        done = d < 0.05
        hit[idx[done]] = True
        alive[idx[done]] = False
        alive[idx[t[idx] > far * 2]] = False
    idx = np.nonzero(hit)[0]
    lum = np.zeros(len(gx))
    mat = np.full(len(gx), -1, np.int8)
    if len(idx):
        p = origin[idx] + np.outer(t[idx], direction)
        _, mt_hit = eval_parts(parts, p)
        e = 0.08
        grad = np.zeros((len(idx), 3))
        for k in range(3):
            off = np.zeros(3)
            off[k] = e
            grad[:, k] = eval_parts(parts, p + off)[0] - eval_parts(parts, p - off)[0]
        n_c = grad / np.maximum(np.linalg.norm(grad, axis=1, keepdims=True), 1e-9)
        n_w = n_c @ m.T  # to world
        light = norm([-0.55, 0.75, 0.45])
        diffuse = np.clip(n_w @ light, 0, 1)
        view_dir = -v
        rim = np.clip(1 - np.abs(n_w @ view_dir), 0, 1) ** 3
        refl = 2 * np.outer(n_w @ light, np.ones(3)) * n_w - light
        spec = np.clip(refl @ view_dir, 0, 1) ** 12
        albedo = np.array(model["albedo"])[mt_hit]
        shine = np.array(model["shine"])[mt_hit]
        # Cloth folds: streaks around the cloak's vertical axis, plus grain.
        ang = np.arctan2(p[:, 0], p[:, 2])
        folds = 0.5 + 0.5 * np.sin(ang * 9 + p[:, 1] * 0.25 + 1.3 * np.sin(p[:, 1] * 0.31 + ang * 2))
        cloth = (mt_hit == CLOAK) | (mt_hit == HOOD)
        tex = np.where(cloth, 1.0 - model["fold"] / 2 + model["fold"] * folds, 1.0)
        grain = (hash_noise(np.round(p[:, 0] * 1.3), np.round(p[:, 1] * 1.3 + p[:, 2])) - 0.5) * 0.18
        value = albedo * tex * (0.28 + 0.85 * diffuse) + shine * spec + 0.25 * rim * albedo + grain * albedo
        lum[idx] = value
        mat[idx] = mt_hit
        # The face opening: hood points whose normal looks along the hood's forward.
        rel = p - refs["head"]
        rel_n = rel / np.maximum(np.linalg.norm(rel, axis=1, keepdims=True), 1e-9)
        face = (mt_hit == HOOD) & ((rel_n @ refs["face_fwd"]) > 0.5)
        lum[idx[face]] = 0.0
        mat[idx[face]] = VOID
    lum = lum.reshape(h, w)
    mat = mat.reshape(h, w)
    return lum, mat, refs


def palette_lum(pal):
    return np.array([(0.3 * r + 0.59 * g + 0.11 * b) / 255 for r, g, b in pal])


def to_canvas(model, lum, mat, pal, extra=None):
    """Supersampled render to palette indexes on the full canvas (0 empty)."""
    x0, y0, x1, y1 = VIEW
    h, w = (y1 - y0), (x1 - x0)
    cov = (mat >= 0).reshape(h, SS, w, SS).mean(axis=(1, 3))
    lsum = np.where(mat >= 0, lum, 0).reshape(h, SS, w, SS).sum(axis=(1, 3))
    n = (mat >= 0).reshape(h, SS, w, SS).sum(axis=(1, 3))
    avg = lsum / np.maximum(n, 1)
    blade = (mat == BLADE).reshape(h, SS, w, SS).sum(axis=(1, 3))
    opaque = (cov >= 0.42) | (blade >= 2)
    plum = palette_lum(pal)
    ramp = list(range(1, 16))
    # Ordered dither for a pre-rendered grain.
    bayer = np.array([[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]]) / 16.0 - 0.5
    dither = np.tile(bayer, (h // 4 + 1, w // 4 + 1))[:h, :w] * model["dither"]
    target = np.clip(avg + dither, 0, 1)
    choice = np.array(ramp)[np.argmin(np.abs(target[..., None] - plum[ramp][None, None, :]), axis=2)]
    out = np.zeros((h, w), np.int32)
    out[opaque] = choice[opaque]
    # Dark edge where the figure meets the background (silhouette readability).
    pad = np.pad(opaque, 1)
    edge = opaque & ~(pad[:-2, 1:-1] & pad[2:, 1:-1] & pad[1:-1, :-2] & pad[1:-1, 2:])
    blade_px = blade >= 2
    out[edge & ~blade_px] = np.maximum(out[edge & ~blade_px], model["edge_index"])
    canvas = np.zeros((CANVAS, CANVAS), np.int32)
    canvas[OY + y0:OY + y1, OX + x0:OX + x1] = out
    return canvas


def draw_smear(model, anim, tick, facing):
    """Levels 1..3 (outer to edge) of the band the blade swept since the last tick, and its depth."""
    from PIL import Image, ImageDraw

    path = blade_path(model, anim, tick, facing)
    img = Image.new("L", (CANVAS * 2, CANVAS * 2), 0)
    d = ImageDraw.Draw(img)
    for f0, level in ((0.4, 1), (0.66, 2), (0.9, 3)):
        for (h0, t0), (h1, t1) in zip(path, path[1:]):
            quad = []
            for h, t in ((h0, t0), (h1, t1)):
                quad.append((h[0] + (t[0] - h[0]) * f0, h[1] + (t[1] - h[1]) * f0))
                quad.append((t[0], t[1]))
            poly = [quad[0], quad[1], quad[3], quad[2]]
            d.polygon([((OX + x) * 2, (OY + y) * 2) for x, y in poly], fill=level)
    depth = sum(t[2] for _, t in path) / len(path)
    return np.array(img.resize((CANVAS, CANVAS), Image.NEAREST)), depth


def load_puppet(hero):
    path = K.anims_path(hero).with_suffix("") / "puppet.json"
    return path, json.loads(path.read_text())


def frame_name(anim, tick):
    return f"pp_{anim}_{tick:02d}"


def render_facing(job):
    """Render one facing of an anim to PNGs. Returns that facing's hitboxes, one per hit tick."""
    model, anim, name, facing, pal, out_dir = job
    from PIL import Image

    pal_arr = np.array(pal, np.uint8)
    cols = [None] + [_hex(c) for c in model["smear_colors"]]
    hits = []
    for tick in range(anim["ticks"]):
        pose = pose_at(model, anim, tick)
        lum, mat, refs = render(model, pose, facing)
        canvas = to_canvas(model, lum, mat, pal)
        rgba = np.zeros((CANVAS, CANVAS, 4), np.uint8)
        rgba[..., :3] = pal_arr[canvas]
        rgba[..., 3] = np.where(canvas > 0, 255, 0)
        if tick > 0 and tick in anim.get("smear", []):
            band, depth = draw_smear(model, anim, tick, facing)
            _, _, dbody = project(np.array([0, 30, 0]) + pose["root"], facing)
            keep = _blade_pixels(canvas, pal) if depth > dbody else canvas > 0
            for level in (1, 2, 3):
                sel = (band == level) & ~keep
                rgba[sel, :3] = cols[level]
                rgba[sel, 3] = 255
        if tick in anim.get("burst", []):
            _burst(rgba, refs, facing, model)
        if tick in anim.get("hit", []):
            hits.append(_hitbox(model, anim, tick, facing))
        Image.fromarray(rgba, "RGBA").save(out_dir / f"{frame_name(name, tick)}_f{facing}.png")
    return facing, hits


def render_anim(hero, name, facings=range(5), out_dir=None, pal=None):
    from concurrent.futures import ProcessPoolExecutor

    path, data = load_puppet(hero)
    model, anim = data["model"], data["anims"][name]
    out_dir = out_dir or path.parent
    jobs = [(model, anim, name, f, pal, out_dir) for f in facings]
    with ProcessPoolExecutor(max_workers=len(jobs)) as pool:
        return dict(pool.map(render_facing, jobs))


def blade_path(model, anim, tick, facing, steps=8):
    """The blade (hand, tip) in screen space at sub-tick steps from the last tick to this one."""
    out = []
    for i in range(steps + 1):
        refs = build_parts(model, pose_at(model, anim, tick - 1 + i / steps))[1]
        out.append((project(refs["hand"], facing), project(refs["tip"], facing)))
    return out


def _hex(c):
    c = c.lstrip("#")
    return np.array([int(c[i:i + 2], 16) for i in (0, 2, 4)], np.uint8)


def _blade_pixels(canvas, pal):
    lum = palette_lum(pal)
    return (canvas > 0) & (lum[canvas] > 0.62)


def _burst(rgba, refs, facing, model):
    x, y, _ = project(refs["tip"], facing)
    cx, cy = OX + x, OY + y
    cols = [_hex(c) for c in model["smear_colors"]]
    for r, col in ((5, cols[0]), (3.2, cols[1]), (1.6, cols[2])):
        for yy in range(int(cy - r - 1), int(cy + r + 2)):
            for xx in range(int(cx - r - 1), int(cx + r + 2)):
                dx, dy = xx - cx, yy - cy
                # A four-point star.
                if (abs(dx) * abs(dy) < r * 0.9 and math.hypot(dx, dy) <= r * 1.6) or math.hypot(dx, dy) <= r * 0.6:
                    if 0 <= xx < CANVAS and 0 <= yy < CANVAS:
                        rgba[yy, xx, :3] = col
                        rgba[yy, xx, 3] = 255


def _hitbox(model, anim, tick, facing):
    """[xMin, xMax, yMin, yMax] around the outer blade swept over the hit tick and the one before."""
    xs, ys = [], []
    for h, t in blade_path(model, anim, tick, facing, 6):
        for f in (0.4, 1.0):
            xs.append(h[0] + (t[0] - h[0]) * f)
            ys.append(h[1] + (t[1] - h[1]) * f)
    pad = 3
    return [int(min(xs) - pad), int(max(xs) + pad), int(min(ys) - pad), int(max(ys) + pad)]


def hero_palette(hero):
    import anim_studio as A

    st = A.Studio()
    return st.palette(st.hero(hero))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    r = sub.add_parser("render")
    r.add_argument("hero")
    r.add_argument("anims", nargs="*")
    r.add_argument("--facing", type=int, action="append")
    s = sub.add_parser("sheet")
    s.add_argument("hero")
    s.add_argument("anims", nargs="+")
    s.add_argument("--scale", type=int, default=2)
    s.add_argument("--with", dest="extra", action="append", default=[], help="a stock frame column, e.g. Walk:0")
    g = sub.add_parser("gif", help="real-speed GIF: one row per facing, the hero's idle beside it")
    g.add_argument("hero")
    g.add_argument("anims", nargs="+")
    g.add_argument("--scale", type=int, default=3)
    g.add_argument("--idle", default="Walk:0")
    w = sub.add_parser("write")
    w.add_argument("hero")
    args = ap.parse_args()
    pal = hero_palette(args.hero)
    path, data = load_puppet(args.hero)
    if args.cmd == "render":
        names = args.anims or list(data["anims"])
        all_hits = {}
        for name in names:
            hits = render_anim(args.hero, name, args.facing or range(5), pal=pal)
            all_hits[name] = hits
            print(f"{name}: {data['anims'][name]['ticks']} ticks rendered, hits {hits}")
        cache = path.parent / "puppet_hits.json"
        old = json.loads(cache.read_text()) if cache.exists() else {}
        for name, hits in all_hits.items():
            old.setdefault(name, {}).update({str(f): v for f, v in hits.items()})
        cache.write_text(json.dumps(old, indent=1))
    elif args.cmd == "sheet":
        sheet(args.hero, args.anims, args.scale, args.extra, pal)
    elif args.cmd == "gif":
        gif(args.hero, args.anims, args.scale, args.idle, pal)
    elif args.cmd == "write":
        write(args.hero, data)


def sheet(hero, names, scale, extra, pal):
    from PIL import Image, ImageDraw
    import anim_studio as A

    path, data = load_puppet(hero)
    folder = path.parent
    x0, y0, x1, y1 = -44, -84, 52, 22
    cw, chh = (x1 - x0) * scale, (y1 - y0) * scale
    cols = max(data["anims"][n]["ticks"] for n in names) + len(extra)
    rows = 5 * len(names)
    img = Image.new("RGB", (cw * cols, chh * rows), (128, 134, 148))
    d = ImageDraw.Draw(img)
    st = A.Studio() if extra else None
    for ni, name in enumerate(names):
        for facing in range(5):
            row = ni * 5 + facing
            items = []
            for ref in extra:
                px = st.lib.pixels(ref, "WitchKing", facing)
                items.append((ref, K.image(px, pal, 1)))
            for tick in range(data["anims"][name]["ticks"]):
                p = folder / f"{frame_name(name, tick)}_f{facing}.png"
                if p.exists():
                    items.append((f"{tick}", Image.open(p).convert("RGBA")))
            for c, (label, im) in enumerate(items):
                crop = im.crop((OX + x0, OY + y0, OX + x1, OY + y1)).resize((cw, chh), Image.NEAREST)
                img.paste(crop, (c * cw, row * chh), crop)
                d.text((c * cw + 2, row * chh + 1), f"{label} f{facing}", fill=(255, 255, 0))
                ox, oy = (-x0) * scale + c * cw, (-y0) * scale + row * chh
                d.point([(ox, oy)], fill=(255, 0, 0))
    out = ROOT / "build/anim" / f"puppet_{'_'.join(names)}.png"
    out.parent.mkdir(parents=True, exist_ok=True)
    img.save(out)
    print(out)


def gif(hero, names, scale, idle, pal):
    """The anims played back to back at game speed (a tick is 2 screen frames), idle frames between."""
    from PIL import Image, ImageDraw
    import anim_studio as A

    path, data = load_puppet(hero)
    folder = path.parent
    st = A.Studio()
    x0, y0, x1, y1 = -44, -86, 52, 22
    cw, ch = (x1 - x0) * scale, (y1 - y0) * scale
    idles = [K.image(st.lib.pixels(idle, "WitchKing", f), pal, 1) for f in range(5)]
    seq = [None] * 8
    for name in names:
        seq += [(name, t) for t in range(data["anims"][name]["ticks"])]
    seq += [None] * 8
    frames = []
    for item in seq:
        img = Image.new("RGB", (cw * 2, ch * 5), (128, 134, 148))
        d = ImageDraw.Draw(img)
        for f in range(5):
            ref = idles[f].crop((OX + x0, OY + y0, OX + x1, OY + y1)).resize((cw, ch), Image.NEAREST)
            img.paste(ref, (0, f * ch), ref)
            cur = idles[f] if item is None else                 Image.open(folder / f"{frame_name(item[0], item[1])}_f{f}.png").convert("RGBA")
            cur = cur.crop((OX + x0, OY + y0, OX + x1, OY + y1)).resize((cw, ch), Image.NEAREST)
            img.paste(cur, (cw, f * ch), cur)
            d.text((2, f * ch + 2), f"stock idle f{f}", fill=(255, 255, 0))
        frames.append(img)
    out = ROOT / "build/anim" / f"puppet_{'_'.join(names)}.gif"
    frames[0].save(out, save_all=True, append_images=frames[1:], duration=33, loop=0)
    print(out)


def write(hero, data):
    """Puppet anims into the hero's anim file as png frames (one tick each)."""
    path, _ = load_puppet(hero)
    hits = json.loads((path.parent / "puppet_hits.json").read_text())
    anims = K.load(hero)
    for name, anim in data["anims"].items():
        frames = []
        for tick in range(anim["ticks"]):
            entry = {"png": frame_name(name, tick), "like": data.get("like", "Walk:0")}
            if tick in anim.get("hit", []):
                k = anim["hit"].index(tick)
                boxes = {f: hits[name][f][k] for f in hits.get(name, {}) if k < len(hits[name][f])}
                if "2" in boxes:
                    entry["hit"] = boxes["2"]
                    entry["facings"] = {f: {"hit": b} for f, b in boxes.items() if f != "2"}
            frames.append(entry)
        anims["anims"][f"pp_{name}"] = {"speed": 1, "note": "puppet3d.py render (do not edit frames by hand)",
                                        "frames": frames}
    out = K.save(hero, anims)
    print(f"Wrote {len(data['anims'])} anims to {out}")


if __name__ == "__main__":
    main()
