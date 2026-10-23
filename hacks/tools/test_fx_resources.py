"""Exercise the FX helpers against host-backed native palette tables."""

from pathlib import Path
import shutil
import subprocess


def test_fx_resources(tmp_path: Path) -> None:
    root = Path(__file__).resolve().parents[2]
    compiler = shutil.which("clang") or shutil.which("cc")
    assert compiler, "Install clang or cc for FX resource regressions"
    source = (root / "hacks/lib/fx_kit.c").read_text()
    # Only hardware addresses change. Compile the production allocation and cleanup paths.
    source = (
        '#include "types.h"\nextern u8 testTags[16], testRefs[16];\n'
        + source.replace("((u8 *)0x030053d0)", "testTags").replace(
            "((u8 *)0x030053e0)", "testRefs"
        )
    )
    translated = tmp_path / "fx_kit.c"
    translated.write_text(source)
    executable = tmp_path / "fx-resources.exe"
    subprocess.run(
        [
            compiler,
            "-std=gnu11",
            "-Wall",
            "-Wextra",
            "-Werror",
            *(
                f"-I{root / path}"
                for path in (
                    "include",
                    "carved/include",
                    "build/include",
                    "hacks",
                    "hacks/lib",
                )
            ),
            str(translated),
            str(root / "hacks/tests/fx_resources.c"),
            "-o",
            str(executable),
        ],
        check=True,
    )
    subprocess.run([str(executable)], check=True)
