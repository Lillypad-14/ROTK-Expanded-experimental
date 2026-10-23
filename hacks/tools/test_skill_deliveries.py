"""Check delivery recipes and run the actual delivery runtime on the host."""

import json
from pathlib import Path
import shutil
import subprocess
import pytest
from skill_deliveries import generate

ROOT = Path(__file__).resolve().parents[2]


def test_generate(tmp_path: Path) -> None:
    generate(
        ROOT / "hacks/content/skill_deliveries.json",
        ROOT / "hacks/content/skill_fx.json",
        tmp_path,
        tmp_path,
    )
    assert (
        "SKILL_DELIVERY_TEST_BOLT = 0"
        in (tmp_path / "skill_delivery_ids.h").read_text()
    )
    assert "Piercing bolt" in (tmp_path / "deliveries.html").read_text()


@pytest.mark.parametrize(
    "field,value,message",
    [
        ("elements", ["ice"], "unknown element"),
        ("elements", ["fire", "fire"], "duplicate element"),
        ("elements", [1], "unknown element"),
        ("status", "missing", "unknown status"),
        ("statusPower", [1], "five levels"),
        ("statusPower", [21846] * 5, "statusPower"),
        ("statusTicks", 1801, "statusTicks"),
        ("cooldownTicks", -1, "cooldownTicks"),
        ("speed", 0, "requires speed"),
        ("speed", 17, "speed"),
        ("radius", True, "radius"),
        ("lifetime", 301, "lifetime"),
        ("damage", [1, 2], "five levels"),
        ("damage", [1, 2, 3, 4, 65536], "damage"),
        ("maxTargets", 0, "maxTargets"),
        ("maxRepeats", 9, "maxRepeats"),
        ("repeatTicks", 0, "repeatTicks"),
        ("pierceTargets", 9, "pierceTargets"),
        ("travelEffect", "missing", "unknown delivery effect"),
        ("impactSound", "missing", "unknown delivery sound"),
        ("stopOnWalls", 1, "boolean"),
        ("kind", "missing", "unknown delivery kind"),
    ],
)
def test_invalid(tmp_path: Path, field: str, value: object, message: str) -> None:
    document = json.loads((ROOT / "hacks/content/skill_deliveries.json").read_text())
    document["deliveries"][0][field] = value
    manifest = tmp_path / "deliveries.json"
    manifest.write_text(json.dumps(document))
    with pytest.raises(ValueError, match=message):
        generate(manifest, ROOT / "hacks/content/skill_fx.json", tmp_path, tmp_path)


def test_runtime(tmp_path: Path) -> None:
    compiler = shutil.which("clang") or shutil.which("cc")
    assert compiler
    generate(
        ROOT / "hacks/content/skill_deliveries.json",
        ROOT / "hacks/content/skill_fx.json",
        tmp_path,
        tmp_path,
    )
    executable = tmp_path / "delivery-tests.exe"
    subprocess.run(
        [
            compiler,
            "-std=c11",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-DHACK_mod_options=1",
            f"-I{tmp_path}",
            *(
                f"-I{ROOT / path}"
                for path in (
                    "include",
                    "carved/include",
                    "build/include",
                    "build/hack_include",
                    "hacks",
                )
            ),
            *(
                str(ROOT / path)
                for path in (
                    "hacks/tests/skill_delivery.c",
                    "hacks/lib/targeting.c",
                    "hacks/patches/skill_expansion/skill_delivery.c",
                    "hacks/patches/debug_cheats/delivery_preview.c",
                )
            ),
            "-o",
            str(executable),
        ],
        check=True,
    )
    subprocess.run([str(executable)], check=True)
