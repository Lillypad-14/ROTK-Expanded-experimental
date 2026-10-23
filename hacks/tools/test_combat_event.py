"""Host regressions for the shared combat event dispatcher."""

from pathlib import Path
import shutil
import subprocess


def test_combat_event(tmp_path: Path) -> None:
    root = Path(__file__).resolve().parents[2]
    compiler = shutil.which("clang") or shutil.which("cc")
    assert compiler, "Install clang or cc for combat event regressions"
    executable = tmp_path / "combat-event.exe"
    subprocess.run(
        [
            compiler,
            "-std=c11",
            "-Wall",
            "-Wextra",
            "-Werror",
            *(
                f"-I{root / path}"
                for path in ("include", "carved/include", "build/include", "hacks")
            ),
            *(
                str(root / path)
                for path in (
                    "hacks/tests/combat_event.c",
                    "hacks/lib/combat_event.c",
                    "hacks/lib/combat_source.c",
                )
            ),
            "-o",
            str(executable),
        ],
        check=True,
    )
    subprocess.run([str(executable)], check=True)
