"""Compile hero skill specs (hacks/content/heroes/<hero>.json) into kits for spec_kit.c.

A spec lists the hero's first page (6 skills, the last one in Herbal Healing's slot) and 9 stock
passives, and optionally page 2: 5 or 6 skills plus 9 more stock passives (page2Passives). Each skill is one of:

  bolt     a projectile: damage, speed, radius, pierce, splash, stun, status
  burst    a burst around the caster: damage, radius, stun, status
  blast    a burst at the nearest foe within range: damage, radius, range, stun, status
  heal     heal the caster: amount (HP per level)
  ward     take less damage: amount (percent per level), seconds
  empower  melee hits deal more: amount (percent per level), seconds
  summon   count allies fight beside the caster for seconds: amount (damage per hit)
  devour   kill the nearest foe within range at threshold percent HP or less: amount (HP healed),
           killCredit (kills toward the next rebirth charge)
  tithe    pay amount percent of max HP for one rebirth charge
  charge   a rider ("anim": "Enemy/Clip", "color", "speed" px per tick, "radius") charges along the
           caster's facing for seconds: amount (damage to each foe it runs over), stun (seconds)
  lash     a whip line along the facing, range long and radius to each side: amount, stun
  shove    kick a cone of foes within radius away (speed) and leave them down: amount, stun
  taunt    foes within radius flinch (stun); amount percent less damage taken per foe, at most
           percent, for seconds
  haste    walk amount percent faster and gain percent Spirit a second, for seconds
  secondwind  heal amount, plus amount per foe within radius (up to 5)
  cleave   for seconds each melee hit also strikes the other foes within radius for amount percent of it
  reflect  for seconds a foe that strikes him takes amount percent of the blow back (trail on it)
  dread    for seconds, every trailTicks (default 30), foes within radius cower for stun
  undying  for seconds no blow takes him below 1 HP, then he heals amount (trail plays)
  absorb   the next amount damage within seconds is soaked up
  lifesteal for seconds his melee hits heal him amount percent of their damage
  rampart  for seconds blows from foes farther than radius lose amount percent
  madness  for seconds his melee hits deal amount percent more, and he takes percent more damage
  grapple  hook the nearest foe within range (trail along the rope) and haul himself to it at speed:
           amount, stun on arrival
  flurry   the nearest foe within range takes count cuts of amount, one every trailTicks (trail each)
  blind    foes within radius take amount, and their blows on the players miss percent of the time
           for seconds
  plunder  for seconds each kill drops loot amount percent of the time (trail at the drop)
  bomb     a keg (trail, held) blows after seconds: amount and stun to foes within radius, target
           layers at the blast
  broadside count blasts walk out along the facing, spread px apart, one every trailTicks: amount and
           stun within radius of each, target layers at each
  pierce   a bolt flies range along the facing (trail in flight): foes within radius of its line take
           amount and stun
  fan      count bolts spread around the facing, each a pierce
  cover    a shield (trail, held) planted ahead for seconds: within radius of it he takes amount
           percent less damage
  ignite   a burning bolt (trail in flight) hits the nearest foe in range: amount (and status). If it
           dies within seconds, foes within radius of it take the same (target layers)
  aim      for seconds each second standing still adds amount percent damage, at most percent
           (trail each second)
  hail     for seconds, every trailTicks (default 8), a bolt drops within radius of where he stood:
           amount and stun to foes near it, target layers there
  swoop    a flyer ("anim": "Enemy/Clip", "color") dives from the sky onto the nearest foe within
           range over seconds, plays "strike" (Enemy/Clip) there and bursts like a blast: damage,
           radius, stun. Its "target" fx layers play on arrival.

charge and swoop may leave "trail" (one fx layer) every "trailTicks" frames. A summon's "spread"
is the pixels between the caster and each ally (default 22): far allies walk in.

heal, ward and empower may add "burst": {"damage": [5], "radius", "stun", "knockback", "impact",
"sound"} around the caster. Any skill may add "rallySeconds" (the caster's summons stay longer).
empower may add "status" (poison, burn, slow), "statusSeconds" and "statusPower": every melee hit
applies it while the empower lasts. Colors are FX_COLOR_* names or "OBJ:<ObjPalettes id>".

Every skill may have "castSound" (an sfx id or an SFX_* name) and "fx": up to 3 layers
{"shape": "GROUND_POOL", "color": "NAZGUL", "layer": "front" | "behind", "at": "self" | "target",
"seconds": n (loop that long, default once), "delay": seconds (wait, at the spot it would have
played)}. "target" is the blast's foe, each summoned ally, the devoured foe or a swoop's strike point.
A layer may play a skill_fx.json effect instead: {"effect": "gondor_shield_wall", "seconds": 6 (hold its
last frame until then)}. A layer may play an enemy clip instead of a shape: {"anim": "GroundSkeleton/ExtraPose00",
"facing": 4, "ticks": 4, "seconds": 2 (hold the last frame until then), "color": "SHADOW", ...}.
A summon's "look": "GoblinScout" draws its allies as that enemy (walk, stand, attack, death), in
"lookColor" (an fx color) or the enemy's own palette.

A passive is a stock name (HARDY) or the hero's own, for heroes 8..15: {"name", "icon", "text",
"levels": [hero level per purchased level, 1 to 5 of them], "stats": {"STAT_MELEE_ARMOR": [5 per-level
increments], ...} (at most 3), "effect": one of PASSIVE_EFFECTS, "amount": [5 per-level values]}.
Effects change the spec engine: rebirthCharges (more charges), rebirthHeal (percent), chargeKills
(fewer kills per charge), summonSeconds, devourThreshold (percent), summonCount, bleedDamage (per
stack), leapChain (more foes), weakenPercent, finisherPercent, frenzyStacks.

A spec's "primary" block makes the melee swings shoot: {"frame": 5, "damage": [5 per level], "speed",
"pierce", "travel", "impact", "stun", "status", ...} is a bolt loosed from the hero along his facing when
each swing reaches that frame (level 1 + 1 per 5 hero levels). The swing clip needs no hit frame.

A spec's "rebirth" block makes the hero cheat death: {"charges": 3, "start": 1, "killsPerCharge": 15,
"heal": 40, "seconds": 2, "burstDamage": 20, "radius": 48, "stun": 1.0, "sound": n}. A killing blow
with a charge left leaves 1 HP and keeps the hero untouchable for seconds, then heals heal percent
and bursts. Kills earn charges back. The HUD shows the charges between the HP and Spirit bars.

Per-level values are lists of 5 (skill levels 1..5). Descriptions are written from "text" with
{damage}, {amount}, {seconds} and {stun} filled in.

The kits are linked apart from the modcode, into fixed per hero slots at MEM_HERO_KITS
(SkillExpansionKitSlot, hacks/layout.cfg). hero_registry.py writes their C (generate), the build
links it against the modcode's symbols (link), and hero_fast.py does both again against the last
built game, so a spec change needs no rebuild:

    uv run python hacks/tools/hero_kit.py link build/hack.elf build/hero_kits.bin
"""

from __future__ import annotations

import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

SOUNDS = {"none": 0, "missile": 0x13, "fireburst": 0x0D, "stun": 0x09}
STATUSES = ("none", "poison", "burn", "slow")
CASTS = {"quick": "SpecKitQuickPolicy", "normal": "SpecKitNormalPolicy", "long": "SpecKitLongPolicy",
         "ultimate": "SpecKitUltimatePolicy"}
EFFECTS = {"bolt": 0, "burst": 0, "blast": 0, "heal": 1, "ward": 2, "empower": 3, "summon": 4, "devour": 5,
           "tithe": 6, "charge": 7, "swoop": 8, "fear": 9, "phase": 10, "zone": 11, "drain": 12, "root": 13,
           "stampede": 14, "charm": 15, "mark": 16, "dismay": 17, "dash": 18, "bounty": 19, "barrage": 20,
           "despair": 21, "leap": 22, "weaken": 23, "bleed": 24, "venom": 25, "finisher": 26, "frenzy": 27,
           "trap": 28, "backstep": 29, "pull": 30, "sleep": 31, "lash": 32, "shove": 33, "taunt": 34,
           "haste": 35, "secondwind": 36, "cleave": 37, "reflect": 38, "dread": 39, "undying": 40, "absorb": 41,
           "lifesteal": 42, "rampart": 43, "madness": 44, "grapple": 45, "flurry": 46, "blind": 47, "plunder": 48,
           "bomb": 49, "broadside": 50, "pierce": 51, "fan": 52, "cover": 53, "ignite": 54, "aim": 55, "hail": 56}
# Effects that act on one foe within "range".
RANGED_EFFECTS = ("blast", "devour", "swoop", "charm", "barrage", "leap", "finisher", "pull", "sleep", "lash", "grapple", "flurry", "pierce", "fan", "ignite")
# Effects with a reach ("radius", 8..160) and an optional per-foe "amount" and "status".
AREA_EFFECTS = ("fear", "phase", "zone", "drain", "root", "charm", "mark", "dismay", "dash", "barrage", "leap",
                "weaken", "trap", "backstep", "pull", "sleep", "finisher", "lash", "shove", "taunt", "secondwind",
                "cleave", "dread", "rampart", "grapple", "blind", "bomb", "broadside",
                "pierce", "fan", "cover", "ignite", "hail")
# Effects with no target or area of their own: bites while they last, or Frenzy.
BUFF_EFFECTS = ("bleed", "venom", "frenzy")
# Effects whose look may leave a trail, pulse or impact layer.
TRAIL_EFFECTS = ("charge", "swoop", "stampede", "zone", "drain", "phase", "charm", "dash", "bounty", "barrage",
                 "venom", "trap", "backstep", "pull", "sleep", "lash", "cleave", "reflect", "dread", "undying", "grapple", "flurry", "plunder", "bomb", "pierce", "fan",
                 "cover", "ignite", "aim")
# Effects that take "percent" (spec_kit.h SpecSkill.percent).
PERCENT_EFFECTS = {"drain": 50, "mark": 50, "dismay": 5, "bounty": 5, "despair": 15, "weaken": 50, "finisher": 30,
                   "taunt": 60, "haste": 2, "madness": 25, "blind": 50, "aim": 60}
# Effects that need seconds besides the area ones.
TIMED_EFFECTS = ("ward", "empower", "summon", "charge", "stampede", "bounty", "despair", "bleed", "venom",
                 "frenzy", "haste", "reflect", "undying", "absorb", "lifesteal", "madness", "plunder", "aim")
# Instant effects: no seconds.
INSTANT_EFFECTS = ("dismay", "leap", "finisher", "lash", "shove", "secondwind", "grapple", "broadside", "pierce", "fan")
LAYERS = {"front": "FX_LAYER_FRONT", "behind": "FX_LAYER_BEHIND"}
PASSIVE_EFFECTS = {"stats": 0, "rebirthCharges": 1, "rebirthHeal": 2, "chargeKills": 3, "summonSeconds": 4,
                   "devourThreshold": 5, "summonCount": 6, "bleedDamage": 7, "leapChain": 8,
                   "weakenPercent": 9, "finisherPercent": 10, "frenzyStacks": 11}
PASSIVE_FIELDS = {"name", "icon", "text", "levels", "stats", "effect", "amount"}
SPEC_PASSIVE_FIRST, SPEC_PASSIVE_FIRST_HERO = 160, 8
# Kit slots at MEM_HERO_KITS (SKILL_EXPANSION_KIT_SLOTS in skill_expansion.h).
KIT_SLOTS = 64
REBIRTH_FIELDS = {"charges", "start", "killsPerCharge", "heal", "seconds", "burstDamage", "radius", "stun", "sound"}
SKILL_FIELDS = {"name", "cost", "levels", "cast", "icon", "text", "effect", "fx", "damage", "speed",
                "radius", "pierce", "splash", "stun", "status", "statusSeconds", "statusPower", "travel",
                "impact", "sound", "range", "amount", "seconds", "lifetime", "knockback", "count", "threshold",
                "killCredit", "castSound", "look", "lookColor", "burst", "rallySeconds", "anim", "color",
                "strike", "trail", "trailTicks", "spread", "percent", "stackSeconds"}
FRAMES = 30
SOURCE = Path("build/generated/hero_spec_kits.c")
# The modcode's compiler flags, written by the build (make/hack.mk) so the kits compile the same way.
CFLAGS = Path("build/modcode_cflags.txt")
TOOLS = Path(os.environ.get("LOCALAPPDATA", "")) / "rotk-tools/arm-gcc/bin"
# Spec places already warned about a shared palette (each warns once per run).
WARNED: set[str] = set()


class SpecError(ValueError):
    pass


def slug(name: str) -> str:
    return re.sub(r"[^A-Za-z0-9]", "", name.title())


def spec_path(root: Path, name: str) -> Path:
    return root / "hacks/content/heroes" / f"{slug(name).lower()}.json"


def require(condition: bool, where: str, message: str) -> None:
    if not condition:
        raise SpecError(f"{where}: {message}")


def levels5(value, where: str, field: str, low: int, high: int) -> list[int]:
    require(isinstance(value, list) and len(value) == 5 and all(isinstance(v, int) for v in value),
            where, f"{field} needs 5 numbers, one per skill level")
    require(all(low <= v <= high for v in value), where, f"{field} values must be {low}..{high}")
    return value


def catalog(root: Path) -> dict:
    """Names a spec may use: icons, fx shapes and colors, delivery effects, passives."""
    icons = {}
    for header in (root / "hacks/patches").glob("*/*_icons.h"):
        for symbol in re.findall(r"extern const \w+ (\w+Icon\w+)", header.read_text()):
            icons[symbol] = header.relative_to(root / "hacks").as_posix()
    fx = (root / "hacks/lib/fx_kit.h").read_text()
    effects = {e["key"]: e["id"] for e in json.loads((root / "hacks/content/skill_fx.json").read_text())["effects"]}
    passives = set(re.findall(r"PASSIVE_SKILL_(\w+) = \d+,", (root / "include/skill.h").read_text()))
    sfx = set(re.findall(r"\b(SFX_\w+) =", (root / "include/sfx.h").read_text()))
    stats = set(re.findall(r"\b(STAT_\w+) =", (root / "include/stats.h").read_text()))
    return {
        "icons": icons,
        "shapes": set(re.findall(r"FX_SHAPE_(\w+) = ", fx)),
        "colors": set(re.findall(r"FX_COLOR_(\w+)\b", fx)) - {"CUSTOM", "OBJ_PALETTE"},
        "effects": effects,
        "passives": passives - {"COUNT"},
        "sfx": sfx,
        "stats": stats,
        "root": root,
    }


def sound_id(value, where: str, names: dict) -> str:
    """An sfx id as C: a number or an SFX_* name."""
    if value is None:
        return "0"
    if isinstance(value, int):
        require(0 <= value <= 0xffff, where, "sound ids are 0..65535")
        return str(value)
    require(value in names["sfx"], where, "sound must be an sfx id or an SFX_* name from include/sfx.h")
    return value


def enemy_type(root: Path, name: str, where: str) -> int:
    """ENEMY_TYPE_* id of an enemy named as in anim_studio.py enemies (GoblinScout)."""
    snake = re.sub(r"(?<=[a-z0-9])(?=[A-Z])", "_", name).upper()
    match = re.search(rf"ENEMY_TYPE_{snake} = (\d+),", (root / "include/enemy.h").read_text())
    require(match is not None, where, f"unknown enemy {name!r} (anim_studio.py enemies lists them)")
    return int(match.group(1))


def clip_address(root: Path, ref: str, facing: int, where: str) -> int:
    """ROM address of one facing's SpriteAnimation record of "Enemy/Clip"."""
    require(isinstance(ref, str) and ref.count("/") == 1, where, 'anim is "Enemy/Clip", e.g. "GroundSkeleton/ExtraPose00"')
    enemy, clip = ref.split("/")
    symbol = f"EnemyAnimationBank_EnemyType{enemy}_{clip}"
    for line in (root / "carved/config/enemy_graphics.cfg").read_text().splitlines():
        parts = line.split()
        if len(parts) >= 2 and parts[1] == symbol:
            require(0 <= facing <= 4, where, "facing is 0 (up) .. 4 (down)")
            return int(parts[0], 16) + 16 * facing
    raise SpecError(f"{where}: no clip {ref} (anim_studio.py clips {enemy})")


def fx_color(value, where: str, names: dict) -> str:
    """An fx color as C: a FX_COLOR_* name or "OBJ:<ObjPalettes id>"."""
    if isinstance(value, str) and value.startswith("OBJ:"):
        ident = int(value[4:])
        require(0 <= ident < 90, where, "OBJ palette ids are 0..89")
        if not 2 <= ident <= 8 and where not in WARNED:
            WARNED.add(where)
            print(f"warning: {where}: OBJ:{ident} borrows a shared palette slot, which enemies also use; in busy "
                  "fights its colors can scramble or the effect can fail to show. Prefer OBJ:2..8 (always "
                  "loaded): hero_preview.py --enemy E --clip C --all-palettes", file=sys.stderr)
        return f"FX_COLOR_OBJ_PALETTE({ident})"
    require(value in names["colors"], where, f"unknown color {value!r} (FX_COLOR_* or OBJ:<id>)")
    return f"FX_COLOR_{value}"


def fx_layer(fx, where: str, names: dict) -> str:
    """One fx layer as a SpecFx initializer."""
    require(isinstance(fx, dict) and set(fx) <= {"shape", "anim", "effect", "facing", "ticks", "color", "layer",
                                                 "at", "seconds", "delay"}, where,
            "an fx layer is {shape, anim or effect, color, layer, at, seconds, delay, facing, ticks}")
    require(sum(key in fx for key in ("shape", "anim", "effect")) == 1, where,
            "an fx layer has one of shape, anim or effect")
    effect = 0
    if "effect" in fx:
        require(fx["effect"] in names["effects"], where,
                f"fx effect must be a skill_fx.json key: {', '.join(names['effects'])}")
        require(names["effects"][fx["effect"]] < 255, where, "fx effect ids above 254 do not fit")
        effect = names["effects"][fx["effect"]] + 1
        color = "0"
    else:
        require(fx.get("shape", "SPARK") in names["shapes"] and "color" in fx, where,
                "fx needs a shape (FX_SHAPE_*) and a color (FX_COLOR_* or OBJ:<id>) from hacks/lib/fx_kit.h")
        color = fx_color(fx["color"], where, names)
    delay = int(round(fx.get("delay", 0) * FRAMES))
    require(0 <= delay <= 255, where, "fx delay 0..8.5 seconds")
    anim = "NULL"
    if "anim" in fx:
        anim = f"(const SpriteAnimation *)0x{clip_address(names['root'], fx['anim'], fx.get('facing', 4), where):08x}"
    ticks = fx.get("ticks", 0)
    require(0 <= ticks <= 30, where, "fx ticks 0..30")
    require(fx.get("layer", "front") in LAYERS, where, "fx layer is front or behind")
    require(fx.get("at", "self") in ("self", "target"), where, "fx at is self or target")
    frames = int(fx.get("seconds", 0) * FRAMES)
    require(0 <= frames <= 65535, where, "fx seconds out of range")
    return (f"{{.anim = {anim}, .color = {color}, .frames = {frames}, "
            f".shape = FX_SHAPE_{fx.get('shape', 'SPARK')}, .layer = {LAYERS[fx.get('layer', 'front')]}, "
            f".flags = {int(fx.get('at') == 'target')}, .ticks = {ticks}, .effect = {effect}, .delay = {delay}}}")


def fx_layers(value, where: str, names: dict) -> str:
    """A skill's "fx" (one layer or a list of up to 3) as a SpecFx[SPEC_FX_COUNT] initializer."""
    layers = [] if value is None else value if isinstance(value, list) else [value]
    require(len(layers) <= 3, where, "fx takes at most 3 layers")
    return "{" + ", ".join([fx_layer(fx, where, names) for fx in layers] or ["{0}"]) + "}"


def own_passive(passive: dict, where: str, names: dict, symbol: str, hero_id: int, k: int) -> tuple[str, str, str]:
    """A spec's own passive: its SpecPassive definition, its address and its passive id."""
    at = f"{where} passive {passive.get('name', k)}"
    require(hero_id >= SPEC_PASSIVE_FIRST_HERO, at, "own passives exist for heroes 8 and up")
    require(set(passive) <= PASSIVE_FIELDS, at, f"unknown fields {sorted(set(passive) - PASSIVE_FIELDS)}")
    for key in ("name", "icon", "text", "levels"):
        require(key in passive, at, f"needs {key}")
    require(passive["icon"] in names["icons"], at, "unknown icon (any *Icon* symbol in hacks/patches/*/*_icons.h)")
    levels = passive["levels"]
    require(isinstance(levels, list) and 1 <= len(levels) <= 5 and levels == sorted(levels) and
            all(isinstance(v, int) and 1 <= v <= 50 for v in levels), at, "levels: 1 to 5 rising hero levels 1..50")
    stats = passive.get("stats", {})
    require(isinstance(stats, dict) and len(stats) <= 3 and all(st in names["stats"] for st in stats), at,
            "stats: at most 3 STAT_* names from include/stats.h")
    records = []
    for stat, values in stats.items():
        records.append(f"{{.statIndex = {stat}, .perLevel = {{{', '.join(map(str, levels5(values, at, stat, 0, 999)))}}}}}")
    records += ["{.statIndex = STAT_NONE}"] * (3 - len(records))
    effect = passive.get("effect", "stats")
    require(effect in PASSIVE_EFFECTS, at, f"effect must be one of {', '.join(PASSIVE_EFFECTS)}")
    amount = levels5(passive["amount"], at, "amount", 0, 255) if effect != "stats" else [0] * 5
    text = passive["text"]
    for key, values in (("amount", amount),):
        span = str(values[0]) if values[0] == values[-1] else f"{values[0]}-{values[-1]}"
        text = text.replace("{" + key + "}", span)
    description = f"{passive['name']} (Level @1) - {text}"
    require(len(description) <= 160, at, "description too long for the skill tree")
    name = f"{symbol}Passive{k}"
    required = levels + [-1] * (6 - len(levels))
    body = (f"static const SpecPassive {name} = {{.def = {{.skill = {{.records = {{{', '.join(records)}}}, "
            f".initialRequiredLevel = {levels[0]}, .maxLevel = {len(levels)}}}, "
            f".requiredLevels = {{{', '.join(map(str, required))}}}, .iconPassiveSkillId = PASSIVE_SKILL_HARDY, "
            f".icon = {passive['icon']}, .description = {json.dumps(description)}}}, "
            f".effect = {PASSIVE_EFFECTS[effect]}, .amount = {{{', '.join(map(str, amount))}}}}};")
    # The kit's id names the slot only. skill_expansion.c buildTables adds 9 per player index, and
    # spec_kit.c resolves the id through that player's hero.
    ident = SPEC_PASSIVE_FIRST + k
    return body, f"&{name}", str(ident)


def rebirth(spec: dict, where: str, names: dict, symbol: str) -> tuple[str, str]:
    """The spec's rebirth block as a SpecRebirth definition and its address, or ("", "NULL")."""
    block = spec.get("rebirth")
    if block is None:
        return "", "NULL"
    at = f"{where} rebirth"
    require(isinstance(block, dict) and set(block) <= REBIRTH_FIELDS, at,
            f"rebirth keys are {', '.join(sorted(REBIRTH_FIELDS))}")
    charges, start, per = block.get("charges", 3), block.get("start", 1), block.get("killsPerCharge", 15)
    require(1 <= charges <= 5 and 0 <= start <= charges, at, "charges 1..5, start 0..charges")
    require(1 <= per and charges * per <= 255, at, "killsPerCharge at least 1, charges * killsPerCharge at most 255")
    heal, radius = block.get("heal", 40), block.get("radius", 48)
    require(1 <= heal <= 100 and 1 <= radius <= 255, at, "heal 1..100 percent, radius 1..255")
    frames, stun = int(block.get("seconds", 2) * FRAMES), int(block.get("stun", 1) * FRAMES)
    require(1 <= frames <= 600 and 0 <= stun <= 255, at, "seconds up to 20, stun up to 8")
    damage = block.get("burstDamage", 20)
    require(0 <= damage <= 9999, at, "burstDamage 0..9999")
    name = f"{symbol}Rebirth"
    text = (f"static const SpecRebirth {name} = {{.frames = {frames}, .burstDamage = {damage}, "
            f".sound = {sound_id(block.get('sound'), at, names)}, .maxCharges = {charges}, .startCharges = {start}, "
            f".killsPerCharge = {per}, .healPercent = {heal}, .burstRadius = {radius}, .burstStun = {stun}}};")
    return text, f"&{name}"


def describe(skill: dict) -> str:
    def span(values):
        return str(values[0]) if values[0] == values[-1] else f"{values[0]}-{values[-1]}"

    fills = {
        "damage": span(skill.get("damage", [0] * 5)),
        "amount": span(skill.get("amount", [0] * 5)),
        "seconds": str(skill.get("seconds", 0)),
        "stun": f"{skill.get('stun', 0)}s",
    }
    text = skill.get("text", "")
    for key, value in fills.items():
        text = text.replace("{" + key + "}", value)
    return f"{skill['name']} (Level @1) - {text} @2 Spirit."


def delivery(skill: dict, where: str, names: dict) -> str:
    effect = skill["effect"]
    damage = levels5(skill.get("damage"), where, "damage", 0, 9999)
    status = skill.get("status", "none")
    require(status in STATUSES, where, f"status must be one of {', '.join(STATUSES)}")
    power = levels5(skill.get("statusPower", [1] * 5 if status != "none" else [0] * 5), where, "statusPower", 0, 999)
    radius = skill.get("radius", 6 if effect == "bolt" else 48)
    require(1 <= radius <= 128, where, "radius must be 1..128")
    speed = skill.get("speed", 6 if effect == "bolt" else 0)
    require(effect != "bolt" or 1 <= speed <= 16, where, "a bolt needs speed 1..16")
    pierce = skill.get("pierce", 1)
    stun = int(skill.get("stun", 0) * FRAMES)
    for key in ("travel", "impact"):
        require(skill.get(key) is None or skill[key] in names["effects"], where,
                f"{key} must be one of {', '.join(names['effects'])}")
    sound = skill.get("sound", "missile" if effect == "bolt" else "fireburst")
    require(sound in SOUNDS, where, f"sound must be one of {', '.join(SOUNDS)}")
    travel = names["effects"].get(skill.get("travel", "preview_ward" if effect == "bolt" else None), 65535)
    impact = names["effects"].get(skill.get("impact", "preview_impact"), 65535)
    fields = {
        "damage": "{" + ",".join(map(str, damage)) + "}",
        "lifetime": skill.get("lifetime", 45 if effect == "bolt" else 1),
        "travelEffect": travel, "trailEffect": 65535, "impactEffect": impact,
        "launchSound": 0, "impactSound": SOUNDS[sound], "stunTicks": min(stun, 300),
        "kind": int(effect != "bolt"), "speed": speed, "radius": radius,
        "splashRadius": skill.get("splash", 0), "maxTargets": 8, "pierceTargets": pierce,
        "maxRepeats": 1, "repeatTicks": 15, "trailTicks": 6, "knockback": skill.get("knockback", 0),
        "stopOnWalls": 1, "elements": 0, "elementsKnown": 0, "status": STATUSES.index(status),
        "statusTicks": int(skill.get("statusSeconds", 0) * FRAMES), "statusPower": "{" + ",".join(map(str, power)) + "}",
        "cooldownTicks": 0,
    }
    require(status == "none" or fields["statusTicks"] > 0, where, "a status needs statusSeconds")
    return "{" + ", ".join(f".{k} = {v}" for k, v in fields.items()) + "}"


def compile_hero(root: Path, hero: dict, names: dict) -> tuple[str, set[str]]:
    path = spec_path(root, hero["name"])
    where = path.relative_to(root).as_posix()
    spec = json.loads(path.read_text())
    symbol = slug(hero["name"])
    hero_id = hero["id"]
    skills = spec.get("skills", [])
    page2 = spec.get("page2", [])
    require(len(skills) == 6, where, "skills needs 6 entries (the 6th takes Herbal Healing's slot)")
    # Page 2 is all or nothing: the skill tree shows it whole or not at all.
    require(len(page2) in (0, 5, 6), where, "page2 needs 5 or 6 skills, or none (no page 2)")
    passives = spec.get("passives", [])
    require(len(passives) == 9 and all(isinstance(p, dict) or p in names["passives"] for p in passives), where,
            "passives needs 9 entries: stock passive names (FEARLESS, HARDY, WISE) or the hero's own passives")
    page2_passives = spec.get("page2Passives", [])
    require(len(page2_passives) == (9 if page2 else 0), where,
            "page2Passives needs 9 stock passive names when page2 has skills, none otherwise")
    require(all(p in names["passives"] for p in page2_passives), where, "unknown page2Passives name")
    stock_names = [p for p in passives if isinstance(p, str)] + page2_passives
    require(len(set(stock_names)) == len(stock_names), where,
            "a passive appears twice (each page's passives must be distinct)")
    spec_keys = {"hero", "passives", "skills", "page2", "page2Passives", "rebirth", "frenzy", "primary", "note"}
    require(set(spec) <= spec_keys, where, f"unknown keys {sorted(set(spec) - spec_keys)}")
    headers = set()
    out = [f"// {hero['name']} ({where})"]
    rebirth_def, rebirth_ref = rebirth(spec, where, names, symbol)
    if rebirth_def:
        out.append(rebirth_def)
    frenzy_ref = "NULL"
    if "frenzy" in spec:
        block = spec["frenzy"]
        require(isinstance(block, dict) and set(block) <= {"max", "perStack", "decaySeconds"}, where,
                "frenzy is {max, perStack, decaySeconds}")
        most, per = block.get("max", 10), block.get("perStack", 4)
        require(1 <= most <= 30 and 1 <= per <= 50, where, "frenzy max 1..30, perStack 1..50 percent")
        out.append(f"static const SpecFrenzy {symbol}Frenzy = {{.decayFrames = {int(block.get('decaySeconds', 3) * FRAMES)}, "
                   f".maxStacks = {most}, .perStack = {per}}};")
        frenzy_ref = f"&{symbol}Frenzy"
    primary_ref = "NULL"
    if "primary" in spec:
        block = spec["primary"]
        require(isinstance(block, dict) and "frame" in block and "damage" in block, where,
                "primary is {frame, damage, speed, travel, impact, ...}: a bolt loosed on that frame of every swing")
        frame = block["frame"]
        require(isinstance(frame, int) and 0 <= frame <= 30, where, "primary frame 0..30")
        arrow = block.get("arrow", False)
        require(isinstance(arrow, bool), where, "primary arrow is true (the game's arrow) or false (a drawn bolt)")
        sub = dict({k: v for k, v in block.items() if k not in ("frame", "arrow")}, effect="bolt")
        out.append(f"static const SkillDeliveryDef {symbol}PrimaryDelivery = {delivery(sub, where + ' primary', names)};")
        out.append(f"static const SpecPrimary {symbol}Primary = {{.delivery = &{symbol}PrimaryDelivery, "
                   f".frame = {frame}, .arrow = {int(arrow)}}};")
        primary_ref = f"&{symbol}Primary"
    passive_refs, passive_ids = ["NULL"] * 9, []
    for k, passive in enumerate(passives):
        if isinstance(passive, dict):
            body, ref, ident = own_passive(passive, where, names, symbol, hero_id, k)
            out.append(body)
            headers.add(names["icons"][passive["icon"]])
            passive_refs[k] = ref
            passive_ids.append(ident)
        else:
            passive_ids.append(f"PASSIVE_SKILL_{passive}")
    rows = ["NULL"] * 13
    page2_defs = []
    for index, skill in enumerate(skills + page2):
        row = index if index < 6 else index + 1
        at = f"{where} skill {skill.get('name', index)}"
        require(set(skill) <= SKILL_FIELDS, at, f"unknown fields {sorted(set(skill) - SKILL_FIELDS)}")
        for key in ("name", "cost", "levels", "icon", "effect"):
            require(key in skill, at, f"needs {key}")
        require(skill["effect"] in EFFECTS, at, f"effect must be one of {', '.join(EFFECTS)}")
        require(skill["icon"] in names["icons"], at, "unknown icon (any *Icon* symbol in hacks/patches/*/*_icons.h)")
        headers.add(names["icons"][skill["icon"]])
        levels = levels5(skill["levels"], at, "levels", 1, 50)
        require(levels == sorted(levels), at, "levels must rise")
        require(skill.get("cast", "normal") in CASTS, at, f"cast must be one of {', '.join(CASTS)}")
        require(len(describe(skill)) <= 160, at, "description too long for the skill tree")
        fx = fx_layers(skill.get("fx"), at, names)
        name = f"{symbol}Row{row}"
        effect = skill["effect"]
        if effect in ("bolt", "burst", "blast", "swoop"):
            out.append(f"static const SkillDeliveryDef {name}Delivery = {delivery(skill, at, names)};")
            deliver = f"&{name}Delivery"
            amount = [0] * 5
        else:
            deliver = "NULL"
            amount = levels5(skill.get("amount", [0] * 5 if effect in AREA_EFFECTS else None), at, "amount",
                             0 if effect in AREA_EFFECTS else 1, 999)
            require(effect not in INSTANT_EFFECTS or "seconds" not in skill, at, f"{effect} is instant: no seconds")
            require(effect not in ("ward", "empower", "tithe") or max(amount) <= 90, at,
                    "ward, empower and tithe percents must be at most 90")
        require(effect != "tithe" or "rebirth" in spec, at, "tithe needs the spec's rebirth block")
        frames = int(skill.get("seconds", 0) * FRAMES)
        require(effect not in (*TIMED_EFFECTS, *AREA_EFFECTS) or effect in INSTANT_EFFECTS or frames > 0, at,
                f"{effect} needs seconds")
        target = skill.get("range", 128) if effect in RANGED_EFFECTS else 0
        require(0 <= target <= 255, at, "range must be 0..255")
        count = skill.get("count", {"summon": 2, "stampede": 3, "barrage": 6, "trap": 1, "venom": 5, "flurry": 5,
                                     "broadside": 5, "fan": 3}.get(effect, 0))
        require(effect != "leap" or 0 <= count <= 8, at, "leap count (extra leaps in the chain) 0..8")
        require(effect != "trap" or 1 <= count <= 8, at, "trap count 1..8")
        require(effect != "frenzy" or 1 <= count <= 30, at, "frenzy count (the raised cap) 1..30")
        require(effect != "barrage" or 1 <= count <= 8, at, "barrage count (foes) must be 1..8")
        require(effect != "summon" or 1 <= count <= 4, at, "summon count must be 1..4")
        require(effect != "stampede" or 1 <= count <= 6, at, "stampede count (riders) must be 1..6")
        threshold = skill.get("threshold", 30 if effect == "devour" else 0)
        require(effect != "devour" or 1 <= threshold <= 100, at, "devour threshold must be 1..100 percent")
        credit = skill.get("killCredit", 0)
        require(0 <= credit <= 255, at, "killCredit must be 0..255")
        require(effect == "summon" or ("look" not in skill and "lookColor" not in skill), at,
                "look and lookColor are for summons")
        look = enemy_type(root, skill["look"], at) if "look" in skill else 0xFF
        look_color = fx_color(skill["lookColor"], at, names) if "lookColor" in skill else "0"
        extra = []
        if "burst" in skill:
            require(effect in ("heal", "ward", "empower") and isinstance(skill["burst"], dict), at,
                    "burst is for heal, ward and empower: {damage, radius, stun, knockback, impact, sound}")
            sub = dict(skill["burst"], effect="burst")
            out.append(f"static const SkillDeliveryDef {name}Burst = {delivery(sub, at + ' burst', names)};")
            extra.append(f".burst = &{name}Burst")
        rally_frames = int(skill.get("rallySeconds", 0) * FRAMES)
        require(0 <= rally_frames <= 1800, at, "rallySeconds up to 60")
        if rally_frames:
            extra.append(f".rallyFrames = {rally_frames}")
        if effect in ("empower", *AREA_EFFECTS) and skill.get("status", "none") != "none":
            require(skill["status"] in STATUSES, at, f"status must be one of {', '.join(STATUSES)}")
            power = skill.get("statusPower", 1)
            require(isinstance(power, int) and 1 <= power <= 999, at, f"{effect}'s statusPower is one number 1..999")
            extra.append(f".status = {STATUSES.index(skill['status'])}, "
                         f".statusFrames = {int(skill.get('statusSeconds', 3) * FRAMES)}, .statusPower = {power}")
        if effect in AREA_EFFECTS:
            reach = skill.get("radius", 40)
            require(8 <= reach <= 160, at, f"{effect} radius 8..160")
            speed = skill.get("speed", {"fear": 1, "charm": 1, "dash": 4, "leap": 6, "backstep": 6, "pull": 4, "shove": 6, "grapple": 8}.get(effect, 0))
            require(0 <= speed <= 8, at, f"{effect} speed 0..8 px per tick")
            extra.append(f".chargeRadius = {reach}, .chargeSpeed = {speed}")
            if effect == "dash":
                extra.append(f".stunFrames = {int(skill.get('stun', 1) * FRAMES)}")
        if effect in PERCENT_EFFECTS:
            percent = skill.get("percent", PERCENT_EFFECTS[effect])
            require(0 <= percent <= 200, at, f"{effect} percent 0..200")
            extra.append(f".percent = {percent}")
        require(effect in PERCENT_EFFECTS or "percent" not in skill, at, f"percent is for {', '.join(PERCENT_EFFECTS)}")
        if effect in ("charge", "stampede"):
            require("anim" in skill and "color" in skill, at, f"{effect} needs anim (Enemy/Clip) and color")
            speed, reach = skill.get("speed", 6), skill.get("radius", 20)
            require(1 <= speed <= 12 and 4 <= reach <= 64, at, "charge speed 1..12 px per tick, radius 4..64")
            extra.append(f".chargeAnim = (const SpriteAnimation *)0x{clip_address(root, skill['anim'], 0, at):08x}, "
                         f".chargeColor = {fx_color(skill['color'], at, names)}, .chargeSpeed = {speed}, "
                         f".chargeRadius = {reach}, .stunFrames = {int(skill.get('stun', 1) * FRAMES)}")
        elif effect == "swoop":
            require("anim" in skill and "color" in skill, at, "swoop needs anim (Enemy/Clip) and color")
            require(frames <= 120, at, "a swoop's seconds (its flight time) are at most 4")
            extra.append(f".chargeAnim = (const SpriteAnimation *)0x{clip_address(root, skill['anim'], 0, at):08x}, "
                         f".chargeColor = {fx_color(skill['color'], at, names)}")
            if "strike" in skill:
                extra.append(f".strikeAnim = (const SpriteAnimation *)0x{clip_address(root, skill['strike'], 0, at):08x}")
        else:
            require("anim" not in skill and ("color" not in skill or effect in ("bleed", "venom")), at,
                    "anim and color are for charges, stampedes and swoops (color also for bleed and venom sparks)")
        require(effect == "swoop" or "strike" not in skill, at, "strike is for swoops")
        if "trail" in skill:
            require(effect in TRAIL_EFFECTS, at, f"trail is for {', '.join(TRAIL_EFFECTS)}")
            ticks = skill.get("trailTicks", 4)
            require(1 <= ticks <= 60, at, "trailTicks 1..60")
            extra.append(f".trail = {fx_layer(skill['trail'], at + ' trail', names)}, .trailTicks = {ticks}")
        if effect in ("bleed", "venom"):
            extra.append(f".statusFrames = {int(skill.get('stackSeconds', 3) * FRAMES)}")
            if "color" in skill:
                extra.append(f".chargeColor = {fx_color(skill['color'], at, names)}")
            if effect == "venom":
                require(1 <= count <= 9, at, "venom count (stacks to paralyze) 1..9")
        require(effect != "taunt" or skill.get("percent", 60) <= 90, at, "taunt percent (the most damage taken off) 0..90")
        if effect in ("leap", "trap", "backstep", "venom", "lash", "shove", "taunt", "dread", "grapple", "bomb", "broadside", "pierce", "fan", "hail"):
            extra.append(f".stunFrames = {int(skill.get('stun', 1) * FRAMES)}")
        require(effect != "fan" or 1 <= count <= 7, at, "fan count (bolts) 1..7")
        require(effect != "cover" or max(amount) <= 90, at, "cover percent at most 90")
        if effect == "hail" and "trailTicks" not in skill:
            extra.append(".trailTicks = 8")
        elif effect == "hail":
            extra.append(f".trailTicks = {skill['trailTicks']}")
        if effect in ("flurry", "broadside"):
            require(1 <= count <= 10, at, f"{effect} count 1..10")
            if "trail" not in skill:
                extra.append(f".trailTicks = {skill.get('trailTicks', 6)}")
        if "spread" in skill:
            require(effect in ("summon", "trap", "broadside") and 8 <= skill["spread"] <= 120,
                    at, "spread is a summon's, trap ring's or broadside's 8..120 pixels")
            extra.append(f".spread = {skill['spread']}")
        out.append(
            f"static const SpecSkill {name} = {{.delivery = {deliver}, .amount = {{{', '.join(map(str, amount))}}}, "
            f".frames = {frames}, .sound = {sound_id(skill.get('castSound'), at, names)}, .effect = {EFFECTS[effect]}, "
            f".targetRange = {target}, .count = {count}, .threshold = {threshold}, .killCredit = {credit}, "
            f".lookEnemy = {look}, .lookColor = {look_color}, .fx = {fx}"
            + "".join(", " + e for e in extra) + "};"
        )
        rows[row] = f"&{name}"
        out.append(f"static const ActiveSkill {name}Active = SPEC_KIT_CAST_ROW({int(skill['cost'])});")
        out.append(f"static const s8 {name}Levels[] = {{{', '.join(map(str, levels))}, -1}};")
        if index >= 6:
            page2_defs.append(
                f"static const SkillExpansionActiveDef {name}Def = {{.row = &{name}Active, .icon = {skill['icon']}, "
                f".requiredLevels = {name}Levels, .description = {json.dumps(describe(skill))}, "
                f".resolve = specKit_resolve, .baseRow = 0, .policy = &{CASTS[skill.get('cast', 'normal')]}, "
                f".id = 0x{((hero_id + 1) << 8) | (row + 1):x}}};"
            )
    out += page2_defs
    first = range(6)
    sym = lambda i: f"{symbol}Row{i}"
    out += [
        f"static const SpecKitTable {symbol}SpecTable = {{.rows = {{{', '.join(rows)}}}, .rebirth = {rebirth_ref}, "
        f".passives = {{{', '.join(passive_refs)}}}, .frenzy = {frenzy_ref}, .primary = {primary_ref}}};",
        f"static const ActiveSkill {symbol}Actives[HERO_ACTIVE_SKILL_COUNT] = {{"
        + ", ".join(f"SPEC_KIT_CAST_ROW({int(skills[i]['cost'])})" for i in first) + "};",
        f"static const void *const {symbol}Icons[HERO_ACTIVE_SKILL_COUNT] = {{"
        + ", ".join(skills[i]["icon"] for i in first) + "};",
        f"static const s8 {symbol}Levels[HERO_ACTIVE_SKILL_COUNT][SKILL_EXPANSION_REQUIRED_LEVEL_COUNT] = {{"
        + ", ".join("{" + ", ".join(map(str, skills[i]["levels"])) + ", -1}" for i in first) + "};",
        f"static const char *const {symbol}Descriptions[HERO_ACTIVE_SKILL_COUNT] = {{"
        + ", ".join(json.dumps(describe(skills[i])) for i in first) + "};",
        f"static const u16 {symbol}ActiveIds[HERO_ACTIVE_SKILL_COUNT] = {{"
        + ", ".join(f"0x{((hero_id + 1) << 8) | (i + 1):x}" for i in first) + "};",
        f"static const u8 {symbol}Passives[HERO_PASSIVE_SKILL_COUNT] = {{"
        + ", ".join(passive_ids) + "};",
    ]
    slot = [
        f"[{hero_id}] = {{.specRows = &{symbol}SpecTable, .kit = {{",
        f"    .activeSkills = {symbol}Actives, .icons = {symbol}Icons, .requiredLevels = {symbol}Levels,",
        f"    .descriptions = {symbol}Descriptions, .passiveSkillIds = {symbol}Passives,",
        f"    .artHeroId = {hero['stockHero']}, .reset = specKit_reset, .cleanup = specKit_cleanup,",
        "    .tick = specKit_tick, .meleeDamage = specKit_meleeDamage, .incomingDamage = specKit_incomingDamage,",
        "    .rangedDamage = specKit_rangedDamage, .onKill = specKit_onKill,",
        "    .drawHud = specKit_drawHud,",
        "    .resolvers = {specKit_resolve, specKit_resolve, specKit_resolve, specKit_resolve, specKit_resolve, specKit_resolve},",
        "    .policies = {" + ", ".join(f"&{CASTS[skills[i].get('cast', 'normal')]}" for i in first) + "},",
        f"    .activeIds = {symbol}ActiveIds,",
        "    .newActives = {" + ", ".join(
            f"&{sym(i + 7)}Def" if i < len(page2) else "NULL" for i in range(6)) + "},",
        "    .newPassiveSkillIds = {" + ", ".join(
            [f"PASSIVE_SKILL_{p}" for p in page2_passives] or ["SKILL_EXPANSION_PASSIVE_ID_NONE"] * 9) + "},",
        "}}",
    ]
    return "\n".join(out), "\n".join(slot), headers


def spec_heroes(root: Path, heroes: list[dict]) -> list[dict]:
    """The heroes that play a kit compiled from a spec (hero_registry.py marks them)."""
    return [hero for hero in heroes if hero.get("specKit") and spec_path(root, hero["name"]).exists()]


def generate(root: Path, heroes: list[dict], output: Path) -> None:
    """The kits' C source: their tables, then the slot array the linker puts at MEM_HERO_KITS."""
    names = catalog(root)
    bodies, headers, slots = [], set(), []
    for hero in spec_heroes(root, heroes):
        require(hero["id"] < KIT_SLOTS, hero["name"], f"kit slots exist for hero ids 0..{KIT_SLOTS - 1}")
        body, slot, used = compile_hero(root, hero, names)
        bodies.append(body)
        slots.append("    " + slot.replace("\n", "\n    "))
        headers |= used
    # First in the region (hacks/hero_kits.ld): slot N is hero N's. Empty slots are zeros.
    array = ("const SkillExpansionKitSlot HeroKitSlots[SKILL_EXPANSION_KIT_SLOTS]\n"
             '    __attribute__((section(".hero_kit_slots"), used)) = ')
    text = "// Generated by hacks/tools/hero_kit.py from hacks/content/heroes/*.json.\n"
    text += '#include "patches/heroes/spec_kit.h"\n#include "lib/fx_kit.h"\n'
    # A first page row's own ActiveSkill and levels are used only when it moves to page 2.
    text += '#pragma GCC diagnostic ignored "-Wunused-const-variable"\n'
    text += "".join(f'#include "{h}"\n' for h in sorted(headers))
    text += "\n#if PC_EDITION && HACK_skill_expansion && HACK_ringwraith\n\n"
    text += "".join(body + "\n\n" for body in bodies)
    text += array + "{\n" + ",\n".join(slots) + "\n};\n"
    text += "#else\n" + array + "{{0}};\n#endif\n"
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(text, newline="\n")
    # Kits linked into the modcode came from this include.
    (root / "build/hack_include/hero_spec_kits.inc").unlink(missing_ok=True)


def tool(name: str) -> str:
    found = shutil.which(name) or (TOOLS / (name + ".exe") if (TOOLS / (name + ".exe")).exists() else None)
    if not found:
        raise SpecError(f"{name} not found (on PATH or in {TOOLS})")
    return str(found)


def link(root: Path, source: Path, elf: Path, cflags: Path) -> bytes:
    """The region's bytes: source compiled with the modcode's flags, linked at MEM_HERO_KITS
    against elf's symbols (the modcode it runs with)."""
    flags = cflags.read_text().split()
    gcc, objcopy = tool("arm-none-eabi-gcc"), tool("arm-none-eabi-objcopy")
    with tempfile.TemporaryDirectory() as temp:
        obj, out, binary = (Path(temp) / name for name in ("kits.o", "kits.elf", "kits.bin"))
        steps = [
            [gcc, *flags, str(source), "-o", str(obj)],
            [gcc, "-nostartfiles", "-nodefaultlibs", "-mthumb", "-mthumb-interwork", "-T", "hacks/hero_kits.ld",
             "-Wl,-L,build", f"-Wl,--just-symbols={elf}", "-o", str(out), str(obj)],
            [objcopy, "-O", "binary", "-j", ".kits", str(out), str(binary)],
        ]
        for step in steps:
            result = subprocess.run(step, cwd=root, capture_output=True, text=True)
            if result.returncode:
                raise SpecError(f"{Path(step[0]).name} failed:\n{result.stdout}{result.stderr}")
        return binary.read_bytes()


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description="Link the spec kits into the hero kits region.")
    parser.add_argument("command", choices=["link"])
    parser.add_argument("elf", type=Path, help="the linked modcode (build/hack.elf)")
    parser.add_argument("output", type=Path, help="the region's bytes (build/hero_kits.bin)")
    args = parser.parse_args()
    here = Path(__file__).resolve().parents[2]
    args.output.write_bytes(link(here, here / SOURCE, args.elf.resolve(), here / CFLAGS))
    print(f"Wrote {args.output} ({args.output.stat().st_size} bytes)")
