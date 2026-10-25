"""Check stock cast requirements against ordinary and unique equipment families."""

from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def test_active_requirements(tmp_path: Path) -> None:
    compiler = shutil.which("clang") or shutil.which("cc")
    assert compiler
    output = tmp_path / "active-requirements.exe"
    subprocess.run(
        [
            compiler,
            "-std=gnu11",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-ffunction-sections",
            "-fdata-sections",
            "-Wl,--gc-sections",
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
            str(ROOT / "hacks/tests/active_requirements.c"),
            "-o",
            str(output),
        ],
        check=True,
    )
    subprocess.run([str(output)], check=True)
