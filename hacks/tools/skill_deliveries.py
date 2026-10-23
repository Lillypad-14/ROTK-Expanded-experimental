"""Compile reusable ability delivery recipes and their browser reference."""

from html import escape
import json
from pathlib import Path
from skill_fx import ROOT, fields, key, number, require

SOUNDS = {"none": 0, "missile": 0x13, "fireburst": 0x0D, "stun": 0x09}
LIMITS = {
    "lifetime": (1, 300),
    "speed": (0, 16),
    "radius": (1, 128),
    "splashRadius": (0, 128),
    "maxTargets": (1, 8),
    "pierceTargets": (1, 8),
    "maxRepeats": (1, 8),
    "repeatTicks": (1, 255),
    "trailTicks": (1, 255),
    "knockback": (0, 32),
    "stunTicks": (0, 300),
    "statusTicks": (0, 1800),
    "cooldownTicks": (0, 1800),
}
EFFECTS = ("travelEffect", "trailEffect", "impactEffect")
ORDER = (
    "lifetime",
    *EFFECTS,
    "launchSound",
    "impactSound",
    "stunTicks",
    "kind",
    "speed",
    "radius",
    "splashRadius",
    "maxTargets",
    "pierceTargets",
    "maxRepeats",
    "repeatTicks",
    "trailTicks",
    "knockback",
    "stopOnWalls",
    "elements",
    "elementsKnown",
    "status",
    "statusTicks",
    "statusPower",
    "cooldownTicks",
)


def generate(manifest: Path, effects: Path, output: Path, preview: Path) -> None:
    document = json.loads(manifest.read_text())
    fields(document, {"version", "deliveries"}, "delivery manifest")
    require(document["version"] == 1, "unsupported delivery version")
    catalog = {
        effect["key"]: effect["id"]
        for effect in json.loads(effects.read_text())["effects"]
    }
    recipes = document["deliveries"]
    require(isinstance(recipes, list), "deliveries must be a list")
    number(len(recipes), 1, 32, "delivery count")
    definitions, names, identifiers, rows = [], [], [], []
    used = set()
    for index, recipe in enumerate(recipes):
        fields(
            recipe,
            {
                "key",
                "id",
                "name",
                "damage",
                *(f for f in ORDER if f != "elementsKnown"),
            },
            "delivery",
        )
        identity = key(recipe["key"])
        require(identity not in used, "duplicate delivery key")
        used.add(identity)
        require(
            number(recipe["id"], 0, 31, "delivery ID") == index,
            "delivery IDs must be consecutive",
        )
        name = recipe["name"]
        require(
            isinstance(name, str)
            and name.isascii()
            and name.isprintable()
            and 1 <= len(name) <= 16,
            "invalid delivery name",
        )
        damage = recipe["damage"]
        require(
            isinstance(damage, list) and len(damage) == 5, "damage requires five levels"
        )
        for value in damage:
            number(value, 0, 65535, "damage")
        values = {}
        elements = recipe["elements"]
        require(
            elements is None or isinstance(elements, list),
            "elements must be null or a list",
        )
        require(
            elements is None
            or all(isinstance(e, str) and e in ("fire", "poison") for e in elements),
            "unknown element",
        )
        require(
            elements is None or len(set(elements)) == len(elements), "duplicate element"
        )
        values["elements"] = sum({"fire": 1, "poison": 2}[e] for e in (elements or []))
        values["elementsKnown"] = int(elements is not None)
        status = recipe["status"]
        require(status in ("none", "poison", "burn", "slow"), "unknown status")
        values["status"] = ("none", "poison", "burn", "slow").index(status)
        power = recipe["statusPower"]
        require(
            isinstance(power, list) and len(power) == 5,
            "statusPower requires five levels",
        )
        for value in power:
            number(
                value, 1 if status in ("poison", "burn") else 0, 21845, "statusPower"
            )
        values["statusPower"] = "{" + ",".join(map(str, power)) + "}"
        for field, (low, high) in LIMITS.items():
            values[field] = number(recipe[field], low, high, field)
        require(
            recipe["status"] == "none" or values["statusTicks"] > 0,
            "status requires duration",
        )
        require(recipe["kind"] in ("straight", "burst"), "unknown delivery kind")
        values["kind"] = int(recipe["kind"] == "burst")
        require(
            values["kind"] != 0 or values["speed"] > 0,
            "straight delivery requires speed",
        )
        require(
            values["pierceTargets"] <= values["maxTargets"],
            "piercing exceeds target budget",
        )
        require(type(recipe["stopOnWalls"]) is bool, "stopOnWalls must be boolean")
        values["stopOnWalls"] = int(recipe["stopOnWalls"])
        for field in EFFECTS:
            require(
                recipe[field] is None or recipe[field] in catalog,
                "unknown delivery effect",
            )
            values[field] = 65535 if recipe[field] is None else catalog[recipe[field]]
        for field in ("launchSound", "impactSound"):
            require(recipe[field] in SOUNDS, "unknown delivery sound")
            values[field] = SOUNDS[recipe[field]]
        definitions.append(
            "{{"
            + ",".join(map(str, damage))
            + "},"
            + ",".join(str(values[field]) for field in ORDER)
            + "}"
        )
        names.append(json.dumps(name))
        identifiers.append(f"    SKILL_DELIVERY_{identity.upper()} = {index},")
        cells = [
            name,
            recipe["kind"],
            "/".join(map(str, damage)),
            str(recipe["speed"]),
            str(recipe["radius"]),
            str(recipe["splashRadius"]),
            str(recipe["pierceTargets"]),
            f"{recipe['maxRepeats']} / {recipe['repeatTicks']} ticks",
            str(recipe["maxTargets"]),
            str(recipe["elements"]),
            recipe["status"],
            str(recipe["cooldownTicks"]),
        ]
        rows.append(
            "<tr>"
            + "".join("<td>" + escape(cell) + "</td>" for cell in cells)
            + "</tr>"
        )
    output.mkdir(parents=True, exist_ok=True)
    (output / "skill_delivery_ids.h").write_text(
        "#pragma once\nenum SkillDeliveryId {\n"
        + "\n".join(identifiers)
        + f"\n    SKILL_DELIVERY_COUNT = {len(recipes)}\n}};\n",
        newline="\n",
    )
    (output / "skill_delivery_data.inc").write_text(
        "// Generated by hacks/tools/skill_deliveries.py.\nconst SkillDeliveryDef SkillDeliveryDefinitions[] = {"
        + ",".join(definitions)
        + "};\nconst char *const SkillDeliveryNames[] = {"
        + ",".join(names)
        + "};\n",
        newline="\n",
    )
    preview.mkdir(parents=True, exist_ok=True)
    headers = (
        "Recipe",
        "Kind",
        "Damage L1..5",
        "Pixels/tick",
        "Radius",
        "Splash",
        "Pierce",
        "Attempts/target",
        "Target budget",
        "Elements",
        "Status",
        "Cooldown",
    )
    (preview / "deliveries.html").write_text(
        '<!doctype html><meta charset="utf-8"><title>Skill deliveries</title><style>body{font:16px system-ui;background:#182228;color:#eee;margin:2rem}table{border-collapse:collapse}th,td{padding:.6rem;border:1px solid #678}a{color:#acf}</style><h1>Skill deliveries</h1><p><a href="index.html">Effects</a> | <a href="timelines.html">Timelines</a></p><p>30 game ticks per second. Contact attempts include misses. Each delivery has its own target and repeat budgets.</p><table><tr>'
        + "".join("<th>" + header + "</th>" for header in headers)
        + "</tr>"
        + "".join(rows)
        + "</table>",
        newline="\n",
    )
    print(f"Skill deliveries: {len(recipes)} validated")


if __name__ == "__main__":
    generate(
        ROOT / "hacks/content/skill_deliveries.json",
        ROOT / "hacks/content/skill_fx.json",
        ROOT / "build/hack_include",
        ROOT / "build/skill_fx_preview",
    )
