"""Host regressions for hero moves and their level/passive scaling."""

from pathlib import Path
import shutil
import subprocess
import pytest

ROOT = Path(__file__).resolve().parents[2]


@pytest.mark.parametrize("hero", ["sam", "smeagol", "aragorn", "frodo"])
def test_hero_moves(tmp_path: Path, hero: str) -> None:
    compiler = shutil.which("clang") or shutil.which("cc")
    assert compiler
    output = tmp_path / (hero + ".exe")
    subprocess.run(
        [
            compiler,
            "-std=gnu11",
            "-Wall",
            "-Wextra",
            "-Werror",
            *(["-Wno-unused-parameter"] if hero == "frodo" else []),
            f"-DTEST_SAM={int(hero == 'sam')}",
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
            str(
                ROOT
                / (
                    "hacks/tests/aragorn.c"
                    if hero == "aragorn"
                    else "hacks/tests/frodo.c"
                    if hero == "frodo"
                    else "hacks/tests/hero_moves.c"
                )
            ),
            *([str(ROOT / "hacks/lib/combat_event.c")] if hero == "aragorn" else []),
            "-o",
            str(output),
        ],
        check=True,
    )
    subprocess.run([str(output)], check=True)
