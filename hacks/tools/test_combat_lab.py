"""Exercise the actual Combat Lab counters without the game renderer."""

from pathlib import Path
import shutil
import subprocess


def test_combat_lab(tmp_path: Path) -> None:
    root = Path(__file__).resolve().parents[2]
    compiler = shutil.which("clang") or shutil.which("cc")
    assert compiler, "Install clang or cc for Combat Lab regressions"
    executable = tmp_path / "combat-lab.exe"
    subprocess.run(
        [
            compiler,
            "-std=c11",
            "-Wall",
            "-Wextra",
            "-Werror",
            *(
                f"-I{root / path}"
                for path in (
                    "include",
                    "carved/include",
                    "build/include",
                    "build/hack_include",
                    "hacks",
                )
            ),
            str(root / "hacks/tests/combat_lab.c"),
            str(root / "hacks/patches/debug_cheats/combat_lab_stats.c"),
            "-o",
            str(executable),
        ],
        check=True,
    )
    subprocess.run([str(executable)], check=True)
