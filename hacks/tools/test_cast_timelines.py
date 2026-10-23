"""Validate authored timelines and exercise actual cast/bench C paths on the host."""

from copy import deepcopy
import json
from pathlib import Path
import shutil
import subprocess
import pytest
from cast_timelines import generate

ROOT = Path(__file__).resolve().parents[2]


def test_generate(tmp_path):
    generate(
        ROOT / "hacks/content/cast_timelines.json",
        ROOT / "hacks/content/skill_fx.json",
        tmp_path,
        tmp_path,
    )
    assert (
        "HACKLIB_TIMELINE_QUICK = 0" in (tmp_path / "cast_timeline_ids.h").read_text()
    )
    assert "Windup: 6 ticks" in (tmp_path / "timelines.html").read_text()


@pytest.mark.parametrize(
    "field,value,message",
    [
        ("releaseTick", True, "release tick"),
        ("recoveryTicks", 3600, "recovery ticks"),
        ("id", 3, "consecutive"),
        (
            "events",
            [{"tick": 6, "effect": "missing", "anchor": "owner"}],
            "unknown timeline effect",
        ),
        (
            "events",
            [{"tick": 6, "effect": "preview_ward", "anchor": "missing"}],
            "unknown timeline anchor",
        ),
        (
            "events",
            [{"tick": 16, "effect": "preview_ward", "anchor": "owner"}],
            "event tick",
        ),
        (
            "events",
            [
                {"tick": 6, "effect": "preview_ward", "anchor": "owner"},
                {"tick": 0, "effect": "preview_ward", "anchor": "owner"},
            ],
            "event tick",
        ),
    ],
)
def test_invalid(tmp_path, field, value, message):
    document = json.loads((ROOT / "hacks/content/cast_timelines.json").read_text())
    document["timelines"][0][field] = deepcopy(value)
    manifest = tmp_path / "timelines.json"
    manifest.write_text(json.dumps(document))
    with pytest.raises(ValueError, match=message):
        generate(manifest, ROOT / "hacks/content/skill_fx.json", tmp_path, tmp_path)


def test_runtime(tmp_path):
    compiler = shutil.which("clang") or shutil.which("cc")
    assert compiler
    generate(
        ROOT / "hacks/content/cast_timelines.json",
        ROOT / "hacks/content/skill_fx.json",
        tmp_path,
        tmp_path,
    )
    executable = tmp_path / "timeline-tests.exe"
    subprocess.run(
        [
            compiler,
            "-std=c11",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-ffunction-sections",
            "-fdata-sections",
            "-Wl,--gc-sections",
            "-DHACK_debug_cheats=1",
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
                    "hacks/tests/cast_timeline.c",
                    "hacks/lib/cast_timeline.c",
                    "hacks/patches/skill_expansion/skill_expansion_timeline.c",
                    "hacks/patches/debug_cheats/ability_bench.c",
                )
            ),
            "-o",
            str(executable),
        ],
        check=True,
    )
    subprocess.run([str(executable)], check=True)
